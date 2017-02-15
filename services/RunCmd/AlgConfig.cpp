#include <sstream>
#include <boost/algorithm/string.hpp>
#include "AlgConfig.h"


template <typename T>
inline std::string my_to_string(const T &val)
{
    std::ostringstream oss;
    oss << val << std::flush;
    return oss.str();
}

bool XgBoostConfig::parseArg(Json::Value &root, std::string &err)
{
    using namespace std;

    for (Json::ValueIterator itr = root.begin() ; itr != root.end() ; ++itr) {
        string key = my_to_string(itr.key());
        string value = my_to_string(*itr);
        boost::trim_if(key, boost::is_any_of("\"" SPACES));
        boost::trim_if(value, boost::is_any_of("\"" SPACES));
        if (key[0] == '_') continue;
        // DLOG(INFO) << key << " = " << value;
        m_strCmd.append(" ").append(key).append("=").append(value);
    } // for
    
    return true;
}


void XgBoostConfig::run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    // DLOG(INFO) << "cmd: " << m_strCmd;
    using namespace std;

    string cmd = "echo \"\" > ";
    cmd.append(CONFIG_FILE);
    pClient->client()->runCmd(resp, cmd);

    pClient->client()->runCmd(resp, m_strCmd);
}

