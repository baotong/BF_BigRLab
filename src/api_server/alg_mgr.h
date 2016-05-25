#ifndef _ALG_MGR_H_
#define _ALG_MGR_H_

#include "rpc_module.h"
#include "AlgMgrService.h"
#include <map>
#include <string>
#include <ctime>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>

#define ALG_MGR_SERV_PORT       9001

namespace BigRLab {

class AlgMgrServiceHandler : virtual public AlgMgrServiceIf {
public:
    struct AlgSvrRecord {
        AlgSvrRecord( const AlgSvrInfo &_SvrInfo )
                    : svrInfo(_SvrInfo)
                    , lastAlive(time(0))
        {}

        const std::string& addr() const
        { return svrInfo.addr; }

        uint16_t port() const
        { return (uint16_t)(svrInfo.port); }

        AlgSvrInfo  svrInfo;
        time_t      lastAlive;
    };

    typedef boost::shared_ptr<AlgSvrRecord>     AlgSvrRecordPtr;

    struct AlgSvrSubTable : std::multimap<std::string, AlgSvrRecordPtr>
                          , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

    // { algname : {addr : Record} }
    struct AlgSvrTable : std::map<std::string, AlgSvrSubTable>
                       , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

public:
    virtual int32_t addSvr(const std::string& algName, const AlgSvrInfo& svrInfo);
    virtual void rmSvr(const std::string& algName, const AlgSvrInfo& svrInfo);
    virtual void informAlive(const std::string& algName, const AlgSvrInfo& svrInfo); // TODO timer

    void getAlgSvrList(std::vector<AlgSvrInfo> & _return, const std::string& name);

private:
    AlgSvrTable     m_mapSvrTable;
};


typedef ThriftServer< AlgMgrServiceIf, AlgMgrServiceProcessor > AlgMgrServer;
extern AlgMgrServer::Pointer   g_pAlgMgrServer;
extern boost::shared_ptr<AlgMgrServiceHandler> g_pAlgMgrHandler;

extern void start_alg_mgr();
extern void stop_alg_mgr();


} // namespace BigRLab


#endif

