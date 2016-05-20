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

protected:
    std::string         m_strName;
    std::vector<AlgSvrInfo>     m_arrAlgSvrInfo;
};

typedef boost::shared_ptr<Service>    ServicePtr;


} // namespace BigRLab


#endif

