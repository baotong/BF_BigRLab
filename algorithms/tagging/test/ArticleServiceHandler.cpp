#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>
// #include <boost/foreach.hpp>
// #include <boost/tuple/tuple.hpp>
// #include <boost/range/combine.hpp>

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

    boost::shared_ptr<void> pCleanup((void*)-1, [&](void*){
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

    boost::shared_ptr<void> pCleanup((void*)-1, [&](void*){
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

void ArticleServiceHandler::tagging(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t method, const int32_t k1, const int32_t k2, const int32_t searchK, const int32_t topk)
{
    // DLOG(INFO) << "ArticleServiceHandler::tagging() ...";
    // DLOG(INFO) << "method: " << method;
    // DLOG(INFO) << "text: " << text;
    // DLOG(INFO) << "k1: " << k1;
    // DLOG(INFO) << "k2: " << k2;
    // DLOG(INFO) << "searchK: " << searchK;
    // DLOG(INFO) << "topk: " << topk;

    if (method == CONCUR)
        do_tagging_concur(_return, text, k1, k2 );
    else if (method == KNN)
        do_tagging_knn(_return, text, k1, k2, searchK);
    else
        THROW_INVALID_REQUEST("Invalid tagging method!");

    if (topk > 0 && topk < _return.size())
        _return.resize(topk);

    // DLOG(INFO) << "_return.size() = " << _return.size();
}

void ArticleServiceHandler::do_tagging_concur(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t k1, const int32_t k2)
{
    using namespace std;

    // DLOG(INFO) << "do_tagging_concur";

    typedef std::map<std::string, double>   CandidateType;
    CandidateType candidates; // 存储结果 tag:weight

    // 找 keyword
    std::vector<KeywordResult> keywords;
    keyword( keywords, text, k1 );

    for (auto &kw : keywords) {
        // get concur(ki)
        auto cList = g_pConcurTable->lookup( kw.word );
        if (cList.empty())
            continue;
        // DLOG(INFO) << "keyword: " << kw;
        auto endIt = (k2 >= cList.size()) ? cList.end() : cList.begin() + k2;
        for (auto cit = cList.begin(); cit != endIt; ++cit) {
            auto &cItem = *cit;
            string &cij = *(boost::get<ConcurTable::StringPtr>(cItem.item));
            double &cwij = cItem.weight;
            // DLOG(INFO) << "cij = " << cij << " cwij = " << cwij;
            auto ret = candidates.insert(std::make_pair(cij, 0.0));
            auto it = ret.first;
            if (cij == kw.word)
                it->second += kw.weight;
            else
                it->second += kw.weight * cwij;
        } // for cItem
    } // for kw

    // sort candidates
    boost::ptr_vector<CandidateType::value_type, boost::view_clone_allocator> 
            ptrArray(candidates.begin(), candidates.end());
    ptrArray.sort([](const CandidateType::value_type &lhs, const CandidateType::value_type &rhs)->bool {
        return lhs.second > rhs.second;
    });

    _return.resize( ptrArray.size() );
    for (size_t i = 0; i != ptrArray.size(); ++i) {
        _return[i].tag = ptrArray[i].first;
        _return[i].weight = ptrArray[i].second;
    } // for
}

#if 0
void ArticleServiceHandler::do_tagging_concur(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t k1, const int32_t k2)
{
    using namespace std;

    // DLOG(INFO) << "do_tagging_concur";

    typedef std::map<std::string, double>   CandidateType;
    CandidateType candidates; // 存储结果 tag:weight

    // 找 keyword
    std::vector<KeywordResult> keywords;
    keyword( keywords, text, k1 );

    for (auto &kw : keywords) {
        // get concur(ki)
        auto cList = g_pConcurTable->lookup( kw.word );
        if (cList.empty())
            continue;
        // DLOG(INFO) << "keyword: " << kw;
        auto endIt = (k2 >= cList.size()) ? cList.end() : cList.begin() + k2;
        for (auto cit = cList.begin(); cit != endIt; ++cit) {
            auto &cItem = *cit;
            string &cij = *(boost::get<ConcurTable::StringPtr>(cItem.item));
            // auto pCij = boost::get<ConcurTable::StringPtr>(cItem.item);
            // assert( pCij );
            // string &cij = *pCij;
            // assert(!cij.empty());
            double &cwij = cItem.weight;
            // DLOG(INFO) << "cij = " << cij << " cwij = " << cwij;
            if (g_TagSet.count(cij)) {
                // cij in tagset
                auto ret = candidates.insert(std::make_pair(cij, 0.0));
                auto it = ret.first;
                if (cij == kw.word)
                    it->second += kw.weight;
                else
                    it->second += kw.weight * cwij;
            } // if
        } // for cItem
    } // for kw

    // sort candidates
    boost::ptr_vector<CandidateType::value_type, boost::view_clone_allocator> 
            ptrArray(candidates.begin(), candidates.end());
    ptrArray.sort([](const CandidateType::value_type &lhs, const CandidateType::value_type &rhs)->bool {
        return lhs.second > rhs.second;
    });

    _return.resize( ptrArray.size() );
    for (size_t i = 0; i != ptrArray.size(); ++i) {
        _return[i].tag = ptrArray[i].first;
        _return[i].weight = ptrArray[i].second;
    } // for
}
#endif

void ArticleServiceHandler::do_tagging_knn(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t k1, const int32_t k2, const int32_t searchK)
{
    using namespace std;

    // DLOG(INFO) << "do_tagging_knn";
    
    if (k2 <= 0)
        THROW_RUNTIME_ERROR("Invalid k2 value, must be greater than 0");

    typedef std::map<std::string, double>   CandidateType;
    CandidateType candidates; // 存储结果 tag:weight

    // 找 keyword
    std::vector<KeywordResult> keywords;
    keyword( keywords, text, k1 );

    for (auto &kw : keywords) {
        // get knn result
        vector<string>      knnResult;
        vector<float>       knnDist;
        g_pAnnDB->kNN_By_Word(kw.word, k2, knnResult, knnDist, (size_t)searchK);
        if (knnResult.empty())
            continue;
        for (size_t i = 0; i != knnResult.size(); ++i) {
            string &cij = knnResult[i];
            double cwij = knnDist[i];
            if (g_TagSet.count(cij)) {
                // cij in tagset
                auto ret = candidates.insert(std::make_pair(cij, 0.0));
                auto it = ret.first;
                if (cij == kw.word)
                    it->second += kw.weight;
                else
                    it->second += kw.weight * ((2 - cwij) / 2);
            } // if
        } // for i
    } // for kw

    // sort candidates
    boost::ptr_vector<CandidateType::value_type, boost::view_clone_allocator> 
            ptrArray(candidates.begin(), candidates.end());
    ptrArray.sort([](const CandidateType::value_type &lhs, const CandidateType::value_type &rhs)->bool {
        return lhs.second > rhs.second;
    });

    _return.resize( ptrArray.size() );
    for (size_t i = 0; i != ptrArray.size(); ++i) {
        _return[i].tag = ptrArray[i].first;
        _return[i].weight = ptrArray[i].second;
    } // for
}


void ArticleServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    Json::Value     resp;

    // DLOG(INFO) << "KnnService received request: " << request;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");

    try {
        string strMethod = root["method"].asString();
        string text = root["text"].asString();
        int    k1 = root["k1"].asInt();
        int    k2 = root["k2"].asInt();
        int    searchK = -1, topk = 0, method = -1;

        boost::trim_right(text);

        if (root.isMember("searchk"))
            searchK = root["searchk"].asInt();
        if (root.isMember("topk"))
            topk = root["topk"].asInt();

        // DLOG(INFO) << "method: " << strMethod;
        // DLOG(INFO) << "text: " << text;
        // DLOG(INFO) << "k1: " << k1;
        // DLOG(INFO) << "k2: " << k2;
        // DLOG(INFO) << "searchK: " << searchK;
        // DLOG(INFO) << "topk: " << topk;

        vector<TagResult> result;
        if ("concur" == strMethod)
            method = CONCUR;
        else if ("knn" == strMethod)
            method = KNN;
        else
            THROW_INVALID_REQUEST("Invalid method name " << strMethod);

        tagging(result, text, method, k1, k2, (size_t)searchK, topk);

        // DLOG(INFO) << "result.size() = " << result.size();

        if (result.empty()) {
            resp["result"] = "null";
        } else {
            for (auto &v : result) {
                Json::Value item;
                item["tag"] = v.tag;
                item["weight"] = v.weight;
                resp["result"].append(item);
            } // for
        } // if

        resp["status"] = 0;

        Json::FastWriter writer;  
        _return = writer.write(resp);

    } catch (const InvalidRequest &err) {
        throw err;
    } catch (const std::exception &ex) {
        LOG(ERROR) << "handleRequest fail: " << ex.what();
        THROW_INVALID_REQUEST("handleRequest fail: " << ex.what());
    } // try
}

} // namespace Article

