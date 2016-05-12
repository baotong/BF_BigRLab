#include "api_server.h"
#include "service_manager.h"
#include <glog/logging.h>


using namespace BigRLab;
using namespace std;

// TODO set by arg
static const char            *g_cstrServerConfFileName = "conf/server.conf";
// IO service 除了server用之外，还可用作信号处理和定时器，不应该为server所独有
static IoServicePtr          g_pIoService;
static IoServiceWorkPtr      g_pWork;
static ThreadGroupPtr        g_pIoThrgrp;
static boost::shared_ptr<APIServer>       g_pApiServer;

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
    g_pIoService = boost::make_shared<boost::asio::io_service>();
    g_pWork = boost::make_shared<boost::asio::io_service::work>(std::ref(*g_pIoService));
    g_pIoThrgrp = boost::make_shared<ThreadGroup>();

    APIServerHandler handler;
    ServerType::options opts(handler);
    g_pApiServer.reset(new APIServer(opts, g_pIoService, g_pIoThrgrp, g_cstrServerConfFileName));
    g_pWorkMgr.reset(new WorkManager<WorkItem>(g_pApiServer->nWorkThreads()) );
    cout << g_pApiServer->toString() << endl;
}

static
void stop_server() 
{
    if (g_pApiServer && g_pApiServer->isRunning())
        g_pApiServer->stop();
    //!! 在信号处理函数中不可以操作io_service, core dump
    // g_pWork.reset();
    // g_pIoService->stop();
}

static
void start_server()
{
    g_pWorkMgr->start();
    g_pApiServer->run();
}


int main( int argc, char **argv )
{
    try {
        google::InitGoogleLogging(argv[0]);

        // Test::test1(argc, argv);
        
        init();

        boost::asio::signal_set signals(*g_pIoService, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal)
                { stop_server(); } );

        start_server();

        cout << "Terminating server program..." << endl;
        g_pWork.reset();
        g_pIoService->stop();
        g_pIoThrgrp->join_all();
        g_pWorkMgr->stop();

    } catch ( const std::exception &ex ) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

