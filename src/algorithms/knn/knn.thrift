/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators knn.thrift
 */

namespace * KNN

exception InvalidRequest {
    1: string reason
}

struct Result {
    1: string item,
    2: double weight
}

service KnnService {
    list<Result> queryByItem( 1:string item, 2:i32 n ) throws (1:InvalidRequest err),
    list<Result> queryByVector( 1:list<double> values, 2:i32 n ) throws (1:InvalidRequest err),
    list<string> queryByVectorNoWeight( 1:list<double> values, 2:i32 n ) throws (1:InvalidRequest err),
    list<string> queryByItemNoWeight( 1:string item, 2:i32 n ) throws (1:InvalidRequest err),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}


