/*
 *** Usage:
 * Command:
 * service tagging method:concur in:infile out:outfile k1:100 k2:100 topk:10
 * k1: number of keyword
 * k2: k in knn if use knn tagging, or first k concur items if use concur.
 * topk: top k final result (default all), if topk <= 0, get all results
 *
 * http:
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"method":"knn","k1":100,"k2":100,"topk":5,"text":"Put your text here"}' http://localhost:9000/tagger
 * 返回值可能是空的，此时返回json字段 "result":"null", "status":"0"
 */
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

    enum TaggingMethod {
        CONCUR, KNN
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
        // std::set<std::string> tmp{"wordseg", "keyword"};
        // m_setValidReqType.swap(tmp);
    }

    // virtual bool init( int argc, char **argv );
    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
    virtual std::size_t addServer( const BigRLab::AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() );
    virtual std::string toString() const;
    // use Service::rmServer()
    // virtual void rmServer( const BigRLab::AlgSvrInfo& svrInfo );

private:
    IdleClientQueue  m_queIdleClients;
};


#endif
