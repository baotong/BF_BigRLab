#ifndef _FTRL_SERVICE_HANDLER_H_
#define _FTRL_SERVICE_HANDLER_H_

#include "FtrlService.h"
#include "FtrlModel.h"
#include "db.hpp"

namespace FTRL {

class FtrlServiceHandler : virtual public FtrlServiceIf {
public:
    virtual double lrPredict(const std::string& id, const std::string& data);
    virtual bool setValue(const std::string& id, const double value);
    virtual void handleRequest(std::string& _return, const std::string& request);
private:
    bool checkId(const std::string &id);
};

} // namespace FTRL

DECLARE_int32(epoch);
DECLARE_string(model);
DECLARE_int64(update_cnt);
extern std::unique_ptr<FtrlModel>       g_pFtrlModel;
extern std::unique_ptr<DB>              g_pDb;

#endif

