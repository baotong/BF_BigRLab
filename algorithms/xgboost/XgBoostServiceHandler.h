#ifndef _XGBOOST_SERVICE_HANDLER_H_
#define _XGBOOST_SERVICE_HANDLER_H_

#include "XgBoostService.h"

namespace XgBoostSvr {

class XgBoostServiceHandler : public XgBoostServiceIf {
public:
    virtual void predictStr(std::vector<double> & _return, const std::string& input, const bool leaf);
    virtual void predictVec(std::vector<double> & _return, const std::vector<int64_t> & indices, 
                const std::vector<double> & values);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace XgBoostSvr

#endif

