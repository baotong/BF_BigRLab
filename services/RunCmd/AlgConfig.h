#ifndef _ALG_CONFIG_H_
#define _ALG_CONFIG_H_

#include <memory>
#include <atomic>
#include <map>
#include <functional>
// #include <glog/logging.h>
#include "RunCmdServiceSo.h"

class AlgConfig {
public:
    typedef std::shared_ptr<AlgConfig>      pointer;
public:
    virtual bool parseArg(Json::Value &root, std::string &err) = 0;
    virtual void run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp) = 0;
    void killApp(const RunCmdService::RunCmdClientPtr &pClient, const std::string &appName, int checkCnt = 10);

protected:
    std::string     m_strCmd;
};


class AlgConfigMgr {
    typedef std::function<AlgConfig*(void)>     NewInstFunc;

public:
    static bool registerNewInst(const std::string &algName, const NewInstFunc &newInstFn)
    {
        // DLOG(INFO) << "Registering alg " << algName;
        auto ret = m_mapAlgNewInst.insert(std::make_pair(algName, newInstFn));
        return ret.second;
    }

    static AlgConfig::pointer newInst(const std::string &algName)
    {
        auto it = m_mapAlgNewInst.find(algName);
        if (it != m_mapAlgNewInst.end())
            return AlgConfig::pointer((it->second)());
        return nullptr;
    }

private:
    static std::map<std::string, NewInstFunc>   m_mapAlgNewInst;
};


#endif

