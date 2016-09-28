/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ftrl.thrift
 */

namespace * FTRL

exception InvalidRequest {
    1: string reason
}

service FtrlService {
    double lrPredict( 1:string input ) throws (1:InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
