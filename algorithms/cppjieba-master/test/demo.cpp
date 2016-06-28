/*
 * https://github.com/yanyiwu/cppjieba
 * ./demo -service -algname jieba -algmgr localhost:9001 -port 10080
 */
#include "jieba.hpp"
#include "Article2Vector.h"
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "ArticleServiceHandler.h"
// #include <boost/foreach.hpp>
// #include <boost/tuple/tuple.hpp>
// #include <boost/range/combine.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>

#define SERVICE_LIB_NAME        "article"

using namespace BigRLab;

DEFINE_string(filter, "x", "Filter out specific kinds of word in word segment");
DEFINE_string(dict, "../dict/jieba.dict.utf8", "自带词典");
DEFINE_string(hmm, "../dict/hmm_model.utf8", "Path to hmm model file");
DEFINE_string(user_dict, "../dict/user.dict.utf8", "用户自定义词典");
DEFINE_string(idf, "../dict/idf.utf8", "训练过的idf，关键词提取");
DEFINE_string(stop_words, "../dict/stop_words.utf8", "停用词");
DEFINE_int32(n_jieba_inst, 0, "Number of jieba instances");
DEFINE_bool(service, false, "Whether this program run in service mode");
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

typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;
typedef BigRLab::ThriftServer< Article::ArticleServiceIf, Article::ArticleServiceProcessor > ArticleAlgServer;
static AlgMgrClient::Pointer                  g_pAlgMgrClient;
static ArticleAlgServer::Pointer              g_pThisServer;
static boost::asio::io_service                g_io_service;
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

static bool validate_algname(const char* flagname, const std::string &value) 
{ 
    if (!FLAGS_service)
        return true;
    return check_not_empty(flagname, value); 
}
static const bool algname_dummy = gflags::RegisterFlagValidator(&FLAGS_algname, &validate_algname);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    using namespace std;

    if (!FLAGS_service)
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
    if (!FLAGS_service)
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
    if (!FLAGS_service)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{
    if (!FLAGS_service)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);

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
        exit(-1);
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
    boost::shared_ptr< Article::ArticleServiceIf > 
            pHandler = boost::make_shared< Article::ArticleServiceHandler >();
    // Test::test1(pHandler);
    g_pThisServer = boost::make_shared< ArticleAlgServer >(pHandler, g_nThisPort);
    try {
        g_pThisServer->start(); //!! NOTE blocking until quit
    } catch (const std::exception &ex) {
        cerr << "Start this alg server fail, " << ex.what() << endl;
        exit(-1);
    } // try
}

