#ifndef _KNN_CONFIG_H_
#define _KNN_CONFIG_H_

#include "AlgConfig.h"


class KnnConfig : public AlgConfig {
    static std::atomic_int      s_nSvrPort;
public:
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
    std::string     m_strFastTextMethod;
    std::string     m_strInputFile;
    std::string     m_strOutputFile;
    std::string     m_strService;
    int             m_nTrees;
};


#endif

