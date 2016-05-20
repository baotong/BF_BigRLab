/*
 * BUILD:
 * GLOG_logtostderr=1 ./wordknn.bin -build -idata vec3.txt -nfields 200 -ntrees 10 -idx index.ann -wt words_table.txt
 * LOAD & Start Server
 * GLOG_logtostderr=1 ./wordknn.bin -nfields 200 -idx index.ann -wt words_table.txt -algname knn_star -algmgr localhost:9001 -svraddr 192.168.210.128:10080
 */
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "KnnService.h"
#include "WordAnnDB.h"
#include <iostream>
#include <fstream>
#include <cctype>
#include <thread>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>

using std::cout; using std::cerr; using std::endl;

static boost::shared_ptr<WordAnnDB> g_pWordAnnDB;

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

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
DEFINE_string(svraddr, "", "Address of this RPC algorithm server, in form of addr:port");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");

namespace {

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
{ 
    if (!FLAGS_build)
        return true;
    return check_not_empty(flagname, value); 
}
static const bool idata_dummy = gflags::RegisterFlagValidator(&FLAGS_idata, &validate_idata);

static bool validate_nfields(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool nfields_dummy = gflags::RegisterFlagValidator(&FLAGS_nfields, &validate_nfields);

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
    using namespace std;

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
bool validate_svraddr(const char* flagname, const std::string &value) 
{
    using namespace std;

    if (FLAGS_build)
        return true;

    if (!check_not_empty(flagname, value))
        return false;

    string::size_type pos = value.find_last_of(':');
    if (string::npos == pos) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    g_strThisAddr = value.substr(0, pos);
    if (g_strThisAddr.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    string strPort = value.substr(pos + 1, string::npos);
    if (strPort.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!boost::conversion::try_lexical_convert(strPort, g_nThisPort)) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!g_nThisPort) {
        cerr << "Invalid port number specified by arg " << flagname << endl;
        return false;
    } // if

    return true;
}
static const bool svraddr_dummy = gflags::RegisterFlagValidator(&FLAGS_svraddr, &validate_svraddr);

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

namespace KNN {

class KnnServiceHandler : virtual public KnnServiceIf {
public:
    virtual void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n);
    virtual void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n);
};

#define THROW_INVALID_REQUEST(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        InvalidRequest __input_request_err; \
        __input_request_err.reason = std::move(__err_stream.str()); \
        throw __input_request_err; \
    } while (0)

