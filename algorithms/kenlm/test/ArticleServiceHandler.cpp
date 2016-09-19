#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include "StringTrie.hpp"
#include "ngram_model.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>

#define TIMEOUT     60000   // 1min

#define ON_FINISH_CLASS(name, deleter) \
    std::unique_ptr<void, std::function<void(void*)> > \
        name((void*)-1, [&, this](void*) deleter )

namespace Article {

using namespace std;

inline
std::ostream& operator << (std::ostream &os, const std::deque<StringTrie::elem_pointer> &path)
{
    for (const auto &v : path)
        os << *v << " ";
    return os;
}

void ArticleServiceHandler::setFilter( const std::string &strFilter )
{
    THROW_INVALID_REQUEST("not implemented");
}

void ArticleServiceHandler::knn(const GramArray &arr, StringMatrix &result, int32_t k)
{
    vector<float>   dist;
    GramMatrix      mat(arr.size());

    result.clear();
    result.resize(arr.size());

    for (size_t i = 0; i != arr.size(); ++i) {
        mat[i].reserve(k + 1);
        g_pAnnDB->kNN_By_Gram(arr[i], (size_t)k, mat[i], dist);
        mat[i].insert(mat[i].begin(), arr[i]);
        result[i].resize(mat[i].size());
        for (size_t j = 0; j != result[i].size(); ++j)
            result[i][j].swap(mat[i][j].word);
    } // for i
}

void ArticleServiceHandler::creativeRoutine(std::vector<Result> & _return, 
            const std::string& input, const int32_t k, const int32_t bSearchK, const int32_t topk)
{
    if (input.empty())
        THROW_INVALID_REQUEST("Input string is empty!");

    if (k <= 0 || bSearchK <= 0 || topk <= 0)
        THROW_INVALID_REQUEST("Invalid argument!");

    GramArray tagResult;
    g_pJieba->wordSegment(input, tagResult);
    if (tagResult.empty())
        return;

// #ifndef NDEBUG
    // DLOG(INFO) << "After word segment:";
    // ostringstream ostr;
    // std::copy(tagResult.begin(), tagResult.end(), ostream_iterator<Jieba::Gram>(ostr, " "));
    // DLOG(INFO) << ostr.str();
// #endif 

    StringMatrix knnResult;
    knn(tagResult, knnResult, k);
    if (knnResult.empty())
        return;

    beam_search( _return, knnResult, (size_t)bSearchK, (size_t)topk );
}

void ArticleServiceHandler::beam_search( std::vector<Result> &result, 
                    const StringMatrix &strMat, std::size_t searchK, std::size_t topk )
{
    struct NodeCmp {
        bool operator()(const StringTrie::Node::pointer &lhs,
                        const StringTrie::Node::pointer &rhs) const
        { return lhs->weight() < rhs->weight(); }
    };

    typedef std::multiset<StringTrie::Node::pointer, NodeCmp>   WorkSet;

    if (strMat.size() <= 1)
        return;

    StringTrie                              trie;
    WorkSet                                 lastLevel, curLevel;
    std::deque<StringTrie::elem_pointer>    path;
    std::vector<std::size_t>                searchkVec;

    searchkVec.resize( strMat.size(), searchK );
    // for (auto it = searchkVec.rbegin(); it != searchkVec.rend(); ++it)
        // *it = searchK * (std::distance(searchkVec.rbegin(), it) + 1);

    // std::reverse(searchkVec.begin(), searchkVec.end());

    // DLOG(INFO) << "searchkVec:";
    // std::copy(searchkVec.begin(), searchkVec.end(), ostream_iterator<size_t>(DLOG(INFO), " "));

    auto rowIt = strMat.begin();
    auto& firstRow = *rowIt++;
    // add first row to tree
    for (auto &v : firstRow) {
        auto ret = trie.addNode( trie.root(), v );
        if (ret.second)
            curLevel.insert(ret.first);
    } // for
    // trie.traverse(cout);
    // cout << endl;
    // cout << curLevel.size() << endl;

    // from 2nd row to end
    for (auto searchkIt = searchkVec.begin()+1; rowIt != strMat.end(); ++rowIt, ++searchkIt) {
        curLevel.swap(lastLevel);
        curLevel.clear();
        // for every word in this row
        for (auto &word : *rowIt) {
            // for every node in last level
            for (auto &parent : lastLevel) {
                auto ret = trie.addNode(parent, word);
                if (ret.second) {
                    auto pNewChild = ret.first;
                    pNewChild->getPath(path);
                    double score = g_pLMmodel->score(path.begin(), path.end());
                    pNewChild->setWeight(score);
                    // DLOG(INFO) << score << "\t" << path;
                    // DLOG(INFO) << "searchK = " << *searchkIt;
                    // try to add to cur level
                    if (curLevel.size() < *searchkIt) {
                        curLevel.insert(pNewChild);   
                    } else if (pNewChild->weight() > (*curLevel.begin())->weight()) {
                        auto pRemoved = *curLevel.begin();
                        curLevel.erase(curLevel.begin());
                        pRemoved->removeSelf();
                        curLevel.insert(pNewChild);
                    } else {
                        pNewChild->removeSelf();
                    } // if
                } // if ret.second
            } // for parent
        } // for word
    } // for

    // trie.syncElems();

    result.clear();
    for (auto it = curLevel.rbegin(); it != curLevel.rend() && topk; ++it) {
        auto &node = *it;
        node->getPath(path);
        if (path.empty())
            continue;
        --topk;
        ostringstream stream;
        stream << path << flush;
        DLOG(INFO) << path;
        result.emplace_back();
        result.back().text = std::move(stream.str());
        boost::trim_right(result.back().text);
        result.back().score = node->weight();
    } // for

// #ifndef NDEBUG
    // cout << "Final results:" << endl;
    // for (auto it = curLevel.rbegin(); it != curLevel.rend(); ++it) {
        // auto &node = *it;
        // node->getPath(path);
        // cout << node->weight() << "\t" << path << endl;
    // } // for
// #endif
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
        string text = root["text"].asString();
        int    k = root["k"].asInt();
        int    bSearchK = root["bsearchk"].asInt();
        int    topk = root["topk"].asInt();
        // int    searchK = -1, topk = 0, method = -1;

        vector<Result> result;
        creativeRoutine(result, text, k, bSearchK, topk);

        // DLOG(INFO) << "result.size() = " << result.size();

        if (result.empty()) {
            resp["result"] = "null";
        } else {
            for (auto &v : result) {
                Json::Value item;
                item["text"] = v.text;
                item["score"] = v.score;
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

