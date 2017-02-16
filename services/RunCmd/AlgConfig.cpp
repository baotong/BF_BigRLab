#include <sstream>
#include <boost/algorithm/string.hpp>
#include "AlgConfig.h"


using namespace std;

template <typename T>
inline std::string my_to_string(const T &val)
{
    std::ostringstream oss;
    oss << val << std::flush;
    return oss.str();
}

bool XgBoostConfig::parseArg(Json::Value &root, std::string &err)
{
    try {
        m_strTask = root["_task_"].asString(); 
    } catch (...) {
        err = "\"_task_\" not specified!";
        return false;
    } // try

    if ("train" == m_strTask) {
        return parseTrainArg(root, err);
    } else if ("deploy" == m_strTask) {
        return parseDeployArg(root, err);
    } else {
        err = "Wrong task type!";
    } // if

    return false;
}


bool XgBoostConfig::parseTrainArg(Json::Value &root, std::string &err)
{
    m_strCmd = "xgboost "; 
    m_strCmd.append(CONFIG_FILE);

    for (Json::ValueIterator itr = root.begin() ; itr != root.end() ; ++itr) {
        string key = my_to_string(itr.key());
        string value = my_to_string(*itr);
        boost::trim_if(key, boost::is_any_of("\"" SPACES));
        boost::trim_if(value, boost::is_any_of("\"" SPACES));
        if (key[0] == '_') continue;
        // DLOG(INFO) << key << " = " << value;
        m_strCmd.append(" ").append(key).append("=").append(value);
    } // for

    m_strCmd.append(" ").append("model_out=").append(MODEL_FILE);
    
    return true;
}


bool XgBoostConfig::parseDeployArg(Json::Value &root, std::string &err)
{
    m_strCmd = "xgboost_svr "; 
    return false;
}


void XgBoostConfig::run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    // DLOG(INFO) << "cmd: " << m_strCmd;
    using namespace std;

    if ("train" == m_strTask) {
        string cmd = "echo \"\" > ";
        cmd.append(CONFIG_FILE);
        pClient->client()->runCmd(resp, cmd);
        pClient->client()->runCmd(resp, m_strCmd);
    } else if ("deploy" == m_strTask) {
        ;
    } // if
}

