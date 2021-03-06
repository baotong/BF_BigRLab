/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators ../../alg_common/AlgCommon.thrift
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators article.thrift
 */

include "../../alg_common/AlgCommon.thrift"

namespace * Article

struct KeywordResult {
    1: string word,
    2: double weight
}

struct KnnResult {
    1: i64 id,
    2: double distance,
    3: string label,
    4: double score
}

struct TagResult {
    1: string tag,
    2: double weight
}

service ArticleService {
    void setFilter( 1:string filter ),

    list<TagResult> tagging( 1:string text, 2:i32 method, 3:i32 k1, 4:i32 k2, 5:i32 searchK, 6:i32 topk ) throws (1:AlgCommon.InvalidRequest err),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:AlgCommon.InvalidRequest err)
}
