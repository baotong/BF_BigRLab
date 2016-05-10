#include "service_manager.h"
#include <glog/logging.h>
#include <boost/network/utils/thread_group.hpp>
#include <boost/network/include/http/server.hpp>
#include <boost/network/uri.hpp>
#include <asio.hpp>

using namespace BigRLab;
using namespace std;

using ThreadPool = boost::network::utils::thread_pool;
using ThreadGroup = boost::network::utils::thread_group;
typedef std::shared_ptr<ThreadGroup>            ThreadGroupPtr;
typedef std::shared_ptr<asio::io_service>       IoServicePtr;
typedef std::shared_ptr<asio::io_service::work> IoServiceWorkPtr;

// TODO set by arg
static const char                   *g_cstrServerConfFileName = "conf/server.conf";
// IO service 除了server用之外，还可用作信号处理和定时器，不应该为server所独有
static IoServicePtr          g_pIoService;
static IoServiceWorkPtr      g_pWork;
static ThreadGroupPtr        g_pIoThrgrp;
static ThreadGroupPtr        g_pWorkThrgrp;

struct ServerHandler;
typedef boost::network::http::server<ServerHandler> ServerType;

struct ServerHandler {
    void operator()(const ServerType::request& req,
                    ServerType::connection_ptr conn) 
    {
        cout << "Received client request from " << req.source << endl; // source 已经包含port
        cout << "destination = " << req.destination << endl;  // 去除了URL port
        cout << "method = " << req.method << endl;
        cout << "http version = " << (uint32_t)(req.http_version_major) 
            << "." << (uint32_t)(req.http_version_minor) << endl;
        cout << "headers:" << endl;
        for (const auto &header : req.headers)
            cout << header.name << " = " << header.value << endl;

        SLEEP_SECONDS(1);

        stringstream os;
        os << "Hello, from server thread " << THIS_THREAD_ID << endl << flush;
        conn->write(os.str());
    }
};

// 正确的结束方式: gracefully cleanly terminate
/*
 * 1. server->stop();
 * 2. io_service->stop();       g_pWork.reset(); g_pIoService->stop();
 * 3. threadgroup->join_all()
 */
class APIServer {
public:
    static const uint32_t       DEFAULT_N_IO_THREADS = 5;
    static const uint32_t       DEFAULT_N_WORK_THREADS = 100;
    static const uint16_t       DEFAULT_PORT = 9000;
public:
public:
    explicit APIServer( const ServerType::options &_Opts,
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
        LOG(INFO) << "APIServer default constructor";
        options().address("0.0.0.0").reuse_address(true)
                 .io_service(m_pIoService); 
    }

    explicit APIServer( const ServerType::options &_Opts,
                        const IoServicePtr &_pIoSrv, 
                        const ThreadGroupPtr &_pThrgrp, 
                        const char *confFileName );

    ~APIServer()
    {
        stop();
        delete m_pServer;
        m_pServer = NULL;
    }

    // void setHandler( ServerHandler *_pH )
    // { m_pHandler = _pH; }
    
    void run();
    void stop();

    std::string toString() const
    {
        using namespace std;

        stringstream os;

        os << "APIServer: " << endl;
        os << "port: " << m_nPort << endl;
        os << "n_io_threads: " << m_nIoThreads << endl;
        os << "n_work_threads: " << m_nWorkThreads << endl;

        return os.str();
    }

    std::shared_ptr<asio::io_service> ioService()
    { return m_pIoService; }

    ServerType::options& options()
    { return m_Options; }

    bool isRunning() const
    { return m_bRunning; }

private:
    PropertyTable       m_mapProperties;
    uint16_t            m_nPort;
    uint32_t            m_nIoThreads;
    uint32_t            m_nWorkThreads;
    bool                m_bRunning;
    ServerType*         m_pServer;
    ServerType::options m_Options;

    std::shared_ptr<asio::io_service>           m_pIoService;
    std::shared_ptr<ThreadGroup>                m_pIoThrgrp;
};

static std::unique_ptr<APIServer>       g_pApiServer;

APIServer::APIServer( const ServerType::options &_Opts,
                      const IoServicePtr &_pIoSrv, 
                      const ThreadGroupPtr &_pThrgrp, 
                      const char *confFileName ) 
                : APIServer(_Opts, _pIoSrv, _pThrgrp)
{
    parse_config_file( confFileName, m_mapProperties );

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
    } // for

    options().port(to_string(m_nPort))
        .thread_pool(std::make_shared<ThreadPool>(m_nIoThreads, m_pIoService, m_pIoThrgrp));
}

void APIServer::run()
{
    if ( m_bRunning ) {
        cerr << "APIServer already running!" << endl;
        return;
    } // if

    m_bRunning = true;

    m_pServer = new ServerType(m_Options);
    m_pServer->run();
}

void APIServer::stop()
{
    m_bRunning = false;
    m_pServer->stop();   // 是异步的 async_stop
    // 不能在这里join这些 io thread 需要io_service结束后
    // m_pIoThrgrp->join_all();
}

namespace Test {

    void print_PropertyTable( const PropertyTable &ppt )
    {
        for (const auto &v : ppt) {
            cout << v.first << ": ";
            for (const auto &vv : v.second)
                cout << vv << "; ";
            cout << endl;
        } // for
    }

    void test1(int argc, char **argv)
    {
        ServiceManager::pointer pServiceMgr = ServiceManager::getInstance();

        PropertyTable ppt;
        parse_config_file( argv[1], ppt );
        print_PropertyTable( ppt );

        exit(0);
    }

} // namespace Test

static
void init()
{
    g_pIoService = std::make_shared<asio::io_service>();
    g_pWork = std::make_shared<asio::io_service::work>(std::ref(*g_pIoService));
    g_pIoThrgrp = std::make_shared<ThreadGroup>();
    g_pWorkThrgrp = std::make_shared<ThreadGroup>();

    ServerHandler handler;
    ServerType::options opts(handler);
    g_pApiServer.reset(new APIServer(opts, g_pIoService, g_pIoThrgrp, g_cstrServerConfFileName));
    cout << g_pApiServer->toString() << endl;
}

static
void shutdown() 
{
    if (g_pApiServer && g_pApiServer->isRunning())
        g_pApiServer->stop();
    //!! 在信号处理函数中不可以操作io_service, core dump
    // g_pWork.reset();
    // g_pIoService->stop();
}

int main( int argc, char **argv )
{
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);

        // Test::test1(argc, argv);
        
        init();

        asio::signal_set signals(*g_pIoService, SIGINT, SIGTERM);
        signals.async_wait( [](const std::error_code& error, int signal)
                { shutdown(); } );

        g_pApiServer->run();

        cout << "Terminating server program..." << endl;
        g_pWork.reset();
        g_pIoService->stop();
        g_pIoThrgrp->join_all();
        g_pWorkThrgrp->join_all();

    } catch ( const std::exception &ex ) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        // shutdown();
        // g_pWork.reset();
        // g_pIoService->stop();
        exit(-1);
    } // try

    return 0;
}

