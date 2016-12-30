/*
 * GLOG_logtostderr=1 ./demo -algname tagger -algmgr localhost:9001 -port 10080 -concur ../data/wiki_cooccur.out -tagset ../data/tagset.txt -vec warplda -warpldaModel ../data/warplda_train.model -warpldaVocab ../data/warplda.vocab -cluster_pred manual -cluster_model ../data/cluster.model -word_cluster ../data/word_cluster.txt -threshold 0.1
 *
 * get cluster id by knn
 * GLOG_logtostderr=1 ./demo -algname tagger -algmgr localhost:9001 -port 10080 -concur ../data/wiki_cooccur.out -tagset ../data/tagset.txt -vec warplda -warpldaModel ../data/warplda_train.model -warpldaVocab ../data/warplda.vocab -cluster_pred knn -cluster_idx ../data/cluster.idx -nfeatures 100 -word_cluster ../data/word_cluster.txt -threshold 0.1
 *
 * test:
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"method":"concur","k1":100,"k2":100,"topk":5,"text":"苹果是一家很牛逼的公司"}' http://localhost:9000/tagger
 *
 * -vec wordvec -vecdict filename
 * -vec clusterid -vecdict filename
 */
#include <cstdio>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "rpc_module.h"
#include "AlgMgrService.h"
#include "ArticleServiceHandler.h"

#define SERVICE_LIB_NAME        "tagging2"

#define TIMER_CHECK            15          // 15s

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

DEFINE_string(vec, "", "How article converted to vector, \"wordvec\" or \"clusterid\" or \"warplda\"");
DEFINE_string(vecdict, "", "File contains word info, word vector or word clusterID");
DEFINE_string(warpldaModel, "", "warplda model file");
DEFINE_string(warpldaVocab, "", "warplda vocab file");

DEFINE_string(cluster_pred, "", "预测请求文本cluster的方法，manual knn");
DEFINE_string(cluster_model, "", "由yakmo生成的cluster信息文件");
DEFINE_string(cluster_idx, "", "cluster annoy tree");
DEFINE_int32(nfeatures, 0, "cluster feature 个数");
DEFINE_string(word_cluster, "", "单词属于每一个cluster的概率");
DEFINE_double(threshold, 0.0, "tag属于text cluster的阈值");

// DEFINE_string(wt, "", "File of word table");
// DEFINE_string(idx, "", "File of word annoy tree");
// DEFINE_int32(idxlen, 0, "vector len of annoy vector");


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

Article2Vector::pointer                 g_pVecConverter;
ClusterPredict::pointer                 g_pClusterPredict;
WordClusterDB::pointer                  g_pWordClusterDB;
// boost::shared_ptr<WordAnnDB>            g_pAnnDB;
boost::shared_ptr<ConcurTable>          g_pConcurTable;
std::set<std::string>                   g_TagSet;

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

// static bool validate_wt(const char* flagname, const std::string &value)
// { return check_not_empty( flagname, value ); }
// static bool wt_dummy = gflags::RegisterFlagValidator(&FLAGS_wt, &validate_wt);

// static bool validate_idx(const char* flagname, const std::string &value)
// { return check_not_empty( flagname, value ); }
// static bool idx_dummy = gflags::RegisterFlagValidator(&FLAGS_idx, &validate_idx);

// static bool validate_idxlen(const char* flagname, gflags::int32 value) 
// { return check_above_zero(flagname, value); }
// static const bool idxlen_dummy = gflags::RegisterFlagValidator(&FLAGS_idxlen, &validate_idxlen);

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

// SharedQueue<Jieba::pointer>      g_JiebaPool;
Jieba::pointer g_pJieba;

static volatile bool                                       g_bLoginSuccess = false;
static std::unique_ptr< boost::asio::deadline_timer >      g_Timer;
std::set<std::string>                   g_setArgFiles;
std::unique_ptr<std::thread>            g_pSvrThread;


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

// void test_anndb()
// {
    // vector<string> result;
    // vector<float>  distances;
    // g_pAnnDB->kNN_By_Word("你他妈的去死吧", 10, result, distances);
    // for (size_t i = 0; i < result.size(); ++i)
        // cout << result[i] << "\t" << distances[i] << endl;
// }

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

void test_cluster_predict()
{
    ClusterPredict::pointer pClusterPred = 
            boost::make_shared<ClusterPredictManual>("../data/cluster_model_test");
    vector<double> vec(5, 0.0);
    uint32_t cid = pClusterPred->predict(vec);
    cout << "cid = " << cid << endl;
}

