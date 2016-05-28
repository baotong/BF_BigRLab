#include "knn_service.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <glog/logging.h>

using namespace BigRLab;
using namespace std;

Service* create_instance()
{ return new KnnService("knn_star"); }


typedef std::map< std::string, std::vector<KNN::Result> >  QuerySet;

struct QueryWork : BigRLab::WorkItemBase {
    QueryWork( QuerySet::iterator _Iter,
            int _K,
            std::atomic_size_t *_Counter,
            boost::condition_variable *_Cond,
            KnnService::IdleClientQueue *_Clients,
            const char *_SrvName )
      : iter(_Iter), k(_K)
      , counter(_Counter)
      , condCounter(_Cond)
      , idleClients(_Clients)
      , srvName(_SrvName) {}

    virtual void run()
    {
        DLOG(INFO) << "Service " << srvName << " querying \"" << iter->first << "\"";

        auto on_finish = [this](void*) {
            ++*counter;
            condCounter->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << iter->first << "\" fail! no client obj available.";
                return;
            } // if

            try {
                pClient->client()->queryByItem( iter->second, iter->first, k );
                done = true;
                idleClients->putBack( pClient );

            } catch (const KNN::InvalidRequest &err) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << iter->first << "\" caught InvalidRequest: " << err.reason;
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << iter->first << "\" fail! " << ex.what();
            } // try
        } while (!done);
    }

    int                            k;
    KnnService::IdleClientQueue    *idleClients;
    std::atomic_size_t             *counter;
    boost::condition_variable      *condCounter;
    QuerySet::iterator             iter;
    const char                     *srvName;
};

struct QueryWorkFile : BigRLab::WorkItemBase {
    QueryWorkFile( int _K, 
                   const std::string &_Item,
                   KnnService::IdleClientQueue *_Clients,
                   std::atomic_size_t *_Counter,
                   boost::condition_variable *_Cond,
                   boost::mutex *_FileMtx,
                   std::ofstream *_OutFile,
                   const char *_SrvName )
          : k(_K)
          , item(_Item)
          , idleClients(_Clients)
          , counter(_Counter)
          , condCounter(_Cond)
          , fileMtx(_FileMtx)
          , ofs(_OutFile)
          , srvName(_SrvName) {}

    virtual void run()
    {
        DLOG(INFO) << "Service " << srvName << " querying \"" << item << "\"";

        auto on_finish = [this](void*) {
            ++*counter;
            condCounter->notify_all();
        };

        boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        bool done = false;

        do {
            auto pClient = idleClients->getIdleClient();
            if (!pClient) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << item << "\" fail! no client obj available.";
                return;
            } // if

            try {
                std::vector<KNN::Result> result;
                pClient->client()->queryByItem( result, item, k );
                done = true;
                idleClients->putBack( pClient );

                // write to file
                if (!result.empty()) {
                    boost::unique_lock<boost::mutex> flk( *fileMtx );
                    *ofs << result.size() << " most like items for " << item << endl;
                    for (const auto &v : result)
                        *ofs << v.item << "\t\t" << v.weight << endl;
                    flk.unlock();
                } // if

            } catch (const KNN::InvalidRequest &err) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << item << "\" caught InvalidRequest: " << err.reason;
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << item << "\" fail! " << ex.what();
            } // try
        } while (!done);
    }


    int                            k;
    std::string                    item;
    KnnService::IdleClientQueue    *idleClients;
    std::atomic_size_t             *counter;
    boost::condition_variable      *condCounter;
    boost::mutex                   *fileMtx;
    std::ofstream                  *ofs;
    const char                     *srvName;
};


KnnService::KnnClientPtr KnnService::IdleClientQueue::getIdleClient()
{
    // DLOG(INFO) << "IdleClientQueue::getIdleClient() size = " << this->size();

    KnnClientPtr pRet;

    do {
        KnnClientWptr wptr;
        if (!this->timed_pop(wptr, TIMEOUT))
            return KnnClientPtr();      // return empty ptr when no client available
        pRet = wptr.lock();
        // DLOG_IF(INFO, !pRet) << "IdleClientQueue::getIdleClient() got empty ptr";
    } while (!pRet);

    return pRet;
}

