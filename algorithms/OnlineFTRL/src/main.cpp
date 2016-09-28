#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <chrono>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <boost/filesystem.hpp>
#include "rpc_module.h"
#include "common.hpp"
#include "alg_common.hpp"
#include "get_local_ip.hpp"
#include "AlgMgrService.h"
#include "register_svr.hpp"
#include "FtrlServiceHandler.h"

#define SERVICE_LIB_NAME        "ftrl"

#define TIMER_REJOIN            15          // 15s

DEFINE_int32(epoch, 1, "Number of iteration, default 1");
DEFINE_string(model, "", "model file");
DEFINE_int64(update_cnt, 1000, "Number of data record for updating model (default 100)");        // TODO
DEFINE_int64(data_life, 24, "How long time does the server keep the data (in hours) (default 24)");
DEFINE_int64(data_buf, 1000000, "data buf size for updating model (default 1000000)");
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(addr, "", "Address of this algorithm server, use system detected if not specified.");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

typedef BigRLab::ThriftServer< FTRL::FtrlServiceIf, FTRL::FtrlServiceProcessor >  FtrlAlgSvr;
static AlgMgrClient::Pointer                                                      g_pAlgMgrClient;
static FtrlAlgSvr::Pointer                                                        g_pThisServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo>                                     g_pSvrInfo;

static std::unique_ptr< boost::asio::deadline_timer >                             g_Timer;

std::unique_ptr<FtrlModel>       g_pFtrlModel;
std::unique_ptr<DB>              g_pDb;


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

static bool validate_algname(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool algname_dummy = gflags::RegisterFlagValidator(&FLAGS_algname, &validate_algname);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    using namespace std;

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

static 
bool validate_n_work_threads(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);

static bool validate_model(const char* flagname, const std::string &value)
{ return check_not_empty(flagname, value); }
static const bool model_dummy = gflags::RegisterFlagValidator(&FLAGS_model, &validate_model);

static bool validate_epoch(const char* flagname, gflags::int32 value)
{ return check_above_zero(flagname, value); }
static const bool epoch_dummy = gflags::RegisterFlagValidator(&FLAGS_epoch, &validate_epoch);

static bool validate_update_cnt(const char* flagname, gflags::int64 value)
{ return check_above_zero(flagname, value); }
static const bool update_cnt_dummy = gflags::RegisterFlagValidator(&FLAGS_update_cnt, &validate_update_cnt);

static bool validate_data_life(const char* flagname, gflags::int64 value)
{ return check_above_zero(flagname, value); }
static const bool data_life_dummy = gflags::RegisterFlagValidator(&FLAGS_data_life, &validate_data_life);

static bool validate_data_buf(const char* flagname, gflags::int64 value)
{ return check_above_zero(flagname, value); }
static const bool data_buf_dummy = gflags::RegisterFlagValidator(&FLAGS_data_buf, &validate_data_buf);
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
void service_init()
{
    using namespace std;

    // check -model
    {
        ifstream ifs(FLAGS_model, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot read model file " << FLAGS_model << " specified by -model");
    }
    g_pFtrlModel.reset(new FtrlModel);
    g_pFtrlModel->init(FLAGS_model);

    g_pDb.reset(new DB);
    
    // TODO timer
    
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
void start_rpc_service()
{
    using namespace std;

    cout << "Registering server..." << endl;
    g_pAlgMgrClient = boost::make_shared< AlgMgrClient >(g_strAlgMgrAddr, g_nAlgMgrPort);
    boost::thread register_thr(register_svr, g_pAlgMgrClient.get(), FLAGS_algname, g_pSvrInfo.get());
    register_thr.detach();

    // start this alg server
    cout << "Launching alogrithm server... " << endl;
    boost::shared_ptr< FTRL::FtrlServiceIf > 
            pHandler = boost::make_shared< FTRL::FtrlServiceHandler >();
    g_pThisServer = boost::make_shared< FtrlAlgSvr >(pHandler, g_nThisPort, 
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
    using namespace std;

    // Test::test1();
    // return 0;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        boost::asio::io_service     io_service;
        auto pIoServiceWork = boost::make_shared< boost::asio::io_service::work >(std::ref(io_service));

        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal) { 
            if (g_Timer)
                g_Timer->cancel();
            stop_server(); 
        } );

        g_Timer.reset(new boost::asio::deadline_timer(std::ref(io_service)));
        g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_REJOIN));
        g_Timer->async_wait(rejoin);

        auto io_service_thr = boost::thread( [&]{ io_service.run(); } );

        do_service_routine();

        pIoServiceWork.reset();
        io_service.stop();
        io_service_thr.join();

        stop_client();

        LOG(INFO) << argv[0] << " done!";

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        return -1;
    } // try

    return 0;
}


#if 0
namespace Test {
using namespace std;
void test1()
{
    // time_t start = std::time(0);
    // SLEEP_SECONDS(1);
    // cout << "Wall time passed: " << std::difftime(std::time(0), start) << endl;
    // time_t maxTime = std::numeric_limits<time_t>::max();
    // cout << maxTime << endl;
    // cout << std::numeric_limits<double>::max() << endl;
    cout << sizeof(std::shared_ptr<string>) << endl;
    cout << sizeof(string::iterator) << endl;
    cout << sizeof(std::map<string,int>::iterator) << endl;
}
} // namespace Test

#endif

