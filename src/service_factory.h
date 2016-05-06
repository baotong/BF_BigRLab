#ifndef _SERVICE_FACTORY_H_
#define _SERVICE_FACTORY_H_

#include "service.h"
#include <boost/thread.hpp>

namespace BigRLab {

class ServiceFactory {
public:
    typedef std::shared_ptr<ServiceFactory> pointer;

    struct ServiceInfo {
        ServiceInfo( const Service::pointer &_Service, void *_Handle )
            : pService(_Service), pHandle(_Handle) {}

        ~ServiceInfo();

        Service::pointer    pService;
        void*               pHandle;
    };

    typedef std::shared_ptr<ServiceInfo>            ServiceInfoPtr;
    typedef std::map<std::string, ServiceInfoPtr>   ServiceTable;

public:
    static pointer getInstance();

    void addService( const char *confFileName );
    bool removeService( const std::string &srvName );

    ServiceTable& services() { return m_mapServices; }

private:
    ServiceFactory() = default;
    ServiceFactory( const ServiceFactory &rhs ) = delete;
    ServiceFactory( ServiceFactory &&rhs ) = delete;
    ServiceFactory& operator= (const ServiceFactory &rhs) = delete;
    ServiceFactory& operator= (ServiceFactory &&rhs) = delete;

    static pointer m_pInstance;

private:
    ServiceTable   m_mapServices;
};


} // namespace BigRLab


#endif

