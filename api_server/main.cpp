/*
 * GLOG_logtostderr=1 ./apiserver.bin
 */
/*
 * TODO list
 */
/*
 * Tests
 * Test Service knn
 * loadlib ../services/knn/knn_service.so
 * service knn_star items 10 李宇春 姚明 章子怡
 * service knn_star file 10 test.txt out.txt
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"item":"李宇春","n":10}' http://localhost:9000/knn_star
 *
 * Test Service article
 * loadlib ../services/article/article_service.so
 * service jieba keyword jieba_test.txt out.txt 5
 * service jieba article2vector jieba_test.txt out.txt
 * service jieba knn jieba_test.txt out.txt 10
 * service jieba knn_label jieba_test.txt out.txt 10 5
 * service jieba knn_score jieba_test.txt out.txt 10
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"reqtype":"wordseg","content":"我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。"}' http://localhost:9000/jieba
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"reqtype":"keyword","topk":5,"content":"我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。"}' http://localhost:9000/jieba
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"reqtype":"knn","n":10,"content":"我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。"}' http://localhost:9000/jieba
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"reqtype":"knn_label","n":10,"content":"我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。"}' http://localhost:9000/jieba
 *
 * To run as a service in background
 * nohup ./apiserver.bin -b > nohup.log 2>&1 &
 */
#include "api_server.h"
#include "service_manager.h"
#include "alg_mgr.h"
#include <fstream>
#include <iomanip>
#include <gflags/gflags.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace BigRLab {

class ConsoleWriter : public Writer {
public:  
    virtual bool readLine( std::string &line )
    { return std::getline(std::cin, line); }

    virtual void writeLine( const std::string &msg )
    { std::cout << boost::trim_copy(msg) << std::endl; }
};

} // namespace BigRLab

using namespace BigRLab;
using namespace std;

// IO service 除了server用之外，还可用作信号处理和定时器，不应该为server所独有
static IoServicePtr          g_pIoService;
static IoServiceWorkPtr      g_pWork;
static ThreadGroupPtr        g_pIoThrgrp;
static boost::shared_ptr< boost::thread > g_pRunServerThread;

DEFINE_int32(n_work_threads, 100, "Number of work threads on API server");
DEFINE_int32(n_io_threads, 5, "Number of io threads on API server");
DEFINE_int32(port, 9000, "API server port");
DEFINE_int32(alg_mgr_port, 9001, "Algorithm manager server port");
DEFINE_bool(b, false, "Run apiserver in background");

namespace {

static inline
bool check_above_zero(const char* flagname, gflags::int32 value)
{
    if (value <= 0) {
        std::cerr << "value of " << flagname << " must be greater than 0" << std::endl;
        return false;
    } // if
    return true;
}

static inline
bool check_port(const char *flagname, gflags::int32 value)
{
    if (value < 1024 || value > 65535) {
        std::cerr << "Port number must between 1024 and 65535" << std::endl;
        return false;
    } // if
    return true;
}

static
bool validate_port(const char *flagname, gflags::int32 value)
{
    bool ret = check_port(flagname, value);
    if (ret)
        g_nApiSvrPort = (uint16_t)value;
    return ret;
}
static bool port_dummy = gflags::RegisterFlagValidator(&FLAGS_port, &validate_port);

static
bool validate_alg_mgr_port(const char *flagname, gflags::int32 value)
{
    bool ret = check_port(flagname, value);
    if (ret)
        g_nAlgMgrPort = (uint16_t)value;
    return ret;
}
static bool alg_mgr_port_dummy = gflags::RegisterFlagValidator(&FLAGS_alg_mgr_port, &validate_alg_mgr_port);

static 
bool validate_n_work_threads(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);


} // namespace


namespace Test {

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
    g_pApiServer.reset(new APIServer(g_nApiSvrPort, FLAGS_n_io_threads, FLAGS_n_work_threads, 
                    opts, g_pIoService, g_pIoThrgrp));

    std::size_t nWorkQueLen = FLAGS_n_work_threads * 100;
    if (nWorkQueLen > 30000) {
        if (FLAGS_n_work_threads > 30000)
            nWorkQueLen = FLAGS_n_work_threads;
        else
            nWorkQueLen = 30000;
    } // if
    g_pWorkMgr.reset(new WorkManager(g_pApiServer->nWorkThreads(), nWorkQueLen) );

    if (FLAGS_b) {
        g_pCmdQue.reset( new SharedQueue<WorkItemCmd::pointer>(1) );
        g_pWriter.reset( new HttpWriter );
    } else {
        g_pWriter.reset( new ConsoleWriter );
    } // if

    cout << g_pApiServer->toString() << endl;
}

static
void stop_server() 
{
    if (FLAGS_b)
        g_pCmdQue->timed_push( WorkItemCmd::pointer(), 5000 );

    if (g_pApiServer && g_pApiServer->isRunning())
        g_pApiServer->stop();

    // DLOG(INFO) << "stop_server() done";
    //!! 在信号处理函数中不可以操作io_service, core dump
    // g_pWork.reset();
    // g_pIoService->stop();
}

static
void start_server()
{
    cout << "Launching API server..." << endl;

    g_pWorkMgr->start();

    g_pRunServerThread.reset(new boost::thread([]{
                try {
                    g_pApiServer->run();
                } catch ( const std::exception &ex ) {
                    LOG(ERROR) << "Start server fail " << ex.what();
                    g_pApiServer->stop();
                } // try
        }));

    cout << "API server ready!" << endl;

    cout << "Launching Algorithm Manager server..." << endl;
    start_alg_mgr();
    cout << "Algorithm Manager server ready!" << endl;
}

