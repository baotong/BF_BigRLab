#include "service.h"
#include "KnnService.h"
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <boost/thread/lockable_adapter.hpp>

extern "C" {
    extern BigRLab::Service* create_instance();
}


class KnnService : public BigRLab::Service {
    typedef BigRLab::ThriftClient< KNN::KnnServiceClient > KnnClient;

    struct KnnClientTable : std::map< std::string, std::vector<KnnClient::Pointer> >
                          , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

    // TODO suitable for lockfree queue
    struct KnnClientPool : std::deque< boost::weak_ptr<KnnClient> >
                         , boost::basic_lockable_adapter<boost::mutex>
    {};

public:
    KnnService( const std::string &name ) : Service(name) {}

    virtual bool init( int argc, char **argv );
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
private:
    KnnClientTable   m_mapClientTable;
    KnnClientPool    m_arrClientPool;
};



