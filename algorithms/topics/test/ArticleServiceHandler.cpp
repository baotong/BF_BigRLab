#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/algorithm/string.hpp>
// #include "common.hpp"

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

void ArticleServiceHandler::toVector(std::vector<double> & _return, const std::string& sentence, const bool wordseg)
{
    vector<string> wordsegResult;
    if (wordseg)
        wordSegment( wordsegResult, sentence );
    else
        boost::split(wordsegResult, sentence, boost::is_any_of(SPACES), boost::token_compress_on);

    vector<float> fResult;
    g_pVecConverter->convert2Vector( wordsegResult, fResult );
    _return.assign( fResult.begin(), fResult.end() );
}

void ArticleServiceHandler::knn(std::vector<KnnResult> & _return, const std::string& sentence, 
                const int32_t n, const int32_t searchK, const bool wordseg)
{
    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value for knn!");

    vector<double> _result;
    toVector(_result, sentence, wordseg);

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
        string text = root["text"].asString();
        int topk = root["topk"].asInt();
        bool wordseg = true;
        if (root.isMember("wordseg")) {
            int iWordSeg = root["wordseg"].asInt();
            wordseg = iWordSeg ? true : false;
        } // if
        size_t searchK = (size_t)-1;
        if (root.isMember("search_k"))
            searchK = (size_t)(root["search_k"].asUInt64());

        vector<KnnResult> result;
        knn(result, text, topk, searchK, wordseg);

        for (auto &v : result ) {
            Json::Value item;
            item["id"] = (Json::Int64)(v.id);
            item["distance"] = v.distance;
            resp["result"].append(item);
        } // for

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

