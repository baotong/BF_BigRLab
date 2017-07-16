#ifndef _SERVICE_MANAGER_H_
#define _SERVICE_MANAGER_H_

#include "service.h"
#include <boost/thread.hpp>

namespace BigRLab {

// 两个map，service 和 servicelib
class ServiceManager {
public:
    typedef boost::shared_ptr<ServiceManager> pointer;

    struct ServiceLib {
        typedef boost::shared_ptr<ServiceLib>   pointer;
        typedef Service* (*NewInstFunc)(const char*);
        typedef const char* (*LibNameFunc)();

        ServiceLib( const std::string &_Path )
                : path(_Path), pHandle(NULL), pNewInstFn(NULL), pLibName(NULL)
        { loadLib(); }

        ~ServiceLib();
        void loadLib();

        Service::pointer newInstance( const std::string &name )
        { 
            Service::pointer ret;
            if (pNewInstFn) {
                ret.reset(pNewInstFn(name.c_str())); 
                ret->setWorkMgr(g_pWorkMgr);
                ret->setWriter(g_pWriter);
            } // if
            return ret;
        }

        std::string    path;
        void           *pHandle;
        NewInstFunc    pNewInstFn;
        LibNameFunc    pLibName;
    };

    struct ServiceLibTable : std::map< std::string, ServiceLib::pointer >
                           , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

    struct ServiceTable : std::map<std::string, Service::pointer>
                        , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

public:
    ~ServiceManager()
    {
        /*
         * NOTE!!! Manually control the destructor order
         * 手动控制析构顺序，service的析构函数在lib.so中实现
         */
        m_mapServices.clear();
        m_mapServiceLibs.clear();
    }

    static pointer getInstance();

    void loadServiceLib( const std::string &path );
    bool rmServiceLib( const std::string &name );

    bool getService( const std::string &srvName, Service::pointer &pSrv );
    int  addAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo );
    void rmAlgServer( const std::string& algName, const AlgSvrInfo& svrInfo );

    ServiceTable& services() { return m_mapServices; }
    ServiceLibTable& serviceLibs() { return m_mapServiceLibs; }

private:
    /**
     * @brief
     *
     * @param srvName
     * @param libName
     *
     * @return first: newly added or existed service, or nullptr
     *         second: status
     */
    std::pair<Service::pointer, int> addService( const std::string &srvName, const std::string &libName );

    /**
     * @brief 
     *
     * @param srvName
     * @param force false: only remove services with 0 servers
     *
     * @return 
     */
    bool removeService( const std::string &srvName, bool force = false );

private:
    ServiceManager() = default;
    ServiceManager( const ServiceManager &rhs ) = delete;
    ServiceManager( ServiceManager &&rhs ) = delete;
    ServiceManager& operator= (const ServiceManager &rhs) = delete;
    ServiceManager& operator= (ServiceManager &&rhs) = delete;

    static pointer m_pInstance;

private:
    ServiceTable       m_mapServices;
    ServiceLibTable    m_mapServiceLibs;
};

// extern ServiceManager::pointer      g_pServiceMgr;


} // namespace BigRLab


#endif

