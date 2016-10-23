/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators topics.thrift
 */

namespace * Topics

exception InvalidRequest {
    1: string reason
}

struct Result {
    1: i64      topicId,
    2: double   possibility
}

service TopicService {
    list<Result> predictTopic( 1:string text, 2:i32 topk, 3:i32 nIter, 4:i32 perplexity, 5:i32 nSkip ) 
                    throws (1:InvalidRequest err),
    string handleRequest( 1:string request ) throws (1:InvalidRequest err)
}
