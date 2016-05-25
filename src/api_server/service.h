#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <set>
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

    struct ServerSet : std::set< AlgSvrInfo, AlgSvrInfoCmp >
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
    
    ServerSet& servers()
    { return m_setAlgSvrInfo; }

    void setWorkMgr( const WorkManager::Pointer &_Ptr )
    { m_pWorkMgr = _Ptr; }
    WorkManager::Pointer getWorkMgr() const
    { return m_pWorkMgr; }

    virtual void addServer( const AlgSvrInfo& svrInfo )
    {
        boost::unique_lock<ServerSet> lock(m_setAlgSvrInfo);
        // overwrite
        auto ret = m_setAlgSvrInfo.insert( svrInfo );
        if (!ret.second) {
            auto it = m_setAlgSvrInfo.erase(ret.first);
            m_setAlgSvrInfo.insert( it, svrInfo );
        } // if
    }

    virtual std::string toString() const 
    {
        std::stringstream stream;
        stream << "Service " << name() << std::endl;
        stream << "Online servers:\n" << "IP:Port\t\tnWorkes:" << std::endl; 
        for (const auto &v : m_setAlgSvrInfo)
            stream << v.addr << ":" << v.port << "\t\t" << v.maxConcurrency << std::endl;
        stream.flush();
        return stream.str();
    }

protected:
    std::string                 m_strName;
    WorkManager::Pointer        m_pWorkMgr;
    ServerSet                   m_setAlgSvrInfo;
};

typedef boost::shared_ptr<Service>    ServicePtr;


} // namespace BigRLab


#endif