KnnService::KnnClientArr::KnnClientArr(const BigRLab::AlgSvrInfo &svr, 
                                    IdleClientQueue *idleQue, int n) 
{
    // DLOG(INFO) << "KnnClientArr constructor " << svr.addr << ":" << svr.port
            // << " n = " << n;
    
    clients.reserve(n);
    for (int i = 0; i < n; ++i) {
        auto pClient = boost::make_shared<KnnClient>(svr.addr, (uint16_t)(svr.port));
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

/**
 * @brief  在apiserver的主线程中执行
 * service knn items k item1 item2 ... itemn
 * service knn file k input output
 */
void KnnService::handleCommand( std::stringstream &stream )
{
    using namespace std;

    string cmd;
    stream >> cmd;

    if (cmd.empty())
        ERR_RET("Service " << name() << ": command cannot be empty!");

    auto do_with_items = [&] {
        int k;
        stream >> k;
        if (bad_stream(stream))
            ERR_RET("Service " << name() << ": read k value fail!");

        COND_RET( k <= 0, "Service " << name() << " handleCommand(): Invalid k value" );

        // LOG(INFO) << "k = " << k;

        QuerySet                     querySet;
        string                       item;
        atomic_size_t                counter;
        boost::mutex                 mtx;
        boost::condition_variable    cond;

        counter = 0;
        while (stream >> item) {
            auto ret = querySet.insert( std::make_pair(item, QuerySet::mapped_type()) );
            WorkItemBasePtr pQueryWork = boost::make_shared<QueryWork>( ret.first, k, &counter, 
                    &cond, &m_queIdleClients, this->name().c_str() );
            // LOG_IF(FATAL, !getWorkMgr()) << "getWorkMgr() returned nullptr!";
            getWorkMgr()->addWork( pQueryWork );
        } // while

        if (querySet.empty())
            ERR_RET("Service " << name() << ": item list cannot be empty!");

        boost::unique_lock<boost::mutex> lock(mtx);
        if (!cond.wait_for( lock, boost::chrono::milliseconds(2 * TIMEOUT), 
                    [&]()->bool {return counter >= querySet.size();} )) {
            cout << "Wait timeout, " << " result may be incomplete." << endl;
        } // if

        // print the result
        for (const auto &v : querySet) {
            cout << "Query results for item \"" << v.first << "\":" << endl;
            for (const auto &sub : v.second)
                cout << sub.item << "\t\t" << sub.weight << endl;
        } // for
        cout << endl;
    };

    auto do_with_file = [&] {
        int k;
        stream >> k;
        if (bad_stream(stream))
            ERR_RET("Service " << name() << ": read k value fail!");

        COND_RET( k <= 0, "Service " << name() << " handleCommand(): Invalid k value" );

        string inFilename, outFilename;
        stream >> inFilename >> outFilename;
        if (bad_stream(stream))
            ERR_RET("Service " << name() << ": cannot read file names for input and output!");

        ifstream ifs(inFilename, ios::in);
        if (!ifs)
            ERR_RET("Service " << name() << ": cannot open file " << inFilename << " for reading!");
        ofstream ofs(outFilename, ios::out);
        if (!ofs)
            ERR_RET("Service " << name() << ": cannot open file " << outFilename << " for writting!");

        string                       item;
        atomic_size_t                counter;
        boost::mutex                 condMtx;
        boost::mutex                 fileMtx;
        boost::condition_variable    cond;

        counter = 0;
        size_t itemCount = 0;
        while (ifs >> item) {
            ++itemCount;
            WorkItemBasePtr pQueryWork = boost::make_shared<QueryWorkFile>
                (k, item, &m_queIdleClients, &counter, &cond, &fileMtx, &ofs, this->name().c_str());
            getWorkMgr()->addWork( pQueryWork );
        } // while

        boost::unique_lock<boost::mutex> lock(condMtx);
        if (!cond.wait_for( lock, boost::chrono::seconds(300), 
                    [&]()->bool {return counter >= itemCount;} )) {
            cout << "Wait timeout, " << " result may be incomplete." << endl;
        } // if
    };

    if ("items" == cmd) {
        do_with_items();
    } else if ("file" == cmd) {
        do_with_file();
    } else {
        cerr << "Invalid command!" << endl;
    }// if
}

// 在apiserver的工作线程中执行
void KnnService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    DLOG(INFO) << "Service " << name() << " received request: " << pWork->body;
    // throw InvalidInput("Service knn test exception.");
    send_response(pWork->conn, ServerType::connection::ok, "Service knn running...\n");
}

void KnnService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    int n = svrInfo.maxConcurrency / 5;
    if (n < 5)
        n = 5;
    else if (n > 20)
        n = 20;

    DLOG(INFO) << "KnnService::addServer() " << svrInfo.addr << ":" << svrInfo.port
              << " maxConcurrency = " << svrInfo.maxConcurrency
              << ", going to create " << n << " client instances.";
    // SLEEP_SECONDS(1);
    auto pClient = boost::make_shared<KnnClientArr>(svrInfo, &m_queIdleClients, n);
    // DLOG(INFO) << "pClient->size() = " << pClient->size();
    if (!pClient->empty())
        Service::addServer(svrInfo, boost::static_pointer_cast<ServerAttr>(pClient));

    DLOG(INFO) << "KnnService::addServer() m_queIdleClients.size() = " << m_queIdleClients.size();
}

std::string KnnService::toString() const
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
        KnnClientArr *p = static_cast<KnnClientArr*>(sp.get());
        stream << p->size() << endl;
    } // for
    stream.flush();
    return stream.str();
}

/*
 * bool KnnService::init( int argc, char **argv )
 * {
 *     int nInstances = 0;
 * 
 *     for (const auto &v : this->algServerList()) {
 *         stringstream stream;
 *         stream << v.addr << ":" << v.port << flush;
 *         string addr = std::move(stream.str());
 *         if (v.nWorkThread < 5) {
 *             nInstances = v.nWorkThread;
 *         } else {
 *             nInstances = v.nWorkThread / 10;
 *             if (nInstances < 5)
 *                 nInstances = 5;
 *             else if (nInstances > 10)
 *                 nInstances = 10;
 *         } // if
 *         for (int i = 0; i < nInstances; ++i) {
 *             auto pClient = boost::make_shared<KnnClient>(v.addr, (uint16_t)(v.port));
 *             try {
 *                 pClient->start();
 *             } catch (const std::exception &ex) {
 *                 LOG(ERROR) << "KnnService::init() fail to connect with alg server "
 *                         << addr << ", " << ex.what();
 *                 continue;
 *             } // try
 *             m_mapClientTable[addr].push_back(pClient);
 *             m_queIdleClients.push_back(pClient);
 *         } // for
 *     } // for
 * 
 *     // shuffle
 *     std::random_device rd;
 *     std::mt19937 g(rd());
 *     std::shuffle(m_queIdleClients.begin(), m_queIdleClients.end(), g);
 * 
 *     LOG(INFO) << "Totally " << m_queIdleClients.size() << " client instances for service " << this->name();
 * 
 *     return true;
 * }
 */
