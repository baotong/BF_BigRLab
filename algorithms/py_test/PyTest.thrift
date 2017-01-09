/*
 * thrift --gen py:new_style,utf8strings ../alg_common/AlgCommon.thrift
 * thrift --gen py:new_style,utf8strings PyTest.thrift
 * 要生成c++的代码，支持*.so调用。
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ../alg_common/AlgCommon.thrift
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators PyTest.thrift
 */

include "../alg_common/AlgCommon.thrift"

namespace * PyTest


struct Result {
    1: i32 id,
    2: string word
}


service PyService {
    list<Result> segment( 1:string text ) throws (1:AlgCommon.InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:AlgCommon.InvalidRequest err)
}
