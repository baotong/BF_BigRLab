#include "XgBoostServiceHandler.h"
#include "xgboost_learner.h"
#include "alg_common.hpp"
#include <json/json.h>

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
        THROW_INVALID_REQUEST("Cannot build matrix from input string");

    XgBoostLearner::pointer pLearner;
    if (!g_LearnerPool.timed_pop(pLearner, TIMEOUT))
        THROW_INVALID_REQUEST("No available xgboost object!");

    boost::shared_ptr<void> pCleanup((void*)0, [&](void*){
        g_LearnerPool.push( pLearner );
    });

    vector<float> fResult;
    pLearner->predict( pMat.get(), fResult );
    _return.assign( fResult.begin(), fResult.end() );
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
    

}


} // namespace XgBoostSvr

