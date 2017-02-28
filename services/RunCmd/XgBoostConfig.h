#ifndef _XGBOOST_CONFIG_H_
#define _XGBOOST_CONFIG_H_

#include "AlgConfig.h"


class XgBoostConfig : public AlgConfig {
    constexpr static const char *CONFIG_FILE = "/tmp/xgboost_empty.conf";
    // constexpr static const char *MODEL_FILE = "/tmp/xgboost.model";
    static std::atomic_int      s_nSvrPort;
public:
    // XgBoostConfig() : m_strCmd("xgboost ") {}    //!! ERROR

    virtual bool parseArg(Json::Value &root, std::string &err);
    virtual void run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp);

private:
    bool parseTrainArg(Json::Value &root, std::string &err);
    bool parseOnlineArg(Json::Value &root, std::string &err);
    void runTrain(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp);
    void runOnline(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp);
    void runOffline(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp);

private:
    std::string     m_strTask;
};


#endif

