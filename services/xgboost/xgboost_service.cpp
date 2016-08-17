#include "xgboost_service.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <cstring>
#include <glog/logging.h>
#include "common.hpp"

using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new XgBoostService(name); }

const char* lib_name()
{ return "xgboost"; }

XgBoostService::XgBoostClientPtr XgBoostService::IdleClientQueue::getIdleClient()
{
    // DLOG(INFO) << "IdleClientQueue::getIdleClient() size = " << this->size();

    XgBoostClientPtr pRet;

    do {
        XgBoostClientWptr wptr;
        if (!this->timed_pop(wptr, TIMEOUT))
            return XgBoostClientPtr();      // return empty ptr when no client available
        pRet = wptr.lock();
        // DLOG_IF(INFO, !pRet) << "IdleClientQueue::getIdleClient() got empty ptr";
    } while (!pRet);

    return pRet;
}

XgBoostService::XgBoostClientArr::XgBoostClientArr(const BigRLab::AlgSvrInfo &svr, 
                                    IdleClientQueue *idleQue, int n) 
{
    // DLOG(INFO) << "XgBoostClientArr constructor " << svr.addr << ":" << svr.port
            // << " n = " << n;
    
    clients.reserve(n);
    for (int i = 0; i < n; ++i) {
        auto pClient = boost::make_shared<XgBoostClient>(svr.addr, (uint16_t)(svr.port));
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


struct XgBoostTask : BigRLab::WorkItemBase {
    XgBoostTask( std::size_t _Id,
                 std::string &_Data, 
                 const bool _Leaf,
                 XgBoostService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , leaf(_Leaf)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName)
    { strData.swap(_Data); }

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(_pOnFinish, { ++*counter; cond->notify_all(); });

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<double> result;
                pClient->client()->predictStr( result, strData, leaf );
                done = true;
                idleClients->putBack( pClient );

                // write to file
                if (!result.empty()) {
                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t";
                    if (!leaf) {
                        for (auto& v : result)
                            *ofs << v << " ";
                    } else {
                        for (size_t i = 0; i < result.size(); ++i) {
                            uint32_t v = (uint32_t)result[i];
                            if (v)
                                *ofs << v << " ";
                                // *ofs << i << ":" << v << " ";
                        } // for
                    } // if
                    *ofs << endl << flush;
                } // if

            } catch (const XgBoostSvr::InvalidRequest &err) {
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
    std::string                     strData;
    const bool                      leaf;
    XgBoostService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
};

// 在ApiServer主线程中执行
// service xgboost reqtype infile outfile
void XgBoostService::handleCommand( std::stringstream &stream )
{
    using namespace std;

#define MY_WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        getWriter()->writeLine(__write_line_stream.str()); \
    } while (0)

    string reqtype, infile, outfile;
    stream >> reqtype >> infile >> outfile;
    if (bad_stream(stream))
        THROW_RUNTIME_ERROR("Usage: service " << name() << " reqtype infile outfile ...");

    // if (!isValidReq(reqtype))
        // THROW_RUNTIME_ERROR("Invalid reqtype " << reqtype);

    ifstream ifs(infile, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("Cannot open " << infile << " for reading.");
    ofstream ofs(outfile, ios::out);
    if (!ofs)
        THROW_RUNTIME_ERROR("Cannot open " << outfile << " for writting.");

    bool leaf = false;

    if ("leaf" == reqtype)
        leaf = true;
    else if ("predict" != reqtype)
        THROW_RUNTIME_ERROR("reqtype must be \"predict\" or \"leaf\"");

    string                       line;
    size_t                       lineno = 0;
    atomic_size_t                counter;
    boost::condition_variable    cond;
    boost::mutex                 mtx;

    counter = 0;

    auto do_task = [&] {
        while ( getline(ifs, line) ) {
            boost::trim_right( line );
            WorkItemBasePtr pWork = boost::make_shared<XgBoostTask>
                (lineno, line, leaf, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str());
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    do_task();

    boost::unique_lock<boost::mutex> lock(mtx);
    cond.wait( lock, [&]()->bool {return counter >= lineno;} );

    MY_WRITE_LINE("Job Done!"); // -b 要求必须输出一行文本
#undef MY_WRITE_LINE
}

// 在apiserver的工作线程中执行
void XgBoostService::handleRequest(const BigRLab::WorkItemPtr &pWork)
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

        } catch (const XgBoostSvr::InvalidRequest &err) {
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

std::size_t XgBoostService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    if (n < 5)
        n = 5;
    else if (n > 50)
        n = 50;

    DLOG(INFO) << "XgBoostService::addServer() " << svrInfo.addr << ":" << svrInfo.port
              << " maxConcurrency = " << svrInfo.maxConcurrency
              << ", going to create " << n << " client instances.";
    // SLEEP_SECONDS(1);
    auto pClient = boost::make_shared<XgBoostClientArr>(svrInfo, &m_queIdleClients, n);
    // DLOG(INFO) << "pClient->size() = " << pClient->size();
    if (pClient->empty())
        return SERVER_UNREACHABLE;
    
    DLOG(INFO) << "XgBoostService::addServer() m_queIdleClients.size() = " << m_queIdleClients.size();

    return Service::addServer(svrInfo, boost::static_pointer_cast<ServerAttr>(pClient));
}

std::string XgBoostService::toString() const
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
        XgBoostClientArr *p = static_cast<XgBoostClientArr*>(sp.get());
        stream << p->size() << endl;
    } // for
    stream.flush();
    return stream.str();
}


