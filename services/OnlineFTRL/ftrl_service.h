/*
 *** Usage:
 * Command 暂不支持update model
 * service ftrl in:data/agaricus.txt.test out:data/out.txt req:predict
 *
 * http 方式见 CppSDK/ftrl
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d '{"data":"1:1 9:1 19:1 21:1 24:1 34:1 36:1 39:1 42:1 53:1 56:1 65:1 69:1 77:1 86:1 88:1 92:1 95:1 102:1 106:1 117:1 122:1","id":0,"req":"predict"}' http://localhost:9000/ftrl
 */
#ifndef _FTRL_SERVICE_H_
#define _FTRL_SERVICE_H_

#include "service.h"
#include "FtrlService.h"
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


class FtrlService : public BigRLab::Service {
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
    static const uint32_t       TIMEOUT = (1800 * 1000); // 30min
public:
    typedef BigRLab::ThriftClient< FTRL::FtrlServiceClient > FtrlClient;
    typedef FtrlClient::Pointer                             FtrlClientPtr;
    typedef boost::weak_ptr<FtrlClient>                     FtrlClientWptr;

    struct IdleClientQueue : BigRLab::SharedQueue< FtrlClientWptr > {
        FtrlClientPtr getIdleClient();
        
        void putBack( const FtrlClientPtr &pClient )
        { this->push( pClient ); }
    };

    struct FtrlClientArr : ServerAttr {
        FtrlClientArr(const BigRLab::AlgSvrInfo &svr, 
                     IdleClientQueue *idleQue, int n);

        bool empty() const
        { return clients.empty(); }

        std::size_t size() const
        { return clients.size(); }

        std::vector<FtrlClientPtr> clients;
    };

public:
    FtrlService( const std::string &name ) : BigRLab::Service(name) 
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
