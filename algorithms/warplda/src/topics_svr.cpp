/*
 * GLOG_logtostderr=1 ./topics_svr -model train.model -vocab train.vocab -algname topic_eval -algmgr localhost:9001 -port 10080
 * GLOG_logtostderr=1 ./topics_svr -model train.model -vocab train.vocab -algname topic_eval -algmgr localhost:9001 -port 10080 -n_work_threads 10
 */
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
// #include <glog/logging.h>
#include <gflags/gflags.h>

#include "rpc_module.h"
#include "common.hpp"
#include "alg_common.hpp"
#include "get_local_ip.hpp"
#include "AlgMgrService.h"
#include "register_svr.hpp"
#include "TopicServiceHandler.h"

#define SERVICE_LIB_NAME        "warplda"

#define TIMER_REJOIN            15          // 15s

DEFINE_string(model, "", "model file");
DEFINE_string(vocab, "", "vocabulary file");
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(addr, "", "Address of this algorithm server, use system detected if not specified.");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 0, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 0, "Number of io threads on RPC server");
DEFINE_int32(n_inst, 0, "Number of topic model object instances");

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

typedef BigRLab::ThriftServer< Topics::TopicServiceIf, 
            Topics::TopicServiceProcessor >   TopicsAlgSvr;
static AlgMgrClient::Pointer                  g_pAlgMgrClient;
static TopicsAlgSvr::Pointer                  g_pThisServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo> g_pSvrInfo;

static std::unique_ptr< boost::asio::deadline_timer >      g_Timer;

std::unique_ptr< LockFreeQueue<TopicModule*> >      g_queTopicModules;
static std::vector< std::shared_ptr<TopicModule> >  s_arrTopicModules;


namespace {
using namespace std;
static inline
bool check_above_zero(const char* flagname, gflags::int32 value)
{
    if (value <= 0) {
        cerr << "value of " << flagname << " must be greater than 0" << endl;
        return false;
    } // if
    return true;
}

static inline
bool check_not_empty(const char* flagname, const std::string &value)
{
    if (value.empty()) {
        cerr << "value of " << flagname << " cannot be empty" << endl;
        return false;
    } // if
    return true;
}

static bool validate_model(const char* flagname, const std::string &value)
{ return check_not_empty(flagname, value); }
static const bool model_dummy = gflags::RegisterFlagValidator(&FLAGS_model, &validate_model);

static bool validate_vocab(const char* flagname, const std::string &value)
{ return check_not_empty(flagname, value); }
static const bool vocab_dummy = gflags::RegisterFlagValidator(&FLAGS_vocab, &validate_vocab);

static bool validate_algname(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool algname_dummy = gflags::RegisterFlagValidator(&FLAGS_algname, &validate_algname);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    if (!check_not_empty(flagname, value))
        return false;

    string::size_type pos = value.find_last_of(':');
    if (string::npos == pos) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    g_strAlgMgrAddr = value.substr(0, pos);
    if (g_strAlgMgrAddr.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    string strPort = value.substr(pos + 1, string::npos);
    if (strPort.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!boost::conversion::try_lexical_convert(strPort, g_nAlgMgrPort)) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!g_nAlgMgrPort) {
        cerr << "Invalid port number specified by arg " << flagname << endl;
        return false;
    } // if

    return true;
}
static const bool algmgr_dummy = gflags::RegisterFlagValidator(&FLAGS_algmgr, &validate_algmgr);

static
bool validate_port(const char *flagname, gflags::int32 value)
{
    if (value < 1024 || value > 65535) {
        cerr << "Invalid port number! port number must be in [1025, 65535]" << endl;
        return false;
    } // if
    g_nThisPort = (uint16_t)FLAGS_port;
    return true;
}
static const bool port_dummy = gflags::RegisterFlagValidator(&FLAGS_port, &validate_port);
} // namespace

static
void stop_server()
{
    if (g_pThisServer)
        g_pThisServer->stop();
}

static
void stop_client()
{
    if (g_pAlgMgrClient) {
        (*g_pAlgMgrClient)()->rmSvr(FLAGS_algname, *g_pSvrInfo);
        g_pAlgMgrClient->stop();
    } // if
}

static
void rejoin(const boost::system::error_code &ec)
{
    // DLOG(INFO) << "rejoin timer called";

    int waitTime = TIMER_REJOIN;

    if (g_bLoginSuccess) {
        try {
            if (!g_pAlgMgrClient->isRunning())
                g_pAlgMgrClient->start(50, 300);
            (*g_pAlgMgrClient)()->addSvr(FLAGS_algname, *g_pSvrInfo);
        } catch (const std::exception &ex) {
            LOG(ERROR) << "Connection with apiserver lost, re-connecting...";
            g_pAlgMgrClient.reset();
            g_pAlgMgrClient = boost::make_shared< AlgMgrClient >(g_strAlgMgrAddr, g_nAlgMgrPort);
            waitTime = 5;
        } // try
    } // if

    g_Timer->expires_from_now(boost::posix_time::seconds(waitTime));
    g_Timer->async_wait(rejoin);
}

