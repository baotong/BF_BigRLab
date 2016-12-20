/*
 * c++ -o test stress_test.cpp -lcurl -ljsoncpp -lglog -lgflags -lboost_thread -lboost_system -std=c++11 -pthread -O3 -Wall -g
 * cat agaricus.txt.test | ./test -server http://localhost:9000/ftrl -thread 16
 */
#include "../service_cli.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <iterator>
#include <memory>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <glog/logging.h>
#include <json/json.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>


DEFINE_string(server, "", "APIServer addr in format \"http://ip:port\"");
DEFINE_int32(thread, 0, "Number of thread.");

static std::vector<std::string>     g_arrData;
static std::atomic_size_t           g_nID(0);
static std::atomic_size_t           g_nFinCnt(0);
static std::size_t                  g_nLastFinCnt = 0;
static std::atomic_bool             g_bRunning(false);
static std::unique_ptr< boost::asio::deadline_timer >      g_Timer;

using namespace std;
using namespace BigRLab;

static
void thread_routine()
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

    string reqstr;
    int ret = 0;
    while (g_bRunning) {
        size_t id = g_nID++;
        string& data = g_arrData[id % g_arrData.size()];
        genReqStr(id, data, reqstr);

        auto tp1 = std::chrono::high_resolution_clock::now();
        ret = pSrv->doRequest(reqstr);
        auto tp2 = std::chrono::high_resolution_clock::now();
        ++g_nFinCnt;

        if (ret) {
            LOG(ERROR) << "Request error: " << pSrv->errmsg();
        } else {
            LOG(INFO) << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count();
        } // if
    } // while
}


static
void on_timer(const boost::system::error_code &ec)
{
    size_t finCnt = g_nFinCnt;
    LOG(WARNING) << (finCnt - g_nLastFinCnt) << " tests done in last second.";
    g_nLastFinCnt = finCnt;

    if (g_bRunning) {
        g_Timer->expires_from_now(boost::posix_time::seconds(1));
        g_Timer->async_wait(on_timer);
    } // if
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

        if (FLAGS_thread <= 0)
            FLAGS_thread = (int)std::thread::hardware_concurrency();

        // load dataset
        {
            string line, data, value;
            while (getline(*inFile, line)) {
                stringstream stream(line);
                stream >> value;
                getline(stream, data);
                boost::trim(data);
                g_arrData.push_back(data);
            } // while
        }
        // DLOG(INFO) << g_arrData.size();

        auto pGlobalCleanup = ServiceCli::globalInit();

        boost::asio::io_service                io_service;
        auto pIoServiceWork = std::make_shared< boost::asio::io_service::work >(std::ref(io_service));
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
        signals.async_wait( [&](const boost::system::error_code& error, int signal) { 
            g_bRunning = false;
            if (g_Timer) g_Timer->cancel();
            pIoServiceWork.reset();
            io_service.stop();
        } );

        g_Timer.reset(new boost::asio::deadline_timer(std::ref(io_service)));
        g_Timer->expires_from_now(boost::posix_time::seconds(1));
        g_Timer->async_wait(on_timer);

        // create thread work
        boost::thread_group         thrgroup;
        g_bRunning = true;
        for (int i = 0; i < FLAGS_thread; ++i)
            thrgroup.create_thread(thread_routine);

        io_service.run();

        cout << "Terminating..." << endl;

        thrgroup.join_all();

        cout << "Totally handled " << g_nFinCnt << " tests" << endl;

        return 0;

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

