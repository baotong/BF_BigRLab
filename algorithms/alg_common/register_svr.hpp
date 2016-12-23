#ifndef _REGISTER_SVR_HPP_
#define _REGISTER_SVR_HPP_

#include <atomic>
#include <csignal>
#include "AlgMgrService.h"

typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient >                AlgMgrClient;

static std::atomic_bool     g_bLoginSuccess(false);

static
void register_svr( AlgMgrClient *pClient, const std::string &algname, BigRLab::AlgSvrInfo *pSvrInfo )
{
    using namespace std;

    try {
        SLEEP_MILLISECONDS(500);   // let thrift server start first
        if (!pClient->start(50, 300)) {
            cerr << "AlgMgr server unreachable!" << endl;
            std::raise(SIGTERM);
            return;
        } // if
        (*pClient)()->rmSvr(algname, *pSvrInfo);
        int ret = (*pClient)()->addSvr(algname, *pSvrInfo);
        if (ret != BigRLab::SUCCESS) {
            cerr << "Register alg server fail!";
            switch (ret) {
                case BigRLab::ALREADY_EXIST:
                    cerr << " server with same addr:port already exists!";
                    break;
                case BigRLab::SERVER_UNREACHABLE:
                    cerr << " this server is unreachable by algmgr! check the server address setting.";
                    break;
                case BigRLab::NO_SERVICE:
                    cerr << " service lib has not benn loaded on apiserver!";
                    break;
                case BigRLab::INTERNAL_FAIL:
                    cerr << " fail due to internal error!";
                    break;
            } // switch
            cerr << endl;
            std::raise(SIGTERM);
            return;
        } // if

        g_bLoginSuccess = true;

    } catch (const std::exception &ex) {
        cerr << "Unable to connect to algmgr server, " << ex.what() << endl;
        std::raise(SIGTERM);
    } // try
}



#endif

