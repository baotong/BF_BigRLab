#include <sys/types.h>
#include <sys/wait.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <glog/logging.h>
#include <json/json.h>
#include "common.hpp"
#include "alg_common.hpp"
#include "RunCmdServiceHandler.h"


namespace RunCmd {

using namespace std;


void RunCmdServiceHandler::getAlgMgr(std::string& _return)
{
    ostringstream oss;
    oss << g_strAlgMgrAddr << ":" << g_nAlgMgrPort << flush;
    _return = oss.str();
}


int32_t RunCmdServiceHandler::runCmd(const std::string& cmd)
{
    DLOG(INFO) << "Received cmd: " << cmd;
    return system(cmd.c_str());
}


void RunCmdServiceHandler::readCmd(std::string& _return, const std::string& cmd)
{
    DLOG(INFO) << "Received cmd: " << cmd;

    Json::Value     resp;
    int         retcode;
    string      output;

    retcode = doRunCmd(cmd, output);

    resp["status"] = retcode;
    resp["output"] = output;

    Json::FastWriter writer;  
    _return = writer.write(resp);
}


int RunCmdServiceHandler::doRunCmd(const std::string &cmd, std::string &output)
{
    int retval = 0;

    string cmdStr = "stdbuf -o0 ";
    cmdStr.append(cmd).append(" 2>&1");

    FILE *fp = popen(cmdStr.c_str(), "r");
    setlinebuf(fp);

    typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source >
                    FDRdStream;
    FDRdStream ppStream( fileno(fp), boost::iostreams::never_close_handle );

    stringstream ss;
    ss << ppStream.rdbuf();

    output = ss.str();

    retval = pclose(fp);
    retval = WEXITSTATUS(retval);

    return retval;
}


} // namespace RunCmd
