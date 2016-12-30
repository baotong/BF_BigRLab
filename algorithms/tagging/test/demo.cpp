/*
 * Usage
 * ./demo -algname tagger -algmgr localhost:9001 -port 10080 -concur concur.data -tagset tagset.data -idx idx.ann -idxlen 200 -wt wt.data
 */
#include "jieba.hpp"
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "ArticleServiceHandler.h"
#include <cstdio>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <glog/logging.h>
#include <gflags/gflags.h>

#define SERVICE_LIB_NAME        "tagging"

#define TIMER_REJOIN            15          // 15s

using namespace BigRLab;

DEFINE_string(filter, "x", "Filter out specific kinds of word in word segment");
DEFINE_string(dict, "../dict/jieba.dict.utf8", "自带词典");
DEFINE_string(hmm, "../dict/hmm_model.utf8", "Path to hmm model file");
DEFINE_string(user_dict, "../dict/user.dict.utf8", "用户自定义词典");
DEFINE_string(idf, "../dict/idf.utf8", "训练过的idf，关键词提取");
DEFINE_string(stop_words, "../dict/stop_words.utf8", "停用词");
DEFINE_int32(n_jieba_inst, 0, "Number of jieba instances");
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(addr, "", "Address of this algorithm server, use system detected if not specified.");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");

DEFINE_string(concur, "", "File of concur table");
DEFINE_string(tagset, "", "File of tagset");
DEFINE_string(wt, "", "File of word table");
DEFINE_string(idx, "", "File of word annoy tree");
DEFINE_int32(idxlen, 0, "vector len of annoy vector");


static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;
typedef BigRLab::ThriftServer< Article::ArticleServiceIf, Article::ArticleServiceProcessor > ArticleAlgServer;
static AlgMgrClient::Pointer                  g_pAlgMgrClient;
static ArticleAlgServer::Pointer              g_pThisServer;
static boost::asio::io_service                g_io_service;
static boost::shared_ptr<BigRLab::AlgSvrInfo> g_pSvrInfo;

boost::shared_ptr<WordAnnDB>            g_pAnnDB;
boost::shared_ptr<ConcurTable>          g_pConcurTable;
std::set<std::string>   g_TagSet;

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

static bool validate_concur(const char* flagname, const std::string &value)
{ return check_not_empty( flagname, value ); }
static bool concur_dummy = gflags::RegisterFlagValidator(&FLAGS_concur, &validate_concur);

static bool validate_tagset(const char* flagname, const std::string &value)
{ return check_not_empty( flagname, value ); }
static bool tagset_dummy = gflags::RegisterFlagValidator(&FLAGS_tagset, &validate_tagset);

static bool validate_wt(const char* flagname, const std::string &value)
{ return check_not_empty( flagname, value ); }
static bool wt_dummy = gflags::RegisterFlagValidator(&FLAGS_wt, &validate_wt);

static bool validate_idx(const char* flagname, const std::string &value)
{ return check_not_empty( flagname, value ); }
static bool idx_dummy = gflags::RegisterFlagValidator(&FLAGS_idx, &validate_idx);

static bool validate_idxlen(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool idxlen_dummy = gflags::RegisterFlagValidator(&FLAGS_idxlen, &validate_idxlen);

static
void get_local_ip()
{
    using boost::asio::ip::tcp;
    using boost::asio::ip::address;
    using namespace std;

class IpQuery {
    typedef tcp::socket::endpoint_type  endpoint_type;
public:
    IpQuery( boost::asio::io_service& io_service,
             const std::string &svrAddr )
            : socket_(io_service)
    {
        std::string server = boost::replace_first_copy(svrAddr, "localhost", "127.0.0.1");

        string::size_type pos = server.find(':');
        if (string::npos == pos)
            THROW_RUNTIME_ERROR( server << " is not a valid address, must in format ip:addr" );

        uint16_t port = 0;
        if (!boost::conversion::try_lexical_convert(server.substr(pos+1), port) || !port)
            THROW_RUNTIME_ERROR("Invalid port number!");

        endpoint_type ep( address::from_string(server.substr(0, pos)), port );
        socket_.async_connect(ep, std::bind(&IpQuery::handle_connect,
                    this, std::placeholders::_1));
    }

private:
    void handle_connect(const boost::system::error_code& error)
    {
        if( error )
            THROW_RUNTIME_ERROR("fail to connect to " << socket_.remote_endpoint() << " error: " << error);
        g_strThisAddr = socket_.local_endpoint().address().to_string();
    }

private:
    tcp::socket     socket_;
};

    // start routine
    cout << "Trying to get local ip addr..." << endl;
    boost::asio::io_service io_service;
    IpQuery query(io_service, FLAGS_algmgr);
    io_service.run();
}

} // namespace

SharedQueue<Jieba::pointer>      g_JiebaPool;

static volatile bool                                       g_bLoginSuccess = false;
static std::unique_ptr< boost::asio::deadline_timer >      g_Timer;


