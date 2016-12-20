/*
 * thrift --gen py:new_style,utf8strings PyTest.thrift
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators PyTest.thrift
 */

namespace * PyTest

exception InvalidRequest {
    1: string reason
}


struct Result {
    1: i32 id,
    2: string word
}


service PyService {
    list<Result> segment( 1:string text ) throws (1:InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