void test_word_cluster_db()
{
    auto pWordCluster = boost::make_shared<WordClusterDB>("../data/word_cluster.txt");
    auto ret = pWordCluster->query("LOFT", 200);
    if (ret.second)
        cout << "cluster probability = " << ret.first << endl;
    else
        cout << "Not found!" << endl;
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
        g_bLoginSuccess = false;
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
void rejoin()
{
    // DLOG(INFO) << "rejoin()";

    if (g_bLoginSuccess) {
        try {
            if (!g_pAlgMgrClient->isRunning())
                g_pAlgMgrClient->start(50, 300);
            (*g_pAlgMgrClient)()->addSvr(FLAGS_algname, *g_pSvrInfo);
        } catch (const std::exception &ex) {
            LOG(ERROR) << "Connection with apiserver lost, re-connecting...";
            g_pAlgMgrClient.reset();
            g_pAlgMgrClient = boost::make_shared< AlgMgrClient >(g_strAlgMgrAddr, g_nAlgMgrPort);
        } // try
    } // if
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

    cout << "Creating article to vector converter..." << endl;

    if ("wordvec" == FLAGS_vec) {
        if (FLAGS_vecdict.empty()) {
            THROW_RUNTIME_ERROR("You have to specify vecdict file thru -vecdict");
        } else {
            ifstream ifs(FLAGS_vecdict, ios::in);
            if (!ifs)
                THROW_RUNTIME_ERROR("Cannot open vecdict file " << FLAGS_vecdict);
        } // if

        // get n_class
        stringstream stream;
        stream << "tail -1 " << FLAGS_vecdict << " | awk \'{print NF}\'" << flush;

        boost::shared_ptr<FILE> fp;
        fp.reset(popen(stream.str().c_str(), "r"), [](FILE *ptr){
            if (ptr) pclose(ptr);        
        });

        uint32_t nClasses = 0;
        if( fscanf(fp.get(), "%u", &nClasses) != 1 || !nClasses )
            THROW_RUNTIME_ERROR("Cannot get num of classes from file " << FLAGS_vecdict);
        fp.reset();

        --nClasses;

        LOG(INFO) << "Detected nClasses = " << nClasses << " from file " << FLAGS_vecdict;

        g_pVecConverter = boost::make_shared<Article2VectorByWordVec>( nClasses, FLAGS_vecdict.c_str() );
        g_setArgFiles.insert(FLAGS_vecdict);

    } else if ("clusterid" == FLAGS_vec) {
        if (FLAGS_vecdict.empty()) {
            THROW_RUNTIME_ERROR("You have to specify vecdict file thru -vecdict");
        } else {
            ifstream ifs(FLAGS_vecdict, ios::in);
            if (!ifs)
                THROW_RUNTIME_ERROR("Cannot open vecdict file " << FLAGS_vecdict);
        } // if

        // get n_class
        stringstream stream;
        stream << "cat " << FLAGS_vecdict << " | awk \'{print $2}\' | sort -nr | head -1" << flush;

        boost::shared_ptr<FILE> fp;
        fp.reset(popen(stream.str().c_str(), "r"), [](FILE *ptr){
            if (ptr) pclose(ptr);        
        });

        uint32_t nClasses = 0;
        if( fscanf(fp.get(), "%u", &nClasses) != 1 || !nClasses )
            THROW_RUNTIME_ERROR("Cannot get num of classes from file " << FLAGS_vecdict);
        fp.reset();

        ++nClasses;

        LOG(INFO) << "Detected nClasses = " << nClasses << " from file " << FLAGS_vecdict;

        g_pVecConverter = boost::make_shared<Article2VectorByCluster>( nClasses, FLAGS_vecdict.c_str() );
        g_setArgFiles.insert(FLAGS_vecdict);
        
    } else if ("warplda" == FLAGS_vec) {
        if (FLAGS_warpldaModel.empty()) {
            THROW_RUNTIME_ERROR("You have to specify model file thru -warpldaModel");
        } else {
            ifstream ifs(FLAGS_warpldaModel, ios::in);
            if (!ifs)
                THROW_RUNTIME_ERROR("Cannot open model file " << FLAGS_warpldaModel);
        } // if

        if (FLAGS_warpldaVocab.empty()) {
            THROW_RUNTIME_ERROR("You have to specify vocab file thru -warpldaVocab");
        } else {
            ifstream ifs(FLAGS_warpldaVocab, ios::in);
            if (!ifs)
                THROW_RUNTIME_ERROR("Cannot open vocab file " << FLAGS_warpldaVocab);
        } // if

        g_pVecConverter = boost::make_shared<Article2VectorByWarplda>(FLAGS_warpldaModel, FLAGS_warpldaVocab);
        g_setArgFiles.insert(FLAGS_warpldaModel);
        g_setArgFiles.insert(FLAGS_warpldaVocab);
        // DLOG(INFO) << "Detected " << g_pVecConverter->nClasses() << " classes.";

    } else {
        THROW_RUNTIME_ERROR( FLAGS_vec << " is not a valid argument");
    } // if

    // LOG(INFO) << "FLAGS_n_jieba_inst = " << FLAGS_n_jieba_inst;
    
    cout << "Creating jieba instances..." << endl;
    // for (int i = 0; i < FLAGS_n_jieba_inst; ++i) {
        // auto pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
                // FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
        // pJieba->setFilter( FLAGS_filter );
        // g_JiebaPool.push(pJieba);
    // } // for
    g_pJieba = boost::make_shared<Jieba>(FLAGS_dict, FLAGS_hmm, 
                FLAGS_user_dict, FLAGS_idf, FLAGS_stop_words);
    g_pJieba->setFilter( FLAGS_filter );
    g_setArgFiles.insert(FLAGS_dict);
    g_setArgFiles.insert(FLAGS_hmm);
    g_setArgFiles.insert(FLAGS_user_dict);
    g_setArgFiles.insert(FLAGS_idf);
    g_setArgFiles.insert(FLAGS_stop_words);

    cout << "Loading tagset..." << endl;
    load_tagset(FLAGS_tagset, g_TagSet);
    g_setArgFiles.insert(FLAGS_tagset);

    cout << "Loading concur table..." << endl;
    g_pConcurTable.reset( new ConcurTable );
    g_pConcurTable->loadFromFile( FLAGS_concur, g_TagSet );
    g_setArgFiles.insert(FLAGS_concur);

    cout << "Loading cluster model..." << endl;
    if (FLAGS_cluster_pred.empty()) {
        THROW_RUNTIME_ERROR("-cluster_pred not set!");
    } else if ("manual" == FLAGS_cluster_pred) {
        g_pClusterPredict = boost::make_shared<ClusterPredictManual>(FLAGS_cluster_model);
        g_setArgFiles.insert(FLAGS_cluster_model);
    } else if ("knn" == FLAGS_cluster_pred) {
        THROW_RUNTIME_ERROR_IF(FLAGS_nfeatures <= 0, "Invalid -nfeatures arg!");
        g_pClusterPredict = boost::make_shared<ClusterPredictKnn>(FLAGS_cluster_idx, FLAGS_nfeatures);
        g_setArgFiles.insert(FLAGS_cluster_idx);
    } else {
        THROW_RUNTIME_ERROR("Unknown -cluster_pred arg!");
    } // if

    cout << "Loading word cluster db..." << endl;
    g_pWordClusterDB = boost::make_shared<WordClusterDB>(FLAGS_word_cluster);
    g_setArgFiles.insert(FLAGS_word_cluster);

    LOG(INFO) << "Totally " << g_TagSet.size() << " tags in tagset.";
    LOG(INFO) << "Totally " << g_pConcurTable->size() << " concur records loaded.";

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

static
void check_update()
{
    using namespace std;

    // DLOG(INFO) << "check_update()";

    if (!g_bLoginSuccess)
        return;

    bool    hasUpdate = false;

    for (auto& name : g_setArgFiles) {
        // DLOG(INFO) << "name = " << name;
        string updateName = name + ".update";
        if (boost::filesystem::exists(updateName)) {
            LOG(INFO) << "Detected update for " << name;
            try {
                boost::filesystem::rename(updateName, name);
                hasUpdate = true;
            } catch (...) {}
        } // if
    } // for

    if (hasUpdate) {
        LOG(INFO) << "Restarting service for updating...";
        stop_client();
        stop_server();
        if (g_pSvrThread && g_pSvrThread->joinable())
            g_pSvrThread->join();
        // g_JiebaPool.clear();
        g_pSvrThread.reset(new std::thread([]{
            try {
                do_service_routine();
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Start service fail! " << ex.what();
                std::raise(SIGTERM);
            } // try             
        }));
    } // if
}

static
void timer_check(const boost::system::error_code &ec)
{
    rejoin();
    check_update();
    g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_CHECK));
    g_Timer->async_wait(timer_check);
}


