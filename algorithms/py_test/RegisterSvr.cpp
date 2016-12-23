#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iterator>
#include <memory>
#include <thread>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include "common.hpp"
#include "rpc_module.h"
#include "alg_common.hpp"
#include "register_svr.hpp"


int main(int argc, char **argv)
try {
    using namespace std;

    // for (int i = 1; i < argc; ++i)
        // cout << argv[i] << " ";
    // cout << endl;

    string          algmgrIp = argv[1];
    uint16_t        algmgrPort = boost::lexical_cast<uint16_t>(argv[2]);

    BigRLab::AlgSvrInfo     algSvrInfo;
    algSvrInfo.addr = argv[3];
    algSvrInfo.port = (int16_t)boost::lexical_cast<uint16_t>(argv[4]);
    algSvrInfo.serviceName = argv[5];
    string          serviceName = argv[6];
    algSvrInfo.maxConcurrency = boost::lexical_cast<int>(argv[7]);

    AlgMgrClient    cli(algmgrIp, algmgrPort);
    cli.start(50, 300);
    cli.client()->rmSvr(serviceName, algSvrInfo);
    cli.client()->addSvr(serviceName, algSvrInfo);

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught by main: " << ex.what() << std::endl;
    return -1;
}
