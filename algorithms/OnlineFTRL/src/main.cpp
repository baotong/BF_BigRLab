#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <chrono>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <boost/filesystem.hpp>
#include "rpc_module.h"
#include "common.hpp"
#include "alg_common.hpp"
#include "get_local_ip.hpp"
#include "register_svr.hpp"
#include "AlgMgrService.h"
#include "fast_ftrl_solver.h"
#include "ftrl_train.h"
#include "util.h"

#define SERVICE_LIB_NAME        "ftrl"

#define TIMER_REJOIN            15          // 15s

DEFINE_int32(epoch, 1, "Number of iteration, default 1");
DEFINE_string(model, "", "model file");
DEFINE_int64(update_cnt, 100, "Number of data record for updating model");
DEFINE_string(algname, "", "Name of this algorithm");
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(addr, "", "Address of this algorithm server, use system detected if not specified.");
DEFINE_int32(port, 0, "listen port of ths algorithm server.");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 4, "Number of io threads on RPC server");

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

// typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;
// typedef BigRLab::ThriftServer< Article::ArticleServiceIf, Article::ArticleServiceProcessor > ArticleAlgServer;
// static AlgMgrClient::Pointer                  g_pAlgMgrClient;
// static ArticleAlgServer::Pointer              g_pThisServer;
// static boost::asio::io_service                g_io_service;
// static boost::shared_ptr<BigRLab::AlgSvrInfo> g_pSvrInfo;


namespace {
using namespace std;
static inline
bool check_above_zero(const char* flagname, gflags::int32 value)
{
    if (value <= 0) {
        cerr << "value of " << flagname << " must be greater than 0" << endl;
        return false;
    } // if
    return true;
}
static inline
bool check_not_empty(const char* flagname, const std::string &value)
{
    if (value.empty()) {
        cerr << "value of " << flagname << " cannot be empty" << endl;
        return false;
    } // if
    return true;
}

static bool validate_algname(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool algname_dummy = gflags::RegisterFlagValidator(&FLAGS_algname, &validate_algname);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    using namespace std;

    if (!check_not_empty(flagname, value))
        return false;

    string::size_type pos = value.find_last_of(':');
    if (string::npos == pos) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    g_strAlgMgrAddr = value.substr(0, pos);
    if (g_strAlgMgrAddr.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    string strPort = value.substr(pos + 1, string::npos);
    if (strPort.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!boost::conversion::try_lexical_convert(strPort, g_nAlgMgrPort)) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!g_nAlgMgrPort) {
        cerr << "Invalid port number specified by arg " << flagname << endl;
        return false;
    } // if

    return true;
}
static const bool algmgr_dummy = gflags::RegisterFlagValidator(&FLAGS_algmgr, &validate_algmgr);

static
bool validate_port(const char *flagname, gflags::int32 value)
{
    if (value < 1024 || value > 65535) {
        cerr << "Invalid port number! port number must be in [1025, 65535]" << endl;
        return false;
    } // if
    g_nThisPort = (uint16_t)FLAGS_port;
    return true;
}
static const bool port_dummy = gflags::RegisterFlagValidator(&FLAGS_port, &validate_port);

static 
bool validate_n_work_threads(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);

static bool validate_model(const char* flagname, const std::string &value)
{ return check_not_empty(flagname, value); }
static const bool model_dummy = gflags::RegisterFlagValidator(&FLAGS_model, &validate_model);

static bool validate_epoch(const char* flagname, const std::string &value)
{ return check_above_zero(flagname, value); }
static const bool epoch_dummy = gflags::RegisterFlagValidator(&FLAGS_epoch, &validate_epoch);

static bool validate_update_cnt(const char* flagname, const std::string &value)
{ return check_above_zero(flagname, value); }
static const bool update_cnt_dummy = gflags::RegisterFlagValidator(&FLAGS_update_cnt, &validate_update_cnt);
} // namespace


int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {

        do_service_routine();

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        return -1;
    } // try

    return 0;
}