namespace Test {

using namespace std;

void test1()
{
    auto pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
    pJieba->setFilter( FLAGS_filter );
    string s = "我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。";
    Jieba::WordVector result;
    pJieba->wordSegment(s, result);
    copy(result.begin(), result.end(), ostream_iterator<string>(cout, " "));
    cout << endl;
}

void test2()
{
    cout << "Initializing jieba array..." << endl;
    vector<Jieba::pointer> vec(10);
    for (auto& p : vec)
        p = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
    cout << "Done!" << endl;
}

void test_anndb()
{
    using namespace std;
    vector<string> result;
    vector<float>  distances;
    g_pAnnDB->kNN_By_Word("你他妈的去死吧", 10, result, distances);
    for (size_t i = 0; i < result.size(); ++i)
        cout << result[i] << "\t" << distances[i] << endl;
}

void test_concur_table()
{
    auto& cList = g_pConcurTable->lookup("fuck");
    if (cList.empty()) {
        cout << "Cannot find required item in concur table!" << endl;
        return;
    } // if
    for (auto &v : cList)
        cout << *(boost::get<ConcurTable::StringPtr>(v.item)) << ":" << v.weight << " ";
    cout << endl;
}

} // namespace Test


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
void start_rpc_service()
{
    using namespace std;

    cout << "Trying to start rpc service..." << endl;

    try {
        get_local_ip();
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
                std::raise(SIGTERM);
                return;
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
                return;
            } // if

            g_bLoginSuccess = true;

        } catch (const std::exception &ex) {
            cerr << "Unable to connect to algmgr server, " << ex.what() << endl;
            std::raise(SIGTERM);
        } // try
    };
    boost::thread register_thr(register_svr);
    register_thr.detach();

    // start this alg server
    cout << "Launching alogrithm server... " << endl;
    boost::shared_ptr< Article::ArticleServiceIf > 
            pHandler = boost::make_shared< Article::ArticleServiceHandler >();
    // Test::test1(pHandler);
    g_pThisServer = boost::make_shared< ArticleAlgServer >(pHandler, g_nThisPort, 
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
void load_tagset(const std::string &filename, std::set<std::string> &tagSet)
{
    using namespace std;

    ifstream ifs(filename, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("load_tagset cannot read file " << filename);
    
    string line, item;
    while (getline(ifs, line)) {
        stringstream stream(line);
        stream >> item;
        tagSet.insert( std::move(item) );
    } // while

    // ofstream ofs("/tmp/1.data", ios::out);
    // for (auto &v : tagSet)
        // ofs << v << endl;
}

static
void service_init()
{
    using namespace std;

    if (FLAGS_n_jieba_inst <= 0 || FLAGS_n_jieba_inst > FLAGS_n_work_threads) {
        LOG(INFO) << "Adjust -n_jieba_inst from old value " << FLAGS_n_jieba_inst 
            << " to new value -n_work_threads " << FLAGS_n_work_threads;
        FLAGS_n_jieba_inst = FLAGS_n_work_threads;
    } // if

    // check idx file
    {
        ifstream ifs(FLAGS_idx, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open annoy index file " << FLAGS_idx << " for reading!");
    }

    // LOG(INFO) << "FLAGS_n_jieba_inst = " << FLAGS_n_jieba_inst;
    
    bool success = true;

    cout << "Creating jieba instances..." << endl;
    std::thread thrCreateJiebaInst([&] {
        try {
            for (int i = 0; i < FLAGS_n_jieba_inst; ++i) {
                auto pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
                        FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
                pJieba->setFilter( FLAGS_filter );
                g_JiebaPool.push(pJieba);
            } // for
        } catch (const std::exception &ex) {
            cerr << ex.what() << endl;
            success = false;
        } // try
    });

    cout << "Loading tagset..." << endl;
    std::thread thrLoadTagSet([&]{
        try {
            load_tagset(FLAGS_tagset, g_TagSet);
        } catch (const std::exception &ex) {
            cerr << ex.what() << endl;
            success = false;
        } // try
    });

    cout << "Loading concur table..." << endl;
    std::thread thrLoadConcur([&]{
        try {
            g_pConcurTable.reset( new ConcurTable );
            g_pConcurTable->loadFromFile( FLAGS_concur, g_TagSet );
        } catch (const std::exception &ex) {
            cerr << ex.what() << endl;
            success = false;
        } // try
    });

    cout << "Loading Annoy tree..." << endl;
    std::thread thrLoadIdx([&]{
        try {
            g_pAnnDB.reset( new WordAnnDB(FLAGS_idxlen) );
            g_pAnnDB->loadIndex( FLAGS_idx.c_str() );
            g_pAnnDB->loadWordTable( FLAGS_wt.c_str() );
        } catch (const std::exception &ex) {
            cerr << ex.what() << endl;
            success = false;
        } // try
    });

    thrCreateJiebaInst.join();
    thrLoadTagSet.join();
    thrLoadConcur.join();
    thrLoadIdx.join();

    if (!success) {
        std::raise(SIGTERM);
        return;
    } // if

    LOG(INFO) << "Totally " << g_TagSet.size() << " tags in tagset.";
    LOG(INFO) << "Totally " << g_pConcurTable->size() << " concur records loaded.";
    LOG(INFO) << "Totally " << g_pAnnDB->size() << " items loaded from annoy tree.";

    // Test::test_anndb();
    // Test::test_concur_table();
    // exit(0);
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

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        // Test::test1();
        // Test::test2();
        // return 0;
        
        // install signal handler
        auto pIoServiceWork = boost::make_shared< boost::asio::io_service::work >(std::ref(g_io_service));

        boost::asio::signal_set signals(g_io_service, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal) { 
            if (g_Timer)
                g_Timer->cancel();
            try { stop_server(); } catch (...) {}
        } );

        g_Timer.reset(new boost::asio::deadline_timer(std::ref(g_io_service)));

        g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_REJOIN));
        g_Timer->async_wait(rejoin);

        auto io_service_thr = boost::thread( [&]{ g_io_service.run(); } );

        do_service_routine();

        pIoServiceWork.reset();
        g_io_service.stop();
        if (io_service_thr.joinable())
            io_service_thr.join();

        stop_client();

        cout << argv[0] << " done!" << endl;

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        return -1;
    } // try

    return 0;
}


