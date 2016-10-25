/*
 *** Usage:
* Command format: service $srvName in:$infile out:$outfile [topk:$topk] [iter:$niter] [perplexity:$perplexity] [skip:nskip]
* service topic_eval in:/tmp/in.txt out:/tmp/out.txt topk:5 iter:1
*
* http example:
* curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"topk":5,"niter":1,"perplexity":10,"nskip":2,"text":"www.sauritchsurfboards.com/ recreation/sports/aquatic_sports watch out jeremy sherwin is here over the past six months you may have noticed this guy in every surf magazine published jeremy is finally getting his run more.. copyright surfboards 2004 all rights reserved june 6 2004 new launches it s new and improved site you can now order custom surfboards online more improvements to come.. top selling models middot rocket fish middot speed egg middot classic middot squash"}' http://localhost:9000/topic_eval
 */
#ifndef _TOPIC_SERVICE_H_
#define _TOPIC_SERVICE_H_

#include "service.h"
#include "TopicService.h"
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


class TopicService : public BigRLab::Service {
public:
    constexpr static const int DEFAULT_ITER = 10;
    constexpr static const int DEFAULT_PERPLEXITY = 10;
    constexpr static const int DEFAULT_SKIP = 2;
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
    constexpr static const uint32_t       TIMEOUT = (30 * 1000); // 30s
public:
    typedef BigRLab::ThriftClient< Topics::TopicServiceClient > TopicClient;
    typedef TopicClient::Pointer                             TopicClientPtr;
    typedef boost::weak_ptr<TopicClient>                     TopicClientWptr;

    struct IdleClientQueue : BigRLab::SharedQueue< TopicClientWptr > {
        TopicClientPtr getIdleClient();
        
        void putBack( const TopicClientPtr &pClient )
        { this->push( pClient ); }
    };

    struct TopicClientArr : ServerAttr {
        TopicClientArr(const BigRLab::AlgSvrInfo &svr, 
                     IdleClientQueue *idleQue, int n);

        bool empty() const
        { return clients.empty(); }

        std::size_t size() const
        { return clients.size(); }

        std::vector<TopicClientPtr> clients;
    };

public:
    TopicService( const std::string &name ) : BigRLab::Service(name) 
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
