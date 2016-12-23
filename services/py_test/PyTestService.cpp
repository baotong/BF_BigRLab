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
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/format.hpp>
#include <glog/logging.h>
#include "common.hpp"
#include "PyTestService.h"

using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new PyTestService(name); }

const char* lib_name()
{ return "PyTest"; }

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


struct PyTestTask : BigRLab::WorkItemBase {
    PyTestTask( std::size_t _Id,
                 std::string &_Text, 
                 PyTestService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName)
    { text.swap(_Text); }

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<PyTest::Result> result;
                pClient->client()->segment( result, text );
                done = true;
                idleClients->putBack( pClient );

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
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try
        } while (!done);
    }

    std::size_t                     id;
    std::string                     text;
    PyTestService::IdleClientQueue  *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
};


// 在ApiServer主线程中执行
// service servicename reqtype infile outfile
void PyTestService::handleCommand( std::stringstream &stream )
{
    using namespace std;

#define MY_WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        getWriter()->writeLine(__write_line_stream.str()); \
    } while (0)

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

    string                       line;
    size_t                       lineno = 0;
    atomic_size_t                counter(0);
    boost::condition_variable    cond;
    boost::mutex                 mtx;

    while ( getline(ifs, line) ) {
        boost::trim( line );
        WorkItemBasePtr pWork = boost::make_shared<PyTestTask>
                (lineno, line, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        getWorkMgr()->addWork( pWork );
        ++lineno;
    } // while

    boost::unique_lock<boost::mutex> lock(mtx);
    cond.wait( lock, [&]()->bool {return counter >= lineno;} );

    MY_WRITE_LINE("Job Done!"); // -b 要求必须输出一行文本
#undef MY_WRITE_LINE
}

// 在apiserver的工作线程中执行
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

std::size_t PyTestService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    if (n < 5)
        n = 5;
    else if (n > 50)
        n = 50;

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


