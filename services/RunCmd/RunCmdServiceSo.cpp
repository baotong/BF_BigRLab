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
#include "AlgConfig.h"

using namespace BigRLab;
using namespace std;

Service* create_instance(const char *name)
{ return new RunCmdService(name); }

const char* lib_name()
{ return "RunCmd"; }


void RunCmdService::handleCommand( std::stringstream &stream )
{ THROW_RUNTIME_ERROR("RunCmdService::handleCommand not implemented!"); }


void RunCmdService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    using namespace std;

    Json::Reader    reader;
    Json::Value     root;

    string      ip;
    string      alg, resp;

    if (!reader.parse(pWork->body, root)) {
        LOG(ERROR) << "json parse fail!";
        RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, -1, 
                name() << ": json parse fail!");
        return;
    } // if

    try {

        // for (Json::ValueIterator itr = root.begin() ; itr != root.end() ; ++itr)
            // DLOG(INFO) << itr.key() << " = " << *itr;

        // get "_ip_"
        {
            Json::Value &jv = root["_ip_"];
            if (!jv) {
                RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, -1, 
                        name() << ": No ip info found in json!");
                return;
            } else {
                ip = jv.asString();
            } // if
        } // get _ip_

        // get "_alg_"
        {
            Json::Value &jv = root["_alg_"];
            if (!jv) {
                RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, -1, 
                        name() << ": No alg info found in json!");
                return;
            } else {
                alg = jv.asString();
            } // if
        } // get _alg_

        auto it = m_mapIpClient.find(ip);
        if (it == m_mapIpClient.end()) {
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, -1, 
                    name() << ": No server found with ip " << ip);
            return;
        } // if

        auto &pClient = it->second;

        AlgConfig::pointer pAlgConfig = AlgConfigMgr::newInst(alg);
        if (!pAlgConfig) {
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, -1, 
                    name() << ": Cannot found algconfig whose name is " << alg);
            return;
        } // if

        string parseErr;
        if (!pAlgConfig->parseArg(root, parseErr)) {
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, -1, 
                    name() << " parse arg error: " << parseErr);
            return;
        } // if

        try {
            pAlgConfig->run(pClient, resp);
        } catch (const AlgCommon::InvalidRequest &err) {
            RESPONSE_ERROR(pWork->conn, BigRLab::ServerType::connection::ok, INVALID_REQUEST, 
                    name() << " InvalidRequest: " << err.reason);
            return;
        } // try

        send_response(pWork->conn, BigRLab::ServerType::connection::ok, resp);

    } catch (const std::exception &ex) {
        LOG(ERROR) << "System exception: " << ex.what() << "\nRemoving server: " << ip;
        m_mapIpClient.erase(ip);
    } // try
}


// 新alg server加入时的处理, 不用改
std::size_t RunCmdService::addServer( const BigRLab::AlgSvrInfo& svrInfo, const ServerAttr::Pointer& )
{
    // 要根据实际需求确定连接数量
    int n = svrInfo.maxConcurrency;
    // 建立与alg server声明的并发数同等数量的连接
    // if (n < 5)
        // n = 5;
    // else if (n > 50)
        // n = 50;

    DLOG(INFO) << "RunCmdService::addServer() " << svrInfo.addr << ":" << svrInfo.port
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
std::string RunCmdService::toString() const
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


