#include "service_manager.h"
#include "alg_mgr.h"
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
// ServiceManager::pointer      g_pServiceMgr;

ServiceManager::pointer ServiceManager::getInstance()
{
    static boost::once_flag     onceFlag;
    boost::call_once(onceFlag, [] {
        if (!m_pInstance)
            m_pInstance.reset( new ServiceManager );
    });

    return m_pInstance;
}

// void ServiceManager::addService( int argc, char **argv )
void ServiceManager::addService( const std::string &cmd )
{
    using namespace std;

    DLOG(INFO) << "addService() cmd = " << cmd;

    stringstream stream(cmd);
    vector<string> strArgs;
    string arg;

    while (stream >> arg)
        strArgs.push_back(arg);

    if (strArgs.size() == 0)
        THROW_RUNTIME_ERROR("Invalid command!");

    vector<char*> cstrArgs( strArgs.size() );
    for (size_t i = 0; i < strArgs.size(); ++i)
        cstrArgs[i] = const_cast<char*>(strArgs[i].c_str());

#define CLOSE_HANDLE_THROW_ERROR(handle, args) \
    do { \
        if (handle) { \
            dlclose(handle); \
            handle = NULL; \
        } \
        THROW_RUNTIME_ERROR(args); \
    } while (0)

    const char *libpath = cstrArgs[0];

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

    if (!pSrv->init((int)(cstrArgs.size()), &cstrArgs[0]))
        CLOSE_HANDLE_THROW_ERROR(srvHandle, "addService init service" << pSrv->name() << " fail!");

    ServiceInfoPtr pSrvInfo(new ServiceInfo(pSrv, srvHandle));
    pSrvInfo->cmdString = cmd;
    // insert
    {
        boost::unique_lock<ServiceTable> lock(m_mapServices);
        auto ret = m_mapServices.insert(std::make_pair(pSrv->name(), std::move(pSrvInfo)));
        if (!ret.second)
            THROW_RUNTIME_ERROR("Service " << pSrv->name() << " already exists!");
    } // insert

    // get servers already registered
    std::vector<AlgSvrInfo> _servers;
    g_pAlgMgrHandler->getAlgSvrList( _servers, pSrv->name() );
    for (const auto &v : _servers)
        pSrv->addServer( v );

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

int ServiceManager::addAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo )
{
    DLOG(INFO) << "ServiceManager::addAlgServer() algName = " << algName
              << ", server = " << svrInfo.addr << ":" << svrInfo.port;

    ServicePtr pSrv;
    if (getService(algName, pSrv))
        return pSrv->addServer( svrInfo );
    // 若某个algsvr启动服务器地址设置错误，而相应的lib.so没有加载，是检测不到无法连接错误
    return SUCCESS;
}

void ServiceManager::rmAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo )
{
    DLOG(INFO) << "ServiceManager::rmAlgServer() algName = " << algName
              << ", server = " << svrInfo.addr << ":" << svrInfo.port;

    ServicePtr pSrv;
    if (getService(algName, pSrv))
        pSrv->rmServer( svrInfo );
}

ServiceManager::ServiceInfo::~ServiceInfo()
{
    if (pHandle) {
        DLOG(INFO) << "Closing handle for service " << this->pService->name();
        dlclose(pHandle);
        pHandle = NULL;
    } // if
}


} // namespace BigRLab

