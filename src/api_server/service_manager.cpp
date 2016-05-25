#include "service_manager.h"
#include <dlfcn.h>

// 要求函数指针名和库中的函数sym名一样
#define LOAD_FUNC( handle, name ) \
    do { \
        char *__load_func_error_str = 0; \
        *(void **) (&(name)) = dlsym(handle, #name); \
        if ((__load_func_error_str = dlerror()) != NULL) { \
            dlclose(handle); \
            THROW_RUNTIME_ERROR( "addService cannot find symbol " \
                    << #name << " in lib file!" ); \
        } \
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

#define CLOSE_HANDLE_THROW_ERROR(handle, args) \
    do { \
        if (handle) { \
            dlclose(handle); \
            handle = NULL; \
        } \
        THROW_RUNTIME_ERROR(args); \
    } while (0)

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
    pSrv->setWorkMgr( g_pWorkMgr );

    // TODO 若在加载lib之前有alg server启动，那么加载lib，创建service实例时候应该读取这些已加入的server
    // 像之前一样
    // g_pApiServer->algMgrClient()->client()->getAlgSvrList(pSrv->algServerList(), pSrv->name());
    // if (pSrv->algServerList().size() == 0)
        // CLOSE_HANDLE_THROW_ERROR(srvHandle, "No alg server found for service " << pSrv->name());

    if (!pSrv->init(argc, argv))
        CLOSE_HANDLE_THROW_ERROR(srvHandle, "addService init service" << pSrv->name() << " fail!");

    ServiceInfoPtr pSrvInfo(new ServiceInfo(pSrv, srvHandle));
    // insert
    {
        boost::unique_lock<ServiceTable> lock(m_mapServices);
        auto ret = m_mapServices.insert(std::make_pair(pSrv->name(), std::move(pSrvInfo)));
        if (!ret.second)
            THROW_RUNTIME_ERROR("Service " << pSrv->name() << " already exists!");
    } // insert

    cout << "Add service success." << endl;
    cout << pSrv->toString() << endl;

    return;
}

bool ServiceManager::removeService( const std::string &srvName )
{
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

void ServiceManager::addAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo )
{
    ServicePtr pSrv;
    if (getService(algName, pSrv))
        pSrv->addServer( svrInfo );
}

ServiceManager::ServiceInfo::~ServiceInfo()
{
    if (pHandle) {
        LOG(INFO) << "Closing handle for service " << this->pService->name();
        dlclose(pHandle);
        pHandle = NULL;
    } // if
}


} // namespace BigRLab

