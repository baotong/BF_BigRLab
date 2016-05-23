/*
 * GLOG_logtostderr=1 ./apiserver.bin -conf ../../conf/server.conf
 */
/*
 * TODO list
 * 1. alg 的新加入和离开 algmgr要主动通知apiserver，然后转发给具体的service
 */
/*
 * Tests
 * Service knn
 * addservice ../services/knn/knn_service.so
 * service knn_star items 10 李宇春 姚明
 */
#include "api_server.h"
#include "service_manager.h"
#include <gflags/gflags.h>

using namespace BigRLab;
using namespace std;

// IO service 除了server用之外，还可用作信号处理和定时器，不应该为server所独有
static IoServicePtr          g_pIoService;
static IoServiceWorkPtr      g_pWork;
static ThreadGroupPtr        g_pIoThrgrp;
static boost::shared_ptr< boost::thread > g_pRunServerThread;

DEFINE_string(conf, "", "server conf file path");

namespace {

static inline
bool check_not_empty(const char* flagname, const std::string &value) 
{
    if (value.empty()) {
        cerr << "value of " << flagname << " cannot be empty" << endl;
        return false;
    } // if
    return true;
}

static bool validate_conf(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool conf_dummy = gflags::RegisterFlagValidator(&FLAGS_conf, &validate_conf);

} // namespace


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

    void test()
    {
        using namespace std;

        SharedQueue<int> que;

        boost::thread thr1([&]{
                SLEEP_SECONDS(7);    
                que.push(501);
            });

        int x;
        if (que.timed_pop(x, 5000))
            cout << "Successfully get value " << x << endl;
        else
            cout << "timeout" << endl;

        thr1.join();

        exit(0);
    }

} // namespace Test

static
void init()
{
    // LOG(INFO) << "Initializing...";
    g_pIoService = boost::make_shared<boost::asio::io_service>();
    g_pWork = boost::make_shared<boost::asio::io_service::work>(std::ref(*g_pIoService));
    g_pIoThrgrp = boost::make_shared<ThreadGroup>();

    APIServerHandler handler;
    ServerType::options opts(handler);
    g_pApiServer.reset(new APIServer(opts, g_pIoService, g_pIoThrgrp, FLAGS_conf.c_str()));
    // g_pWorkMgr.reset(new WorkManager<WorkItemBase>(g_pApiServer->nWorkThreads()) );
    WorkManager::init(g_pApiServer->nWorkThreads());
    g_pWorkMgr = WorkManager::getInstance();
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
                try {
                    g_pApiServer->run();
                } catch ( const std::exception &ex ) {
                    LOG(ERROR) << "Start server fail " << ex.what();
                    g_pApiServer->stop();
                } // try
        }));
}

static
void start_shell()
{
    using namespace std;
    //!! 第一种方法编译错误，必须先 typedef
    // typedef std::map< std::string, std::function<bool(std::stringstream&) > CmdProcessTable;
    typedef std::function<bool(std::stringstream&)> CmdProcessor;
    typedef std::map< std::string, CmdProcessor > CmdProcessTable;

    auto addService = [](stringstream &stream)->bool {
        vector<string> strArgs;
        string arg;

        while (stream >> arg)
            strArgs.push_back(arg);

        if (strArgs.size() == 0)
            return false;

        vector<char*> cstrArgs( strArgs.size() );
        for (size_t i = 0; i < strArgs.size(); ++i)
            cstrArgs[i] = const_cast<char*>(strArgs[i].c_str());

        try {
            ServiceManager::getInstance()->addService( (int)(cstrArgs.size()), &cstrArgs[0] );
        } catch (const std::exception &ex) {
            cout << ex.what() << endl;
        } // try

        return true;
    };

    auto lsService = [](stringstream &stream)->bool {
        return true;
    };

    CmdProcessTable cmdTable;
    cmdTable["addservice"] = addService;
    cmdTable["lsservice"] = lsService;
    // TODO rmservice, lsservice

    string line;

    cout << "BigRLab shell launched." << endl;

#define INVALID_CMD { \
        cout << "Invalid command!" << endl; \
        continue; }

#define CHECKED_INPUT(stream, val) { \
        stream >> val;              \
        if (bad_stream(stream)) \
            INVALID_CMD }

    // wait server start
    for (int i = 0; i < 20; ++i) {
        SLEEP_MILLISECONDS(100);
        if (g_pApiServer->isRunning())
            break;
    } // for

    while (true) {
        if (!g_pApiServer->isRunning()) {
            cout << "Server has been shutdown!" << endl;
            break;
        } // if

        cout << "\nBigRLab: " << flush;
        if (!getline(cin, line))
            break;

        if (line.empty())
            continue;

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
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        // Test::test1(argc, argv);
        // Test::test();
        
        init();

        boost::asio::signal_set signals(*g_pIoService, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal)
                { stop_server(); } );

        start_server();
        start_shell();

        cout << "Terminating server program..." << endl;
        stop_server();
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

