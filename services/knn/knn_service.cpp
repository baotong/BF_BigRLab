#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <cstring>
#include <glog/logging.h>
#include "common.hpp"
#include "knn_service.h"

using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new KnnService(name); }

const char* lib_name()
{ return "knn"; }

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
        // DLOG(INFO) << "Service " << srvName << " querying \"" << iter->first << "\"";

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

            } catch (const AlgCommon::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << iter->first << "\" caught InvalidRequest: " << err.reason;
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Service " << srvName << " query \"" 
                    << iter->first << "\" fail! " << ex.what();
            } // try
        } while (!done);
    }

    QuerySet::iterator             iter;
    int                            k;
    std::atomic_size_t             *counter;
    boost::condition_variable      *condCounter;
    KnnService::IdleClientQueue    *idleClients;
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
        // DLOG(INFO) << "Service " << srvName << " querying \"" << item << "\"";

        // auto on_finish = [this](void*) {
            // ++*counter;
            // condCounter->notify_all();
        // };

        // boost::shared_ptr<void> pOnFinish((void*)0, on_finish);

        ON_FINISH_CLASS(pCleanup, {++*counter; condCounter->notify_all();});

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

                ostringstream output(ios::out);
                if (!result.empty()) {
                    output << item << "\t";
                    for (auto& v : result)
                        output << v.item << ":" << (2.0 - v.weight) / 2.0 << " ";
                    output << flush;
                } // if
                boost::unique_lock<boost::mutex> flk( *fileMtx );
                *ofs << output.str() << endl;

                // write to file
                // if (!result.empty()) {
                    // boost::unique_lock<boost::mutex> flk( *fileMtx );
                    // *ofs << result.size() << " most like items for " << item << endl;
                    // for (const auto &v : result)
                        // *ofs << v.item << "\t\t" << v.weight << endl;
                    // flk.unlock();
                // } // if

            } catch (const AlgCommon::InvalidRequest &err) {
                done = true;
                idleClients->putBack( pClient );
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

#define MY_WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        getWriter()->writeLine(__write_line_stream.str()); \
    } while (0)

    stringstream ostream;
    string cmd;
    stream >> cmd;

    if (cmd.empty()) {
        MY_WRITE_LINE("Service " << name() << ": command cannot be empty!");
        return;
    } // if

    auto do_with_items = [&] {
        int k;
        stream >> k;
        if (bad_stream(stream)) {
            MY_WRITE_LINE("Service " << name() << ": read k value fail!");
            return;
        } // if

        if (k <= 0) {
            MY_WRITE_LINE("Service " << name() << " handleCommand(): Invalid k value");
            return;
        } // if

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

        if (querySet.empty()) {
            MY_WRITE_LINE("Service " << name() << ": item list cannot be empty!");
            return;
        } // if

        boost::unique_lock<boost::mutex> lock(mtx);
        if (!cond.wait_for( lock, boost::chrono::milliseconds(2 * TIMEOUT), 
                    [&]()->bool {return counter >= querySet.size();} )) {
            ostream << "Wait timeout, " << " result may be incomplete." << endl;
        } // if

        // print the result
        for (const auto &v : querySet) {
            ostream << "Query results for item \"" << v.first << "\":" << endl;
            for (const auto &sub : v.second)
                ostream << sub.item << "\t\t" << sub.weight << endl;
        } // for
    };

    auto do_with_file = [&] {
        int k;
        stream >> k;
        if (bad_stream(stream)) {
            MY_WRITE_LINE("Service " << name() << ": read k value fail!");
            return;
        } // if

        if (k <= 0) {
            MY_WRITE_LINE("Service " << name() << " handleCommand(): Invalid k value");
            return;
        } // if

        string inFilename, outFilename;
        stream >> inFilename >> outFilename;
        if (bad_stream(stream)) {
            MY_WRITE_LINE("Service " << name() << ": cannot read file names for input and output!");
            return;
        } // if

        ifstream ifs(inFilename, ios::in);
        if (!ifs) {
            MY_WRITE_LINE("Service " << name() << ": cannot open file " << inFilename << " for reading!");
            return;
        } // if
        ofstream ofs(outFilename, ios::out);
        if (!ofs) {
            MY_WRITE_LINE("Service " << name() << ": cannot open file " << outFilename << " for writting!");
            return;
        } // if

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
            ostream << "Wait timeout, " << " result may be incomplete." << endl;
        } // if
    };

    if ("items" == cmd) {
        do_with_items();
    } else if ("file" == cmd) {
        do_with_file();
    } else {
        getWriter()->writeLine("Invalid command!");
    }// if

    getWriter()->writeLine(ostream.str());

#undef MY_WRITE_LINE
}

// 在apiserver的工作线程中执行
void KnnService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    DLOG(INFO) << "Service " << name() << " received request: " << pWork->body;

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

        } catch (const AlgCommon::InvalidRequest &err) {
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

std::size_t KnnService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    // if (n < 5)
        // n = 5;
    // else if (n > 50)
        // n = 50;

    DLOG(INFO) << "KnnService::addServer() " << svrInfo.addr << ":" << svrInfo.port
              << " maxConcurrency = " << svrInfo.maxConcurrency
              << ", going to create " << n << " client instances.";
    // SLEEP_SECONDS(1);
    auto pClient = boost::make_shared<KnnClientArr>(svrInfo, &m_queIdleClients, n);
    // DLOG(INFO) << "pClient->size() = " << pClient->size();
    if (pClient->empty())
        return SERVER_UNREACHABLE;
    
    DLOG(INFO) << "KnnService::addServer() m_queIdleClients.size() = " << m_queIdleClients.size();

    return Service::addServer(svrInfo, boost::static_pointer_cast<ServerAttr>(pClient));
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

