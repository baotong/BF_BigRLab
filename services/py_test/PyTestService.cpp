#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/format.hpp>
#include <glog/logging.h>
#include "common.hpp"
#include "PyTestService.h"

using namespace BigRLab;
using namespace std;

// lib接口函数实现
Service* create_instance(const char *name)
{ return new PyTestService(name); }
// 返回实际名称，与alg server 中定义的LIB_NAME保持一致
const char* lib_name()
{ return "PyTest"; }

// 获取空闲连接，除名称外，其他不用改
PyTestService::PyTestClientPtr PyTestService::IdleClientQueue::getIdleClient()
{
    // DLOG(INFO) << "IdleClientQueue::getIdleClient() size = " << this->size();

    PyTestClientPtr pRet;

    do {
        PyTestClientWptr wptr;
        if (!this->timed_pop(wptr, TIMEOUT))
            return PyTestClientPtr();      // return empty ptr when no client available
        pRet = wptr.lock();
        // DLOG_IF(INFO, !pRet) << "IdleClientQueue::getIdleClient() got empty ptr";
    } while (!pRet);

    return pRet;
}

// 连接对象构造函数，除名称外都不用改
PyTestService::PyTestClientArr::PyTestClientArr(const BigRLab::AlgSvrInfo &svr, 
                                    IdleClientQueue *idleQue, int n) 
{
    // DLOG(INFO) << "PyTestClientArr constructor " << svr.addr << ":" << svr.port
            // << " n = " << n;
    
    clients.reserve(n);
    for (int i = 0; i < n; ++i) {
        auto pClient = boost::make_shared<PyTestClient>(svr.addr, (uint16_t)(svr.port));
        if (pClient->start(50, 300)) // totally wait 15s
            clients.push_back(pClient);
#ifndef NDEBUG
        else
            DLOG(INFO) << "Fail to create client instance to " << svr.addr << ":" << svr.port;
#endif
    } // for

    // DLOG(INFO) << "clients.size() = " << clients.size();

    // idleQue insert and shuffle
    if (!clients.empty()) {
        boost::unique_lock<boost::mutex> lock( idleQue->mutex() );
        idleQue->insert( idleQue->end(), clients.begin(), clients.end() );
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(idleQue->begin(), idleQue->end(), g);
    } // if
}


// handleCommand 批处理作业类，根据需求编写
struct PyTestTask : BigRLab::WorkItemBase {
    PyTestTask( std::size_t _Id,                // 行号
                 std::string &_Text,            // 行文本
                 PyTestService::IdleClientQueue *_IdleClients, // 本lib的空闲连接队列
                 std::atomic_size_t *_Counter,      // 计数器
                 boost::condition_variable *_Cond,  // 条件变量用于通知作业完成
                 boost::mutex *_Mtx,                // 互斥锁用于写入输出文件
                 std::ofstream *_Ofs,               // 输出文件对象
                 const char *_SrvName )             // service_name
        : id(_Id)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName)
    { text.swap(_Text); }

    // 作业处理函数，由WorkMgr调度
    virtual void run()
    {
        using namespace std;

        // 无论成功还是失败，本函数完成后都要加计数器和通知条件变量
        // 定义在 common.hpp
        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        bool done = false;

        do {
            // 先取出一个空闲连接(thrift client)
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                // 远程服务的返回类型，见 alg server *.thrift 定义
                vector<PyTest::Result> result;
                // 调用远程服务
                pClient->client()->segment( result, text );
                // 调用成功，设置完成标志，并将用过的连接(client)放回到空闲连接池里
                done = true;
                idleClients->putBack( pClient );

                // 将结果写入到文件，先写入到stringstream，再批量写入文件，提高效率
                ostringstream output(ios::out);
                if (!result.empty()) {
                    output << id << "\t";
                    for (auto& v : result)
                        output << v.id << ":" << v.word << " ";
                    output << flush;
                } else {
                    output << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << output.str() << endl;

            } catch (const PyTest::InvalidRequest &err) {
                // 由非法输入导致的调用远程服务失败，因为连接还是好的，
                // 所以处理上和成功一样，设置标志变量done为完成，再把用过的连接放回空闲连接队列
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                // 由其他原因，多为alg server挂掉，连接丢失，导致的调用失败。
                // 此时改连接已失效，不必放回空闲队列。
                // 不用设置完成标志，这样程序循环就会自动取下一个空闲连接来处理作业。
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try
        } while (!done);
    }

    // 根据业务需要增加成员变量如 topk 等
    std::size_t                     id;
    std::string                     text;
    PyTestService::IdleClientQueue  *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
};