void KnnServiceHandler::queryByItem(std::vector<Result> & _return, 
            const std::string& item, const int32_t n)
{
    using namespace std;

    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value " << n);

    vector<string>    result;
    vector<float>    distances;

    g_pWordAnnDB->kNN_By_Word( item, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

// 若用double，annoy buildidx会崩溃
void KnnServiceHandler::queryByVector(std::vector<Result> & _return, 
            const std::vector<double> & values, const int32_t n)
{
    using namespace std;

    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value " << n);

    vector<string>    result;
    vector<float>    distances;
    vector<float>       fValues( values.begin(), values.end() );

    g_pWordAnnDB->kNN_By_Vector( fValues, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

#undef THROW_INVALID_REQUEST

} // namespace KNN


typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient > AlgMgrClient;
typedef BigRLab::ThriftServer< KNN::KnnServiceIf, KNN::KnnServiceProcessor > KnnAlgServer;
static AlgMgrClient::Pointer    g_pAlgMgrClient;
static KnnAlgServer::Pointer    g_pThisServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo>  g_pSvrInfo;
static boost::asio::io_service                 g_io_service;

static
bool get_local_ip( std::string &result )
{
    using namespace boost::asio::ip;

    try {
        boost::asio::io_service netService;
        boost::asio::io_service::work work(netService);
        
        std::thread ioThr( [&]{ netService.run(); } );

        udp::resolver   resolver(netService);
        udp::resolver::query query(udp::v4(), "www.baidu.com", "");
        udp::resolver::iterator endpoints = resolver.resolve(query);
        udp::endpoint ep = *endpoints;
        udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        result = std::move( addr.to_string() );

        netService.stop();
        ioThr.join();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "get_local_ip() error, Exception: " << e.what() << std::endl;
    } // try

    return false;
}


static
void start_rpc_service()
{
    using namespace std;

    // fill svrinfo
    string addr, line;
    if (get_local_ip(addr)) {
        if (addr != g_strThisAddr) {
            cout << "The local addr you provided is " << g_strThisAddr 
                << " which differs with that system detected " << addr
                << " Do you want to use the system detected addr? (y/n)y?" << endl;
            getline(cin, line);
            if (!line.empty() && tolower(line[0]) == 'n' )
                addr = g_strThisAddr;
        } // if
    } else {
        addr = g_strThisAddr;
    } // if

    g_pSvrInfo = boost::make_shared<BigRLab::AlgSvrInfo>();
    g_pSvrInfo->addr = addr;
    g_pSvrInfo->port = (int16_t)g_nThisPort;
    g_pSvrInfo->nWorkThread = FLAGS_n_work_threads;

    // start client to alg_mgr
    g_pAlgMgrClient = boost::make_shared< AlgMgrClient >(g_strAlgMgrAddr, g_nAlgMgrPort);
    try {
        g_pAlgMgrClient->start();
        int ret = (*g_pAlgMgrClient)()->addSvr(FLAGS_algname, *g_pSvrInfo);
        if (ret != BigRLab::SUCCESS) {
            cerr << "Register alg server fail, return value is " << ret << endl;
            exit(-1);
        } // if
    } catch (const std::exception &ex) {
        cerr << "Unable to connect to algmgr server, " << ex.what() << endl;
        exit(-1);
    } // try

    // start this alg server
    boost::shared_ptr< KNN::KnnServiceIf > pHandler = boost::make_shared< KNN::KnnServiceHandler >();
    g_pThisServer = boost::make_shared< KnnAlgServer >(pHandler, g_nThisPort);
    try {
        g_pThisServer->start();
    } catch (const std::exception &ex) {
        cerr << "Start this alg server fail, " << ex.what() << endl;
        exit(-1);
    } // try
}

static
void load_data_file( const char *filename )
{
    using namespace std;

    ifstream ifs( filename, ios::in );
    if (!ifs)
        throw runtime_error( string("Cannot open file ").append(filename) );

    string line;
    while (getline(ifs, line)) {
        try {
            g_pWordAnnDB->addRecord( line );

        } catch (const InvalidInput &errInput) {
            // cerr << errInput.what() << endl;
            LOG(ERROR) << errInput.what();
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
void do_load_routine()
{
    cout << "Program working in LOAD mode." << endl;
    cout << "Loading annoy index..." << endl;
    g_pWordAnnDB->loadIndex( FLAGS_idx.c_str() );
    cout << "Loading word table..." << endl;
    g_pWordAnnDB->loadWordTable( FLAGS_wt.c_str() );

    cout << "Total " << g_pWordAnnDB->size() << " words in database." << endl;

    start_rpc_service();
}

static
void finish()
{
    (*g_pAlgMgrClient)()->rmSvr(FLAGS_algname, *g_pSvrInfo);
    g_pAlgMgrClient->stop();
    g_pThisServer->stop();
}

int main( int argc, char **argv )
{
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        // install signal handler
        auto pIoServiceWork = boost::make_shared< boost::asio::io_service::work >(std::ref(g_io_service));
        boost::asio::signal_set signals(g_io_service, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal)
                { finish(); } );

        auto io_service_thr = std::thread( [&]{ g_io_service.run(); } );

        g_pWordAnnDB.reset( new WordAnnDB(FLAGS_nfields) );

        if (FLAGS_build)
            do_build_routine();
        else
            do_load_routine();

        pIoServiceWork.reset();
        g_io_service.stop();
        if (io_service_thr.joinable())
            io_service_thr.join();
        cout << argv[0] << " done!" << endl;

    } catch (const std::exception &ex) {
        cerr << "main caught exception: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}


