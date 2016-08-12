#include "XgBoostServiceHandler.h"
#include "xgboost_learner.h"
#include "alg_common.hpp"

#define TIMEOUT     30000

namespace XgBoostSvr {

using namespace std;

void XgBoostServiceHandler::predictStr(std::vector<double> & _return, 
                    const std::string& input, const bool leaf)
{
    if (input.empty())
        THROW_INVALID_REQUEST("Input string cannot be empty!");

    XgBoostLearner::pointer pLearner;
    if (!g_LearnerPool.timed_pop(pLearner, TIMEOUT))
        THROW_INVALID_REQUEST("No available xgboost object!");

    boost::shared_ptr<void> pCleanup((void*)0, [&](void*){
        g_LearnerPool.push( pLearner );
    });

}

void XgBoostServiceHandler::predictVec(std::vector<double> & _return, 
                    const std::vector<int64_t> & indices, 
                    const std::vector<double> & values)
{

}

void XgBoostServiceHandler::handleRequest(std::string& _return, const std::string& request)
{

}


} // namespace XgBoostSvr

