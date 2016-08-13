#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>

#define TIMEOUT     30000

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

    boost::shared_ptr<void> pCleanup((void*)0, [&](void*){
        g_JiebaPool.push( pJieba );
    });

    try {
        pJieba->wordSegment(sentence, _return);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Jieba wordSegment error: " << ex.what();
        THROW_INVALID_REQUEST("Jieba wordSegment error: " << ex.what());
    } // try
}

void ArticleServiceHandler::keyword(std::vector<KeywordResult> & _return, 
            const std::string& sentence, const int32_t k)
{
    if (sentence.empty())
        THROW_INVALID_REQUEST("Input sentence cannot be empty!");

    if (k <= 0)
        THROW_INVALID_REQUEST("Invalid k value " << k);

    Jieba::pointer pJieba;
    if (!g_JiebaPool.timed_pop(pJieba, TIMEOUT))
        THROW_INVALID_REQUEST("No available Jieba object!");

    boost::shared_ptr<void> pCleanup((void*)0, [&](void*){
        g_JiebaPool.push( pJieba );
    });

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
        THROW_INVALID_REQUEST("Jieba extract keyword error: " << ex.what());
    } // try
}

void ArticleServiceHandler::toVector(std::vector<double> & _return, const std::string& sentence)
{
    vector<string> wordsegResult;
    wordSegment( wordsegResult, sentence );

    vector<float> fResult;
    g_pVecConverter->convert2Vector( wordsegResult, fResult );
    _return.assign( fResult.begin(), fResult.end() );
}

void ArticleServiceHandler::knn(std::vector<KnnResult> & _return, const std::string& sentence, 
                const int32_t n, const int32_t searchK, const std::string& reqtype)
{
    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value for knn!");

    vector<double> _result;
    toVector(_result, sentence);

    vector<ValueType>    sentenceVec(_result.begin(), _result.end());
    vector<IdType>       resultIds;
    vector<ValueType>    resultDists;

    typedef boost::tuple<KnnResult&, IdType&, ValueType&> IterType;

    try {
        g_pAnnDB->kNN_By_Vector( sentenceVec, (size_t)n, resultIds, resultDists, (size_t)searchK );
        _return.resize( resultIds.size() );
        BOOST_FOREACH( IterType v, boost::combine(_return, resultIds, resultDists) ) {
            v.get<0>().id = v.get<1>();
            v.get<0>().distance = v.get<2>();
        } // for_each

        if ("knn_label" == reqtype) {
            if (g_arrstrLabel.empty())
                THROW_INVALID_REQUEST("Label data not loaded!");
            std::for_each(_return.begin(), _return.end(), [&](KnnResult &res){
                if (res.id < g_arrstrLabel.size())
                    res.label = g_arrstrLabel[ res.id ];
            });
        } else if ("knn_score" == reqtype) {
            if (g_arrfScore.empty())
                THROW_INVALID_REQUEST("Score data not loaded!");
            std::for_each(_return.begin(), _return.end(), [&](KnnResult &res){
                if (res.id < g_arrfScore.size())
                    res.score = g_arrfScore[ res.id ];
            });
        } // if

    } catch (const InvalidRequest &ex) {
        throw ex;
    } catch (const std::exception &ex) {
        LOG(ERROR) << "ArticleService knn fail: " << ex.what();
        THROW_INVALID_REQUEST("ArticleService knn fail:: " << ex.what());
    } // try
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
            keyword(result, content, topk);
            for (auto& v : result) {
                Json::Value item;
                item["word"] = v.word;
                item["weight"] = v.weight;
                resp["result"].append(item);
            } // for

        } else if (reqtype.compare(0, 3, "knn") == 0) {
            vector<KnnResult> result;
            int n = root["n"].asInt();
            size_t searchK = (size_t)-1;
            if (root.isMember("search_k"))
                searchK = (size_t)(root["search_k"].asUInt64());
            knn( result, content, n, searchK, reqtype );
            for (auto& v : result) {
                Json::Value item;
                item["id"] = (Json::Int64)(v.id);
                item["distance"] = v.distance;
                if ("knn_label" == reqtype)
                    item["label"] = v.label;
                else if ("knn_score" == reqtype)
                    item["score"] = v.score;
                resp["result"].append(item);
            } // for

        } else {
            THROW_INVALID_REQUEST("Invalid reqtype " << reqtype);
        } // if
    } catch (const InvalidRequest &err) {
        throw err;
    } catch (const std::exception &ex) {
        LOG(ERROR) << "handleRequest fail: " << ex.what();
        THROW_INVALID_REQUEST("handleRequest fail: " << ex.what());
    } // try

    resp["status"] = 0;

    Json::FastWriter writer;  
    _return = writer.write(resp);
}

} // namespace Article

