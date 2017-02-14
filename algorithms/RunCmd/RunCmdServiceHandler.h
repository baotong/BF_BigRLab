#ifndef _RUN_CMD_SERVICE_HANDLER_H_
#define _RUN_CMD_SERVICE_HANDLER_H_

#include "RunCmdService.h"

namespace RunCmd {

class RunCmdServiceHandler : public RunCmdServiceIf {
public:
    virtual void runCmd(std::string& _return, const std::string& cmd);
private:
    int doRunCmd(const std::string &cmd, std::string &output);
};

} // namespace RunCmd

#endif

