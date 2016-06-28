#include "ArticleServiceHandler.h"
#include "Jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>

#define TIMEOUT     5000

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
    Json::Value     resp;

    // DLOG(INFO) << "KnnService received request: " << request;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");
    
    try {
        string content = root["content"].asString();
        string reqtype = root["reqtype"].asString();
        if ("wordseg" == reqtype) {
            wordSegment( result, content );
            for (auto &v : result)
                resp["result"].append(v);
        } else {
            THROW_INVALID_REQUEST("Invalid reqtype " << reqtype);
        } // if
    } catch (const std::exception &ex) {
        THROW_INVALID_REQUEST("handleRequest fail: " << ex.what());
    } // try

    resp["status"] = 0;

    Json::FastWriter writer;  
    _return = writer.write(resp);
}

} // namespace Article

