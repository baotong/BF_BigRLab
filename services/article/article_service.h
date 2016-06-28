#ifndef _ARTICLE_SERVICE_H_
#define _ARTICLE_SERVICE_H_

#include "service.h"
#include "ArticleService.h"
#include "AlgMgrService.h"
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <set>
#include <atomic>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/condition_variable.hpp>

extern "C" {
    extern BigRLab::Service* create_instance(const char *name);
    extern const char* lib_name();
}


class ArticleService : public BigRLab::Service {
public:
    enum StatusCode {
        OK = 0,
        NO_SERVER,
        INVALID_REQUEST
    };

public:
    static const uint32_t       TIMEOUT = 5000;     // 5s
public:
    typedef BigRLab::ThriftClient< Article::ArticleServiceClient > ArticleClient;
    typedef ArticleClient::Pointer                             ArticleClientPtr;
    typedef boost::weak_ptr<ArticleClient>                     ArticleClientWptr;

    struct IdleClientQueue : BigRLab::SharedQueue< ArticleClientWptr > {
        ArticleClientPtr getIdleClient();
        
        void putBack( const ArticleClientPtr &pClient )
        { this->push( pClient ); }
    };

    struct ArticleClientArr : ServerAttr {
        ArticleClientArr(const BigRLab::AlgSvrInfo &svr, 
                     IdleClientQueue *idleQue, int n);

        bool empty() const
        { return clients.empty(); }

        std::size_t size() const
        { return clients.size(); }

        std::vector<ArticleClientPtr> clients;
    };

public:
    ArticleService( const std::string &name ) : BigRLab::Service(name) 
    {
        std::set<std::string> tmp{"wordseg"};
        m_setValidReqType.swap(tmp);
    }

    // virtual bool init( int argc, char **argv );
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
    virtual std::size_t addServer( const BigRLab::AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() );
    virtual std::string toString() const;
    // use Service::rmServer()
    // virtual void rmServer( const BigRLab::AlgSvrInfo& svrInfo );
    
    bool isValidReq( const std::string &reqtype ) const
    { return m_setValidReqType.count(reqtype) > 0; } 

private:
    IdleClientQueue  m_queIdleClients;
    std::set<std::string> m_setValidReqType;
};


#endif
