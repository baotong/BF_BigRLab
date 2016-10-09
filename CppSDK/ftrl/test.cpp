/*
 * c++ -o test test.cpp -lcurl -ljsoncpp -lglog -lgflags -std=c++11 -pthread -g
 * cat agaricus.txt.test | ./test -server http://localhost:9000/ftrl -req predict
 * cat agaricus.txt.test | ./test -server http://localhost:9000/ftrl -req update
 */
#include "../service_cli.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iterator>
#include <memory>
#include <algorithm>
#include <glog/logging.h>
#include <json/json.h>
#include <boost/algorithm/string.hpp>

#define THIS_THREAD_ID        std::this_thread::get_id()
#define SLEEP_SECONDS(x)      std::this_thread::sleep_for(std::chrono::seconds(x))
#define SLEEP_MILLISECONDS(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))

DEFINE_string(server, "", "APIServer addr in format \"http://ip:port\"");
DEFINE_string(req, "predict", "predict (default) or update");

using namespace std;
using namespace BigRLab;


void test_predict(istream *inFile)
{
    auto pSrv = std::make_shared<ServiceCli>(FLAGS_server);
    pSrv->setHeader( "Content-Type: BigRLab_Request" );

    auto genReqStr = [](size_t id, const string &data, string &out) {
        out.clear();
        Json::Value     reqJson;
        reqJson["req"] = "predict";
        reqJson["id"] = (Json::UInt64)id;
        reqJson["data"] = data;
        Json::FastWriter writer;  
        out = writer.write(reqJson);
    };

    size_t id = 0;
    string line, data, value, reqstr;
    int ret = 0;
    while (getline(*inFile, line)) {
        stringstream stream(line);
        stream >> value;
        getline(stream, data);
        boost::trim(data);
        // DLOG(INFO) << "value = " << value << " data = " << data;
        genReqStr(id, data, reqstr);
        // DLOG(INFO) << "reqstr = " << reqstr;
        ret = pSrv->doRequest(reqstr);
        if (ret)
            cerr << "Request error: " << pSrv->errmsg() << endl;
        else
            cout << boost::trim_copy(pSrv->respString()) << endl;
        ++id;
    } // while
}

void test_update(istream *inFile)
{
    auto pSrv = std::make_shared<ServiceCli>(FLAGS_server);
    pSrv->setHeader( "Content-Type: BigRLab_Request" );

    auto genReqStr = [](size_t id, const string &value, string &out) {
        out.clear();
        Json::Value     reqJson;
        reqJson["req"] = "update";
        reqJson["id"] = (Json::UInt64)id;
        reqJson["data"] = value;
        Json::FastWriter writer;  
        out = writer.write(reqJson);
    };

    size_t id = 0;
    string line, value, reqstr;
    int ret = 0;
    while (getline(*inFile, line)) {
        stringstream stream(line);
        stream >> value;
        // DLOG(INFO) << "value = " << value << " data = " << data;
        genReqStr(id, value, reqstr);
        // DLOG(INFO) << "reqstr = " << reqstr;
        ret = pSrv->doRequest(reqstr);
        if (ret)
            cerr << "Request error: " << pSrv->errmsg() << endl;
        else
            cout << boost::trim_copy(pSrv->respString()) << endl;
        ++id;
    } // while
}


int main( int argc, char **argv )
{
    try {
        google::InitGoogleLogging(argv[0]);
        int idx = gflags::ParseCommandLineFlags(&argc, &argv, true);

        if (FLAGS_server.empty())
            THROW_RUNTIME_ERROR("-server not set!");

        std::shared_ptr<std::istream> inFile;
        if (idx >= argc) {
            inFile.reset(&cin, [](istream*){});
        } else {
            inFile.reset(new ifstream(argv[idx], ios::in));
            if (!(*inFile))
                THROW_RUNTIME_ERROR("Cannot open " << argv[idx] << " for reading!");
        } // if

        auto pGlobalCleanup = ServiceCli::globalInit();

        if (FLAGS_req == "predict")
            test_predict(inFile.get());
        else if (FLAGS_req == "update")
            test_update(inFile.get());
        else
            THROW_RUNTIME_ERROR("-req invalid!");

        return 0;

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

