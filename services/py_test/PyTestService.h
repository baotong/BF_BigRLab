/*
 * Test
 * service pytest in:data/in.txt out:data/out.txt
 * curl -i -X POST -H "Content-Type: BigRLab_Request" -d 'HelloWorld' http://localhost:9000/pytest
 */


#ifndef _PY_TEST_SERVICE_H_
#define _PY_TEST_SERVICE_H_

#include "service.h"
#include "PyService.h"          // 在alg server端有thrift生成，$alg_server/gen-cpp
#include "AlgMgrService.h"      // $apiserver/gen-cpp   alg_mgr 由thrift生成的代码
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <set>
#include <atomic>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/condition_variable.hpp>

// 本lib对外接口，必须用extern "C"声明
extern "C" {
    extern BigRLab::Service* create_instance(const char *name);
    extern const char* lib_name();
}


class PyTestService : public BigRLab::Service {
public:

    // handleRequest 返回状态码，照抄
    enum StatusCode {
        OK = 0,
        NO_SERVER,
        INVALID_REQUEST
    };

public:
    // 等待空闲client超时值，当请求作业很多而服务器有限时，后到作业需要等待。因此需要设置一个超时值。
    static const uint32_t       TIMEOUT = (600 * 1000);     // 10min
public:
    // PyTest 是alg server thrift中定义的namespace
    // PyServiceClient 见 $alg_server/gen-cpp/PyService.h
    // ThriftClient 见 $COMMON_DIR/rpc_module.h  是对Thrift client的封装
    // 在做二次开发时，只需将 "PyTest" "PyServiceClient" 等词替换成实际的即可。
    typedef BigRLab::ThriftClient< PyTest::PyServiceClient >   PyTestClient;
    typedef PyTestClient::Pointer                             PyTestClientPtr;
    typedef boost::weak_ptr<PyTestClient>                     PyTestClientWptr;

    // 定义空闲连接队列（池），需要将PyTestClientWptr 替换成实际的。
    struct IdleClientQueue : BigRLab::SharedQueue< PyTestClientWptr > {
        PyTestClientPtr getIdleClient();
        
        void putBack( const PyTestClientPtr &pClient )
        { this->push( pClient ); }
    };

    // 同上，单词替换，其他不改
    struct PyTestClientArr : ServerAttr {
        PyTestClientArr(const BigRLab::AlgSvrInfo &svr, 
                     IdleClientQueue *idleQue, int n);

        bool empty() const
        { return clients.empty(); }

        std::size_t size() const
        { return clients.size(); }

        std::vector<PyTestClientPtr> clients;
    };

    // 剩下部分同上，只需替换名称
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
