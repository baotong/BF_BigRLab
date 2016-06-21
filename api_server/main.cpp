/*
 * GLOG_logtostderr=1 ./apiserver.bin
 */
/*
 * TODO list
 */
/*
 * Tests
 * Service knn
 * loadlib ../services/knn/knn_service.so
 * service knn_star items 10 李宇春 姚明 章子怡
 * service knn_star file 10 test.txt out.txt
 * curl -i -X POST -H "Content-Type: application/json" -d '{"item":"李宇春","n":10}' http://localhost:9000/knn_star
 */
#include "api_server.h"
#include "service_manager.h"
#include "alg_mgr.h"
#include "stream_buf.h"
#include <fstream>
#include <iomanip>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <gflags/gflags.h>

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
// DEFINE_string(log_dir, "", "Sepcify dir to keep the log.");

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
    using namespace boost::interprocess;

    struct shm_remover {
        shm_remover() { shared_memory_object::remove(SHM_NAME); /* DLOG(INFO) << "shm_remover constructor"; */ }
        ~shm_remover() { shared_memory_object::remove(SHM_NAME); /* DLOG(INFO) << "shm_remover destructor"; */ }
    };

    boost::shared_ptr<shm_remover>             pShmRemover;
    boost::shared_ptr<managed_shared_memory>   pShmSegment;
    boost::shared_ptr<StreamBuf>               pStreamBuf;
    boost::shared_ptr<bufferstream>            pStream;

    if (FLAGS_b) {
        pShmRemover.reset( new shm_remover );
        pShmSegment = boost::make_shared<managed_shared_memory>(create_only, SHM_NAME, SHARED_BUF_SIZE);
        pStreamBuf.reset(pShmSegment->construct<StreamBuf>("StreamBuf")(),
                    [&](StreamBuf *p){ if (p) pShmSegment->destroy_ptr(p); });
        pStream = boost::make_shared<bufferstream>(pStreamBuf->buf, SHARED_STREAM_BUF_SIZE);
    } // if

    // auto cleanup = [&] {
        // pStream.reset();
        // pStreamBuf.reset();
        // pShmSegment.reset();
        // pShmRemover.reset();
    // };

    auto resetStream = [&] {
        pStream->clear();
        pStream->seekg(0, std::ios::beg);
        pStream->seekp(0, std::ios::beg);
    };

    auto readLine = [&](string &line)->bool {
        bool ret = false;
        if (FLAGS_b) {
            resetStream();
            scoped_lock<interprocess_mutex> lk(pStreamBuf->mtx);
            // read msg from client
            pStreamBuf->condReq.wait( lk, [&]{return pStreamBuf->reqReady;} );
            ret = getline(*pStream, line);
            pStreamBuf->reqReady = false;
        } else {
            ret = getline(cin, line);
        } // if
        return ret;
    };

    auto writeLine = [&](const string &msg) {
        if (FLAGS_b) {
            pStreamBuf->clear();
            resetStream();
            scoped_lock<interprocess_mutex> lk(pStreamBuf->mtx);
            *pStream << msg << endl << flush;
            pStreamBuf->respReady = true;
            lk.unlock();
            pStreamBuf->condResp.notify_all();
        } else {
            cout << msg << endl;
        } // if
    };

#define WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        writeLine( __write_line_stream.str() ); \
    } while (0)


    auto autorun = [] {
        ifstream ifs("autoload.conf", ios::in);
        if (!ifs)
            ERR_RET("No autorun.conf found.");

        string path;
        while (getline(ifs, path)) {
            try {
                ServiceManager::getInstance()->loadServiceLib(path);
            } catch (const std::exception &ex) {
                cerr << ex.what() << endl;
            } // try
        } // while
    };

    //!! 第一种方法编译错误，必须先 typedef
    // typedef std::map< std::string, std::function<bool(std::stringstream&) > CmdProcessTable;
    typedef std::function<bool(std::stringstream&)> CmdProcessor;
    typedef std::map< std::string, CmdProcessor > CmdProcessTable;

    auto loadLib = [&](stringstream &stream)->bool {
        string path;
        stream >> path;
        if (bad_stream(stream)) {
            writeLine("Usage: loadlib path");
            return false;
        } // if

        try {
            ServiceManager::getInstance()->loadServiceLib(path);
        } catch (const std::exception &ex) {
            writeLine(ex.what());
        } // try

        return true;
    };

    // 暂不支持此功能，需要将相关service停止并删除
    // auto rmLib = [](stringstream &stream)->bool {
        // string name;
        // stream >> name;  
        // if (bad_stream(stream))
            // return false;

        // ServiceManager::getInstance()->rmServiceLib(name);
        // return true;
    // };
    
    auto greet = [&](stringstream &stream)->bool {
        writeLine("BigRLab APIServer is running...");
        return true;
    };

    auto lsLib = [&](stringstream &stream)->bool {
        stringstream outStream;
        auto &libTable = ServiceManager::getInstance()->serviceLibs();
        boost::shared_lock< ServiceManager::ServiceLibTable > lock(libTable);

        for (const auto &v : libTable)
            outStream << v.first << "\t\t" << v.second->path << endl; 

        lock.unlock();
        outStream.flush();
        writeLine(outStream.str());
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
        writeLine(outStream.str());
        return true;
    };

    auto save = [&](stringstream &stream)->bool {
        ofstream ofs("autoload.conf", ios::out);
        if (!ofs) {
            writeLine("Cannot open autorun.conf for writting!");
            return false;
        } // if

        ServiceManager::ServiceLibTable &table = ServiceManager::getInstance()->serviceLibs();
        boost::shared_lock<ServiceManager::ServiceLibTable> lock(table);
        for (const auto &v : table)
            ofs << v.second->path << endl;
    };

    CmdProcessTable cmdTable;
    cmdTable["loadlib"] = loadLib;
    // cmdTable["rmlib"] = rmLib;
    cmdTable["lslib"] = lsLib;
    cmdTable["lsservice"] = lsService;
    cmdTable["save"] = save;
    cmdTable["hello"] = greet;

    autorun();

    cout << "BigRLab shell launched." << endl;

#define INVALID_CMD { \
        writeLine("Invalid command!"); \
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
            pSrv->handleCommand(stream);
        } else if ("quit" == cmd1) {
            writeLine("BigRLab terminated.");
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
#undef WRITE_LINE

    cout << "BigRLab terminated." << endl;
}


int main( int argc, char **argv )
{
    int retval = 0;

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
                { stop_server(); } );

        start_server();
        start_shell();

        cout << "Terminating server program..." << endl;
        stop_server();
        stop_alg_mgr();
        g_pWork.reset();
        g_pIoService->stop();
        g_pIoThrgrp->join_all();
        g_pWorkMgr->stop();
        g_pRunServerThread->join();

    } catch ( const std::exception &ex ) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        retval = -1;
    } // try

    return retval;
}

