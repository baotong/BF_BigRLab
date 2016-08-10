#include "article_service.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <cstring>
#include <boost/ptr_container/ptr_vector.hpp>
#include <glog/logging.h>

using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new ArticleService(name); }

const char* lib_name()
{ return "article"; }

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


struct ArticleTask : BigRLab::WorkItemBase {
    ArticleTask( std::size_t _Id,
                 std::string &_Article, 
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : id(_Id)
        , idleClients(_IdleClients)
        , counter(_Counter)
        , cond(_Cond)
        , mtx(_Mtx)
        , ofs(_Ofs)
        , srvName(_SrvName)
    { article.swap(_Article); }

    virtual void run()
    {
        using namespace std;

        auto on_finish = [this](void*) {
            ++*counter;
            cond->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<string> result;
                pClient->client()->wordSegment( result, article );
                done = true;
                idleClients->putBack( pClient );

                // write to file
                if (!result.empty()) {
                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t";
                    for (auto& v : result)
                        *ofs << v << " ";
                    *ofs << endl << flush;
                } // if

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
    std::string                     article;
    ArticleService::IdleClientQueue *idleClients;
    std::atomic_size_t              *counter;
    boost::condition_variable       *cond;
    boost::mutex                    *mtx;
    std::ofstream                   *ofs;
    const char                      *srvName;
};

struct ArticleTaskKeyword : public ArticleTask {
    ArticleTaskKeyword( std::size_t _Id,
                 std::string &_Article, 
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName,
                 int _TopK )
        : ArticleTask(_Id, _Article, _IdleClients, _Counter, _Cond, _Mtx, _Ofs, _SrvName)
        , topk(_TopK) {}

    virtual void run()
    {
        using namespace std;

        auto on_finish = [this](void*) {
            ++*counter;
            cond->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KeywordResult> result;
                pClient->client()->keyword( result, article, topk );
                done = true;
                idleClients->putBack( pClient );

                // write to file
                if (!result.empty()) {
                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t";
                    for (auto& v : result)
                        *ofs << v.word << ":" << v.weight << " ";
                    *ofs << endl << flush;
                } // if

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

    int topk;
};

struct ArticleTaskVector : public ArticleTask {
    ArticleTaskVector( std::size_t _Id,
                 std::string &_Article, 
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName )
        : ArticleTask(_Id, _Article, _IdleClients, _Counter, _Cond, _Mtx, _Ofs, _SrvName) {}

    virtual void run()
    {
        using namespace std;

        auto on_finish = [this](void*) {
            ++*counter;
            cond->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<double> result;
                pClient->client()->toVector( result, article );
                done = true;
                idleClients->putBack( pClient );

                // write to file
                if (!result.empty()) {
                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t";
                    for (auto& v : result)
                        *ofs << v << " ";
                    *ofs << endl << flush;
                } // if

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
};

struct ArticleTaskKnn : public ArticleTask {
    ArticleTaskKnn( std::size_t _Id,
                 std::string &_Article, 
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName,
                 int _N, int _SearchK )
        : ArticleTask(_Id, _Article, _IdleClients, _Counter, _Cond, _Mtx, _Ofs, _SrvName)
        , n(_N), searchK(_SearchK) {}

    virtual void run()
    {
        using namespace std;

        auto on_finish = [this](void*) {
            ++*counter;
            cond->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, article, n, searchK, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // write to file
                if (!result.empty()) {
                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t";
                    for (auto& v : result)
                        *ofs << v.id << ":" << v.distance << " ";
                    *ofs << endl << flush;
                } // if

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

    int n, searchK;
    static constexpr const char* REQTYPE = "knn";
};


struct ArticleTaskKnnLabel : ArticleTaskKnn {
    ArticleTaskKnnLabel( std::size_t _Id,
                 std::string &_Article, 
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName,
                 int _N, int _K, int _SearchK )
        : ArticleTaskKnn(_Id, _Article, _IdleClients, _Counter, _Cond, _Mtx, _Ofs, _SrvName, _N, _SearchK)
        , k(_K) {}

    virtual void run()
    {
        using namespace std;

        auto on_finish = [this](void*) {
            ++*counter;
            cond->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, article, n, searchK, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // 在这里统计
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

                    auto it = sorted.begin();
                    auto endPos = ( k <= 0 || k >= sorted.size()) ? sorted.end() : sorted.begin() + k;
                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t";
                    for (; it != endPos; ++it)
                        *ofs << it->first << ":" << it->second << " ";
                    *ofs << endl << flush;
                } // if

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
    
    int k;
    static constexpr const char* REQTYPE = "knn_label";
};


struct ArticleTaskKnnScore : ArticleTaskKnn {
    ArticleTaskKnnScore( std::size_t _Id,
                 std::string &_Article, 
                 ArticleService::IdleClientQueue *_IdleClients, 
                 std::atomic_size_t *_Counter,
                 boost::condition_variable *_Cond,
                 boost::mutex *_Mtx,
                 std::ofstream *_Ofs, 
                 const char *_SrvName,
                 int _N, int _SearchK )
        : ArticleTaskKnn(_Id, _Article, _IdleClients, _Counter, _Cond, _Mtx, _Ofs, _SrvName, _N, _SearchK) {}

    virtual void run()
    {
        using namespace std;

        auto on_finish = [this](void*) {
            ++*counter;
            cond->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient)
                THROW_RUNTIME_ERROR("Fail due to no available rpc client object!");

            try {
                vector<Article::KnnResult> result;
                pClient->client()->knn( result, article, n, searchK, REQTYPE );
                done = true;
                idleClients->putBack( pClient );

                // 在这里统计
                if (!result.empty()) {
                    double avgScore = 0.0;
                    for (auto& v : result)
                        avgScore += v.score;
                    avgScore /= (double)(result.size());

                    boost::unique_lock<boost::mutex> flk( *mtx );
                    *ofs << id << "\t" << avgScore << endl << flush;
                } // if

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
    
    static constexpr const char* REQTYPE = "knn_score";
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

    string reqtype, infile, outfile;
    stream >> reqtype >> infile >> outfile;
    if (bad_stream(stream))
        THROW_RUNTIME_ERROR("Usage: service " << name() << " reqtype infile outfile ...");

    // if (!isValidReq(reqtype))
        // THROW_RUNTIME_ERROR("Invalid reqtype " << reqtype);

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

    auto do_wordseg = [&] {
        while ( getline(ifs, line) ) {
            WorkItemBasePtr pWork = boost::make_shared<ArticleTask>
                (lineno, line, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str());
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    auto do_keyword = [&] {
        int topk;
        stream >> topk;
        if (bad_stream(stream))
            THROW_RUNTIME_ERROR("Cannot read topk value");
        if (topk <= 0)
            THROW_RUNTIME_ERROR("Invalid topk value " << topk);
        while ( getline(ifs, line) ) {
            WorkItemBasePtr pWork = boost::make_shared<ArticleTaskKeyword>
                (lineno, line, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str(), topk);
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    auto do_article2vector = [&] {
        while ( getline(ifs, line) ) {
            WorkItemBasePtr pWork = boost::make_shared<ArticleTaskVector>
                (lineno, line, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str());
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    auto do_knn = [&] {
        int n = 0, searchK = -1;
        stream >> n;
        if (bad_stream(stream))
            THROW_RUNTIME_ERROR("Cannot read n value");
        if (n <= 0)
            THROW_RUNTIME_ERROR("Invalid n value " << n);
        stream >> searchK;
        while ( getline(ifs, line) ) {
            WorkItemBasePtr pWork = boost::make_shared<ArticleTaskKnn>
                (lineno, line, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str(), n, searchK);
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    auto do_knn_label = [&] {
        int n = 0, k = 0, searchK = -1;
        stream >> n;
        if (bad_stream(stream))
            THROW_RUNTIME_ERROR("Cannot read n value");
        if (n <= 0)
            THROW_RUNTIME_ERROR("Invalid n value " << n);
        stream >> k >> searchK;
        // DLOG(INFO) << "n = " << n << " k = " << k << " searchK = " << searchK;
        while ( getline(ifs, line) ) {
            WorkItemBasePtr pWork = boost::make_shared<ArticleTaskKnnLabel>
                (lineno, line, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str(), n, k, searchK);
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    auto do_knn_score = [&] {
        int n = 0, searchK = -1;
        stream >> n;
        if (bad_stream(stream))
            THROW_RUNTIME_ERROR("Cannot read n value");
        if (n <= 0)
            THROW_RUNTIME_ERROR("Invalid n value " << n);
        stream >> searchK;
        while ( getline(ifs, line) ) {
            WorkItemBasePtr pWork = boost::make_shared<ArticleTaskKnnScore>
                (lineno, line, &m_queIdleClients, &counter, &cond, &mtx, &ofs, name().c_str(), n, searchK);
            getWorkMgr()->addWork( pWork );
            ++lineno;
        } // while
    };

    typedef std::function<void(void)> ReqFunction;
    std::map<string, ReqFunction>     reqFuncTable;
    reqFuncTable["wordseg"] = do_wordseg;
    reqFuncTable["keyword"] = do_keyword;
    reqFuncTable["article2vector"] = do_article2vector;
    reqFuncTable["knn"] = do_knn;
    reqFuncTable["knn_label"] = do_knn_label;
    reqFuncTable["knn_score"] = do_knn_score;

    auto it = reqFuncTable.find(reqtype);
    if (it == reqFuncTable.end())
        THROW_RUNTIME_ERROR("Invalid reqtype " << reqtype);

    it->second();

    boost::unique_lock<boost::mutex> lock(mtx);
    cond.wait( lock, [&]()->bool {return counter >= lineno;} );

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


