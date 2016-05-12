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
static boost::shared_ptr< boost::thread > g_pRunServerThread;

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
    cout << "Launching api server..." << endl;

    g_pWorkMgr->start();

    g_pRunServerThread.reset(new boost::thread([]{
                g_pApiServer->run();
        }));
}

static
void start_shell()
{
    //!! 第一种方法编译错误，必须先 typedef
    // typedef std::map< std::string, std::function<bool(std::stringstream&) > CmdProcessTable;
    typedef std::function<bool(std::stringstream&)> CmdProcessor;
    typedef std::map< std::string, CmdProcessor > CmdProcessTable;

    auto addService = [](stringstream &stream)->bool {
        string conf;
        stream >> conf;
        
        if (bad_stream(stream))
            return false;

        try {
            ServiceManager::getInstance()->addService(conf.c_str());
        } catch (const std::exception &ex) {
            cout << ex.what() << endl;
        } // try

        return true;
    };

    CmdProcessTable cmdTable;
    cmdTable["addservice"] = addService;

    string line;

    cout << "BigRLab shell launched." << endl;

#define INVALID_CMD { \
        cout << "Invalid command!" << endl; \
        continue; }

#define CHECKED_INPUT(stream, val) { \
        stream >> val;              \
        if (bad_stream(stream)) \
            INVALID_CMD }

    while (true) {
        if (!g_pApiServer->isRunning()) {
            cout << "Server has been shutdown!" << endl;
            break;
        } // if

        cout << "\nBigRLab: " << flush;
        if (!getline(cin, line))
            break;

        stringstream stream(line);
        string cmd1, cmd2;
        CHECKED_INPUT(stream, cmd1)
        if ("service" == cmd1) {
            CHECKED_INPUT(stream, cmd2)
            ServicePtr pSrv;
            if (!ServiceManager::getInstance()->getService(cmd2, pSrv)) {
                cout << "No service " << cmd2 << " found!" << endl;
                continue;
            } // if
            pSrv->handleCommand(stream);
        } else if ("quit" == cmd1) {
            cout << "BigRLab shell terminated." << endl;
            return;
        } else {
            auto it = cmdTable.find(cmd1);
            if (it == cmdTable.end())
                INVALID_CMD
            else if (!it->second(stream))
                INVALID_CMD
        } // if
    } // while

#undef INVALID_CMD
#undef CHECKED_INPUT

    cout << "BigRLab shell terminated." << endl;
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
        start_shell();

        cout << "Terminating server program..." << endl;
        g_pWork.reset();
        g_pIoService->stop();
        g_pIoThrgrp->join_all();
        g_pWorkMgr->stop();
        g_pRunServerThread->join();

    } catch ( const std::exception &ex ) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

