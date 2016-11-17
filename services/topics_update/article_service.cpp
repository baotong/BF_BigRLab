#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <sstream>
#include <cstring>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/format.hpp>
#include <glog/logging.h>
#include "common.hpp"
#include "article_service.h"

using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new ArticleService(name); }

const char* lib_name()
{ return "topics_update"; }

ArticleService::ArticleClientPtr ArticleService::IdleClientQueue::getIdleClient()
{
    // DLOG(INFO) << "IdleClientQueue::getIdleClient() size = " << this->size();

    ArticleClientPtr pRet;

    do {
        ArticleClientWptr wptr;
        if (!this->timed_pop(wptr, TIMEOUT))
            return ArticleClientPtr();      // return empty ptr when no client available
        pRet = wptr.lock();
        // DLOG_IF(INFO, !pRet) << "IdleClientQueue::getIdleClient() got empty ptr";
    } while (!pRet);

    return pRet;
}

ArticleService::ArticleClientArr::ArticleClientArr(const BigRLab::AlgSvrInfo &svr, 
                                    IdleClientQueue *idleQue, int n) 
{
    // DLOG(INFO) << "ArticleClientArr constructor " << svr.addr << ":" << svr.port
            // << " n = " << n;
    
    clients.reserve(n);
    for (int i = 0; i < n; ++i) {
        auto pClient = boost::make_shared<ArticleClient>(svr.addr, (uint16_t)(svr.port));
        if (pClient->start(50, 300)) // totally wait 15s
            clients.push_back(pClient);
#ifndef NDEBUG
        else
            DLOG(INFO) << "Fail to create client instance to " << svr.addr << ":" << svr.port;
#endif
    } // for

    // DLOG(INFO) << "clients.size() = " << clients.size();

    // idleQue insert and shuffle
    if (!clients.empty()) {
        boost::unique_lock<boost::mutex> lock( idleQue->mutex() );
        idleQue->insert( idleQue->end(), clients.begin(), clients.end() );
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(idleQue->begin(), idleQue->end(), g);
    } // if
}


struct Doc2VecTask : BigRLab::WorkItemBase {
    Doc2VecTask( std::size_t _Id,
                 std::string &_Text, 
                 bool _WordSeg,
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , wordseg(_WordSeg)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName)
    { text.swap(_Text); }

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<double> result;
                pClient->client()->toVector( result, text, wordseg );
                done = true;
                idleClients->putBack( pClient );

                ostringstream output(ios::out);
                if (!result.empty()) {
                    output << id << "\t";
                    for (auto& v : result)
                        output << v << " ";
                    output << flush;
                } else {
                    output << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << output.str() << endl;

            } catch (const Article::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try
        } while (!done);
    }

    std::size_t                     id;
    std::string                     text;
    bool                            wordseg;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
};


struct TopicTask : BigRLab::WorkItemBase {
    TopicTask( std::size_t _Id,
                 std::string &_Text, 
                 int _TopK, int _SearchK, bool _WordSeg,
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , topk(_TopK), searchK(_SearchK), wordseg(_WordSeg)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName)
    { text.swap(_Text); }

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, text, topk, searchK, wordseg, "" );
                done = true;
                idleClients->putBack( pClient );

                ostringstream output(ios::out);
                if (!result.empty()) {
                    output << id << "\t";
                    for (auto& v : result)
                        output << v.id << ":" << v.distance << " ";
                    output << flush;
                } else {
                    output << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << output.str() << endl;

            } catch (const Article::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try
        } while (!done);
    }

    std::size_t                     id;
    std::string                     text;
    int                             topk, searchK;
    bool                            wordseg;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
};


