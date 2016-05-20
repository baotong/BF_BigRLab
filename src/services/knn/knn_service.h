#include "service.h"

extern "C" {
    extern BigRLab::Service* create_instance();
}


class KnnService : public BigRLab::Service {
public:
    KnnService( const std::string &name ) : Service(name) {}

    virtual bool init( int argc, char **argv ) { return true; }
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
};



