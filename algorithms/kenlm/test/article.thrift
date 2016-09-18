/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators article.thrift
 */

namespace * Article

exception InvalidRequest {
    1: string reason
}

struct Result {
    1: string text,
    2: double score
}

service ArticleService {
    void setFilter( 1:string filter ),

    list<Result> creativeRoutine( 1:string input, 2:i32 k, 3:i32 bSearchK, 4:i32 topk )
            throws (1:InvalidRequest err),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
