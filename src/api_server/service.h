#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <memory>
#include "api_server.h"


namespace BigRLab {

class Service {
    friend class ServiceManager;
public:
    typedef std::shared_ptr<Service>    pointer;

public:
    virtual ~Service() = default;

    virtual void handleCommand( std::stringstream &stream ) = 0;
    virtual void handleRequest(const WorkItemPtr &pWork) = 0;

protected:
    PropertyTable& properties()
    { return m_mapProperties; }

    PropertyTable       m_mapProperties;
};

typedef std::shared_ptr<Service>    ServicePtr;


} // namespace BigRLab


#endif

