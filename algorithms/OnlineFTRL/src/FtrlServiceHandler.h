#ifndef _FTRL_SERVICE_HANDLER_H_
#define _FTRL_SERVICE_HANDLER_H_

#include "FtrlService.h"
#include "FtrlModel.h"

namespace FTRL {

class FtrlServiceHandler : virtual public FtrlServiceIf {
public:
    virtual double lrPredict(const std::string& input);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace FTRL

extern std::unique_ptr<FtrlModel>       g_pFtrlModel;

#endif

