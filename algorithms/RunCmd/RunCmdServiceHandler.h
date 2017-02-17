#ifndef _RUN_CMD_SERVICE_HANDLER_H_
#define _RUN_CMD_SERVICE_HANDLER_H_

#include "RunCmdService.h"

extern std::string                  g_strAlgMgrAddr;
extern uint16_t                     g_nAlgMgrPort;

namespace RunCmd {

class RunCmdServiceHandler : public RunCmdServiceIf {
public:
    virtual void readCmd(std::string& _return, const std::string& cmd);
    virtual int32_t runCmd(const std::string& cmd);
    virtual void getAlgMgr(std::string& _return);
private:
    int doRunCmd(const std::string &cmd, std::string &output);
};

} // namespace RunCmd

#endif

