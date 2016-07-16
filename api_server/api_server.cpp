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

Writer::pointer              g_pWriter;

boost::shared_ptr<APIServer> g_pApiServer;
uint16_t                     g_nApiSvrPort = 0;

boost::shared_ptr< SharedQueue<WorkItemCmd::pointer> >    g_pCmdQue;


APIServer::APIServer( uint16_t _SvrPort,
        uint32_t _nIoThreads, uint32_t _nWorkThreads,
        const ServerType::options &_Opts,
        const IoServicePtr &_pIoSrv,
        const ThreadGroupPtr &_pThrgrp )
      : m_pServer(NULL)
      , m_bRunning(false)
      , m_nPort( _SvrPort )
      , m_nIoThreads( _nIoThreads )
      , m_nWorkThreads( _nWorkThreads )
      , m_Options(std::move(_Opts))
      , m_pIoService(_pIoSrv)
      , m_pIoThrgrp(_pThrgrp)
{
    options().address("0.0.0.0").reuse_address(true)
        .io_service(m_pIoService).port(to_string(m_nPort))
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
        RESPONSE_ERROR(conn, ServerType::connection::bad_request, APIServer::BAD_REQUEST, 
                    "Only accept POST request.");
        return;
    } // if

    size_t nRead = 0;
    bool   isCmd = false;
    for (const auto &v : req.headers) {
        if (boost::iequals(v.name, "Content-Type")) {
            if (boost::iequals(v.value, "command"))
                isCmd = true;
            // if (!boost::iequals(v.value, "application/json")) {
                // LOG(ERROR) << "Wrong request from " << req.source << ", only accept json content.";
                // send_response(conn, ServerType::connection::bad_request, "Content-Type is not json\n");
                // return;
            // } // if
        } // if
        if (boost::iequals(v.name, "Content-Length")) {
            // nRead = boost::lexical_cast<size_t>(v.value);
            if (!boost::conversion::try_lexical_convert(v.value, nRead)) {
                RESPONSE_ERROR(conn, ServerType::connection::bad_request, APIServer::BAD_REQUEST, 
                        "Content-Length not valid.");
                return;
            } // if
        } // if
    } // for

    if (isCmd) {
        auto pWorkCmd = boost::make_shared<WorkItemCmd>(req, conn, nRead);
        pWorkCmd->readBody(conn);
        return;
    } // if

    WorkItemPtr pWork( new WorkItem(req, conn, nRead) );
    pWork->readBody(conn);
} catch (const std::exception &ex) {
    LOG(ERROR) << "APIServerHandler exception " << ex.what();
}

void WorkItemCmd::readBody( const ServerType::connection_ptr &conn )
{
    using namespace std;

    if (left2Read) {
        conn->read( std::bind(&WorkItem::handleRead, shared_from_this(),
               placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4) );
    } else {
        if (g_pCmdQue) {
            if(!g_pCmdQue->timed_push( boost::static_pointer_cast<WorkItemCmd>(shared_from_this()), TIMEOUT )) {
                RESPONSE_MSG(conn, "System busy!");
            } // if
        } else {
            RESPONSE_MSG(conn, "ApiServer not running in background mode.");
        } // if
    } // if
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
        RESPONSE_ERROR(conn, ServerType::connection::bad_request, APIServer::BAD_REQUEST, 
                "Invalid service name.");
        return;
    } // if

    string::size_type endPos = req.destination.find_first_of("/?=&" SPACES, startPos);
    string::iterator endIt = (string::npos == endPos
                                    ? req.destination.end()
                                    : req.destination.begin() + endPos);

    string srvName( req.destination.begin() + startPos, endIt );

    if (srvName.empty()) {
        RESPONSE_ERROR(conn, ServerType::connection::bad_request, APIServer::BAD_REQUEST, 
                "Invalid service name.");
        return;
    } // if
    
    // LOG(INFO) << "Forwarding request to " << srvName;
    ServicePtr pSrv;
    if (!ServiceManager::getInstance()->getService(srvName, pSrv)) {
        // LOG(INFO) << "Service " << srvName << " not available!";
        RESPONSE_ERROR(conn, ServerType::connection::service_unavailable, APIServer::SERVICE_UNAVAILABLE, 
                "Requested service " << srvName << " not available.");
        return;
    } // if

    pSrv->handleRequest(shared_from_this());
}

bool HttpWriter::readLine( std::string &line )
{
    if (!g_pCmdQue)
        return false;

    m_pWorkCmd = g_pCmdQue->pop();
    if (!m_pWorkCmd)
        return false;

    line.swap(m_pWorkCmd->body);

    return true;
}

void HttpWriter::writeLine( const std::string &msg )
{
    if (m_pWorkCmd) {
        RESPONSE_MSG(m_pWorkCmd->conn, boost::trim_copy(msg));
        m_pWorkCmd.reset();
    } // if
}

} // namespace BigRLab

