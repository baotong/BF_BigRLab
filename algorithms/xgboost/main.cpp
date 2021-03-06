/*
 * To install xgboost
 * 1. install headers
 * sudo cp -a include/xgboost /usr/local/include/
 * sudo cp -a rabit/include/rabit /usr/local/include/
 * sudo cp -a dmlc-core/include/dmlc /usr/local/include/
 * 2. install libs
 * sudo cp lib/libxgboost.so /usr/local/lib/
 * sudo cp rabit/lib/librabit.a /usr/local/lib/
 * sudo cp dmlc-core/libdmlc.a /usr/local/lib/
 * 3. install xgboost exe to system
 * sudo cp xgboost /usr/local/bin/
 *
 * Usage:
 *
 * standalone mode:
 * ./xgboost_svr.bin -standalone -model 0002.model < in10.test
 * service mode:
 * ./xgboost_svr.bin -model 0002.model -algname booster -algmgr localhost:9001 -port 10080
 * GBDT
 * ./xgboost_svr.bin -model train1.model -model2 train2.model -offset 201 -algname booster -algmgr localhost:9001 -port 10080
 */
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iterator>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include "rpc_module.h"
#include "common.hpp"
#include "alg_common.hpp"
#include "get_local_ip.hpp"
#include "xgboost_learner.h"
#include "AlgMgrService.h"
#include "XgBoostServiceHandler.h"
#include "register_svr.hpp"

#define SERVICE_LIB_NAME        "xgboost"

#define TIMER_CHECK            15          // 15s

DEFINE_string(model, "", "Same as model_in of xgboost");
DEFINE_string(model2, "", "Secondary model for GBDT and RNN");
DEFINE_bool(standalone, false, "Whether this program run in standalone mode");
DEFINE_int64(offset, 1, "attribute index offset of data, should be max(trainIdx)+1");
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(addr, "", "Address of this algorithm server, use system detected if not specified.");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");
DEFINE_int32(n_inst, 0, "Number of xgboost object instances");

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

// typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;
typedef BigRLab::ThriftServer< XgBoostSvr::XgBoostServiceIf, 
            XgBoostSvr::XgBoostServiceProcessor > XgBoostAlgSvr;
static AlgMgrClient::Pointer                  g_pAlgMgrClient;
static XgBoostAlgSvr::Pointer                 g_pThisServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo> g_pSvrInfo;

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

static 
bool validate_offset(const char* flagname, gflags::int64 value) 
{
    if (FLAGS_standalone)
        return true;
    return check_above_zero(flagname, value);
}
static const bool offset_dummy = gflags::RegisterFlagValidator(&FLAGS_offset, &validate_offset);

static bool validate_algname(const char* flagname, const std::string &value) 
{ 
    if (FLAGS_standalone)
        return true;
    return check_not_empty(flagname, value); 
}
static const bool algname_dummy = gflags::RegisterFlagValidator(&FLAGS_algname, &validate_algname);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    using namespace std;

    if (FLAGS_standalone)
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
    if (FLAGS_standalone)
        return true;
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
    if (FLAGS_standalone)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{
    if (FLAGS_standalone)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);

} // namespace


using namespace std;
using namespace xgboost;
using namespace BigRLab;


// global vars
static std::unique_ptr<CLIParam>        g_pCLIParam;
static std::unique_ptr<CLIParam>        g_pCLIParam2;
SharedQueue<XgBoostLearner::pointer>    g_LearnerPool;
std::vector<uint32_t>                   g_arrMaxLeafId;
std::vector<bool>                       g_arrTreeMark;
std::set<std::string>                   g_setArgFiles;
std::unique_ptr<std::thread>            g_pSvrThread;

static std::unique_ptr< boost::asio::deadline_timer >      g_Timer;

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
        g_bLoginSuccess = false;
        (*g_pAlgMgrClient)()->rmSvr(FLAGS_algname, *g_pSvrInfo);
        g_pAlgMgrClient->stop();
    } // if
}

static
void finalize()
{
    // LOG(INFO) << "finalize()";
    rabit::Finalize();
}

