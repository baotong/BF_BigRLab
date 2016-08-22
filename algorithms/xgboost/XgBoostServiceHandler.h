#ifndef _XGBOOST_SERVICE_HANDLER_H_
#define _XGBOOST_SERVICE_HANDLER_H_

#include "XgBoostService.h"
#include <gflags/gflags.h>


namespace XgBoostSvr {

class XgBoostServiceHandler : public XgBoostServiceIf {
public:
    virtual void predict(std::vector<double> & _return, const std::string& input, const bool leaf);
    virtual void predict_GBDT(std::vector<double> & _return, const std::string& input, const bool simple);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace XgBoostSvr

DECLARE_int64(offset);

#endif

