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
 *
 * Usage:
 *
 * standalone mode:
 * ./xgboost_svr.bin -standalone -model_in 0002.model < in10.test
 */
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <iterator>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "rpc_module.h"
#include "common.hpp"
#include "alg_common.hpp"
#include "get_local_ip.hpp"
#include "xgboost_learner.h"
#include "AlgMgrService.h"
#include "XgBoostServiceHandler.h"
#include "register_svr.hpp"

#define SERVICE_LIB_NAME        "xgboost"

DEFINE_string(model_in, "", "Same as model_in of xgboost");
DEFINE_bool(standalone, false, "Whether this program run in standalone mode");
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

static bool validate_model_in(const char* flagname, const std::string &value)
{ return check_not_empty(flagname, value); }
static const bool model_in_dummy = gflags::RegisterFlagValidator(&FLAGS_model_in, &validate_model_in);

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
SharedQueue<XgBoostLearner::pointer>    g_LearnerPool;

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
void finalize()
{
    // LOG(INFO) << "finalize()";
    rabit::Finalize();
}

static
std::shared_ptr<void> initialize( int argc, char **argv )
{
    // for (int i = 0; i < argc; ++i)
        // cout << argv[i] << endl;

    std::shared_ptr<void> ret( (void*)0, [](void*){
        // DLOG(INFO) << "doing global cleanup...";
        finalize();
    } );

    rabit::Init(argc, argv);

    std::vector<std::pair<std::string, std::string> > cfg;
    cfg.push_back(std::make_pair("seed", "0"));
    cfg.push_back(std::make_pair("model_in", FLAGS_model_in));
    cfg.push_back(std::make_pair("name_pred", "stdout"));

    g_pCLIParam.reset(new CLIParam);
    g_pCLIParam->Configure(cfg);

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
        learner->predict( pMat.get(), resultVec );
        std::copy( resultVec.begin(), resultVec.end(), ostream_iterator<float>(cout, " ") );
        cout << endl;
    } // while
}

static
void service_init()
{
    using namespace std;

    try {
        g_strThisAddr = get_local_ip(FLAGS_algmgr);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "get_local_ip() fail! " << ex.what();
        exit(-1);
    } // try

    cout << "Detected local ip is " << g_strThisAddr << endl;

    g_pSvrInfo = boost::make_shared<BigRLab::AlgSvrInfo>();
    g_pSvrInfo->addr = g_strThisAddr;
    g_pSvrInfo->port = (int16_t)g_nThisPort;
    g_pSvrInfo->maxConcurrency = FLAGS_n_work_threads;
    g_pSvrInfo->serviceName = SERVICE_LIB_NAME;

    if (FLAGS_n_inst <= 0 || FLAGS_n_inst > FLAGS_n_work_threads) {
        LOG(INFO) << "Adjust -n_inst from old value " << FLAGS_n_inst 
            << " to new value -n_work_threads " << FLAGS_n_work_threads;
        FLAGS_n_inst = FLAGS_n_work_threads;
    } // if

    cout << "Creating xgboost instances..." << endl;
    for (int i = 0; i < FLAGS_n_inst; ++i) {
        auto pInst = std::make_shared<XgBoostLearner>(g_pCLIParam.get());
        g_LearnerPool.push_back( pInst );
    } // for
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
    g_pThisServer = boost::make_shared< XgBoostAlgSvr >(pHandler, g_nThisPort);
    try {
        g_pThisServer->start(); //!! NOTE blocking until quit
    } catch (const std::exception &ex) {
        cerr << "Start this alg server fail, " << ex.what() << endl;
        exit(-1);
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
        signals.async_wait( [](const boost::system::error_code& error, int signal)
                { stop_server(); } );
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

        cout << argv[0] << " done!" << endl;

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

