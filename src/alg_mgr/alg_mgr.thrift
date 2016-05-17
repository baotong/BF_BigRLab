/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators alg_mgr.thrift
 */

namespace * BigRLab

enum ErrCodeType {
    SUCCESS,
    ALREADY_EXIST,
    NO_SERVER
}

exception InvalidRequest {
    1: string desc,
    2: ErrCodeType errno
}

struct AlgSvrInfo {
    1: string addr,
    2: i16    port
}

service AlgMgrService {
    // For alg servers
    i16 availablePort(),
    i32 addSvr( 1:string algName, 2:AlgSvrInfo svrInfo ),
    void rmSvr( 1:string algName, 2:AlgSvrInfo svrInfo ),
    oneway void informAlive( 1:string algName, 2:AlgSvrInfo svrInfo ),

    // For api server
    list<AlgSvrInfo> getAlgSvrList( 1:string name ) throws (1:InvalidRequest err)
}

