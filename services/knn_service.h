#include "service.h"

extern "C" {
    extern BigRLab::Service* create_instance();
}


class KnnService : public BigRLab::Service {
public:
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
};



