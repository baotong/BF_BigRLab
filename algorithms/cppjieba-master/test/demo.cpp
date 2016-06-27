/*
 * https://github.com/yanyiwu/cppjieba
 */
#include "cppjieba/Jieba.hpp"
#include "cppjieba/KeywordExtractor.hpp"
#include "rpc_module.h"
#include "shared_queue.h"
#include "AlgMgrService.h"
#include "WordSegService.h"
#include <sstream>
#include <set>
#include <algorithm>
#include <iterator>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>

#define SPACES " \t\f\r\v\n"

#define SLEEP_MILLISECONDS(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))
#define SLEEP_SECONDS(x)      std::this_thread::sleep_for(std::chrono::seconds(x))

#define SERVICE_LIB_NAME        "TODO"

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

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

static boost::asio::io_service                g_io_service;

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

class Jieba {
public:
    typedef boost::shared_ptr<Jieba>    pointer;
    typedef std::set<std::string>       FilterSet;
    typedef std::vector<std::string>    WordVector;
    typedef std::vector< std::pair<std::string, std::string> > TagResult;
    typedef std::vector<cppjieba::KeywordExtractor::Word>      KeywordResult;

public:
    explicit Jieba(const std::string &dict_path, 
                   const std::string &hmm_path,
                   const std::string &user_dict_path,
                   const std::string &idf_path,
                   const std::string &stop_word_path)
        : DICT_PATH(dict_path)
        , HMM_PATH(hmm_path)
        , USER_DICT_PATH(user_dict_path)
        , IDF_PATH(idf_path)
        , STOP_WORD_PATH(stop_word_path)
    {
        m_pJieba = boost::make_shared<cppjieba::Jieba>(DICT_PATH.c_str(), HMM_PATH.c_str(), USER_DICT_PATH.c_str());
        m_pExtractor = boost::make_shared<cppjieba::KeywordExtractor>(*m_pJieba, IDF_PATH.c_str(), STOP_WORD_PATH.c_str());
    }

    void setFilter( const std::string &strFilter )
    {
        if (strFilter.empty())
            return;

        FilterSet filter; 
        char *cstrFilter = const_cast<char*>(strFilter.c_str());
        for (char *p = strtok(cstrFilter, ";" SPACES); p; p = strtok(NULL, ";" SPACES))
            filter.insert(p);

        setFilter( filter );
    }

    void setFilter( FilterSet &filter )
    { m_setFilter.swap(filter); }

    void tagging( const std::string &content, TagResult &result )
    {
        TagResult _Result;
        m_pJieba->Tag(content, _Result);

        result.clear();
        result.reserve( _Result.size() );
        for (auto &v : _Result) {
            if (!m_setFilter.count(v.second)) {
                result.push_back( TagResult::value_type() );
                result.back().first.swap(v.first);
                result.back().second.swap(v.second);
            } // if
        } // for

        result.shrink_to_fit();
    }

    void wordSegment( const std::string &content, WordVector &result )
    {
        TagResult tagResult;
        tagging(content, tagResult);

        result.resize( tagResult.size() );
        for (std::size_t i = 0; i < result.size(); ++i)
            result[i].swap( tagResult[i].first );
    }

    void keywordExtract( const std::string &content, KeywordResult &result, const std::size_t topk )
    { m_pExtractor->Extract(content, result, topk); }

protected:
    const std::string   DICT_PATH; 
    const std::string   HMM_PATH; 
    const std::string   USER_DICT_PATH; 
    const std::string   IDF_PATH; 
    const std::string   STOP_WORD_PATH; 
    FilterSet                                     m_setFilter;
    boost::shared_ptr<cppjieba::Jieba>            m_pJieba;
    boost::shared_ptr<cppjieba::KeywordExtractor> m_pExtractor;
};

static SharedQueue<Jieba::pointer>      g_JiebaPool;

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
    // if (g_pThisServer)
        // g_pThisServer->stop();
}

static
void stop_client()
{
    // if (g_pAlgMgrClient) {
        // (*g_pAlgMgrClient)()->rmSvr(FLAGS_algname, *g_pSvrInfo);
        // g_pAlgMgrClient->stop();
    // } // if
}


static
void service_init()
{
    if (FLAGS_n_jieba_inst <= 0 || FLAGS_n_jieba_inst > FLAGS_n_work_threads) {
        LOG(INFO) << "Adjust -n_jieba_inst from old value " << FLAGS_n_jieba_inst 
            << " to new value -n_work_threads " << FLAGS_n_work_threads;
        FLAGS_n_jieba_inst = FLAGS_n_work_threads;
    } // if

    boost::thread_group thrgrp;
    for (int i = 0; i < FLAGS_n_jieba_inst; ++i)
        thrgrp.create_thread( [&]{
            auto pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
                        FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
            pJieba->setFilter( FLAGS_filter );
            g_JiebaPool.push(pJieba);
        } );
    thrgrp.join_all();
}

static
void do_service_routine()
{
    service_init();
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

