#ifndef _ALG_CONFIG_H_
#define _ALG_CONFIG_H_

#include <memory>
#include "RunCmdServiceSo.h"

class AlgConfig {
public:
    typedef std::shared_ptr<AlgConfig>      pointer;
public:
    virtual bool parseArg(Json::Value &root, std::string &err) = 0;
    virtual void run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp) = 0;
protected:
    std::string     m_strCmd;
};


class XgBoostConfig : public AlgConfig {
    constexpr static const char *CONFIG_FILE = "/tmp/xgboost_empty.conf";
    constexpr static const char *MODEL_FILE = "/tmp/xgboost.model";
public:
    // XgBoostConfig() : m_strCmd("xgboost ") {}    //!! ERROR

    virtual bool parseArg(Json::Value &root, std::string &err);
    virtual void run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp);

private:
    bool parseTrainArg(Json::Value &root, std::string &err);
    bool parseDeployArg(Json::Value &root, std::string &err);

private:
    std::string     m_strTask;
};


class AlgConfigMgr {
public:
    static AlgConfig::pointer newInst(const std::string &algName)
    {
        if ("xgboost" == algName)
            return std::make_shared<XgBoostConfig>();

        return nullptr;
    }
};


#endif

