#include "api_server.h"
#include "service_manager.h"
#include <glog/logging.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace Test {
    using namespace BigRLab;
    using namespace std;

    void print_request( const ServerType::request& req )
    {
        cout << "Received client request from " << req.source << endl; // source 已经包含port
        cout << "destination = " << req.destination << endl;  // 去除了URL port
        cout << "method = " << req.method << endl;
        cout << "http version = " << (uint32_t)(req.http_version_major)
            << "." << (uint32_t)(req.http_version_minor) << endl;
        cout << "headers:" << endl;
        for (const auto &header : req.headers)
            cout << header.name << " = " << header.value << endl;
    }
} // namespace Test


namespace BigRLab {

boost::shared_ptr<APIServer>                 g_pApiServer;


APIServer::APIServer( const ServerType::options &_Opts,
        const IoServicePtr &_pIoSrv,
        const ThreadGroupPtr &_pThrgrp )
      : m_pServer(NULL)
      , m_bRunning(false)
      , m_nPort( DEFAULT_PORT )
      , m_nIoThreads( DEFAULT_N_IO_THREADS )
      , m_nWorkThreads( DEFAULT_N_WORK_THREADS )
      , m_Options(std::move(_Opts))
      , m_pIoService(_pIoSrv)
      , m_pIoThrgrp(_pThrgrp)
{
    options().address("0.0.0.0").reuse_address(true)
        .io_service(m_pIoService);
}

APIServer::APIServer( const ServerType::options &_Opts,
                      const IoServicePtr &_pIoSrv,
                      const ThreadGroupPtr &_pThrgrp,
                      const char *confFileName )
                : APIServer(_Opts, _pIoSrv, _pThrgrp)
{
    using namespace std;

    parse_config_file( confFileName, m_mapProperties );

    auto check_addr = [](const std::string &value, std::string &addr, uint16_t &port) {
        string::size_type pos = value.find_last_of(':');
        if (string::npos == pos)
            THROW_RUNTIME_ERROR("Invalid address format for algmgr_server");

        addr = value.substr(0, pos);
        if (addr.empty())
            THROW_RUNTIME_ERROR("Invalid address format for algmgr_server");

        string strPort = value.substr(pos + 1, string::npos);
        if (strPort.empty())
            THROW_RUNTIME_ERROR("Invalid address format for algmgr_server");

        if (!boost::conversion::try_lexical_convert(strPort, port))
            THROW_RUNTIME_ERROR("Invalid port format for algmgr_server");

        if (!port)
            THROW_RUNTIME_ERROR("Invalid port format for algmgr_server");
    };

    for (auto &v : m_mapProperties) {
        if ("port" == v.first) {
            if (!(read_from_string(*(v.second.begin()), m_nPort))) {
                cout << "APIServer Invalid value " << *(v.second.begin())
                        << " for port number, set to default." << endl;
                m_nPort = DEFAULT_PORT;
            } // if
        } else if ("n_io_threads" == v.first) {
            if (!(read_from_string(*(v.second.begin()), m_nIoThreads))) {
                cout << "APIServer Invalid value " << *(v.second.begin())
                        << " for n_io_threads, set to default." << endl;
                m_nIoThreads = DEFAULT_N_IO_THREADS;
            } // if
        } else if ("n_work_threads" == v.first) {
            if (!(read_from_string(*(v.second.begin()), m_nWorkThreads))) {
                cout << "APIServer Invalid value " << *(v.second.begin())
                        << " for n_work_threads, set to default." << endl;
                m_nWorkThreads = DEFAULT_N_WORK_THREADS;
            } // if
        } // if 
        // else if ("algmgr_server" == v.first) {
            // const string &addr = *(v.second.begin());
            // check_addr(addr, m_strAlgMgrAddr, m_nAlgMgrPort);
        // } // if
    } // for

    // if (m_strAlgMgrAddr.empty())
        // THROW_RUNTIME_ERROR("No item \"algmgr_server\" found in config file");

    // m_pAlgMgrClient = boost::make_shared< AlgMgrClient >(m_strAlgMgrAddr, m_nAlgMgrPort);

    options().port(to_string(m_nPort))
        .thread_pool(boost::make_shared<ThreadPool>(m_nIoThreads, m_pIoService, m_pIoThrgrp));
}

void APIServer::run()
{
    using namespace std;

    if ( m_bRunning ) {
        cerr << "APIServer already running!" << endl;
        return;
    } // if

    m_bRunning = true;

    // m_pAlgMgrClient->start();
    m_pServer = new ServerType(m_Options);
    m_pServer->run();
}

void APIServer::stop()
{
    m_bRunning = false;
    if (m_pServer)
        m_pServer->stop();   // 是异步的 async_stop
    // if (m_pAlgMgrClient)
        // m_pAlgMgrClient->stop();
    // 不能在这里join这些 io thread 需要io_service结束后
    // m_pIoThrgrp->join_all();
}

// run in io thread
void APIServerHandler::operator()(const ServerType::request& req,
                    ServerType::connection_ptr conn)
try {
    using namespace std;

    // Test::print_request(req);

    if (!boost::iequals(req.method, "POST")) {
        LOG(ERROR) << "Wrong request from " << req.source << ", only accept POST.";
        send_response(conn, ServerType::connection::bad_request, "Only accept POST request");
        return;
    } // if

    size_t nRead;
    for (const auto &v : req.headers) {
        if (boost::iequals(v.name, "Content-Type")) {
            if (!boost::iequals(v.value, "application/json")) {
                LOG(ERROR) << "Wrong request from " << req.source << ", only accept json content.";
                send_response(conn, ServerType::connection::bad_request, "Content-Type is not json");
                return;
            } // if
        } else if (boost::iequals(v.name, "Content-Length")) {
            // nRead = boost::lexical_cast<size_t>(v.value);
            if (!boost::conversion::try_lexical_convert(v.value, nRead)) {
                send_response(conn, ServerType::connection::bad_request, "Content-Length not valid");
                return;
            } // if
        } // if
    } // for

    WorkItemPtr pWork( new WorkItem(req, conn, nRead) );
    pWork->readBody(conn);
} catch (const std::exception &ex) {
    LOG(ERROR) << "APIServerHandler exception " << ex.what();
}

// NOT sure, use boost::asio_read on socket directly
void WorkItem::readBody( const ServerType::connection_ptr &conn )
{
    using namespace std;

    if (left2Read)
        conn->read( std::bind(&WorkItem::handleRead, shared_from_this(),
               placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4) );
    else
        g_pWorkMgr->addWork( boost::static_pointer_cast<WorkItemBase>(shared_from_this()) );
}

// run in io thread
void WorkItem::handleRead(ServerType::connection::input_range range, 
        boost::system::error_code error, std::size_t size, 
        ServerType::connection_ptr conn)
{
    if (error) {
        LOG(ERROR) << "Read connection from " << req.source << " error: " << error;
        return;
    } // if

    if (size > left2Read)
        size = left2Read;

    body.append(boost::begin(range), size);
    left2Read -= size;
    
    readBody( conn );
}

// run in work thread
void WorkItem::run()
{
    using namespace std;
    using connection = ServerType::connection;

    // LOG(INFO) << "WorkItem::run()";

    string::size_type startPos = req.destination.find_first_not_of('/');
    if (string::npos == startPos) {
        send_response(conn, connection::bad_request, "Invalid service name");
        return;
    } // if

    string::size_type endPos = req.destination.find_first_of("/?=&" SPACES, startPos);
    string::iterator endIt = (string::npos == endPos
                                    ? req.destination.end()
                                    : req.destination.begin() + endPos);

    string srvName( req.destination.begin() + startPos, endIt );

    if (srvName.empty()) {
        send_response(conn, connection::bad_request, "Invalid service name");
        return;
    } // if
    
    // LOG(INFO) << "Forwarding request to " << srvName;
    ServicePtr pSrv;
    if (!ServiceManager::getInstance()->getService(srvName, pSrv)) {
        // LOG(INFO) << "Service " << srvName << " not available!";
        send_response(conn, connection::service_unavailable, "Cannot find requested service");
        return;
    } // if

    pSrv->handleRequest(shared_from_this());
}


/*
 * void APIServerHandler::operator()(const ServerType::request& req,
 *         ServerType::connection_ptr conn)
 * {
 *     using namespace std;
 * 
 *     cout << "Received client request from " << req.source << endl; // source 已经包含port
 *     cout << "destination = " << req.destination << endl;  // 去除了URL port
 *     cout << "method = " << req.method << endl;
 *     cout << "http version = " << (uint32_t)(req.http_version_major)
 *         << "." << (uint32_t)(req.http_version_minor) << endl;
 *     cout << "headers:" << endl;
 *     for (const auto &header : req.headers)
 *         cout << header.name << " = " << header.value << endl;
 * 
 *     SLEEP_SECONDS(1);
 * 
 *     stringstream os;
 *     os << "Hello, from server thread " << THIS_THREAD_ID << endl << flush;
 *     conn->write(os.str());
 * }
 */


} // namespace BigRLab

