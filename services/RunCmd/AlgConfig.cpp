#include <sstream>
#include <boost/algorithm/string.hpp>
#include <json/json.h>
#include "AlgConfig.h"


using namespace std;

template <typename T>
inline std::string my_to_string(const T &val)
{
    std::ostringstream oss;
    oss << val << std::flush;
    return oss.str();
}


std::atomic_int XgBoostConfig::s_nSvrPort(10080);

bool XgBoostConfig::parseArg(Json::Value &root, std::string &err)
{
    // get "_task_"
    {
        Json::Value &jv = root["_task_"];
        if (!jv) {
            err = "You have to specify arg _task_!";
            return false;
        } else {
            m_strTask = jv.asString();
        } // if
    } // get _task_

    if ("train" == m_strTask) {
        return parseTrainArg(root, err);
    } else if ("online" == m_strTask) {
        return parseOnlineArg(root, err);
    } else if ("offline" == m_strTask) {
        return true;
    } else {
        err = "Wrong task type!";
    } // if

    return false;
}


bool XgBoostConfig::parseTrainArg(Json::Value &root, std::string &err)
{
    m_strCmd = "xgboost "; 
    m_strCmd.append(CONFIG_FILE);

    // if (!root.isMember("model_out")) {
        // err = "You have to specify arg model_out!";
        // return false;
    // } // if
    
    if (!root["model_out"]) {
        err = "You have to specify arg model_out!";
        return false;
    } // if

    for (Json::ValueIterator itr = root.begin() ; itr != root.end() ; ++itr) {
        string key = my_to_string(itr.key());
        string value = my_to_string(*itr);
        boost::trim_if(key, boost::is_any_of("\"" SPACES));
        boost::trim_if(value, boost::is_any_of("\"" SPACES));
        if (key[0] == '_') continue;
        // DLOG(INFO) << key << " = " << value;
        m_strCmd.append(" ").append(key).append("=").append(value);
    } // for

    // m_strCmd.append(" ").append("model_out=").append(MODEL_FILE);
    
    return true;
}


bool XgBoostConfig::parseOnlineArg(Json::Value &root, std::string &err)
{
    string  serviceName, modelFile;

    ostringstream oss;
    // oss << "GLOG_log_dir=\".\" nohup xgboost_svr.bin -model " << MODEL_FILE;
    oss << "GLOG_log_dir=\".\" nohup xgboost_svr.bin";

    // get "model"
    {
        Json::Value &jv = root["model"];
        if (!jv) {
            err = "You have to specify arg \"model\"!";
            return false;
        } else {
            oss << " -model " << jv.asString();
        } // if
    } // get model

    // get "service"
    {
        Json::Value &jv = root["service"];
        if (!jv) {
            err = "You have to specify arg \"service\"!";
            return false;
        } else {
            oss << " -algname " << jv.asString();
        } // if
    } // get service

    oss.flush();
    m_strCmd = oss.str();

    return true;
}


void XgBoostConfig::run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    // DLOG(INFO) << "cmd: " << m_strCmd;
    using namespace std;

    if ("train" == m_strTask) {
        string cmd = "echo \"\" > ";
        cmd.append(CONFIG_FILE);
        pClient->client()->runCmd(cmd);
        pClient->client()->readCmd(resp, m_strCmd);

    } else if ("online" == m_strTask) {
        // get port algmgr
        // kill old
        // sleep
        string strAlgMgr;
        ostringstream oss;
        pClient->client()->getAlgMgr(strAlgMgr);

        string cmd = "killall xgboost_svr.bin";
        pClient->client()->runCmd(cmd);
        SLEEP_SECONDS(5);
        cmd = "killall -9 xgboost_svr.bin";
        pClient->client()->runCmd(cmd);

        if (++s_nSvrPort > 65534) s_nSvrPort = 10080;
        oss << " -port " << s_nSvrPort << " -algmgr " << strAlgMgr << " &" << flush;
        m_strCmd.append(oss.str());
        int retval = pClient->client()->runCmd(m_strCmd);

        Json::Value     jsonResp;
        jsonResp["status"] = retval;
        if (retval) jsonResp["output"] = "Start online server fail!";
        else jsonResp["output"] = "Start online server success.";
        Json::FastWriter writer;  
        resp = writer.write(jsonResp);

    } else if ("offline" == m_strTask) {
        string cmd = "killall xgboost_svr.bin";
        pClient->client()->runCmd(cmd);
        SLEEP_SECONDS(5);
        cmd = "killall -9 xgboost_svr.bin";
        pClient->client()->runCmd(cmd);

        Json::Value     jsonResp;
        jsonResp["status"] = 0;
        jsonResp["output"] = "Stop server done.";
        Json::FastWriter writer;  
        resp = writer.write(jsonResp);
    } // if
}

