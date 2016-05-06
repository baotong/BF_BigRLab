#include "service_factory.h"
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

ServiceFactory::pointer ServiceFactory::m_pInstance;

ServiceFactory::pointer ServiceFactory::getInstance()
{
    static boost::once_flag     onceFlag;
    boost::call_once(onceFlag, [] {
        if (!m_pInstance)
            m_pInstance.reset( new ServiceFactory );
    });

    return m_pInstance;
}

void ServiceFactory::addService( const char *confFileName )
{
    using namespace std;

    PropertyTable   srvPpt;

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

    if (m_mapServices.find(srvName) != m_mapServices.end())
        throw_runtime_error(stringstream() << "Service " << srvName << " already exists!");

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
    m_mapServices[srvName] = std::move(pSrvInfo);
}

bool ServiceFactory::removeService( const std::string &srvName )
{
    // TODO
    return m_mapServices.erase( srvName );
}

ServiceFactory::ServiceInfo::~ServiceInfo()
{
    if (pHandle) {
        dlclose(pHandle);
        pHandle = NULL;
    } // if
}


} // namespace BigRLab

