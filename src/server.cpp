#include "service_manager.h"
#include <glog/logging.h>
#include <boost/network/utils/thread_group.hpp>
#include <boost/network/include/http/server.hpp>
#include <boost/network/uri.hpp>

using namespace BigRLab;
using namespace std;

// TODO set by arg
static const char                   *g_cstrServerConfFileName = "conf/server.conf";

struct ServerHandler;
typedef boost::network::http::server<ServerHandler> ServerType;

struct ServerHandler {
    void operator()(const ServerType::request& req,
                    ServerType::connection_ptr conn) 
    {
    }
};

class APIServer {
public:
    static const uint32_t       DEFAULT_N_IO_THREADS = 5;
    static const uint32_t       DEFAULT_N_WORK_THREADS = 100;
    static const uint16_t       DEFAULT_PORT = 9000;
public:
    using ThreadPool = boost::network::utils::thread_pool;
    using ThreadGroup = boost::network::utils::thread_group;

public:
    APIServer() : m_pServer(NULL)
                , m_nPort( DEFAULT_PORT )
                , m_nIoThreads( DEFAULT_N_IO_THREADS )
                , m_nWorkThreads( DEFAULT_N_WORK_THREADS )
    {}

    explicit APIServer( const char *confFileName );

    ~APIServer()
    {
        stop();
        delete m_pServer;
        m_pServer = NULL;
    }

    // void setHandler( ServerHandler *_pH )
    // { m_pHandler = _pH; }
    
    void run( ServerHandler &handler );
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

private:
    PropertyTable       m_mapProperties;
    uint16_t            m_nPort;
    uint32_t            m_nIoThreads;
    uint32_t            m_nWorkThreads;
    bool                m_bRunning;
    ServerType*         m_pServer;

    std::shared_ptr<asio::io_service>           m_pIoService;
    std::shared_ptr<asio::io_service::work>     m_pWork;
    std::shared_ptr<ThreadGroup>                m_pIoThrgrp;
};

static std::unique_ptr<APIServer>       g_pApiServer;

APIServer::APIServer( const char *confFileName ) 
            : m_pServer(NULL)
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
}

void APIServer::run( ServerHandler &handler )
{
    m_pIoThrgrp = std::make_shared<ThreadGroup>();
    m_pIoService = std::make_shared<asio::io_service>();
    m_pWork = std::make_shared<asio::io_service::work>(std::ref(*m_pIoService));
    
    ServerType::options     opts(handler);
    opts.address("0.0.0.0").port(to_string(m_nPort))
        .io_service(m_pIoService)
        .reuse_address(true)
        .thread_pool(std::make_shared<ThreadPool>(m_nIoThreads, m_pIoService, m_pIoThrgrp));

    m_pServer = new ServerType(opts);
    m_pServer->run();
}

void APIServer::stop()
{
    m_pServer->stop();
    m_pWork.reset();
    m_pIoService->stop();
    m_pIoThrgrp->join_all();
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
    g_pApiServer.reset( new APIServer(g_cstrServerConfFileName) );
}


int main( int argc, char **argv )
{
    try {
        google::InitGoogleLogging(argv[0]);

        // Test::test1(argc, argv);
        
        init();

    } catch ( const std::exception &ex ) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

