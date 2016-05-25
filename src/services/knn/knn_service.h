#include "service.h"
#include "KnnService.h"
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <atomic>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/condition_variable.hpp>

extern "C" {
    extern BigRLab::Service* create_instance();
}


class KnnService : public BigRLab::Service {
public:
    static const uint32_t       TIMEOUT = 5000;     // 5s
public:
    typedef BigRLab::ThriftClient< KNN::KnnServiceClient > KnnClient;
    typedef KnnClient::Pointer                             KnnClientPtr;
    typedef boost::weak_ptr<KnnClient>                     KnnClientWptr;

    // {"ip:port" : [client]}
    struct KnnClientTable : std::map< std::string, std::vector<KnnClientPtr> >
                          , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

    struct IdleClientQueue : BigRLab::SharedQueue< KnnClientWptr > {
        KnnClientPtr getIdleClient()
        {
            KnnClientPtr pRet;

            do {
                KnnClientWptr wptr;
                if (!this->timed_pop(wptr, TIMEOUT))
                    return KnnClientPtr();      // return empty ptr when no client available
                pRet = wptr.lock();
            } while (!pRet);

            return pRet;
        }
        
        void putBack( const KnnClientPtr &pClient )
        { this->push( pClient ); }
    };

    typedef std::map< std::string, std::vector<KNN::Result> >  QuerySet;

    struct QueryWork : BigRLab::WorkItemBase {
        QueryWork( QuerySet::iterator _Iter,
                   int _K,
                   std::atomic_size_t *_Counter,
                   boost::condition_variable *_Cond,
                   IdleClientQueue *_Clients,
                   const char *_SrvName )
            : iter(_Iter), k(_K)
            , counter(_Counter)
            , condCounter(_Cond)
            , idleClients(_Clients)
            , srvName(_SrvName) {}

        virtual void run();

        int k;
        IdleClientQueue   *idleClients;
        std::atomic_size_t  *counter;
        boost::condition_variable *condCounter;
        QuerySet::iterator iter;
        const char *srvName;
    };

public:
    KnnService( const std::string &name ) : Service(name) {}

    virtual bool init( int argc, char **argv );
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
private:
    KnnClientTable   m_mapClientTable;
    IdleClientQueue  m_queIdleClients;
};



