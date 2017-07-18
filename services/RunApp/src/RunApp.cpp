#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/format.hpp>
#include <json/json.h>
#include <glog/logging.h>
#include "common.hpp"
#include "RunApp.h"


using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new RunAppService(name); }

const char* lib_name()
{ return "RunCmd"; }


std::string RunAppService::build_resp_json(int statusCode, const std::string &msg)
{
    Json::Value jv;
    build_resp_json(statusCode, msg, jv);
    Json::FastWriter writer;  
    std::string strResp = writer.write(jv);
    return strResp;
}


void RunAppService::build_resp_json(int statusCode, const std::string &msg, Json::Value &jv)
{
    jv["status"] = statusCode;
    jv["msg"] = msg;
}


void RunAppService::handleCommand( std::stringstream &stream )
{ THROW_RUNTIME_ERROR("RunAppService::handleCommand not implemented!"); }


void RunAppService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    using namespace std;

    Json::Reader    reader;
    Json::Value     root;

    DLOG(INFO) << "RunAppService::handleRequest() " << pWork->body;

    if (!reader.parse(pWork->body, root)) {
        LOG(ERROR) << "json parse fail!";
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "json parse fail!"));
        return;
    } // if

    string strOp = root["op"].asString();
    if (strOp.empty()) {
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "\"op\" not specified!"));
        return;
    } // if

    string strName = root["name"].asString();
    if (strName.empty()) {
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "\"name\" not specified!"));
        return;
    } // if

    if (strOp == "new") {
        try {
            createNewWork(strName, root, pWork);
        } catch (const std::exception &ex) {
            string msg("create new work fail ");
            msg.append(ex.what());
            RESPONSE_MSG(pWork->conn, build_resp_json(-1, msg));
            return;
        } // try
    } else if (strOp == "query") {
        doQuery(strName, pWork);
    } else {
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "\"op\" invalid value!"));
        return;
    } // if
}


void RunAppService::createNewWork(const std::string &workName, 
            const Json::Value &conf, const BigRLab::WorkItemPtr &pWork)
{
    string strType = conf["type"].asString();
    if (strType.empty()) {
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "\"type\" not specified!"));
        return;
    } // if

    if (strType == "feature") {
        WorkInfo::pointer pWorkInfo = std::make_shared<FeatureWorkInfo>(workName);
        pWorkInfo->init(conf);
        m_mapWorkTable[pWorkInfo->name()] = pWorkInfo;
        pWorkInfo->run();
        RESPONSE_MSG(pWork->conn, build_resp_json(0, "success"));
    } else {
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "\"type\" invalid value!"));
        return;
    } // if
}


void RunAppService::doQuery(const std::string &workName, const BigRLab::WorkItemPtr &pWork)
{
    LOG(INFO) << "Query " << workName;
    // DLOG(INFO) << "m_mapWorkTable.size() = " << m_mapWorkTable.size();

    auto it = m_mapWorkTable.find(workName);
    if (it == m_mapWorkTable.end()) {
        RESPONSE_MSG(pWork->conn, build_resp_json(-1, "Not found!"));
        return;
    } // if

    auto pWorkInfo = it->second;
    if (pWorkInfo->status() == WorkInfo::RUNNING) {
        RESPONSE_MSG(pWork->conn, build_resp_json(1, "still running"));
    } else if (pWorkInfo->status() == WorkInfo::FINISH) {
        if (pWorkInfo->retcode() == 0) {
            Json::Value jv;
            jv["status"] = 0;
            jv["msg"] = pWorkInfo->output();
            jv["duration"] = Json::Value::UInt64(pWorkInfo->duration());
            Json::FastWriter writer;  
            std::string strResp = writer.write(jv);
            send_response(pWork->conn, BigRLab::ServerType::connection::ok, strResp);
        } else {
            ostringstream oss;
            oss << pWorkInfo->output();
            oss << " retcode = " << pWorkInfo->retcode();
            oss.flush();
            Json::Value jv;
            jv["status"] = 2;
            jv["msg"] = oss.str();
            jv["duration"] = Json::Value::UInt64(pWorkInfo->duration());
            Json::FastWriter writer;  
            std::string strResp = writer.write(jv);
            send_response(pWork->conn, BigRLab::ServerType::connection::ok, strResp);
        } // if
    } // if
}


// 新alg server加入时的处理, 不用改
std::size_t RunAppService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    // 建立与alg server声明的并发数同等数量的连接
    // if (n < 5)
        // n = 5;
    // else if (n > 50)
        // n = 50;

    DLOG(INFO) << "RunAppService::addServer() " << svrInfo.addr << ":" << svrInfo.port
              << " maxConcurrency = " << svrInfo.maxConcurrency
              << ", going to create " << n << " client instances.";

    auto pClient = boost::make_shared<RunCmdClient>(svrInfo.addr, (uint16_t)(svrInfo.port));
    if (pClient->start(50, 300)) {
        m_mapIpClient[svrInfo.addr] = pClient;
        DLOG(INFO) << "New online server: " << svrInfo.addr << ":" << svrInfo.port;
    } else {
        LOG(INFO) << "Fail to create client instance to " << svrInfo.addr << ":" << svrInfo.port;
        return SERVER_UNREACHABLE;
    } // if

    return 1;   // TODO
}

// 方便打印输出，不用改
std::string RunAppService::toString() const
{
    using namespace std;

    stringstream stream;
    stream << "Service " << name() << endl;
    stream << "Online servers:" << endl; 
    stream << left << setw(30) << "IP:Port" << setw(20) << "maxConcurrency" 
            << setw(20) << "nClientInst" << endl; 
    for (const auto &v : m_mapServers) {
        stringstream addrStream;
        addrStream << v.first.addr << ":" << v.first.port << flush;
        stream << left << setw(30) << addrStream.str() << setw(20) 
                << v.first.maxConcurrency << setw(20);
        stream << "1" << endl;
    } // for
    stream.flush();
    return stream.str();
}