static
void service_init()
{
    if (FLAGS_n_jieba_inst <= 0 || FLAGS_n_jieba_inst > FLAGS_n_work_threads) {
        LOG(INFO) << "Adjust -n_jieba_inst from old value " << FLAGS_n_jieba_inst 
            << " to new value -n_work_threads " << FLAGS_n_work_threads;
        FLAGS_n_jieba_inst = FLAGS_n_work_threads;
    } // if

    // LOG(INFO) << "FLAGS_n_jieba_inst = " << FLAGS_n_jieba_inst;

    for (int i = 0; i < FLAGS_n_jieba_inst; ++i) {
        auto pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
                FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
        pJieba->setFilter( FLAGS_filter );
        g_JiebaPool.push(pJieba);
    } // for
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
void do_standalone_routine()
{
    using namespace std;

    cout << "Running in standalone mode..." << endl;
    auto pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
            FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
    pJieba->setFilter( FLAGS_filter );

    string content = "我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。";
    Jieba::KeywordResult result;
    pJieba->keywordExtract(content, result, 5);
    for (auto& v : result)
        cout << v << endl;
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
        signals.async_wait( [](const boost::system::error_code& error, int signal)
                { stop_server(); } );
        auto io_service_thr = boost::thread( [&]{ g_io_service.run(); } );

        if (FLAGS_service)
            do_service_routine();
        else
            do_standalone_routine();

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


#if 0
int main(int argc, char **argv)
{
    cppjieba::Jieba jieba(DICT_PATH,
            HMM_PATH,
            USER_DICT_PATH);
    cppjieba::KeywordExtractor extractor(jieba,
            IDF_PATH,
            STOP_WORD_PATH);

    FilterSet filter;

    if (argc > 1) {
        char *strFilter = argv[1];
        for (char *p = strtok(strFilter, ":"); p; p = strtok(NULL, ":"))
            filter.insert(p);
    } // if

    TagResult result;

    string line;
    while (getline(cin, line)) {
        boost::trim(line);
        // tagging
        do_tagging(jieba, line, filter, result);
        for (const auto &v : result) {
            cout << v.first << " ";
            // cout << v.first << "\t" << v.second << endl;
        } // for
        cout << endl;
        // keyword extract
        // ExtractResult result;
        // extract_keywords(extractor, 5, line, result);
        // cout << result << endl;
    } // while

    return 0;
}
#endif

#if 0
int main(int argc, char** argv) {
  cppjieba::Jieba jieba(DICT_PATH,
        HMM_PATH,
        USER_DICT_PATH);
  vector<string> words;
  vector<cppjieba::Word> jiebawords;
  string s;
  string result;

  // ##################### Mix segment
  s = "他来到了网易杭研大厦";
  cout << s << endl;
  cout << "[demo] Cut With HMM" << endl;
  jieba.Cut(s, words, true);
  cout << limonp::Join(words.begin(), words.end(), "/") << endl;
  // #####################

  cout << "[demo] Cut Without HMM " << endl;
  jieba.Cut(s, words, false);
  cout << limonp::Join(words.begin(), words.end(), "/") << endl;

  s = "我来到北京清华大学";
  cout << s << endl;
  cout << "[demo] CutAll" << endl;
  jieba.CutAll(s, words);
  cout << limonp::Join(words.begin(), words.end(), "/") << endl;

  s = "小明硕士毕业于中国科学院计算所，后在日本京都大学深造";
  cout << s << endl;
  cout << "[demo] CutForSearch" << endl;
  jieba.CutForSearch(s, words);
  cout << limonp::Join(words.begin(), words.end(), "/") << endl;

  cout << "[demo] Insert User Word" << endl;
  jieba.Cut("男默女泪", words);
  cout << limonp::Join(words.begin(), words.end(), "/") << endl;
  jieba.InsertUserWord("男默女泪"); // 向词典中加词
  jieba.Cut("男默女泪", words);
  cout << limonp::Join(words.begin(), words.end(), "/") << endl;

  cout << "[demo] CutForSearch Word With Offset" << endl;
  jieba.CutForSearch(s, jiebawords, true);
  cout << jiebawords << endl;

  // ################################# 词性标注，需要重新封装
  cout << "[demo] Tagging" << endl;
  vector<pair<string, string> > tagres;
  s = "我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。";
  jieba.Tag(s, tagres);
  cout << s << endl;
  cout << tagres << endl;;
  // #################################

  // 抽取关键词
  cppjieba::KeywordExtractor extractor(jieba,
        IDF_PATH,
        STOP_WORD_PATH);
  cout << "[demo] Keyword Extraction" << endl;
  const size_t topk = 5;
  vector<cppjieba::KeywordExtractor::Word> keywordres;
  extractor.Extract(s, keywordres, topk);
  cout << s << endl;
  cout << keywordres << endl;
  return EXIT_SUCCESS;
}
#endif 

// typedef std::set<std::string>   FilterSet;
// typedef std::vector< std::pair<std::string, std::string> > TagResult;
// typedef std::vector<cppjieba::KeywordExtractor::Word> ExtractResult;


// const char* const DICT_PATH = "../dict/jieba.dict.utf8"; // 自带词典
// const char* const HMM_PATH = "../dict/hmm_model.utf8";
// const char* const USER_DICT_PATH = "../dict/user.dict.utf8"; // 用户自定义词典
// const char* const IDF_PATH = "../dict/idf.utf8"; // 训练过的idf，关键词提取
// const char* const STOP_WORD_PATH = "../dict/stop_words.utf8";  // 停用词


// static
// void print_usage(const char *appname)
// {
    // cout << "Usage: " << appname << " filterlist" << endl;
    // cout << "filterlist in format of \"x;nr;adj\", sperated by ;" << endl;
// }

// static
// void do_tagging(cppjieba::Jieba &jieba,
                // const std::string &content,
                // const FilterSet &filter,
                // TagResult &result)
// {
    // TagResult _Result;
    // jieba.Tag(content, _Result);

    // result.clear();
    // result.reserve( _Result.size() );
    // for (auto &v : _Result) {
        // if (!filter.count(v.second)) {
            // result.push_back( TagResult::value_type() );
            // result.back().first.swap(v.first);
            // result.back().second.swap(v.second);
        // } // if
    // } // for

    // result.shrink_to_fit();
// }

// static
// void extract_keywords(cppjieba::KeywordExtractor &extractor,
                      // const size_t topk,
                      // const std::string &src,
                      // ExtractResult &result)
// {
    // result.reserve( topk );
    // extractor.Extract(src, result, topk);
// }

