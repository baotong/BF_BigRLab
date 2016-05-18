#include "rpc_module.h"
#include "AlgMgrService.h"
#include <boost/thread.hpp>
#include <glog/logging.h>

using namespace BigRLab;
using namespace std;

typedef ThriftClient<AlgMgrServiceClient> MyClient;

static
void test( const MyClient::Pointer &_pClient )
{
    auto pClient = _pClient->client();
    int ret = 0;

    // test
    {
        AlgSvrInfo info;
        info.addr = "192.168.0.5";
        info.port = 9050;
        info.nWorkThread = 100;
        ret = pClient->addSvr("knn", info);
        if (ret)
            cout << "addSvr fail! ret = " << ret << endl;
    }

    // test
    {
        AlgSvrInfo info;
        info.addr = "10.0.0.5";
        info.port = 10050;
        info.nWorkThread = 90;
        ret = pClient->addSvr("knn", info);
        if (ret)
            cout << "addSvr fail! ret = " << ret << endl;
    }

    // test
    {
        AlgSvrInfo info;
        info.addr = "123.123.123.123";
        info.port = 8888;
        info.nWorkThread = 10;
        ret = pClient->addSvr("svm", info);
        if (ret)
            cout << "addSvr fail! ret = " << ret << endl;
    }

    // test
    {
        std::vector<AlgSvrInfo> svrList;
        pClient->getAlgSvrList( svrList, "knn" );
        for (const auto &v : svrList)
            cout << v.addr << ":" << v.port << " " << v.nWorkThread << endl;
    }
    
    // test
    {
        AlgSvrInfo info;
        info.addr = "123.123.123.123";
        info.port = 8888;
        info.nWorkThread = 10;
        pClient->rmSvr("svm", info);
    }
    
    // test
    {
        std::vector<AlgSvrInfo> svrList;
        pClient->getAlgSvrList( svrList, "svm" );
        if (svrList.empty())
            cout << "No server available for service svm" << endl;
    }
}

int main( int argc, char **argv )
{
    try {
        google::InitGoogleLogging(argv[0]);

        auto pClient = boost::make_shared<MyClient>("localhost", 9001);
        pClient->start();
        test(pClient);

    } catch (const std::exception &ex) {
        LOG(ERROR) << "Exception caught by main " << ex.what();
        exit(-1); 
    } // try

    return 0;
}

