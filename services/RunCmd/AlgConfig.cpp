#include "AlgConfig.h"


using namespace std;

std::map<std::string, AlgConfigMgr::NewInstFunc>   AlgConfigMgr::m_mapAlgNewInst;

void AlgConfig::killApp(const RunCmdService::RunCmdClientPtr &pClient, const std::string &appName, int checkCnt)
{
    string  killCmd = "killall " + appName;
    string  pidOfCmd = "pidof " + appName;
    bool    killed = false;
    RunCmd::CmdResult result;

    pClient->client()->runCmd(killCmd);
    while (!killed && checkCnt--) {
        pClient->client()->readCmd(result, pidOfCmd);
        if (result.retval) {
            killed = true;
            break;
        } // if
        SLEEP_SECONDS(1);
    } // while

    if (!killed) {
        killCmd = "killall -9 " + appName;
        pClient->client()->runCmd(killCmd);
    } // if
}




