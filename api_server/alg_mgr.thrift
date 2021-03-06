/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators alg_mgr.thrift
 * thrift --gen py:new_style,utf8strings alg_mgr.thrift
 */

namespace * BigRLab

enum ErrCodeType {
    SUCCESS,
    ALREADY_EXIST,
    SERVER_UNREACHABLE,
    NO_SERVICE,
    INTERNAL_FAIL
}

/*
 * exception InvalidRequest {
 *     1: string desc,
 *     2: ErrCodeType errCode
 * }
 */

struct AlgSvrInfo {
    1: string addr,
    2: i16    port,
    3: i32    maxConcurrency,
    4: string serviceName
}

service AlgMgrService {
    // For alg servers
    // i16 availablePort(),
    i32 addSvr( 1:string algName, 2:AlgSvrInfo svrInfo ),
    void rmSvr( 1:string algName, 2:AlgSvrInfo svrInfo ),
    oneway void informAlive( 1:string algName, 2:AlgSvrInfo svrInfo ),
}

