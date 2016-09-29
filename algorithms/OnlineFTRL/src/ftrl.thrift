/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ftrl.thrift
 */

namespace * FTRL

exception InvalidRequest {
    1: string reason
}

service FtrlService {
    double lrPredict( 1:string id, 2:string data ) throws (1:InvalidRequest err),
    bool   setValue( 1:string id, 2:double value ) throws (1:InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
