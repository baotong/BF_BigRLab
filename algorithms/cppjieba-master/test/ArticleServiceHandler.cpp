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

void ArticleServiceHandler::keyword(std::vector<KeywordResult> & _return, 
            const std::string& sentence, const int32_t k)
{
    if (sentence.empty())
        THROW_INVALID_REQUEST("Input sentence cannot be empty!");

    Jieba::pointer pJieba;
    if (!g_JiebaPool.timed_pop(pJieba, TIMEOUT))
        THROW_INVALID_REQUEST("No available Jieba object!");

    if (k <= 0)
        THROW_INVALID_REQUEST("Invalid k value " << k);

    Jieba::KeywordResult result;
    try {
        pJieba->keywordExtract(sentence, result, k);
        _return.resize(result.size());
        for (std::size_t i = 0; i < result.size(); ++i) {
            _return[i].word.swap(result[i].word);
            _return[i].weight = result[i].weight;
        } // for

    } catch (const std::exception &ex) {
        LOG(ERROR) << "Jieba extract keyword error: " << ex.what();
    } // try

    g_JiebaPool.push( pJieba );
}

void ArticleServiceHandler::toVector(std::vector<double> & _return, const std::string& sentence)
{
    vector<string> wordsegResult;
    wordSegment( wordsegResult, sentence );

    vector<float> fResult;
    g_pVecConverter->convert2Vector( wordsegResult, fResult );
    _return.assign( fResult.begin(), fResult.end() );
}


void ArticleServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    string          id;
    Json::Value     resp;

    // DLOG(INFO) << "KnnService received request: " << request;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");
    
    try {
        vector<string>  result;
        string content = root["content"].asString();
        string reqtype = root["reqtype"].asString();
        if ("wordseg" == reqtype) {
            wordSegment( result, content );
            for (auto &v : result)
                resp["result"].append(v);

        } else if ("keyword" == reqtype) {
            vector<KeywordResult> result;
            int topk = root["topk"].asInt();
            string content = root["content"].asString();
            keyword(result, content, topk);
            for (auto& v : result)
                resp["result"].append(v.word);

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

