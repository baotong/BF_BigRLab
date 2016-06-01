#include "service.h"
#include "KnnService.h"
#include "AlgMgrService.h"
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <atomic>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/condition_variable.hpp>

extern "C" {
    extern BigRLab::Service* create_instance(const char *name);
    extern const char* lib_name();
}


class KnnService : public BigRLab::Service {
public:
    static const uint32_t       TIMEOUT = 5000;     // 5s
public:
    typedef BigRLab::ThriftClient< KNN::KnnServiceClient > KnnClient;
    typedef KnnClient::Pointer                             KnnClientPtr;
    typedef boost::weak_ptr<KnnClient>                     KnnClientWptr;

    struct IdleClientQueue : BigRLab::SharedQueue< KnnClientWptr > {
        KnnClientPtr getIdleClient();
        
        void putBack( const KnnClientPtr &pClient )
        { this->push( pClient ); }
    };

    struct KnnClientArr : ServerAttr {
        KnnClientArr(const BigRLab::AlgSvrInfo &svr, 
                     IdleClientQueue *idleQue, int n);

        bool empty() const
        { return clients.empty(); }

        std::size_t size() const
        { return clients.size(); }

        std::vector<KnnClientPtr> clients;
    };

public:
    KnnService( const std::string &name ) : BigRLab::Service(name) {}

    // virtual bool init( int argc, char **argv );
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
    virtual std::size_t addServer( const BigRLab::AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() );
    virtual std::string toString() const;
    // use Service::rmServer()
    // virtual void rmServer( const BigRLab::AlgSvrInfo& svrInfo );
private:
    IdleClientQueue  m_queIdleClients;
};