static
void start_rpc_service()
{
    using namespace std;

    // start client to alg_mgr
    // 需要在单独线程中执行，防止和alg_mgr形成死锁
    // 这里调用addSvr，alg_mgr那边调用 Service::addServer
    // 尝试连接本server，而本server还没有启动
    cout << "Registering server..." << endl;
    g_pAlgMgrClient = boost::make_shared< AlgMgrClient >(g_strAlgMgrAddr, g_nAlgMgrPort);
    boost::thread register_thr(register_svr, g_pAlgMgrClient.get(), FLAGS_algname, g_pSvrInfo.get());
    register_thr.detach();

    // start this alg server
    cout << "Launching alogrithm server... " << endl;
    boost::shared_ptr< Topics::TopicServiceIf > 
            pHandler = boost::make_shared< Topics::TopicServiceHandler >();
    // Test::test1(pHandler);
    g_pThisServer = boost::make_shared< TopicsAlgSvr >(pHandler, g_nThisPort, 
            FLAGS_n_io_threads, FLAGS_n_work_threads);
    try {
        g_pThisServer->start(); //!! NOTE blocking until quit
    } catch (const std::exception &ex) {
        cerr << "Start this alg server fail, " << ex.what() << endl;
        std::raise(SIGTERM);
        return;
    } // try
}

static
void service_init()
{
    using namespace std;

    // check vocab
    {
        ifstream ifs(FLAGS_vocab, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open vocab file " << FLAGS_vocab);
    }

    // check model
    {
        ifstream ifs(FLAGS_model, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open model file " << FLAGS_model);
    }

    // adjust n_work_threads and n_io_threads
    int hwConcurrency = (int)(std::thread::hardware_concurrency());
    if (hwConcurrency < 2) hwConcurrency = 2;
    if (FLAGS_n_work_threads <= 0) {
        FLAGS_n_work_threads = hwConcurrency;
        LOG(INFO) << "Reset n_work_threads = " << FLAGS_n_work_threads;
    } // if
    if (FLAGS_n_io_threads <= 0) {
        FLAGS_n_io_threads = std::max(2, FLAGS_n_work_threads / 2);
        LOG(INFO) << "Reset n_io_threads = " << FLAGS_n_io_threads;
    } else if (FLAGS_n_io_threads > FLAGS_n_work_threads) {
        FLAGS_n_io_threads = FLAGS_n_work_threads;
        LOG(INFO) << "Reset n_io_threads = " << FLAGS_n_io_threads;
    } // if

    // adjust n_inst
    if (FLAGS_n_inst <= 0 || FLAGS_n_inst > FLAGS_n_work_threads) {
        LOG(INFO) << "Adjust -n_inst from old value " << FLAGS_n_inst 
            << " to new value -n_work_threads " << FLAGS_n_work_threads;
        FLAGS_n_inst = FLAGS_n_work_threads;
    } // if

    g_queTopicModules.reset(new LockFreeQueue<TopicModule*>(FLAGS_n_inst));
    s_arrTopicModules.reserve(FLAGS_n_inst);
    for (int i = 0; i < FLAGS_n_inst; ++i) {
        auto pInst = std::make_shared<TopicModule>(FLAGS_vocab, FLAGS_model);
        s_arrTopicModules.emplace_back(pInst);
        g_queTopicModules->push(pInst.get());
        // DLOG(INFO) << "nTopics = " << pInst->nTopics();
    } // for

    try {
        g_strThisAddr = get_local_ip(FLAGS_algmgr);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "get_local_ip() fail! " << ex.what();
        std::raise(SIGTERM);
        return;
    } // try

    cout << "Detected local ip is " << g_strThisAddr << endl;

    g_pSvrInfo = boost::make_shared<BigRLab::AlgSvrInfo>();
    g_pSvrInfo->addr = g_strThisAddr;
    g_pSvrInfo->port = (int16_t)g_nThisPort;
    g_pSvrInfo->maxConcurrency = FLAGS_n_work_threads;
    g_pSvrInfo->serviceName = SERVICE_LIB_NAME;
}

static
void do_service_routine()
{
    using namespace std;
    cout << "Initializing service..." << endl;
    service_init();
    cout << "Starting rpc service..." << endl;
    start_rpc_service();
}

int main(int argc, char **argv)
{
    try {
        // google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        // install signal handler
        boost::asio::io_service     io_service;
        auto pIoServiceWork = boost::make_shared< boost::asio::io_service::work >(std::ref(io_service));

        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal) { 
            if (g_Timer)
                g_Timer->cancel();
            try { stop_server(); } catch (...) {}
        } );

        g_Timer.reset(new boost::asio::deadline_timer(std::ref(io_service)));
        g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_REJOIN));
        g_Timer->async_wait(rejoin);

        auto io_service_thr = boost::thread( [&]{ io_service.run(); } );

        do_service_routine();

        pIoServiceWork.reset();
        io_service.stop();
        if (io_service_thr.joinable())
            io_service_thr.join();

        stop_client();

        LOG(INFO) << argv[0] << " done!";

    } catch (const std::exception &ex) {
        std::cerr << "Exception caught by main: " << ex.what() << std::endl;
        return 0;
    } // try

    return 0;
}
