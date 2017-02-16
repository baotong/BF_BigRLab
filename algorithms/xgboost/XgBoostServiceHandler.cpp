#include "XgBoostServiceHandler.h"
#include "xgboost_learner.h"
#include "common.hpp"
#include "alg_common.hpp"
#include <json/json.h>
#include <cassert>

#define TIMEOUT     30000

namespace XgBoostSvr {

using namespace std;
using namespace xgboost;

void XgBoostServiceHandler::predict(std::vector<double> & _return, 
                    const std::string& input, const bool leaf)
{
    if (input.empty())
        THROW_INVALID_REQUEST("Input string cannot be empty");

    std::unique_ptr<DMatrix> pMat( XgBoostLearner::DMatrixFromStr(input) );
    if (!pMat)
        THROW_INVALID_REQUEST("Cannot build matrix from input string: " << input);

    XgBoostLearner::pointer pLearner;
    if (!g_LearnerPool.timed_pop(pLearner, TIMEOUT))
        THROW_INVALID_REQUEST("No available xgboost object!");

    ON_FINISH(pCleanup, {
        g_LearnerPool.push( pLearner );
    });

    vector<float> fResult;
    pLearner->predict( pMat.get(), fResult, leaf );
    _return.assign( fResult.begin(), fResult.end() );

    if (leaf) {
        assert(_return.size() == g_arrMaxLeafId.size());
        if (!g_arrTreeMark[0])
            _return[0] = 0.0;
        for (std::size_t i = 1; i < _return.size(); ++i) {
            if (g_arrTreeMark[i])
                _return[i] += g_arrMaxLeafId[i-1]+1;
            else
                _return[i] = 0.0;
        } // for
    } // if
}

void XgBoostServiceHandler::predict_GBDT(std::vector<double> & _return, 
                    const std::string& input, const bool simple)
{
    predict(_return, input, true); // 先得到叶子序号
    
    stringstream stream;

    if (!simple)
        stream << input;

    for (auto &dv : _return) {
        uint32_t v = (uint32_t)dv;
        if (v) {
            if (simple)
                stream << " " << v << ":1";
            else
                stream << " " << (v + (uint32_t)FLAGS_offset) << ":1";
        } // if v
    } // for

    std::unique_ptr<DMatrix> pMat( XgBoostLearner::DMatrixFromStr(stream.str()) );
    if (!pMat)
        THROW_INVALID_REQUEST("Cannot build matrix from string with leaf feature!");

    XgBoostLearner::pointer pLearner;
    if (!g_LearnerPool.timed_pop(pLearner, TIMEOUT))
        THROW_INVALID_REQUEST("No available xgboost object!");

    ON_FINISH(pCleanup, {
        g_LearnerPool.push( pLearner );
    });

    vector<float> fResult;
    pLearner->predict2( pMat.get(), fResult );
    _return.assign( fResult.begin(), fResult.end() );
}

void XgBoostServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    Json::Value     resp, resJson;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");
    
    try {
        string         reqtype = root["req"].asString();
        string         data = root["data"].asString();
        vector<double> result;

        if ("predict" == reqtype) {
            predict(result, data, false);
            for (auto &v : result)
                resJson.append(v);
            resp["result"].swap(resJson);
        } else if ("leaf" == reqtype) {
            predict(result, data, true);
            for (auto &v : result)
                resJson.append((uint32_t)v);
            resp["result"].swap(resJson);
        } else if ("predict_gbdt" == reqtype) {
            predict_GBDT(result, data, false);
            for (auto &v : result)
                resJson.append(v);
            resp["result"].swap(resJson);
        } else if ("predict_gbdt_simple" == reqtype) {
            predict_GBDT(result, data, true);
            for (auto &v : result)
                resJson.append(v);
            resp["result"].swap(resJson);
        } else {
            THROW_INVALID_REQUEST("Invalid reqtype " << reqtype);
        } // if

        if (result.empty())
            resp["result"] = "null";

    } catch (const AlgCommon::InvalidRequest &err) {
        throw err;
    } catch (const std::exception &ex) {
        LOG(ERROR) << "handleRequest fail: " << ex.what();
        THROW_INVALID_REQUEST("handleRequest fail: " << ex.what());
    } // try

    resp["status"] = 0;

    Json::FastWriter writer;  
    _return = writer.write(resp);
}


} // namespace XgBoostSvr

