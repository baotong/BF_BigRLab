/*
 * Usage as service:
 * GLOG_logtostderr=1 ./demo -algname kenlm -algmgr localhost:9001 -port 10080 --idx ../data/index.ann.500 -idxlen 500 -wt ../data/words_table.txt.500 -model ../test/kenlm/build/text.bin
 *
 *
 *
 * Usage
 * Step 1
 * export GLOG_log_dir="."
 *      生成的log文件 demo.INFO 可能会很大, log文件用于辅助调试
 * Step 2
 * echo "我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。" | ./demo -algname tagger -algmgr localhost:9001 -port 10080 --idx ../data/index.ann.500 -idxlen 500 -wt ../data/words_table.txt.500 -k 5 > all_sentences.txt
 *      将输入语料所有可能的句子组成的新句子输出到 all_sentences.txt, -k 就是knn的最近邻词个数
 * Step 3
 * cd into kenlm build
 * cat all_sentences.txt | bin/query text.bin -v sentence | grep 'Total' | awk '{print $2}' > score.txt
 *      得到句子的得分
 * Step 4
 * 用merge工具 BigRLab/tools/kenlm_merge.cpp 合并 all_sentences.txt and score.txt
 * ./merge all_sentences.txt score.txt merge.txt
 * Step 5
 * 对结果从大到小排序
 * cat merge.txt | sort -k1,1nr > final.txt
 */
#include <cstdio>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "jieba.hpp"
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "ArticleServiceHandler.h"
#include "ngram_model.hpp"

// .so 的名字
#define SERVICE_LIB_NAME        "kenlm"

#define TIMER_REJOIN            15          // 15s

using namespace BigRLab;

DEFINE_string(filter, "x", "Filter out specific kinds of word in word segment");
DEFINE_string(dict, "../dict/jieba.dict.utf8", "自带词典");
DEFINE_string(hmm, "../dict/hmm_model.utf8", "Path to hmm model file");
DEFINE_string(user_dict, "../dict/user.dict.utf8", "用户自定义词典");
DEFINE_string(idf, "../dict/idf.utf8", "训练过的idf，关键词提取");
DEFINE_string(stop_words, "../dict/stop_words.utf8", "停用词");

DEFINE_string(model, "", "filename of kenlm model (binary file)");

DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(addr, "", "Address of this algorithm server, use system detected if not specified.");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");

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

static bool validate_wt(const char* flagname, const std::string &value)
{ return check_not_empty( flagname, value ); }
static bool wt_dummy = gflags::RegisterFlagValidator(&FLAGS_wt, &validate_wt);

static bool validate_idx(const char* flagname, const std::string &value)
{ return check_not_empty( flagname, value ); }
static bool idx_dummy = gflags::RegisterFlagValidator(&FLAGS_idx, &validate_idx);