struct TopicLabelTask : BigRLab::WorkItemBase {
    TopicLabelTask( std::size_t _Id,
                 const std::string &_Text,
                 int _TopK, int _SearchK, bool _WordSeg,
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , text(_Text)
        , topk(_TopK), searchK(_SearchK), wordseg(_WordSeg)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName) {}

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        if (text.empty()) return;

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, text, topk, searchK, wordseg, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // 在这里统计
                ostringstream oss(ios::out);
                if (!result.empty()) {
                    typedef map<string, double> Statistics;
                    Statistics statistics;

                    for (auto& v : result)
                        statistics[v.label] += 1.0;

                    for (auto& v : statistics)
                        v.second /= (double)(result.size());

                    boost::ptr_vector<Statistics::value_type, boost::view_clone_allocator> sorted(statistics.begin(), statistics.end());
                    sorted.sort([](const Statistics::value_type &lhs, const Statistics::value_type &rhs)->bool {
                        return (lhs.second > rhs.second);
                    });

                    oss << id << "\t";
                    auto it = sorted.begin();
                    for (; it != sorted.end(); ++it)
                        oss << it->first << ":" << it->second << " ";
                    oss << flush;
                } else {
                    oss << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << oss.str() << endl;

            } catch (const Article::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try

        } while (!done);
    }

    std::size_t                     id;
    std::string                     text;
    int                             topk, searchK;
    bool                            wordseg;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
    static constexpr const char* REQTYPE = "label";
};


struct TopicScoreTask : BigRLab::WorkItemBase {
    TopicScoreTask( std::size_t _Id,
                 const std::string &_Text,
                 int _TopK, int _SearchK, bool _WordSeg,
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , text(_Text)
        , topk(_TopK), searchK(_SearchK), wordseg(_WordSeg)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName) {}

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        if (text.empty()) return;

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, text, topk, searchK, wordseg, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // 在这里统计
                ostringstream oss(ios::out);
                if (!result.empty()) {
                    double avgScore = 0.0;
                    for (auto& v : result)
                        avgScore += v.score;
                    avgScore /= (double)(result.size());
                    oss << id << "\t" << avgScore << flush;
                } else {
                    oss << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << oss.str() << endl;

            } catch (const Article::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try

        } while (!done);
    }

    std::size_t                     id;
    std::string                     text;
    int                             topk, searchK;
    bool                            wordseg;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
    static constexpr const char* REQTYPE = "score";
};


struct RMSE : public boost::shared_lockable_adapter<boost::shared_mutex> {
    RMSE() : val(0.0), cnt(0) {}
    double          val;
    std::size_t     cnt;
};

struct TopicScoreTestTask : BigRLab::WorkItemBase {
    TopicScoreTestTask( std::size_t _Id,
                 const std::string &_Text,
                 double _Expected,
                 int _TopK, int _SearchK, bool _WordSeg,
                 RMSE *_Rmse,
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , text(_Text), expected(_Expected)
        , topk(_TopK), searchK(_SearchK), wordseg(_WordSeg)
        , pRMSE(_Rmse)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName) {}

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        if (text.empty()) return;

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, text, topk, searchK, wordseg, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // 在这里统计
                ostringstream oss(ios::out);
                if (!result.empty()) {
                    double avgScore = 0.0;
                    for (auto& v : result)
                        avgScore += v.score;
                    avgScore /= (double)(result.size());
                    oss << id << "\t" << avgScore << flush;
                    boost::unique_lock<RMSE> lk(*pRMSE);
                    pRMSE->val += (expected - avgScore) * (expected - avgScore);
                    ++(pRMSE->cnt);
                } else {
                    oss << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << oss.str() << endl;

            } catch (const Article::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try

        } while (!done);
    }

    std::size_t                     id;
    std::string                     text;
    double                          expected;
    int                             topk, searchK;
    bool                            wordseg;
    RMSE                            *pRMSE;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
    static constexpr const char* REQTYPE = "score";
};


struct LabelFactor {
    LabelFactor() : a(0), b(0), c(0)
                  , p(0.0), r(0.0), f1(0.0) {}
/*
 * a: predicted == expected == label
 * b: predicted == label expected != label
 * c: predicted != label expected == label
 * p: a / (a + b)
 * r: a / (a + c)
 * f1: 2 * p * r / (p + r)
 * NOTE!!! 首选判断分母是否为0
 */
    uint32_t a, b, c;
    double p, r, f1;
};

struct LabelFactorDict : public std::map<std::string, LabelFactor>
                       , public boost::shared_lockable_adapter<boost::shared_mutex>
{};


