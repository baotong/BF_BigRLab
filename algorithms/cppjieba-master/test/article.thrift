/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators article.thrift
 */

namespace * Article

exception InvalidRequest {
    1: string reason
}

service ArticleService {
    void setFilter( 1:string filter ),

    list<string> wordSegment( 1:string sentence ),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
