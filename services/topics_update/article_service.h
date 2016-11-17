/*
 * Command: [wordseg 默认1]
 * service topic_pred in:data/test.data out:data/out.txt topk:5 wordseg:0
 * service topic_pred in:data/test.data out:data/out.txt topk:5
 * 只预测label
 * service topic_pred in:/tmp/in.txt out:data/out.txt topk:5 req:label
 * 预测label带标准答案评估
 * service topic_pred in:/tmp/in.txt out:data/out.txt topk:5 req:label_test
 * 将评测统计输出到文件 stat参数指定
 * service topic_pred in:/tmp/in.txt out:data/out.txt topk:5 req:label_test stat:/tmp/stat.txt
 *
 * 只预测score
 * service topic_pred in:/tmp/in.txt out:data/out.txt topk:5 req:score
 * 预测score带标准答案评估
 * service topic_pred in:/tmp/in.txt out:data/out.txt topk:5 req:score_test
 *
 * Http [wordseg 不设置默认1]
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"topk":5,"text":"www.sauritchsurfboards.com/ recreation/sports/aquatic_sports watch out jeremy sherwin is here over the past six months you may have noticed this guy in every surf magazine published jeremy is finally getting his run more.. copyright surfboards 2004 all rights reserved june 6 2004 new launches it s new and improved site you can now order custom surfboards online more improvements to come.. top selling models middot rocket fish middot speed egg middot classic middot squash"}' http://localhost:9000/topic_pred
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"topk":5,"wordseg":0,"text":"watch out jeremy sherwin is here over the past six months you may have noticed this guy in every surf magazine published jeremy is finally getting his run more.. copyright surfboards 2004 all rights reserved june 6 2004 new launches it s new and improved site you can now order custom surfboards online more improvements to come.. top selling models middot rocket fish middot speed egg middot classic middot squash"}' http://localhost:9000/topic_pred
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

public:
    static const uint32_t       TIMEOUT = (600 * 1000);     // 10min
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
