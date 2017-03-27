/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ../../alg_common/AlgCommon.thrift
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ftrl.thrift
 */

include "../../alg_common/AlgCommon.thrift"

namespace * FTRL

service FtrlService {
    double lrPredict( 1:string id, 2:string data ) throws (1:AlgCommon.InvalidRequest err),
    bool   setValue( 1:string id, 2:double value ) throws (1:AlgCommon.InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:AlgCommon.InvalidRequest err)
}
