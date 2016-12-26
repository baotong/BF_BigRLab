/*
 * Test
 * service pytest in:data/in.txt out:data/out.txt
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d 'HelloWorld' http://localhost:9000/pytest
 */


#ifndef _PY_TEST_SERVICE_H_
#define _PY_TEST_SERVICE_H_

#include "service.h"
#include "PyService.h"
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


class PyTestService : public BigRLab::Service {
public:
    enum StatusCode {
        OK = 0,
        NO_SERVER,
        INVALID_REQUEST
    };

public:
    static const uint32_t       TIMEOUT = (600 * 1000);     // 10min
public:
    typedef BigRLab::ThriftClient< PyTest::PyServiceClient >   PyTestClient;
    typedef PyTestClient::Pointer                             PyTestClientPtr;
    typedef boost::weak_ptr<PyTestClient>                     PyTestClientWptr;

    struct IdleClientQueue : BigRLab::SharedQueue< PyTestClientWptr > {
        PyTestClientPtr getIdleClient();
        
        void putBack( const PyTestClientPtr &pClient )
        { this->push( pClient ); }
    };

    struct PyTestClientArr : ServerAttr {
        PyTestClientArr(const BigRLab::AlgSvrInfo &svr, 
                     IdleClientQueue *idleQue, int n);

        bool empty() const
        { return clients.empty(); }

        std::size_t size() const
        { return clients.size(); }

        std::vector<PyTestClientPtr> clients;
    };

public:
    PyTestService( const std::string &name ) : BigRLab::Service(name) 
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
