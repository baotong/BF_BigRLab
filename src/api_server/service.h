#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "api_server.h"


namespace BigRLab {

class Service {
    friend class ServiceManager;
public:
    typedef boost::shared_ptr<Service>    pointer;

    struct AlgSvrInfoCmp {
        bool operator() (const AlgSvrInfo &lhs, const AlgSvrInfo &rhs) const
        {
            int cmp = lhs.addr.compare( rhs.addr );
            if (cmp)
                return cmp < 0;
            return lhs.port < rhs.port;
        }
    };

    struct ServerAttr {
        typedef boost::shared_ptr<ServerAttr> Pointer;
        virtual ~ServerAttr() = default;
    };

    struct ServerTable : std::map< AlgSvrInfo, ServerAttr::Pointer, AlgSvrInfoCmp >
                       , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

public:
    Service( const std::string &_Name ) 
                    : m_strName(_Name) {}

    virtual ~Service() = default;

    virtual bool init( int argc, char **argv ) = 0;
    virtual void handleCommand( std::stringstream &stream ) = 0;
    virtual void handleRequest( const WorkItemPtr &pWork ) = 0;

    const std::string& name() const
    { return m_strName; }
    
    ServerTable& servers()
    { return m_mapServers; }

    void setWorkMgr( const WorkManager::Pointer &_Ptr )
    { m_pWorkMgr = _Ptr; }
    WorkManager::Pointer getWorkMgr() const
    { return m_pWorkMgr; }

    virtual void addServer( const AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() )
    {
        boost::unique_lock<ServerTable> lock(m_mapServers);
        m_mapServers.insert( std::make_pair(svrInfo, p) );
        // overwrite ??
    }

    virtual void rmServer( const AlgSvrInfo& svrInfo )
    {
        boost::unique_lock<ServerTable> lock(m_mapServers);
        m_mapServers.erase( svrInfo );
    }

    virtual std::string toString() const 
    {
        std::stringstream stream;
        stream << "Service " << name() << std::endl;
        stream << "Online servers:\n" << "IP:Port\t\tmaxConcurrency" << std::endl; 
        for (const auto &v : m_mapServers)
            stream << v.first.addr << ":" << v.first.port << "\t\t" << v.first.maxConcurrency << std::endl;
        stream.flush();
        return stream.str();
    }

protected:
    std::string                 m_strName;
    WorkManager::Pointer        m_pWorkMgr;
    ServerTable                 m_mapServers;
};

typedef boost::shared_ptr<Service>    ServicePtr;


} // namespace BigRLab


#endif

