#ifndef _RUN_APP_H_
#define _RUN_APP_H_

#include "service.h"
#include "RunCmdService.h"
#include "AlgMgrService.h"
#include "WorkInfo.h"
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <set>
#include <atomic>
#include <mutex>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/condition_variable.hpp>


extern "C" {
    extern BigRLab::Service* create_instance(const char *name);
    extern const char* lib_name();
}


class RunAppService : public BigRLab::Service {
public:

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
    RunAppService( const std::string &name ) : BigRLab::Service(name) 
    { DLOG(INFO) << "RunAppService construct"; }

    virtual ~RunAppService()
    { DLOG(INFO) << "RunAppService destruct"; }

    virtual void handleRequest(const BigRLab::WorkItemPtr &pWork);
    virtual void handleCommand( std::stringstream &stream );
    virtual std::size_t addServer( const BigRLab::AlgSvrInfo& svrInfo,
                            const ServerAttr::Pointer &p = ServerAttr::Pointer() );
    virtual std::string toString() const;

    void createNewWork(const std::string &name, 
                const Json::Value &conf, const BigRLab::WorkItemPtr &pWork);
    void doQuery(const std::string &workName, const BigRLab::WorkItemPtr &pWork);
    // void killAndRemove( string_ref_type workName );

    static std::string build_resp_json(int statusCode, const std::string &msg);
    static void build_resp_json(int statusCode, const std::string &msg, Json::Value &jv);

private:
    std::map<std::string, RunCmdClientPtr>      m_mapIpClient;
    std::map<std::string, WorkInfo::pointer>    m_mapWorkTable;
    std::mutex          m_Mtx;
};


#endif /* _RUN_APP_H_ */
