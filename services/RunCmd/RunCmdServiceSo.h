/*
 * Test
 */


#ifndef _RUN_CMD_SERVICE_SO_H_
#define _RUN_CMD_SERVICE_SO_H_

#include "service.h"
#include "RunCmdService.h"
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


class RunCmdService : public BigRLab::Service {
public:

    // handleRequest 返回状态码，照抄
    enum StatusCode {
        OK = 0,
        NO_SERVER,
        INVALID_REQUEST
    };

public:
    static const uint32_t       TIMEOUT = (600 * 1000);     // 10min

public:
    typedef BigRLab::ThriftClient< RunCmd::RunCmdServiceClient >   RunCmdClient;
    typedef RunCmdClient::Pointer                             RunCmdClientPtr;
    typedef boost::weak_ptr<RunCmdClient>                     RunCmdClientWptr;

public:
    RunCmdService( const std::string &name ) : BigRLab::Service(name) 
    {
        // std::set<std::string> tmp{"wordseg", "keyword"};
        // m_setValidReqType.swap(tmp);
    }

    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
    virtual std::size_t addServer( const BigRLab::AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() );
    virtual std::string toString() const;

private:
    std::map<std::string, RunCmdClientPtr>      m_mapIpClient;
};


#endif
