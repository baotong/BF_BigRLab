#include "ArticleServiceHandler.h"
#include "Jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>

#define TIMEOUT     5000

#define THROW_INVALID_REQUEST(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        InvalidRequest __invalid_request_err; \
        __invalid_request_err.reason = std::move(__err_stream.str()); \
        throw __invalid_request_err; \
    } while (0)

namespace Article {

using namespace std;

void ArticleServiceHandler::setFilter( const std::string &strFilter )
{
    THROW_INVALID_REQUEST("not implemented");
}

void ArticleServiceHandler::wordSegment(std::vector<std::string> & _return, const std::string& sentence)
{
    if (sentence.empty())
        THROW_INVALID_REQUEST("Input sentence cannot be empty!");

    Jieba::pointer pJieba;
    if (!g_JiebaPool.timed_pop(pJieba, TIMEOUT))
        THROW_INVALID_REQUEST("No available Jieba object!");

    try {
        pJieba->wordSegment(sentence, _return);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Jieba wordSegment error: " << ex.what();
    } // try

    g_JiebaPool.push( pJieba );
}

void ArticleServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    string          id;
    vector<string>  result;

    // DLOG(INFO) << "KnnService received request: " << request;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");
    
    try {
        id = root["id"].asString();
        string content = root["content"].asString();
        wordSegment( result, content );
    } catch (const std::exception &ex) {
        THROW_INVALID_REQUEST("handleRequest fail: " << ex.what());
    } // try

    Json::Value resp;
    resp["status"] = 0;
    resp["id"] = id;
    for (auto &v : result)
        resp["result"].append(v);

    Json::FastWriter writer;  
    _return = writer.write(resp);
}

} // namespace Article