struct TopicLabelTestTask : BigRLab::WorkItemBase {
    TopicLabelTestTask( std::size_t _Id,
                 const std::string &_Text, const std::string &_Expected,
                 int _TopK, int _SearchK, bool _WordSeg,
                 LabelFactorDict *_LfDict,
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , text(_Text), expected(_Expected)
        , topk(_TopK), searchK(_SearchK), wordseg(_WordSeg)
        , lfDict(_LfDict)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName) {}

    virtual void run()
    {
        using namespace std;

        ON_FINISH_CLASS(pCleanup, {++*counter; cond->notify_all();});

        if (text.empty() || expected.empty())
            return;

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, text, topk, searchK, wordseg, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // 在这里统计
                ostringstream oss(ios::out);
                if (!result.empty()) {
                    typedef map<string, double> Statistics;
                    Statistics statistics;

                    for (auto& v : result)
                        statistics[v.label] += 1.0;

                    for (auto& v : statistics)
                        v.second /= (double)(result.size());

                    boost::ptr_vector<Statistics::value_type, boost::view_clone_allocator> sorted(statistics.begin(), statistics.end());
                    sorted.sort([](const Statistics::value_type &lhs, const Statistics::value_type &rhs)->bool {
                        return (lhs.second > rhs.second);
                    });

                    oss << id << "\t";
                    auto it = sorted.begin();
                    auto& predicted = it->first;
                    if (predicted == expected) {
                        oss << "1\t";
                        boost::unique_lock<LabelFactorDict> lk(*lfDict);
                        auto ret = lfDict->insert(std::make_pair(expected, LabelFactor()));
                        auto& lf = ret.first->second;
                        ++lf.a;
                    } else {
                        oss << "0\t";
                        boost::unique_lock<LabelFactorDict> lk(*lfDict);
                        auto ret = lfDict->insert(std::make_pair(expected, LabelFactor()));
                        auto& lfExpected = ret.first->second;
                        ret = lfDict->insert(std::make_pair(predicted, LabelFactor()));
                        auto& lfPredicted = ret.first->second;
                        ++lfPredicted.b;
                        ++lfExpected.c;
                    } // if

                    for (; it != sorted.end(); ++it)
                        oss << it->first << ":" << it->second << " ";
                    oss << flush;
                } else {
                    oss << id << "\tnull" << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *mtx );
                *ofs << oss.str() << endl;

            } catch (const Article::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " caught InvalidRequest: "
                   << err.reason; 
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " caught system exception: " << ex.what();
            } // try

        } while (!done);
    }

    std::size_t                     id;
    std::string                     text, expected;
    int                             topk, searchK;
    bool                            wordseg;
    LabelFactorDict                 *lfDict;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
    static constexpr const char* REQTYPE = "label";
};


