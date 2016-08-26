#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/ptr_container/ptr_vector.hpp>
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

void ArticleServiceHandler::tagging(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t method, const int32_t k1, const int32_t k2, const int32_t searchK)
{
    if (method == CONCUR)
        do_tagging_concur(_return, text, k1, k2 );
    else if (method == KNN)
        do_tagging_knn(_return, text, k1, k2, searchK);
    else
        THROW_INVALID_REQUEST("Invalid tagging method!");
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
        auto endIt = (k2 >= cList.size()) ? cList.end() : cList.begin() + k2;
        for (auto cit = cList.begin(); cit != endIt; ++cit) {
            auto &cItem = *cit;
            string &cij = *(boost::get<ConcurTable::StringPtr>(cItem.item));
            double &cwij = cItem.weight;
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

void ArticleServiceHandler::do_tagging_knn(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t k1, const int32_t k2, const int32_t searchK)
{
    using namespace std;

    // DLOG(INFO) << "do_tagging_knn";

    typedef std::map<std::string, double>   CandidateType;
    CandidateType candidates; // 存储结果 tag:weight

    // 找 keyword
    std::vector<KeywordResult> keywords;
    keyword( keywords, text, k1 );

    for (auto &kw : keywords) {
        // get knn result
        vector<string>      knnResult;
        vector<float>       knnDist;
        g_pAnnDB->kNN_By_Word(kw.word, k2, knnResult, knnDist, searchK);
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




void ArticleServiceHandler::toVector(std::vector<double> & _return, const std::string& sentence)
{
}

void ArticleServiceHandler::knn(std::vector<KnnResult> & _return, const std::string& sentence, 
                const int32_t n, const int32_t searchK, const std::string& reqtype)
{
}

void ArticleServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
}

} // namespace Article

