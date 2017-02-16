/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ../alg_common/AlgCommon.thrift
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators xgboost_svr.thrift
 */

include "../alg_common/AlgCommon.thrift"

namespace * XgBoostSvr

service XgBoostService {
    list<double> predict( 1:string input, 2:bool leaf ) throws (1:AlgCommon.InvalidRequest err),
    list<double> predict_GBDT( 1:string input, 2:bool simple ) throws (1:AlgCommon.InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:AlgCommon.InvalidRequest err)
}