int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        // Test::test1();
        // Test::test2();
        // Test::test_cluster_predict();
        // Test::test_word_cluster_db();
        // return 0;
        
        // install signal handler
        auto pIoServiceWork = boost::make_shared< boost::asio::io_service::work >(std::ref(g_io_service));

        boost::asio::signal_set signals(g_io_service, SIGINT, SIGTERM);
        signals.async_wait( [&](const boost::system::error_code& error, int signal) { 
            if (g_Timer) g_Timer->cancel();
            try { stop_client(); } catch (...) {}
            try { stop_server(); } catch (...) {}
            if (g_pSvrThread && g_pSvrThread->joinable())
                g_pSvrThread->join();
            pIoServiceWork.reset();
            g_io_service.stop();
        } );

        g_Timer.reset(new boost::asio::deadline_timer(std::ref(g_io_service)));
        g_Timer->expires_from_now(boost::posix_time::seconds(TIMER_CHECK));
        g_Timer->async_wait(timer_check);

        g_pSvrThread.reset(new std::thread([]{
            try {
                do_service_routine();
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Start service fail! " << ex.what();
                std::raise(SIGTERM);
            } // try             
        }));

        g_io_service.run();

        cout << argv[0] << " done!" << endl;

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        return -1;
    } // try

    return 0;
}