static
std::shared_ptr<void> initialize( int argc, char **argv )
{
    using namespace std;

    // check model file
    {
        ifstream ifs(FLAGS_model, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot read file " << FLAGS_model << " specified by -model");
    }

    std::shared_ptr<void> ret( (void*)-1, [](void*){
        // DLOG(INFO) << "doing global cleanup...";
        finalize();
    } );

    rabit::Init(argc, argv);

    std::vector<std::pair<std::string, std::string> > cfg;
    cfg.push_back(std::make_pair("seed", "0"));
    cfg.push_back(std::make_pair("model_in", FLAGS_model));
    cfg.push_back(std::make_pair("name_pred", "stdout"));

    g_pCLIParam.reset(new CLIParam);
    g_pCLIParam->Configure(cfg);
    // DLOG(INFO) << g_pCLIParam->model_in;

    if (!FLAGS_model2.empty()) {
        ifstream ifs(FLAGS_model2, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot read file " << FLAGS_model2 << " specified by -model2");
        ifs.close();
        cfg.clear();
        cfg.push_back(std::make_pair("seed", "0"));
        cfg.push_back(std::make_pair("model_in", FLAGS_model2));
        cfg.push_back(std::make_pair("name_pred", "stdout"));
        g_pCLIParam2.reset(new CLIParam);
        g_pCLIParam2->Configure(cfg);
    } // if

    // DLOG(INFO) << g_pCLIParam2->model_in;

    return ret;
}

static
void do_standalone_routine()
{
    std::unique_ptr<XgBoostLearner> learner(new XgBoostLearner(g_pCLIParam.get()));

    vector<float> resultVec;
    string line;
    // ifstream cin("in10.test");
    while (getline(cin, line)) {
        std::unique_ptr<DMatrix> pMat( XgBoostLearner::DMatrixFromStr(line) );
        learner->predict( pMat.get(), resultVec, false );
        // std::copy( resultVec.begin(), resultVec.end(), ostream_iterator<float>(cout, " ") );
        // cout << endl;
        cout << resultVec[0] << endl;
    } // while
}

static
void parse_model( const std::string &modelFile, std::vector<uint32_t> &result,
                    std::vector<bool> &mark )
{
    using namespace std;

    result.clear();
    mark.clear();

    string fakeConf = std::tmpnam(nullptr);
    if (fakeConf.empty())
        THROW_RUNTIME_ERROR("Cannot create tmp file on system");

    // create fake conf
    {
        ofstream ofs( fakeConf, ios::out );
        ofs << "#" << flush;
    }

    string cmd = "xgboost ";
    cmd.append(fakeConf).append(" task=dump ").append("model_in=")
            .append(FLAGS_model).append(" name_dump=stdout");

    // DLOG(INFO) << "cmd: " << cmd; 

    FILE *fp = ::popen(cmd.c_str(), "r");
    if (!fp)
        THROW_RUNTIME_ERROR("Cannot run cmd: " << cmd);

    ON_FINISH( _pcloseFp, { ::pclose(fp); } );

    setlinebuf(fp);

    typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source >
            FDStream;
    FDStream pipeStream( fileno(fp), boost::iostreams::never_close_handle );

    string line;
    uint32_t treeId = 0, leafId = 0;
    double leafVal = 0.0;
    while (getline(pipeStream, line)) {
        if (sscanf(line.c_str(), "booster[%u]:\n", &treeId) == 1) {
            if (treeId != result.size())
                THROW_RUNTIME_ERROR("Parse tree model error! tree id not continuous");
            result.push_back(0);
        } else if (sscanf(line.c_str(), "%u:leaf=%lf\n", &leafId, &leafVal) == 2) {
            if (result.empty())
                THROW_RUNTIME_ERROR("Parse tree model error! leaf appear before tree mark");
            if (leafId > result.back())
                result.back() = leafId;
        } // if
    } // while
    
    // DEBUG
    // cout << "Max leaf id in this tree:" << endl;
    // std::copy(result.begin(), result.end(), ostream_iterator<uint32_t>(cout, " "));
    // cout << endl;
    
    mark.resize( result.size(), false );

    for (size_t i = 0; i < result.size(); ++i)
        mark[i] = result[i] ? true : false;

    if (result.size() > 1) {
        for (size_t i = 1; i < result.size(); ++i)
            result[i] += result[i-1];
    } // for
}

static
void service_init()
{
    using namespace std;

    cout << "Parsing model..." << endl;
    parse_model( FLAGS_model, g_arrMaxLeafId, g_arrTreeMark );
    if (g_arrMaxLeafId.empty())
        THROW_RUNTIME_ERROR("No valid tree found in model file " << FLAGS_model);
    // DEBUG
    // cout << "g_arrMaxLeafId:" << endl;
    // std::copy(g_arrMaxLeafId.begin(), g_arrMaxLeafId.end(), ostream_iterator<uint32_t>(cout, " "));
    // cout << endl;

    if (FLAGS_n_inst <= 0 || FLAGS_n_inst > FLAGS_n_work_threads) {
        LOG(INFO) << "Adjust -n_inst from old value " << FLAGS_n_inst 
            << " to new value -n_work_threads " << FLAGS_n_work_threads;
        FLAGS_n_inst = FLAGS_n_work_threads;
    } // if

    cout << "Creating xgboost instances..." << endl;
    for (int i = 0; i < FLAGS_n_inst; ++i) {
        auto pInst = boost::make_shared<XgBoostLearner>(g_pCLIParam.get(), g_pCLIParam2.get());
        g_LearnerPool.push_back( pInst );
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
void rejoin(const boost::system::error_code &ec)
{
    // DLOG(INFO) << "rejoin timer called";

    int waitTime = TIMER_CHECK;

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
    boost::shared_ptr< XgBoostSvr::XgBoostServiceIf > 
            pHandler = boost::make_shared< XgBoostSvr::XgBoostServiceHandler >();
    // Test::test1(pHandler);
    g_pThisServer = boost::make_shared< XgBoostAlgSvr >(pHandler, g_nThisPort, 
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
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        auto _cleanup = initialize(argc, argv);

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
        g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_CHECK));
        g_Timer->async_wait(rejoin);

        auto io_service_thr = boost::thread( [&]{ io_service.run(); } );

        if (FLAGS_standalone)
            do_standalone_routine();
        else
            do_service_routine();

        pIoServiceWork.reset();
        io_service.stop();
        if (io_service_thr.joinable())
            io_service_thr.join();

        stop_client();

        LOG(INFO) << argv[0] << " done!";

    } catch (const std::exception &ex) {
        cerr << "Exception caught in main: " << ex.what() << endl;
    } // try

    return 0;
}



namespace Test {

typedef unsigned long       ulong;

static
void test_matrix_file( const char *filename )
{
    std::unique_ptr<DMatrix> dtest(
            DMatrix::Load(filename, true, true));

    // cout << *dtest << endl;
    // dtest->SaveToLocalFile("test_save.matrix");

    unsigned long nRow = 0, nCol = 0;
    XGDMatrixNumRow( dtest.get(), &nRow );
    XGDMatrixNumCol( dtest.get(), &nCol );
    cout << "Matrix shape: " << nRow << "x" << nCol << endl;
}

static
void test_row_from_CSR()
{
    string line, item;
    vector<ulong> indp;
    vector<uint32_t> indices;
    vector<float> values;
    uint32_t index = 0;
    float value = 0.0;

    getline(cin, line);
    stringstream stream(line);
    while (stream >> item) {
        if (sscanf(item.c_str(), "%u:%f", &index, &value) != 2)
            continue;
        indices.push_back(index);
        values.push_back(value);
    } // while
    indp.push_back(0);
    indp.push_back(indices.size()+1);

    // cout << indices.size() << endl;

    DMatrix *mat = NULL;
    XGDMatrixCreateFromCSR(&indp[0], &indices[0], &values[0],
                        indp.size(), indices.size(), (DMatrixHandle*)&mat);

    assert(mat);

    unsigned long nRow = 0, nCol = 0;
    XGDMatrixNumRow( mat, &nRow );
    XGDMatrixNumCol( mat, &nCol );
    cout << "Matrix shape: " << nRow << "x" << nCol << endl;
}


} // namespace Test

