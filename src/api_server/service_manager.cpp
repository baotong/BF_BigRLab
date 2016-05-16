#include "service_manager.h"
#include <dlfcn.h>

// 要求函数指针名和库中的函数sym名一样
#define LOAD_FUNC( handle, name ) do {                  \
                            char *__load_func_error_str = 0;    \
                            *(void **) (&(name)) = dlsym(handle, #name);        \
                            if ((__load_func_error_str = dlerror()) != NULL) {       \
                                dlclose(handle);                \
                                throw_runtime_error( stringstream() << "addService cannot find symbol " \
                                        << #name << " in lib file!" );       \
                            }                                       \
                        } while(0) 


namespace BigRLab {

ServiceManager::pointer ServiceManager::m_pInstance;

ServiceManager::pointer ServiceManager::getInstance()
{
    static boost::once_flag     onceFlag;
    boost::call_once(onceFlag, [] {
        if (!m_pInstance)
            m_pInstance.reset( new ServiceManager );
    });

    return m_pInstance;
}

void ServiceManager::addService( const char *confFileName )
{
    using namespace std;

    PropertyTable   srvPpt;

    // LOG(INFO) << "ServiceManager::addService() confFileName = " << confFileName;

    parse_config_file( confFileName, srvPpt );

    const char *srvName = NULL, *libPath = NULL;

    for ( const auto &v : srvPpt ) {
        if (v.first == "name") {
            srvName = v.second.begin()->c_str();
        } else if (v.first == "libpath") {
            libPath = v.second.begin()->c_str();
        } // if
    } // for

    if (!srvName)
        throw InvalidInput( stringstream() << "Cannot find property \"name\" in "
               "conf file " << confFileName );

    if (!libPath)
        throw InvalidInput( stringstream() << "Cannot find property \"libpath\" in "
               "conf file " << confFileName );
    
    // check exist
    {
        boost::shared_lock<ServiceTable> lock(m_mapServices);
        if (m_mapServices.find(srvName) != m_mapServices.end())
            throw_runtime_error(stringstream() << "Service " << srvName << " already exists!");
    } // check exist

    void *srvHandle = dlopen(libPath, RTLD_LAZY);
    if (!srvHandle)
        throw_runtime_error( stringstream() << "addService cannot load service lib "
               << libPath << " " << dlerror() );

    dlerror();

    Service* (*create_instance)();
    LOAD_FUNC(srvHandle, create_instance);

    ServicePtr pSrv(create_instance());
    pSrv->properties() = std::move(srvPpt);

    ServiceInfoPtr pSrvInfo(new ServiceInfo(pSrv, srvHandle));
    // insert
    {
        boost::unique_lock<ServiceTable> lock(m_mapServices);
        auto ret = m_mapServices.insert(std::make_pair(srvName, std::move(pSrvInfo)));
        if (!ret.second)
            throw_runtime_error(stringstream() << "Service " << srvName << " already exists!");
    } // insert

    return;
}

bool ServiceManager::removeService( const std::string &srvName )
{
    // TODO
    boost::unique_lock<ServiceTable> lock(m_mapServices);
    return m_mapServices.erase( srvName );
}

bool ServiceManager::getService( const std::string &srvName, Service::pointer &pSrv )
{
    boost::shared_lock<ServiceTable> lock(m_mapServices);
    auto it = m_mapServices.find( srvName );
    if (it == m_mapServices.end())
        return false;
    pSrv = it->second->pService;
    return true;
}

ServiceManager::ServiceInfo::~ServiceInfo()
{
    if (pHandle) {
        dlclose(pHandle);
        pHandle = NULL;
    } // if
}


} // namespace BigRLab

