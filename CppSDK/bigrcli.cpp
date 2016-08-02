/*
 * compile:
 * c++ -o bigrcli bigrcli.cpp -lgflags -lcurl -std=c++11 -O3 -g
 * usage:
 * ./bigrcli lslib
 * ./bigrcli service knn_star items 10 李宇春 姚明 章子怡
 * ./bigrcli -server http://localhost:9000 service knn_star items 10 李宇春 姚明 章子怡
 */
#include "service_cli.h"
#include <iostream>
#include <string>
#include <gflags/gflags.h>
#include <boost/algorithm/string.hpp>

// #define MAX_TIMEOUT         2592000   // 1 month

#define RETVAL(val, args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return val; \
    } while (0)

DEFINE_int32(timeout, 0, "Timeout in seconds for waiting response.");
DEFINE_string(server, "http://localhost:9000", "APIServer addr in format \"http://ip:port\"");

using namespace std;
using namespace BigRLab;

namespace {
static bool validate_timeout(const char* flagname, gflags::int32 value) 
{ 
    if (FLAGS_timeout < 0)
        RETVAL(false, "timeout arg cannot below zero");
    return true;
}
static const bool timeout_dummy = gflags::RegisterFlagValidator(&FLAGS_timeout, &validate_timeout);
} // namespace


int main(int argc, char **argv)
try {
    int idx = gflags::ParseCommandLineFlags(&argc, &argv, true);
    // cout << "FLAGS_timeout = " << FLAGS_timeout << endl;
    // for (; idx < argc; ++idx)
        // cout << argv[idx] << " ";
    // cout << endl;
    // return 0;

    string cmd, resp;

    if (idx >= argc) {
        cmd = "hello";
    } else {
        for (; idx < argc; ++idx)
            cmd.append(argv[idx]).append(" ");
        cmd.erase(cmd.size()-1);
    } // if

    auto pGlobalCleanup = ServiceCli::globalInit();
    auto pSrv = std::make_shared<ServiceCli>(FLAGS_server);

    if (FLAGS_timeout)
        pSrv->setTimeout( FLAGS_timeout );

    pSrv->setHeader( "Content-Type: BigRLab_Command" );

    int res = pSrv->doRequest( cmd );
    if (res) {
        string &response = pSrv->errmsg();
        boost::trim(response);
        cout << response << endl;
    } else {
        string &response = pSrv->respString();
        boost::trim(response);
        cout << response << endl;
    } // if

    return 0;

} catch (const std::exception &ex) {
    cerr << "Exception caught by main: " << ex.what() << endl;
    exit(-1);
}
