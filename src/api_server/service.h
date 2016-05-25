#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <memory>
#include "api_server.h"


namespace BigRLab {

class Service {
    friend class ServiceManager;
public:
    typedef boost::shared_ptr<Service>    pointer;

public:
    Service( const std::string &_Name ) 
                    : m_strName(_Name) {}

    virtual ~Service() = default;

    virtual void handleCommand( std::stringstream &stream ) = 0;
    virtual void handleRequest(const WorkItemPtr &pWork) = 0;

    const std::string& name() const
    { return m_strName; }

    virtual bool init( int argc, char **argv ) = 0;
    
    std::vector<AlgSvrInfo>& algServerList()
    { return m_arrAlgSvrInfo; }

    void setWorkMgr( const WorkManager::Pointer &_Ptr )
    { m_pWorkMgr = _Ptr; }
    WorkManager::Pointer getWorkMgr() const
    { return m_pWorkMgr; }

    virtual std::string toString() const 
    {
        std::stringstream stream;
        stream << "Service " << name() << std::endl;
        stream << "Online servers:\n" << "IP:Port\t\tnWorkes:" << std::endl; 
        for (const auto &v : m_arrAlgSvrInfo)
            stream << v.addr << ":" << v.port << "\t\t" << v.nWorkThread << std::endl;
        stream.flush();
        return stream.str();
    }

protected:
    std::string                 m_strName;
    std::vector<AlgSvrInfo>     m_arrAlgSvrInfo;
    WorkManager::Pointer        m_pWorkMgr;
};

typedef boost::shared_ptr<Service>    ServicePtr;


} // namespace BigRLab


#endif

