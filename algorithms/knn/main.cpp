/*
 * BUILD:
 * GLOG_logtostderr=1 ./wordknn.bin -build -idata  knn_test/model.vec -ntrees 10 -idx knn_test/index.ann -wt knn_test/words_table.txt
 * LOAD & Start Server
 * GLOG_logtostderr=1 ./wordknn.bin -idata  knn_test/model.vec -idx knn_test/index.ann -wt knn_test/words_table.txt -algname knn_test -algmgr localhost:9001 -port 10080
 * Online test:
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"item":"李宇春","n":10}' http://localhost:9000/knn_test
 */
#include <iostream>
#include <fstream>
#include <cctype>
#include <csignal>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "rpc_module.h"
#include "common.hpp"
#include "alg_common.hpp"
#include "get_local_ip.hpp"
#include "register_svr.hpp"
#include "AlgMgrService.h"
#include "KnnServiceHandler.h"

#define SERVICE_LIB_NAME        "knn"
#define TIMER_REJOIN            15          // 15s

// args for ann
DEFINE_bool(build, false, "work mode build or nobuild");
DEFINE_string(idata, "", "input data filename");
DEFINE_int32(nfields, 0, "num of fields in input data file.");
DEFINE_int32(ntrees, 0, "num of trees to build.");
DEFINE_string(idx, "", "filename to save the tree.");
DEFINE_string(wt, "", "filename to save words table.");
// args for server
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;
typedef BigRLab::ThriftServer< KNN::KnnServiceIf, KNN::KnnServiceProcessor > KnnAlgServer;
static AlgMgrClient::Pointer                  g_pAlgMgrClient;
static KnnAlgServer::Pointer                  g_pThisServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo> g_pSvrInfo;

static std::unique_ptr< boost::asio::deadline_timer >                             g_Timer;

boost::shared_ptr<KNN::WordAnnDB> g_pWordAnnDB;

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

static bool validate_idata(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool idata_dummy = gflags::RegisterFlagValidator(&FLAGS_idata, &validate_idata);

// static bool validate_nfields(const char* flagname, gflags::int32 value) 
// { return check_above_zero(flagname, value); }
// static const bool nfields_dummy = gflags::RegisterFlagValidator(&FLAGS_nfields, &validate_nfields);

static bool validate_ntrees(const char* flagname, gflags::int32 value) 
{ 
    if (!FLAGS_build)
        return true;
    return check_above_zero(flagname, value); 
}
static const bool ntrees_dummy = gflags::RegisterFlagValidator(&FLAGS_ntrees, &validate_ntrees);

static bool validate_idx(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool idx_dummy = gflags::RegisterFlagValidator(&FLAGS_idx, &validate_idx);

static bool validate_wt(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool wt_dummy = gflags::RegisterFlagValidator(&FLAGS_wt, &validate_wt);

static bool validate_algname(const char* flagname, const std::string &value) 
{ 
    if (FLAGS_build)
        return true;
    return check_not_empty(flagname, value); 
}
static const bool algname_dummy = gflags::RegisterFlagValidator(&FLAGS_algname, &validate_algname);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    if (FLAGS_build)
        return true;

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
    if (FLAGS_build) return true;

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
{
    if (FLAGS_build)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{
    if (FLAGS_build)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);

} // namespace

namespace Test {
    using namespace std;

    void test1( boost::shared_ptr< KNN::KnnServiceIf > pHandler )
    {
        vector<string> result;
        pHandler->queryByItemNoWeight( result, "李宇春", 10 );
        exit(0);
    }

} // namespace Test

// 信号处理函数中只做 stop server, 其他工作在main函数中完成，不宜在此做。
// 否则如果apiserver已经退出，SIGINT 退出本程序会core dump
static
void stop_server()
{
    if (g_pThisServer)
        g_pThisServer->stop();
    // LOG(INFO) << "stop server done!";
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
void load_data_file( const char *filename )
{
    using namespace std;

    ifstream ifs( filename, ios::in );
    if (!ifs)
        THROW_RUNTIME_ERROR("Cannot open file " << filename);

    string line;
    size_t lineno = 0;
    while (getline(ifs, line)) {
        ++lineno;
        try {
            g_pWordAnnDB->addRecord( line );
        } catch (const exception &errInput) {
            LOG(ERROR) << "Skip line " << lineno << " " << errInput.what();
            continue;
        } // try
    } // while
}


static
void do_build_routine()
{
    cout << "Program working in BUILD mode." << endl;
    cout << "Loading data file..." << endl;
    load_data_file( FLAGS_idata.c_str() );
    cout << "Building annoy index..." << endl;
    g_pWordAnnDB->buildIndex( FLAGS_ntrees );
    g_pWordAnnDB->saveIndex( FLAGS_idx.c_str() );
    cout << "Saving word table..." << endl;
    g_pWordAnnDB->saveWordTable( FLAGS_wt.c_str() );

    cout << "Total " << g_pWordAnnDB->size() << " words in database." << endl;
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
    boost::shared_ptr< KNN:: KnnServiceIf> 
            pHandler = boost::make_shared< KNN::KnnServiceHandler >();
    g_pThisServer = boost::make_shared< KnnAlgServer >(pHandler, g_nThisPort, 
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

static
void do_load_routine()
{
    cout << "Program working in LOAD mode." << endl;
    cout << "Loading annoy index..." << endl;
    g_pWordAnnDB->loadIndex( FLAGS_idx.c_str() );
    // FLAGS_nfields = g_pWordAnnDB->size();
    // DLOG(INFO) << "FLAGS_nfields = " << FLAGS_nfields;
    cout << "Loading word table..." << endl;
    g_pWordAnnDB->loadWordTable( FLAGS_wt.c_str() );

    cout << "Total " << g_pWordAnnDB->size() << " words in database." << endl;

    do_service_routine();
}


static
void check_nfields()
{
    using namespace std;

    if (FLAGS_nfields > 0)
        return;

    auto readCmd = [](const std::string &cmd, std::string &output)->int {
        int retval = 0;

        FILE *fp = popen(cmd.c_str(), "r");
        setvbuf(fp, NULL, _IONBF, 0);

        typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source >
            FDRdStream;
        FDRdStream ppStream( fileno(fp), boost::iostreams::never_close_handle );

        stringstream ss;
        ss << ppStream.rdbuf();

        output = ss.str();

        retval = pclose(fp);
        retval = WEXITSTATUS(retval);

        return retval;
    };

    std::ostringstream oss;
    oss << "tail -1 " << FLAGS_idata << " | awk \'{print NF}\'" << std::flush;

    string output;
    int retcode = readCmd(oss.str(), output);
    THROW_RUNTIME_ERROR_IF(retcode, "Read -nfields fail!");
    // FLAGS_nfields = boost::lexical_cast<int>(output);
    {
        std::istringstream iss(output);
        iss >> FLAGS_nfields;
        THROW_RUNTIME_ERROR_IF(!iss, "Read -nfields fail!");
    }
    --FLAGS_nfields;
    LOG(WARNING) << "-nfields not set, use auto detected value: " << FLAGS_nfields;
}


int main( int argc, char **argv )
{
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        check_nfields();

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

        g_pWordAnnDB.reset( new KNN::WordAnnDB(FLAGS_nfields) );

        if (FLAGS_build)
            do_build_routine();
        else
            do_load_routine();

        pIoServiceWork.reset();
        io_service.stop();
        io_service_thr.join();

        stop_client();

        cout << argv[0] << " done!" << endl;

    } catch (const std::exception &ex) {
        cerr << "main caught exception: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}


