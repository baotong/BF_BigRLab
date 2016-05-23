#include "service_manager.h"
#include <dlfcn.h>

// 要求函数指针名和库中的函数sym名一样
#define LOAD_FUNC( handle, name ) do {                  \
                            char *__load_func_error_str = 0;    \
                            *(void **) (&(name)) = dlsym(handle, #name);        \
                            if ((__load_func_error_str = dlerror()) != NULL) {       \
                                dlclose(handle);                \
                                THROW_RUNTIME_ERROR( "addService cannot find symbol " \
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

void ServiceManager::addService( int argc, char **argv )
{
    using namespace std;

    // LOG(INFO) << "ServiceManager::addService() confFileName = " << confFileName;

    const char *libpath = argv[0];

    dlerror();
    void *srvHandle = dlopen(libpath, RTLD_LAZY);
    if (!srvHandle)
        THROW_RUNTIME_ERROR( "addService cannot load service lib "
               << libpath << " " << dlerror() );

    dlerror();

    Service* (*create_instance)();
    LOAD_FUNC(srvHandle, create_instance);

    ServicePtr pSrv(create_instance());

    g_pApiServer->algMgrClient()->client()->getAlgSvrList(pSrv->algServerList(), pSrv->name());
    if (pSrv->algServerList().size() == 0)
        THROW_RUNTIME_ERROR("No alg server found for service " << pSrv->name());

    if (!pSrv->init(argc, argv))
        THROW_RUNTIME_ERROR("addService init service" << pSrv->name() << " fail!");

    ServiceInfoPtr pSrvInfo(new ServiceInfo(pSrv, srvHandle));
    // insert
    {
        boost::unique_lock<ServiceTable> lock(m_mapServices);
        auto ret = m_mapServices.insert(std::make_pair(pSrv->name(), std::move(pSrvInfo)));
        if (!ret.second)
            THROW_RUNTIME_ERROR("Service " << pSrv->name() << " already exists!");
    } // insert

    cout << "Add service " << pSrv->name() << " success." << endl;
    cout << "Algorithm servers for " << pSrv->name() << ":" << endl;
    for (const auto &v : pSrv->algServerList())
        cout << v.addr << ":" << v.port << endl;

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

