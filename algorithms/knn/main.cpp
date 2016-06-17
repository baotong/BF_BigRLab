/*
 * BUILD:
 * GLOG_logtostderr=1 ./wordknn.bin -build -idata vec3.txt -nfields 200 -ntrees 10 -idx index.ann -wt words_table.txt
 * LOAD & Start Server
 * GLOG_logtostderr=1 ./wordknn.bin -nfields 200 -idx index.ann -wt words_table.txt -algname knn_star -algmgr localhost:9001 -svraddr 192.168.210.129:10080
 */
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "WordAnnDB.h"
#include <iostream>
#include <fstream>
#include <cctype>
#include <csignal>
#include <thread>
#include <chrono>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <json/json.h>

#define SLEEP_MILLISECONDS(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))
#define SLEEP_SECONDS(x)      std::this_thread::sleep_for(std::chrono::seconds(x))

#define SERVICE_LIB_NAME        "knn"

using std::cout; using std::cerr; using std::endl;

static boost::shared_ptr<KNN::WordAnnDB> g_pWordAnnDB;

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
    virtual void queryByVectorNoWeight(std::vector<std::string> & _return, const std::vector<double> & values, const int32_t n);
    virtual void queryByItemNoWeight(std::vector<std::string> & _return, const std::string& item, const int32_t n);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