static
void start_shell()
{
    using namespace std;

#define readLine(arg)  g_pWriter->readLine(arg)

    auto autorun = [] {
        namespace fs = boost::filesystem;

        fs::path root("ServiceLibs");
        if (!fs::exists(root) || !fs::is_directory(root)) {
            cerr << "Service lib dir \"ServiceLibs\" not found!" << endl;
            return;
        } // if

        fs::recursive_directory_iterator it(root);
        fs::recursive_directory_iterator endit;
        for (; it != endit; ++it) {
            if (fs::is_regular_file(*it) && it->path().extension() == ".so") {
                cout << "Loading service lib " << it->path().filename() << endl;
                try {
                    ServiceManager::getInstance()->loadServiceLib(it->path().string());
                } catch (const std::exception &ex) {
                    cerr << ex.what() << endl;
                } // try
            } // if
        } // if
    };

    //!! 第一种方法编译错误，必须先 typedef
    // typedef std::map< std::string, std::function<bool(std::stringstream&) > CmdProcessTable;
    typedef std::function<bool(std::stringstream&)> CmdProcessor;
    typedef std::map< std::string, CmdProcessor > CmdProcessTable;

    auto greet = [&](stringstream &stream)->bool {
        WRITE_LINE("BigRLab APIServer is running...");
        return true;
    };

    auto scanlib = [&](stringstream &stream)->bool {
        autorun();
        return true;
    };

    auto lsLib = [&](stringstream &stream)->bool {
        stringstream outStream;
        auto &libTable = ServiceManager::getInstance()->serviceLibs();
        boost::shared_lock< ServiceManager::ServiceLibTable > lock(libTable);

        if (libTable.empty()) {
            WRITE_LINE("No lib loaded.");
            return true;
        } // if

        for (const auto &v : libTable)
            outStream << v.first << "\t\t" << v.second->path << endl; 

        lock.unlock();
        outStream.flush();
        WRITE_LINE(outStream.str());
        return true;
    };

    auto lsService = [&](stringstream &stream)->bool {
        vector<string> strArgs;
        string arg;
        stringstream outStream;

        while (stream >> arg)
            strArgs.push_back(arg);

        ServiceManager::ServiceTable &table = ServiceManager::getInstance()->services();
        if (strArgs.size() == 0) {
            // list all
            boost::shared_lock<ServiceManager::ServiceTable> lock(table);
            if (!table.size()) {
                outStream << "No running service." << endl;
            } else {
                for (const auto &v : table)
                    outStream << v.second->toString() << endl;
            } // if
        } else {
            // only list specified in args
            for (const auto &name : strArgs) {
                Service::pointer pSrv;
                if (ServiceManager::getInstance()->getService(name, pSrv))
                    outStream << pSrv->toString() << endl;
                else
                    outStream << "No service named " << name << " found!" << endl;
            } // for
        } // if

        outStream.flush();
        WRITE_LINE(outStream.str());
        return true;
    };

    CmdProcessTable cmdTable;
    cmdTable["scanlib"] = scanlib;
    cmdTable["lslib"] = lsLib;
    cmdTable["lsservice"] = lsService;
    cmdTable["hello"] = greet;

    autorun();

    cout << "BigRLab shell launched." << endl;

#define INVALID_CMD { \
        WRITE_LINE("Invalid command!"); \
        continue; }

#define CHECKED_INPUT(stream, val) { \
        stream >> val;              \
        if (bad_stream(stream)) \
            INVALID_CMD }

    // wait server start
    for (int i = 0; i < 20; ++i) {
        SLEEP_MILLISECONDS(100);
        if (g_pApiServer->isRunning() && g_pAlgMgrServer->isRunning())
            break;
    } // for

    string line;
    while (true) {
        if (!g_pApiServer->isRunning()) {
            cout << "API server not running!" << endl;
            break;
        } // if

        if (!g_pAlgMgrServer->isRunning()) {
            cout << "Algorithm Manager server not running!" << endl;
            break;
        } // if

        if (!FLAGS_b)
            cout << "\nBigRLab: " << flush;

        if (!readLine(line))
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
                WRITE_LINE("No service " << cmd2 << " found!");
                continue;
            } // if
            try {
                pSrv->handleCommand(stream);
            } catch (const std::exception &ex) {
                WRITE_LINE(ex.what());
            } // try
        } else if ("quit" == cmd1) {
            WRITE_LINE("BigRLab terminated.");
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
#undef readLine

    return;
}


int main( int argc, char **argv )
try {
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // log_dir 是glog定义变量
    if (FLAGS_log_dir.empty() && FLAGS_b)
        FLAGS_log_dir = ".";
    // if (!FLAGS_log_dir.empty()) {
        // google::SetLogDestination(google::GLOG_INFO, FLAGS_log_dir.c_str());
        // google::SetLogDestination(google::GLOG_WARNING, FLAGS_log_dir.c_str());
        // google::SetLogDestination(google::GLOG_ERROR, FLAGS_log_dir.c_str());
    // } // if

    // Test::test1(argc, argv);
    // Test::test();
    
    init();

    boost::asio::signal_set signals(*g_pIoService, SIGINT, SIGTERM);
    signals.async_wait( [](const boost::system::error_code& error, int signal)
            { try {stop_server();} catch (...) {} } );

    start_server();
    start_shell();

    cout << "Terminating server program..." << endl;
    SLEEP_SECONDS(1); // wait msg sent to client
    stop_server();
    stop_alg_mgr();
    g_pWork.reset();
    g_pIoService->stop();
    g_pIoThrgrp->join_all();
    g_pWorkMgr->stop();
    g_pRunServerThread->join();

    return 0;

} catch ( const std::exception &ex ) {
    cerr << "Exception caught by main: " << ex.what() << endl;
    return -1;
} // try
