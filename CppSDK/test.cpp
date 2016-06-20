/*
 * c++ -o /tmp/test test.cpp -lglog -lcurl -ljsoncpp -std=c++11 -pthread -g
 */
#include "service_cli.h"
#include "word_knn_cli.h"
#include <iostream>
#include <iterator>
#include <algorithm>
#include <glog/logging.h>

using namespace std;
using namespace BigRLab;

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

void test2( const std::string &item )
{
    auto pSrv = std::make_shared<WordKnnCli>("http://localhost:9000/knn_star");

    vector<string> result;
    int ret = pSrv->queryItem(item, 10, result);

    if (ret)
        cerr << "Request error: " << pSrv->errmsg() << endl;
    else
        copy(result.begin(), result.end(), ostream_iterator<string>(cout, " "));

    cout << endl;
}

// queryVector
/*
 * sed -n '28068p' vec3.txt > /tmp/in.txt
 * cat /tmp/in.txt | /tmp/test
 */
void test3()
{
    auto pSrv = std::make_shared<WordKnnCli>("http://localhost:9000/knn_star");

    string line, name;
    getline(cin, line);
    stringstream stream(line);
    stream >> name;

    vector<double> vec;
    copy( istream_iterator<double>(stream), istream_iterator<double>(), back_inserter(vec) );

    vector<string> result;
    int ret = pSrv->queryVector(vec, 10, result);

    if (ret)
        cerr << "Request error: " << pSrv->errmsg() << endl;
    else
        copy(result.begin(), result.end(), ostream_iterator<string>(cout, " "));

    cout << endl;
}

} // namespace Test


int main( int argc, char **argv )
{
    auto pGlobalCleanup = ServiceCli::globalInit();

    google::InitGoogleLogging(argv[0]);

    try {
        // Test::test1("伟大光荣正确的Charles");
        // Test::test1("李宇春");
        // Test::test1("姚明");
        // Test::test1("章子怡");
        // Test::test2("林志玲");
        Test::test3();
        return 0;

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