// 命令行批处理作业
void PyTestService::handleCommand( std::stringstream &stream )
{
    using namespace std;

#define MY_WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        getWriter()->writeLine(__write_line_stream.str()); \
    } while (0)

    // 参数处理，一般必有in:infile out:outfile，根据业务需求添加
    string arg, infile, outfile;

    while (stream >> arg) {
        auto colon = arg.find(':');
        if (string::npos == colon || 0 == colon || arg.size()-1 == colon)
            THROW_RUNTIME_ERROR("Usage: service " << name() << " arg1:value1 ...");
        string key = arg.substr(0, colon);
        string value = arg.substr(colon + 1);
        // DLOG(INFO) << "arg = " << arg << " key = " << key << " value = " << value;
        // DLOG(INFO) << key << ": " << value;
        if ("in" == key) {
            infile.swap(value);   
        } else if ("out" == key) {
            outfile.swap(value);
        } else {
            THROW_RUNTIME_ERROR("Unrecogonized arg " << key);
        } // if
    } // while

    if (infile.empty())
        THROW_RUNTIME_ERROR("Please specify input file via in:filename");
    if (outfile.empty())
        THROW_RUNTIME_ERROR("Please specify output file via out:filename");

    ifstream ifs(infile, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "Cannot open " << infile << " for reading.");
    ofstream ofs(outfile, ios::out);
    THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open " << outfile << " for writting.");

    // DLOG(INFO) << "infile: " << infile;
    // DLOG(INFO) << "outfile: " << outfile;

    string                       line;          // 行文本，给 Task 的 text
    size_t                       lineno = 0;    // 行号    Task 的 id
    atomic_size_t                counter(0);    // 计数器，通过指针传给Task构造
    boost::condition_variable    cond;          // 等待完成的条件变量，通过指针给Task构造
    boost::mutex                 mtx;           // 互斥锁，通过指针给Task构造

    while ( getline(ifs, line) ) {
        // 去掉两边的空白字符
        boost::trim( line );
        if (line.empty()) continue;
        // 建立作业，见Task的构造函数
        WorkItemBasePtr pWork = boost::make_shared<PyTestTask>
                (lineno, line, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        // 将作业加入到apiserver的执行队列
        getWorkMgr()->addWork( pWork );
        ++lineno;
    } // while

    // 等待所有作业完成
    boost::unique_lock<boost::mutex> lock(mtx);
    cond.wait( lock, [&]()->bool {return counter >= lineno;} );

    MY_WRITE_LINE("Job Done!"); // -b 要求必须输出一行文本
#undef MY_WRITE_LINE
}

// 不用改
void PyTestService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    // DLOG(INFO) << "Service " << name() << " received request: " << pWork->body;

    bool done = false;

    do {
        auto pClient = m_queIdleClients.getIdleClient();
        if (!pClient) {
            LOG(ERROR) << "Service " << name() << " handleRequest fail, "
                    << " no client object available.";
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, NO_SERVER, 
                    name() << ": no algorithm server available.");
            return;
        } // if

        try {
            std::string result;
            pClient->client()->handleRequest( result, pWork->body );
            done = true;
            m_queIdleClients.putBack( pClient );
            send_response(pWork->conn, BigRLab::ServerType::connection::ok, result);

        } catch (const PyTest::InvalidRequest &err) {
            done = true;
            m_queIdleClients.putBack( pClient );
            LOG(ERROR) << "Service " << name() << " caught InvalidRequest: "
                    << err.reason;
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, INVALID_REQUEST, 
                    name() << " InvalidRequest: " << err.reason);
        } catch (const std::exception &ex) {
            LOG(ERROR) << "Service " << name() << " caught exception: "
                    << ex.what();
            // RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, UNKNOWN_EXCEPTION, 
                    // name() << " unknown exception: " << ex.what());
        } // try
    } while (!done);
}

// 新alg server加入时的处理, 不用改
std::size_t PyTestService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    // 建立与alg server声明的并发数同等数量的连接
    // if (n < 5)
        // n = 5;
    // else if (n > 50)
        // n = 50;

    DLOG(INFO) << "PyTestService::addServer() " << svrInfo.addr << ":" << svrInfo.port
              << " maxConcurrency = " << svrInfo.maxConcurrency
              << ", going to create " << n << " client instances.";
    // SLEEP_SECONDS(1);
    auto pClient = boost::make_shared<PyTestClientArr>(svrInfo, &m_queIdleClients, n);
    // DLOG(INFO) << "pClient->size() = " << pClient->size();
    if (pClient->empty())
        return SERVER_UNREACHABLE;
    
    DLOG(INFO) << "PyTestService::addServer() m_queIdleClients.size() = " << m_queIdleClients.size();

    return Service::addServer(svrInfo, boost::static_pointer_cast<ServerAttr>(pClient));
}

// 方便打印输出，不用改
std::string PyTestService::toString() const
{
    using namespace std;

    stringstream stream;
    stream << "Service " << name() << endl;
    stream << "Online servers:" << endl; 
    stream << left << setw(30) << "IP:Port" << setw(20) << "maxConcurrency" 
            << setw(20) << "nClientInst" << endl; 
    for (const auto &v : m_mapServers) {
        stringstream addrStream;
        addrStream << v.first.addr << ":" << v.first.port << flush;
        stream << left << setw(30) << addrStream.str() << setw(20) 
                << v.first.maxConcurrency << setw(20);
        auto sp = v.second;
        PyTestClientArr *p = static_cast<PyTestClientArr*>(sp.get());
        stream << p->size() << endl;
    } // for
    stream.flush();
    return stream.str();
}


