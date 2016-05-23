#include "service.h"
#include "KnnService.h"
#include <map>
#include <vector>
#include <deque>
#include <string>
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
    typedef boost::shared_ptr<KnnClient>                   KnnClientPtr;
    typedef boost::weak_ptr<KnnClient>                     KnnClientWptr;

    struct KnnClientTable : std::map< std::string, std::vector<KnnClient::Pointer> >
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

public:
    KnnService( const std::string &name ) : Service(name) {}

    virtual bool init( int argc, char **argv );
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
private:
    KnnClientTable   m_mapClientTable;
    IdleClientQueue  m_queIdleClients;
};



