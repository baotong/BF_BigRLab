/*
 * https://github.com/yanyiwu/cppjieba
 */
#include "cppjieba/Jieba.hpp"
#include "cppjieba/KeywordExtractor.hpp"
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "WordSegService.h"
#include <sstream>
#include <set>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>

#define SERVICE_LIB_NAME        "wordseg"

DEFINE_bool(service, true, "Whether this program run in service mode");
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(svraddr, "", "Address of this RPC algorithm server, in form of addr:port");
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
bool validate_svraddr(const char* flagname, const std::string &value) 
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

} // namespace


namespace {

static
bool get_local_ip( std::string &result )
{
    using namespace boost::asio::ip;

    try {
        boost::asio::io_service netService;
        boost::asio::io_service::work work(netService);
        
        boost::thread ioThr( [&]{ netService.run(); } );

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

} // namespace






typedef std::set<std::string>   FilterSet;
typedef std::vector< std::pair<std::string, std::string> > TagResult;
typedef std::vector<cppjieba::KeywordExtractor::Word> ExtractResult;

using namespace std;

const char* const DICT_PATH = "../dict/jieba.dict.utf8"; // 自带词典
const char* const HMM_PATH = "../dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "../dict/user.dict.utf8"; // 用户自定义词典
const char* const IDF_PATH = "../dict/idf.utf8"; // 训练过的idf，关键词提取
const char* const STOP_WORD_PATH = "../dict/stop_words.utf8";  // 停用词


static
void print_usage(const char *appname)
{
    cout << "Usage: " << appname << " filterlist" << endl;
    cout << "filterlist in format of \"x;nr;adj\", sperated by ;" << endl;
}

static
void do_tagging(cppjieba::Jieba &jieba,
                const std::string &content,
                const FilterSet &filter,
                TagResult &result)
{
    TagResult _Result;
    jieba.Tag(content, _Result);

    result.clear();
    result.reserve( _Result.size() );
    for (auto &v : _Result) {
        if (!filter.count(v.second)) {
            result.push_back( TagResult::value_type() );
            result.back().first.swap(v.first);
            result.back().second.swap(v.second);
        } // if
    } // for

    // result.shrink_to_fit();
}


static
void extract_keywords(cppjieba::KeywordExtractor &extractor,
                      const size_t topk,
                      const std::string &src,
                      ExtractResult &result)
{
    result.reserve( topk );
    extractor.Extract(src, result, topk);
}


#if 0
int main(int argc, char **argv)
{
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
    } // try

    return 0;
}
#endif



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
