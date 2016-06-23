#include "service_manager.h"
#include "alg_mgr.h"
#include <dlfcn.h>

/*
 * #define CLOSE_HANDLE_THROW_ERROR(handle, args) \
 *     do { \
 *         if (handle) { \
 *             dlclose(handle); \
 *             handle = NULL; \
 *         } \
 *         THROW_RUNTIME_ERROR(args); \
 *     } while (0)
 */

// 要求函数指针名和库中的函数sym名一样
#define LOAD_FUNC( handle, name ) \
    do { \
        char *__load_func_error_str = 0; \
        *(void **) (&(name)) = dlsym(handle, #name); \
        if ((__load_func_error_str = dlerror()) != NULL) { \
            dlclose(handle); \
            handle = NULL; \
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

void ServiceManager::ServiceLib::loadLib()
{
    using namespace std;

    dlerror();
    pHandle = dlopen(path.c_str(), RTLD_LAZY);
    if (!pHandle)
        THROW_RUNTIME_ERROR( "addService cannot load service lib "
               << path << " " << dlerror() );
    dlerror();

    NewInstFunc create_instance;
    LOAD_FUNC(pHandle, create_instance);
    pNewInstFn = create_instance;
    LibNameFunc lib_name;
    LOAD_FUNC(pHandle, lib_name);
    pLibName = lib_name;
}

ServiceManager::ServiceLib::~ServiceLib()
{
    if (pHandle) {
        // DLOG(INFO) << "Closing handle for lib " << path;
        dlclose(pHandle);
        pHandle = NULL;
    } // if
}

// run in the shell
void ServiceManager::loadServiceLib( const std::string &path )
{
    auto p = boost::make_shared<ServiceLib>(path);
    boost::unique_lock<ServiceLibTable> lock(m_mapServiceLibs);
    auto ret = m_mapServiceLibs.insert( std::make_pair(p->pLibName(), p) );
    if (!ret.second)
        THROW_RUNTIME_ERROR("ServiceLib with name " << p->pLibName() << " already exists!");
}

bool ServiceManager::rmServiceLib( const std::string &name )
{
    boost::unique_lock<ServiceLibTable> lock(m_mapServiceLibs);
    return m_mapServiceLibs.erase(name);
}

// called when new alg server register
std::pair<Service::pointer, int> ServiceManager::addService( const std::string &srvName, 
                const std::string &libName )
{
    boost::shared_lock<ServiceLibTable> lock(m_mapServiceLibs);

    auto it = m_mapServiceLibs.find(libName);
    if (m_mapServiceLibs.end() == it) {
        LOG(ERROR) << "AddService() Service lib " << libName << " not loaded!";
        return std::make_pair(Service::pointer(), NO_SERVICE);
    } // if

    Service::pointer pSrv;
    try {
        boost::upgrade_lock<ServiceTable> sLock(m_mapServices);
        auto it1 = m_mapServices.find( srvName );
        if (it1 != m_mapServices.end())
            return std::make_pair(it1->second, ALREADY_EXIST);
        pSrv = it->second->newInstance( srvName );
        if (!pSrv)
            return std::make_pair(pSrv, INTERNAL_FAIL);
        // insert into table
        boost::upgrade_to_unique_lock<ServiceTable> uLock(sLock);
        m_mapServices.insert( std::make_pair(srvName, pSrv) );
    } catch (const std::exception &ex) {
        LOG(ERROR) << "AddService() " << srvName << " create new service instance fail!";
        return std::make_pair(Service::pointer(), INTERNAL_FAIL);
    } // try

    return std::make_pair(pSrv, SUCCESS);
}

bool ServiceManager::removeService( const std::string &srvName, bool force )
{
    boost::upgrade_lock<ServiceTable> sLock(m_mapServices);

    if (force) {
        boost::upgrade_to_unique_lock<ServiceTable> uLock(sLock);
        return m_mapServices.erase( srvName );
    } // if

    auto it = m_mapServices.find( srvName );
    if (m_mapServices.end() == it)
        return false;
    
    // no server in this service, remove it
    if (!it->second->nServers()) {
        m_mapServices.erase(it);
        return true;
    } // if

    // service contain servers, don't remove
    return false;
}

int ServiceManager::addAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo )
{
    DLOG(INFO) << "ServiceManager::addAlgServer() algName = " << algName
              << ", server = " << svrInfo;

    auto ret = addService(algName, svrInfo.serviceName);
    if (!ret.first)
        return ret.second;

    if (!ret.first->addServer(svrInfo)) {
        removeService(algName);
        return INTERNAL_FAIL;
    } // if
    
    return SUCCESS;
}

void ServiceManager::rmAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo )
{
    DLOG(INFO) << "ServiceManager::rmAlgServer() algName = " << algName << ", server = " << svrInfo;

    ServicePtr pSrv;
    if (getService(algName, pSrv)) {
        if (!pSrv->rmServer(svrInfo))
            removeService(algName);
    } // if
}

bool ServiceManager::getService( const std::string &srvName, Service::pointer &pSrv )
{
    boost::shared_lock<ServiceTable> lock(m_mapServices);
    auto it = m_mapServices.find( srvName );
    if (it == m_mapServices.end())
        return false;
    pSrv = it->second;
    return true;
}


} // namespace BigRLab

