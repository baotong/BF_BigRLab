#ifndef _SERVICE_MANAGER_H_
#define _SERVICE_MANAGER_H_

#include "service.h"
#include <boost/thread.hpp>

namespace BigRLab {

class ServiceManager {
public:
    typedef std::shared_ptr<ServiceManager> pointer;

    struct ServiceInfo {
        ServiceInfo( const Service::pointer &_Service, void *_Handle )
            : pService(_Service), pHandle(_Handle) {}

        ~ServiceInfo();

        Service::pointer    pService;
        void*               pHandle;
    };

    typedef std::shared_ptr<ServiceInfo>            ServiceInfoPtr;

    struct ServiceTable : std::map<std::string, ServiceInfoPtr>
                        , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

public:
    static pointer getInstance();

    void addService( const char *confFileName );
    bool removeService( const std::string &srvName );
    bool getService( const std::string &srvName, Service::pointer &pSrv );

    ServiceTable& services() { return m_mapServices; }

private:
    ServiceManager() = default;
    ServiceManager( const ServiceManager &rhs ) = delete;
    ServiceManager( ServiceManager &&rhs ) = delete;
    ServiceManager& operator= (const ServiceManager &rhs) = delete;
    ServiceManager& operator= (ServiceManager &&rhs) = delete;

    static pointer m_pInstance;

private:
    ServiceTable   m_mapServices;
};


} // namespace BigRLab


#endif

