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

void XgBoostServiceHandler::predictStr(std::vector<double> & _return, 
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
        for (std::size_t i = 1; i < _return.size(); ++i)
            _return[i] += g_arrMaxLeafId[i-1]+1;
    } // if
}

void XgBoostServiceHandler::predictVec(std::vector<double> & _return, 
                    const std::vector<int64_t> & indices, 
                    const std::vector<double> & values)
{
    THROW_INVALID_REQUEST("Not implemented!");
}

void XgBoostServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    Json::Value     resp;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");
    
    try {
        string         reqtype = root["req"].asString();
        string         data = root["data"].asString();
        vector<double> result;

        if ("predict" == reqtype) {
            predictStr(result, data, false);
            Json::Value resJson;
            for (auto &v : result)
                resJson.append(v);
            resp["result"].swap(resJson);
        } else if ("leaf" == reqtype) {
            predictStr(result, data, true);
            Json::Value resJson;
            for (auto &v : result)
                resJson.append((uint32_t)v);
            resp["result"].swap(resJson);
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


} // namespace XgBoostSvr

