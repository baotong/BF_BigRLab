/*
 * c++ -o /tmp/test test.cpp -lglog -lcurl -std=c++11 -pthread -g
 */
#include "service_cli.h"
#include <iostream>

using namespace std;

namespace Test {

void test1( const std::string &item )
{
    ServiceCli::pointer pSrv = std::make_shared<ServiceCli>("http://localhost:9000/knn_star");
    string reqStr = "{\"items\":\"";
    reqStr.append(item).append("\",\"n\":10}");
    int ret = pSrv->doRequest(reqStr);

    if (ret)
        cerr << "Request error: " << pSrv->errmsg() << endl;
    else
        cout << pSrv->respString() << endl;
}

} // namespace Test


int main( int argc, char **argv )
{
    auto pGlobalCleanup = ServiceCli::globalInit();

    google::InitGoogleLogging(argv[0]);

    try {
        Test::test1("李宇春");
        Test::test1("姚明");
        Test::test1("章子怡");
        return 0;

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

