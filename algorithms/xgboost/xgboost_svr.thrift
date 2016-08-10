/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators xgboost_svr.thrift
 */

namespace * XgBoostSvr

exception InvalidRequest {
    1: string reason
}

service XgBoostService {
    list<double> predictStr( 1:string input, 2:bool leaf ) throws (1:InvalidRequest err),
    list<double> predictVec( 1:list<i64> indices, 2:list<double> values ) throws (1:InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