// 在ApiServer主线程中执行
// service servicename reqtype infile outfile
void ArticleService::handleCommand( std::stringstream &stream )
{
    using namespace std;

#define MY_WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        getWriter()->writeLine(__write_line_stream.str()); \
    } while (0)

    string arg, infile, outfile, statfile, req;
    int topk = 0;
    int searchK = -1;
    bool wordseg = true;

    while (stream >> arg) {
        auto colon = arg.find(':');
        if (string::npos == colon || 0 == colon || arg.size()-1 == colon)
            THROW_RUNTIME_ERROR("Usage: service " << name() << " arg1:value1 ...");
        string key = arg.substr(0, colon);
        string value = arg.substr(colon + 1);
        // DLOG(INFO) << "arg = " << arg << " key = " << key << " value = " << value;
        // DLOG(INFO) << key << ": " << value;
        if ("topk" == key) {
            if (sscanf(value.c_str(), "%d", &topk) != 1 || topk <= 0)
                THROW_RUNTIME_ERROR("Invalid topk value!");
        } else if ("searchk" == key) {
            if (sscanf(value.c_str(), "%d", &searchK) != 1)
                THROW_RUNTIME_ERROR("Invalid searchk value!");
        } else if ("wordseg" == key) {
            int iWordSeg = 1;
            sscanf(value.c_str(), "%d", &iWordSeg);
            wordseg = iWordSeg ? true : false;
        } else if ("req" == key) {
            req.swap(value);
        } else if ("in" == key) {
            infile.swap(value);   
        } else if ("out" == key) {
            outfile.swap(value);
        } else if ("stat" == key) {
            statfile.swap(value);
        } else {
            THROW_RUNTIME_ERROR("Unrecogonized arg " << key);
        } // if
    } // while

    if (infile.empty())
        THROW_RUNTIME_ERROR("Please specify input file via in:filename");
    if (outfile.empty())
        THROW_RUNTIME_ERROR("Please specify output file via out:filename");

    ifstream ifs(infile, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("Cannot open " << infile << " for reading.");
    ofstream ofs(outfile, ios::out);
    if (!ofs)
        THROW_RUNTIME_ERROR("Cannot open " << outfile << " for writting.");

    string                       line;
    size_t                       lineno = 0;
    atomic_size_t                counter;
    boost::condition_variable    cond;
    boost::mutex                 mtx;

    counter = 0;

    // for label_test
    LabelFactorDict              lfDict;
    // for score_test
    RMSE                         rmse;

    // while ( getline(ifs, line) ) {
    for (; getline(ifs, line); ++lineno) {
        boost::trim( line );
        WorkItemBasePtr pWork;
        if ("doc2vec" == req) {
            pWork = boost::make_shared<Doc2VecTask>
                (lineno, line, wordseg, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        } else if ("label" == req) {
            pWork = boost::make_shared<TopicLabelTask>
                (lineno, line, topk, searchK, wordseg, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        } else if ("label_test" == req) {
            istringstream iss(line, ios::in);
            string expected, text;
            iss >> expected;
            getline(iss, text);
            boost::trim(text);
            pWork = boost::make_shared<TopicLabelTestTask>
                (lineno, text, expected, topk, searchK, wordseg, &lfDict, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        } else if ("score" == req) {
            pWork = boost::make_shared<TopicScoreTask>
                (lineno, line, topk, searchK, wordseg, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        } else if ("score_test" == req) {
            istringstream iss(line, ios::in);
            double expected = 0.0;
            string text;
            iss >> expected;
            if (!bad_stream(iss))
                getline(iss, text);
            boost::trim(text);
            pWork = boost::make_shared<TopicScoreTestTask>
                (lineno, text, expected, topk, searchK, wordseg, &rmse, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        } else {
            pWork = boost::make_shared<TopicTask>
                (lineno, line, topk, searchK, wordseg, &m_queIdleClients, 
                 &counter, &cond, &mtx, &ofs, name().c_str());
        } // if req
        getWorkMgr()->addWork( pWork );
    } // while

    boost::unique_lock<boost::mutex> lock(mtx);
    cond.wait( lock, [&]()->bool {return counter >= lineno;} );

    // deal with lfDict
    if (!lfDict.empty()) {
        double macroP = 0.0, macroR = 0.0, macroF1 = 0.0, micro = 0.0;
        uint32_t sumA = 0, sumB = 0;
        for (auto &kv : lfDict) {
            auto &lf = kv.second;
            lf.p = lf.a ? (double)lf.a / (double)(lf.a + lf.b) : 0.0;
            lf.r = lf.a ? (double)lf.a / (double)(lf.a + lf.c) : 0.0;
            lf.f1 = (lf.p && lf.r) ? 2 * lf.p * lf.r / (lf.p + lf.r) : 0.0;
            macroP += lf.p;
            macroR += lf.r;
            macroF1 += lf.f1;
            sumA += lf.a;
            sumB += lf.b;
        } // for
        macroP /= (double)(lfDict.size());
        macroR /= (double)(lfDict.size());
        macroF1 /= (double)(lfDict.size());
        micro = sumA ? (double)sumA / (double)(sumA + sumB) : 0.0;

        // output statistics
        ostringstream oss;
        oss << boost::format("%-20s\t%20s\t%20s\t%20s") % "label" % "P" % "R" % "F1" << endl;
        for (auto &kv : lfDict) {
            auto& label = kv.first;
            auto& lf = kv.second;
            oss << boost::format("%-20s\t%20.7lf\t%20.7lf\t%20.7lf") 
                    % label.c_str() % lf.p % lf.r % lf.f1 << endl;
        } // for kv
        oss << endl;
        oss << boost::format("%20s\t%20s\t%20s\t%20s") % "MacroP" % "MacroR" % "MacroF1" % "Micro" << endl;
        oss << boost::format("%20.7lf\t%20.7lf\t%20.7lf\t%20.7lf")
                    % macroP % macroR % macroF1 % micro << endl;
        oss << endl << flush;

        if (statfile.empty()) {
            MY_WRITE_LINE(oss.str());
        } else {
            ofstream ofs(statfile, ios::out);
            THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open stat file " << statfile << " for writting!");
            ofs << oss.str() << flush;
        } // if statfile
    } // if lfDict

    // deal with rmse
    if (rmse.cnt) {
        double value = rmse.val / (double)(rmse.cnt);
        value = sqrt(value);
        MY_WRITE_LINE("RMSE = " << value);
    } // if rmse

    MY_WRITE_LINE("Job Done!"); // -b 要求必须输出一行文本
#undef MY_WRITE_LINE
}

// 在apiserver的工作线程中执行
void ArticleService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    // DLOG(INFO) << "Service " << name() << " received request: " << pWork->body;

    bool done = false;

    do {
        auto pClient = m_queIdleClients.getIdleClient();
        if (!pClient) {
            LOG(ERROR) << "Service " << name() << " handleRequest fail, "
                    << " no client object available.";
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, NO_SERVER, 
                    name() << ": no algorithm server available.");
            return;
        } // if

        try {
            std::string result;
            pClient->client()->handleRequest( result, pWork->body );
            done = true;
            m_queIdleClients.putBack( pClient );
            send_response(pWork->conn, BigRLab::ServerType::connection::ok, result);

        } catch (const Article::InvalidRequest &err) {
            done = true;
            m_queIdleClients.putBack( pClient );
            LOG(ERROR) << "Service " << name() << " caught InvalidRequest: "
                    << err.reason;
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, INVALID_REQUEST, 
                    name() << " InvalidRequest: " << err.reason);
        } catch (const std::exception &ex) {
            LOG(ERROR) << "Service " << name() << " caught exception: "
                    << ex.what();
            // RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, UNKNOWN_EXCEPTION, 
                    // name() << " unknown exception: " << ex.what());
        } // try
    } while (!done);
}

std::size_t ArticleService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    if (n < 5)
        n = 5;
    else if (n > 50)
        n = 50;

    DLOG(INFO) << "ArticleService::addServer() " << svrInfo.addr << ":" << svrInfo.port
              << " maxConcurrency = " << svrInfo.maxConcurrency
              << ", going to create " << n << " client instances.";
    // SLEEP_SECONDS(1);
    auto pClient = boost::make_shared<ArticleClientArr>(svrInfo, &m_queIdleClients, n);
    // DLOG(INFO) << "pClient->size() = " << pClient->size();
    if (pClient->empty())
        return SERVER_UNREACHABLE;
    
    DLOG(INFO) << "ArticleService::addServer() m_queIdleClients.size() = " << m_queIdleClients.size();

    return Service::addServer(svrInfo, boost::static_pointer_cast<ServerAttr>(pClient));
}

std::string ArticleService::toString() const
{
    using namespace std;

    stringstream stream;
    stream << "Service " << name() << endl;
    stream << "Online servers:" << endl; 
    stream << left << setw(30) << "IP:Port" << setw(20) << "maxConcurrency" 
            << setw(20) << "nClientInst" << endl; 
    for (const auto &v : m_mapServers) {
        stringstream addrStream;
        addrStream << v.first.addr << ":" << v.first.port << flush;
        stream << left << setw(30) << addrStream.str() << setw(20) 
                << v.first.maxConcurrency << setw(20);
        auto sp = v.second;
        ArticleClientArr *p = static_cast<ArticleClientArr*>(sp.get());
        stream << p->size() << endl;
    } // for
    stream.flush();
    return stream.str();
}


