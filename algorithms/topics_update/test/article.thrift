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

service ArticleService {
    void setFilter( 1:string filter ),

    list<string> wordSegment( 1:string sentence ) throws (1:AlgCommon.InvalidRequest err),
    list<KeywordResult> keyword( 1:string sentence, 2:i32 k ) throws (1:AlgCommon.InvalidRequest err),

    list<double> toVector( 1:string sentence, 2:bool wordseg ) throws (1:AlgCommon.InvalidRequest err),

    list<KnnResult> knn( 1:string sentence, 2:i32 n, 3:i32 searchK, 4:bool wordseg, 5:string reqtype ) throws (1:AlgCommon.InvalidRequest err),

    // for http request, input: json string; return: json string
    string handleRequest( 1:string request ) throws (1:AlgCommon.InvalidRequest err)
}
