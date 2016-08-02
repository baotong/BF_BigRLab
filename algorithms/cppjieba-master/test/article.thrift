/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators article.thrift
 */

namespace * Article

exception InvalidRequest {
    1: string reason
}

struct KeywordResult {
    1: string word,
    2: double weight
}

struct KnnResult {
    1: i64 id,
    2: double distance
}

struct KnnLabelResult {
    1: i64 id,
    2: string label
}

service ArticleService {
    void setFilter( 1:string filter ),

    list<string> wordSegment( 1:string sentence ) throws (1:InvalidRequest err),
    list<KeywordResult> keyword( 1:string sentence, 2:i32 k ) throws (1:InvalidRequest err),

    list<double> toVector( 1:string sentence ) throws (1:InvalidRequest err),

    list<KnnResult> knn( 1:string sentence, 2:i32 n, 3:i32 searchK ) throws (1:InvalidRequest err),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
