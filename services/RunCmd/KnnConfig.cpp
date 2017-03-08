#include <sstream>
#include <json/json.h>
#include <boost/algorithm/string.hpp>
#include "KnnConfig.h"

using namespace std;


std::atomic_int KnnConfig::s_nSvrPort(11080);


struct KnnConfigInst {
    static AlgConfig* newInst()
    { return new KnnConfig; }

    KnnConfigInst()
    { AlgConfigMgr::registerNewInst("knn", KnnConfigInst::newInst); }
};

static KnnConfigInst s_Inst;


bool KnnConfig::parseArg(Json::Value &root, std::string &err)
{
    // get "_task_"
    {
        Json::Value &jv = root["_task_"];
        if (!jv) {
            err = "You have to specify arg _task_!";
            return false;
        } // if
        m_strTask = jv.asString();
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


bool KnnConfig::parseTrainArg(Json::Value &root, std::string &err)
{
    // get method
    {
        Json::Value &jv = root["method"];
        if (!jv) {
            err = "You have to specify method skipgram or cbow";
            return false;
        } // if
        m_strFastTextMethod = jv.asString();
        if (m_strFastTextMethod != "skipgram" && m_strFastTextMethod != "cbow") {
            err = "method must be skipgram or cbow";
            return false;
        } // if
    }

    // get input
    {
        Json::Value &jv = root["input"];
        if (!jv) {
            err = "You have to specify input file";
            return false;
        } // if
        m_strInputFile = jv.asString();
    }

    // get output
    {
        Json::Value &jv = root["output"];
        if (!jv)
            m_strOutputFile = m_strInputFile;
        else
            m_strOutputFile = jv.asString();
    }

    // get nTrees
    {
        Json::Value &jv = root["nTrees"];
        if (!jv) {
            m_nTrees = 10;
        } else {
            m_nTrees = jv.asInt();
            if (m_nTrees <= 0) {
                err = "Invalid nTrees value";
                return false;
            } // if
        } // if
    }

    DLOG(INFO) << "m_strFastTextMethod = " << m_strFastTextMethod;
    DLOG(INFO) << "m_strInputFile = " << m_strInputFile;
    DLOG(INFO) << "m_strOutputFile = " << m_strOutputFile;
    DLOG(INFO) << "m_nTrees = " << m_nTrees;

    return true;
}


bool KnnConfig::parseOnlineArg(Json::Value &root, std::string &err)
{
    // get input
    {
        Json::Value &jv = root["input"];
        if (!jv) {
            err = "You have to specify input file";
            return false;
        } // if
        m_strInputFile = jv.asString();
    }

    // get "service"
    {
        Json::Value &jv = root["service"];
        if (!jv) {
            err = "You have to specify arg service";
            return false;
        } // if
        m_strService = jv.asString();
    } // get service

    return true;
}


void KnnConfig::run(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    // DLOG(INFO) << "cmd: " << m_strCmd;
    using namespace std;

    if ("train" == m_strTask) {
        runTrain(pClient, resp);
    } else if ("online" == m_strTask) {
        runOnline(pClient, resp);
    } else if ("offline" == m_strTask) {
        runOffline(pClient, resp);
    } // if
}


void KnnConfig::runTrain(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    killApp(pClient, "fasttext");    // NOTE!!! 本版一次只能运行一个训练过程

    string output;
    RunCmd::CmdResult result;

    auto response = [&] {
        Json::Value     jsonResp;
        jsonResp["status"] = result.retval;
        jsonResp["output"] = output;
        Json::FastWriter writer;  
        resp = writer.write(jsonResp);
    };

    // fasttext
    {
        ostringstream oss;
        oss << "stdbuf -o0 fasttext " << m_strFastTextMethod
            << " -input " << m_strInputFile << " -output " << m_strOutputFile
            << " -thread 4 2>&1" << flush;
        pClient->client()->readCmd(result, oss.str());
        output.append(result.output);
        if (result.retval) {
            response();
            return;
        } // if
    }

    // build ann idx
    {
        string vecFile = m_strOutputFile + ".vec";
        string wtFile = m_strOutputFile + ".wt";
        string idxFile = m_strOutputFile + ".idx";
        ostringstream oss;
        oss << "GLOG_logtostderr=1 stdbuf -o0 wordknn.bin -build -idata "
            << vecFile << " -ntrees " << m_nTrees << " -idx " << idxFile
            << " -wt " << wtFile << " 2>&1" << flush;
        pClient->client()->readCmd(result, oss.str());
        output.append(result.output);
    }

    response();
}

void KnnConfig::runOnline(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    // get port algmgr
    // kill old
    // sleep
    string strAlgMgr;
    pClient->client()->getAlgMgr(strAlgMgr);

    killApp(pClient, "wordknn.bin");

    string vecFile = m_strInputFile + ".vec";
    string wtFile = m_strInputFile + ".wt";
    string idxFile = m_strInputFile + ".idx";
    if (++s_nSvrPort > 65534) s_nSvrPort = 11080;
    ostringstream oss;
    oss << "GLOG_log_dir=\".\" nohup wordknn.bin -idata " << vecFile
        << " -idx " << idxFile << " -wt " << wtFile << " -algname " << m_strService
        << " -algmgr " << strAlgMgr << " -port " << s_nSvrPort << " &" << flush;
    int retval = pClient->client()->runCmd(oss.str());

    Json::Value     jsonResp;
    jsonResp["status"] = retval;
    if (retval) jsonResp["output"] = "Start online server fail!";
    else jsonResp["output"] = "Start online server success.";
    Json::FastWriter writer;  
    resp = writer.write(jsonResp);
}


void KnnConfig::runOffline(const RunCmdService::RunCmdClientPtr &pClient, std::string &resp)
{
    killApp(pClient, "wordknn.bin");

    Json::Value     jsonResp;
    jsonResp["status"] = 0;
    jsonResp["output"] = "Stop server done.";
    Json::FastWriter writer;  
    resp = writer.write(jsonResp);
}

