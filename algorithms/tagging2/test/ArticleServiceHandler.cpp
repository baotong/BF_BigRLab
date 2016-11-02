#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>
#include <limits>
#include <thread>

#define TIMEOUT     60000   // 1min

#define ON_FINISH_CLASS(name, deleter) \
    std::unique_ptr<void, std::function<void(void*)> > \
        name((void*)-1, [&, this](void*) deleter )

namespace Article {

using namespace std;

void ArticleServiceHandler::setFilter( const std::string &strFilter )
{
    THROW_INVALID_REQUEST("not implemented");
}

void ArticleServiceHandler::wordSegment(std::vector<std::string> & _return, const std::string& sentence)
{
    Jieba::pointer pJieba;
    if (!g_JiebaPool.timed_pop(pJieba, TIMEOUT))
        THROW_INVALID_REQUEST("No available Jieba object!");

    ON_FINISH_CLASS(pCleanup, {g_JiebaPool.push(pJieba);});

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
    if (k <= 0)
        THROW_INVALID_REQUEST("Invalid k value " << k);

    Jieba::pointer pJieba;
    if (!g_JiebaPool.timed_pop(pJieba, TIMEOUT))
        THROW_INVALID_REQUEST("No available Jieba object!");

    ON_FINISH_CLASS(pCleanup, {g_JiebaPool.push(pJieba);});

    Jieba::KeywordResult result;
    try {
        pJieba->keywordExtract(sentence, result, k);
        _return.resize(result.size());
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();
        for (std::size_t i = 0; i < result.size(); ++i) {
            _return[i].word.swap(result[i].word);
            _return[i].weight = result[i].weight;
            if (_return[i].weight < min)
                min = _return[i].weight;
            if (_return[i].weight > max)
                max = _return[i].weight;
        } // for

        // 归一化
        double base = max - min;
        for (auto &v : _return) {
            v.weight -= min;
            v.weight /= base;
        } // for v

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

    if (text.empty())
        THROW_INVALID_REQUEST("Input sentence cannot be empty!");

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
    
    struct Record {
        Record(double _W, uint32_t _C) : weight(_W), count(_C) {}
        double weight;
        uint32_t count;
    };

    typedef std::map<std::string, Record>   CandidateType;
    CandidateType candidates; // 存储结果 tag:weight

    // toVector and get cluster id
    uint32_t cid = 0;
    bool success = true;
    string errmsg;
    std::thread thrCluster([&, this]{
        try {
            vector<string> segment;
            wordSegment(segment, text);
            vector<double> vec;
            //!! article2vec 维度和 cluster 维度必须一致
            g_pVecConverter->convert2Vector(segment, vec);
            cid = g_pClusterPredict->predict(vec);
        } catch (const std::exception &ex) {
            success = false;
            errmsg = std::move(ex.what());
        } // try
    });

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
            auto ret = candidates.insert(std::make_pair(cij, Record(0.0, 0)));
            auto it = ret.first;
            ++(it->second.count);
            if (cij == kw.word)
                it->second.weight += kw.weight;
            else
                it->second.weight += kw.weight * cwij;
        } // for cItem
    } // for kw
    
    thrCluster.join();
    if (!success)
        THROW_INVALID_REQUEST("Get cluster fail: " << errmsg);

    for (auto it = candidates.begin(); it != candidates.end();) {
        if (it->second.count <= 1) {
            it = candidates.erase(it);
        } else {
            it->second.weight *= it->second.count - 1;
            auto ret = g_pWordClusterDB->query(it->first, cid);
            DLOG(INFO) << "cid = " << cid;
            if (ret.second)
                it->second.weight += ret.first >= FLAGS_threshold ? ret.first : 0.0;
            DLOG_IF(INFO, !ret.second) << "Cannot find word " << it->first << " in cluster " << cid;
            ++it;
        } // if
    } // for it

    // sort candidates
    boost::ptr_vector<CandidateType::value_type, boost::view_clone_allocator> 
            ptrArray(candidates.begin(), candidates.end());
    ptrArray.sort([](const CandidateType::value_type &lhs, const CandidateType::value_type &rhs)->bool {
        return lhs.second.weight > rhs.second.weight;
    });

    _return.resize( ptrArray.size() );
    for (size_t i = 0; i != ptrArray.size(); ++i) {
        _return[i].tag = ptrArray[i].first;
        _return[i].weight = ptrArray[i].second.weight;
    } // for
}

void ArticleServiceHandler::do_tagging_knn(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t k1, const int32_t k2, const int32_t searchK)
{
    using namespace std;

    THROW_INVALID_REQUEST("Not implemented!");
    return;

    // DLOG(INFO) << "do_tagging_knn";

#if 0    
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
#endif
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

} // namespace Article