void KnnServiceHandler::queryByItem(std::vector<Result> & _return, 
            const std::string& item, const int32_t n)
{
    using namespace std;

    DLOG(INFO) << "Querying item " << item << " n = " << n;

    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value " << n);

    vector<string>    result;
    vector<float>    distances;

    result.reserve(n);
    distances.reserve(n);

    g_pWordAnnDB->kNN_By_Word( item, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

void KnnServiceHandler::queryByItemNoWeight(std::vector<std::string> & _return, 
            const std::string& item, const int32_t n)
{
    std::vector<Result> result;
    queryByItem(result, item, n);
    _return.resize( result.size() );
    
    // 必须用typedef，否则模板里的逗号会被宏解析为参数分割
    typedef boost::tuple<std::string&, Result&> IterType;
    BOOST_FOREACH( IterType v, boost::combine(_return, result) )
        v.get<0>().swap(v.get<1>().item);
}

// 若用double，annoy buildidx会崩溃
void KnnServiceHandler::queryByVector(std::vector<Result> & _return, 
            const std::vector<double> & values, const int32_t n)
{
    using namespace std;

    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value " << n);

    vector<string>    result;
    vector<float>     distances;
    vector<float>     fValues( values.begin(), values.end() );

    result.reserve(n);
    distances.reserve(n);

    g_pWordAnnDB->kNN_By_Vector( fValues, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

void KnnServiceHandler::queryByVectorNoWeight(std::vector<std::string> & _return, 
            const std::vector<double> & values, const int32_t n)
{
    std::vector<Result> result;
    queryByVector(result, values, n);
    _return.resize( result.size() );
    
    // 必须用typedef，否则模板里的逗号会被宏解析为参数分割
    typedef boost::tuple<std::string&, Result&> IterType;
    BOOST_FOREACH( IterType v, boost::combine(_return, result) )
        v.get<0>().swap(v.get<1>().item);
}

void KnnServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    int             n = 0;

    // DLOG(INFO) << "KnnService received request: " << request;

    typedef std::vector<std::string> StringArray;
    typedef std::map< std::string, StringArray> Query;
    Query           query;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");

    // get n
    try {
        n = root["n"].asInt();
        if (n <= 0)
            THROW_INVALID_REQUEST("Invalid n value");
    } catch (const std::exception &ex) {
        THROW_INVALID_REQUEST("Json parse fail! " << ex.what());
    } // try

    // get query items
    auto items = root["items"];
    if (items.isNull())
        THROW_INVALID_REQUEST("Bad request format!");

    if (items.isArray()) {
        for (auto it = items.begin(); it != items.end(); ++it) {
            try {
                string item = (*it).asString();
                if (!item.empty())
                    query.insert( std::make_pair(item, StringArray()) );
            } catch (const std::exception &ex) {;}
        } // for
    } else {
        try {
            string item = items.asString();
            if (item.empty())
                THROW_INVALID_REQUEST("Bad request format!");
            query.insert( std::make_pair(item, StringArray()) );
        } catch (const std::exception &ex) {
            THROW_INVALID_REQUEST("Json parse fail! " << ex.what());
        } // try
    } // if

    if (query.empty())
        THROW_INVALID_REQUEST("Bad request format! no valid query item");

    for (auto &v : query)
        queryByItemNoWeight(v.second, v.first, n);

    Json::Value     outRoot; // array of record
    for (auto &v : query) {
        Json::Value record, mostLike;
        record["item"] = v.first;
        for (auto &str : v.second)
            mostLike.append(str);
        record["most_like"].swap(mostLike);
        outRoot.append(Json::Value());
        outRoot[ outRoot.size()-1 ].swap(record);
    } // for
    Json::FastWriter writer;  
    _return = writer.write(outRoot);
}

} // namespace KNN

namespace Test {
    using namespace std;

    void test1( boost::shared_ptr< KNN::KnnServiceIf > pHandler )
    {
        vector<string> result;
        pHandler->queryByItemNoWeight( result, "李宇春", 10 );
        exit(0);
    }

} // namespace Test

typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;
typedef BigRLab::ThriftServer< KNN::KnnServiceIf, KNN::KnnServiceProcessor > KnnAlgServer;
static AlgMgrClient::Pointer                  g_pAlgMgrClient;
static KnnAlgServer::Pointer                  g_pThisServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo> g_pSvrInfo;
static boost::asio::io_service                g_io_service;

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
bool get_local_ip( std::string &result )
{
    using namespace boost::asio::ip;

    try {
        boost::asio::io_service netService;
        boost::asio::io_service::work work(netService);
        
        std::thread ioThr( [&]{ netService.run(); } );
        ioThr.detach(); // 必须 detach, 否则当无网络连接时 core dump

        udp::resolver   resolver(netService);
        udp::resolver::query query(udp::v4(), "www.baidu.com", "");
        udp::resolver::iterator endpoints = resolver.resolve(query);
        udp::endpoint ep = *endpoints;
        udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        result = std::move( addr.to_string() );

        netService.stop();

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
    g_pSvrInfo->maxConcurrency = FLAGS_n_work_threads;
    g_pSvrInfo->serviceName = SERVICE_LIB_NAME;

    // start client to alg_mgr
    // 需要在单独线程中执行，防止和alg_mgr形成死锁
    // 这里调用addSvr，alg_mgr那边调用 Service::addServer
    // 尝试连接本server，而本server还没有启动
    cout << "Registering server..." << endl;
    g_pAlgMgrClient = boost::make_shared< AlgMgrClient >(g_strAlgMgrAddr, g_nAlgMgrPort);
    auto register_svr = [&] {
        try {
            SLEEP_MILLISECONDS(500);   // let thrift server start first
            if (!g_pAlgMgrClient->start(50, 300)) {
                cerr << "AlgMgr server unreachable!" << endl;
                exit(-1);
            } // if
            (*g_pAlgMgrClient)()->rmSvr(FLAGS_algname, *g_pSvrInfo);
            int ret = (*g_pAlgMgrClient)()->addSvr(FLAGS_algname, *g_pSvrInfo);
            if (ret != BigRLab::SUCCESS) {
                cerr << "Register alg server fail!";
                switch (ret) {
                    case BigRLab::ALREADY_EXIST:
                        cerr << " server with same addr:port already exists!";
                        break;
                    case BigRLab::SERVER_UNREACHABLE:
                        cerr << " this server is unreachable by algmgr! check the server address setting.";
                        break;
                    case BigRLab::NO_SERVICE:
                        cerr << " service lib has not benn loaded on apiserver!";
                        break;
                    case BigRLab::INTERNAL_FAIL:
                        cerr << " fail due to internal error!";
                        break;
                } // switch
                cerr << endl;
                std::raise(SIGTERM);
            } // if
        } catch (const std::exception &ex) {
            cerr << "Unable to connect to algmgr server, " << ex.what() << endl;
            std::raise(SIGTERM);
        } // try
    };
    boost::thread register_thr(register_svr);
    register_thr.detach();

    // start this alg server
    cout << "Launching alogrithm server... " << endl;
    boost::shared_ptr< KNN::KnnServiceIf > pHandler = boost::make_shared< KNN::KnnServiceHandler >();
    // Test::test1(pHandler);
    g_pThisServer = boost::make_shared< KnnAlgServer >(pHandler, g_nThisPort);
    try {
        g_pThisServer->start(); //!! NOTE blocking until quit
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
        THROW_RUNTIME_ERROR("Cannot open file " << filename);

    string line;
    while (getline(ifs, line)) {
        try {
            g_pWordAnnDB->addRecord( line );
        } catch (const InvalidInput &errInput) {
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
                { stop_server(); } );

        auto io_service_thr = std::thread( [&]{ g_io_service.run(); } );

        g_pWordAnnDB.reset( new KNN::WordAnnDB(FLAGS_nfields) );

        if (FLAGS_build)
            do_build_routine();
        else
            do_load_routine();

        pIoServiceWork.reset();
        g_io_service.stop();
        if (io_service_thr.joinable())
            io_service_thr.join();

        stop_client();

        cout << argv[0] << " done!" << endl;

    } catch (const std::exception &ex) {
        cerr << "main caught exception: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