static bool validate_idxlen(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool idxlen_dummy = gflags::RegisterFlagValidator(&FLAGS_idxlen, &validate_idxlen);

static bool validate_model(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool model_dummy = gflags::RegisterFlagValidator(&FLAGS_model, &validate_model);

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

Jieba::pointer                  g_pJieba;
std::unique_ptr<NGram_Model>    g_pLMmodel;

static bool                                                g_bLoginSuccess = false;
static std::unique_ptr< boost::asio::deadline_timer >      g_Timer;


#if 0
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

void test_wordseg()
{
    cout << "test_wordseg():" << endl;
    Jieba::TagResult result;    
    string line, word, type;
    while (getline(cin, line)) {
        stringstream stream(line);
        stream >> word >> type;
        g_pJieba->tagging(word, result);
        if (!result.empty())
            cout << result[0].first << "\t" << result[0].second << endl;
    } // while
}

typedef std::vector<Jieba::Gram>    GramArray;
typedef std::vector<GramArray>      GramMatrix;

void print_all(const GramMatrix &mat, size_t idx, std::vector<std::size_t> &work )
{
    if (idx >= mat.size()) {
        ostringstream ostr;
        for (size_t i = 0; i != work.size(); ++i) {
            size_t j = work[i];
            ostr << mat[i][j].word << " ";
        } // for
        string output = ostr.str();
        output.erase(output.length()-1);
        cout << output << endl;
        return;
    } // if

    const GramArray &arr = mat[idx];
    for (size_t i = 0; i != arr.size(); ++i) {
        work[idx] = i;
        print_all(mat, idx + 1, work);
    } // for
}

void get_all_sentences( GramArray &arr )
{
    vector<float>   dist;
    GramMatrix      mat(arr.size());

    for (size_t i = 0; i != arr.size(); ++i) {
        g_pAnnDB->kNN_By_Gram(arr[i], (size_t)FLAGS_k, mat[i], dist);
        mat[i].insert(mat[i].begin(), arr[i]);
    } // for i

    DLOG(INFO) << "knn result:";
#ifndef NDEBUG
    size_t total = 1;
    ofstream ofs("knn_result.txt", ios::out);
    for (auto &row : mat) {
        total *= row.size();
        ostringstream ostr;
        copy(row.begin(), row.end(), ostream_iterator<Jieba::Gram>(ostr, " "));
        DLOG(INFO) << ostr.str();
        for (auto &v : row)
            ofs << v.word << " ";
        ofs << endl;
    } // for
    DLOG(INFO) << "Total " << total << " possible sentences.";
#endif

    vector<size_t> work(mat.size(), 0);
    // print_all(mat, 0, work);
}

void experiment()
{
    vector<float> dist;
    string text;    
    while (getline(cin, text)) {
        DLOG(INFO) << "Input text: " << text;
        GramArray tagResult;
        g_pJieba->wordSegment(text, tagResult);
#ifndef NDEBUG
        DLOG(INFO) << "After word segment:";
        ostringstream ostr;
        std::copy(tagResult.begin(), tagResult.end(), ostream_iterator<Jieba::Gram>(ostr, " "));
        DLOG(INFO) << ostr.str();
#endif 
        get_all_sentences(tagResult);
        // std::copy(tagResult.begin(), tagResult.end(), ostream_iterator<Jieba::Gram>(cout, " "));
        // cout << endl;
    } // while
}

} // namespace Test
#endif


static
void stop_server()
{
    DLOG(INFO) << "stop_server()";
    if (g_pThisServer)
        g_pThisServer->stop();
}

static
void stop_client()
{
    DLOG(INFO) << "stop_client()";
    if (g_pAlgMgrClient) {
        (*g_pAlgMgrClient)()->rmSvr(FLAGS_algname, *g_pSvrInfo);
        g_pAlgMgrClient->stop();
    } // if
}


static
void register_svr()
{
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

    cout << "Trying to start rpc service..." << endl;

    try {
        get_local_ip();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "get_local_ip() fail! " << ex.what();
        std::raise(SIGTERM);
        return; //!! 不可以直接exit，还有未join的io_serivice线程
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
void service_init()
{
    using namespace std;

    // check idx file
    {
        ifstream ifs(FLAGS_idx, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open annoy index file " << FLAGS_idx << " for reading!");
    }

    DLOG(INFO) << "Creating jieba instances...";
    g_pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
            FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
    g_pJieba->setFilter( FLAGS_filter );

    DLOG(INFO) << "Loading Annoy tree...";
    g_pAnnDB.reset( new WordAnnDB(FLAGS_idxlen) );
    g_pAnnDB->loadIndex( FLAGS_idx.c_str() );
    g_pAnnDB->loadWordTable( FLAGS_wt.c_str() );

    LOG(INFO) << "Totally " << g_pAnnDB->size() << " items loaded from annoy tree.";

    cout << "Initialzing LM model..." << endl;
    g_pLMmodel.reset(new NGram_Model(FLAGS_model));
    cout << "LM model initialize done!" << endl;
    // Test::experiment();

    // Test::test_wordseg();
    // exit(0);
}

static
void do_service_routine()
{
    using namespace std;
    // cout << "Initializing service..." << endl;
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
            stop_server(); 
        } );

        g_Timer.reset(new boost::asio::deadline_timer(std::ref(g_io_service)));
        g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_REJOIN));
        g_Timer->async_wait(rejoin);

        auto io_service_thr = boost::thread( [&]{ g_io_service.run(); } );

        do_service_routine(); // block on start server, unblock by stop_server

        DLOG(INFO) << "main cleaning up...";
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

/*
 * apiserver 退出后，按 Ctrl-C 退出本程序会收到异常
 * Exception caught by main: No more data to read.
 * 这是由 stop_client rmSvr 远程调用失败引发
 */
