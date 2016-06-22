#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "api_server.h"
#include <iomanip>


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
    Service(const std::string &_Name) : m_strName(_Name) {}
    virtual ~Service() = default;

    // virtual bool init( int argc, char **argv ) = 0;
    virtual void handleCommand( std::stringstream &stream ) = 0;
    virtual void handleRequest( const WorkItemPtr &pWork ) = 0;

    // void setName( const std::string &_Name )
    // { m_strName = _Name; }
    const std::string& name() const
    { return m_strName; }
    
    ServerTable& servers()
    { return m_mapServers; }

    void setWorkMgr( const WorkManager::Pointer &_Ptr )
    { m_pWorkMgr = _Ptr; }
    WorkManager::Pointer getWorkMgr() const
    { return m_pWorkMgr; }

    virtual std::size_t addServer( const AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() )
    {
        DLOG(INFO) << "Service::addServer() " << svrInfo;
        boost::unique_lock<ServerTable> lock(m_mapServers);
        m_mapServers.insert( std::make_pair(svrInfo, p) );
        return m_mapServers.size();
    }

    virtual std::size_t rmServer( const AlgSvrInfo& svrInfo )
    {
        DLOG(INFO) << "Service::rmServer() " << svrInfo;
        boost::unique_lock<ServerTable> lock(m_mapServers);
        m_mapServers.erase( svrInfo );
        return m_mapServers.size();
    }

    virtual std::string toString() const 
    {
        using namespace std;
        stringstream stream;
        stream << "Service " << name() << endl;
        stream.flush();
        return stream.str();
    }

    std::size_t nServers()
    {
        boost::shared_lock<ServerTable> lock(m_mapServers);
        return m_mapServers.size();
    }

protected:
    std::string                 m_strName;
    WorkManager::Pointer        m_pWorkMgr;
    ServerTable                 m_mapServers;
};

typedef boost::shared_ptr<Service>    ServicePtr;


// function declaration
extern void writeLine(const std::string &msg);

#define WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        writeLine( __write_line_stream.str() ); \
    } while (0)

} // namespace BigRLab


#endif

