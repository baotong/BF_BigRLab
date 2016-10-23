#include <cassert>
#include <limits>
#include <vector>
#include <algorithm>
#include <json/json.h>
#include <glog/logging.h>
#include "common.hpp"
#include "alg_common.hpp"
#include "TopicServiceHandler.h"
 
#define TIMEOUT     30000           // 30s

namespace Topics {

void TopicServiceHandler::predictTopic(std::vector<Result> & _return, const std::string& text, 
                const int32_t topk, const int32_t nIter, const int32_t perplexity, const int32_t nSkip)
try {
    if (text.empty())
        THROW_INVALID_REQUEST("Input string cannot be empty");

    TopicModule *pTopicModule = 0;
    if (g_queTopicModules->pop(pTopicModule, TIMEOUT) || !pTopicModule)
        THROW_INVALID_REQUEST("No available TopicModule object!");

    ON_FINISH(pCleanup, {
        g_queTopicModules->push( pTopicModule, TIMEOUT );
    });

    TopicModule::Result     result;
    pTopicModule->predict(text, result, nIter, perplexity, nSkip);

    std::vector<uint32_t>   topicCount(pTopicModule->nTopics(), 0);
    for (auto &v : result)
        ++topicCount[v.second];

    uint32_t min = std::numeric_limits<uint32_t>::max();
    uint32_t max = std::numeric_limits<uint32_t>::min();
    for (uint32_t i = 0; i != topicCount.size(); ++i) {
        if (topicCount[i] < min) min = topicCount[i];
        if (topicCount[i] > max) max = topicCount[i];
        if (topicCount[i]) {
            _return.emplace_back(Result());
            _return.back().topicId = i;
            _return.back().possibility = (double)topicCount[i];
        } // if
    } // if

    if (_return.empty()) return;

    double denominator = (double)(max - min);
    for (auto &v : _return) {
        v.possibility -= min;
        v.possibility /= denominator;
    } // if

    auto cmpfun = [](const Result &lhs, const Result &rhs)->bool {
        return (lhs.possibility > rhs.possibility);  
    };

    if (topk <= 0 || (std::size_t)topk >= _return.size()) {
        std::sort(_return.begin(), _return.end(), cmpfun);
    } else {
        std::partial_sort(_return.begin(), _return.begin() + topk, _return.end(), cmpfun);
        _return.resize(topk);
    } // if

} catch (const InvalidRequest &err) {
    throw err;
} catch (const std::exception &ex) {
    LOG(ERROR) << "Excepthon during predictTopic: " << ex.what();
    THROW_INVALID_REQUEST("Excepthon during predictTopic: " << ex.what());
} // try


void TopicServiceHandler::handleRequest(std::string& _return, const std::string& request)
try {
    using namespace std;

    Json::Reader    reader;
    Json::Value     root;
    Json::Value     resp, resJson;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");
    
    string text = root["text"].asString();
    int    topk = root["topk"].asInt();
    int    nIter = root["niter"].asInt();
    int    perplexity = root["perplexity"].asInt();
    int    nSkip = root["nskip"].asInt();

    vector<Result> result;
    predictTopic(result, text, topk, nIter, perplexity, nSkip);

    if (result.empty()) {
        resp["result"] = "null";
    } else {
        for (auto &item : result) {
            Json::Value v;
            v["topicid"] = Json::Int64(item.topicId);
            v["possibility"] = item.possibility;
            resJson.append(v);
        } // for
        resp["result"].swap(resJson);
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


} // namespace Topics

