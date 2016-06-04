/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators wordseg.thrift
 */

namespace * WordSeg

exception InvalidRequest {
    1: string reason
}

struct ResultItem {
    1: string item,
    2: string tag
}

service WordSegService {
    void setFilter( 1:string filter ),
    list<ResultItem> tag( 1:string sentence ),
    list<string> tagOnlyItem( 1:string sentence ),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
