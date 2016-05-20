#include "knn_service.h"
#include <sstream>
#include <glog/logging.h>

using namespace BigRLab;
using namespace std;

Service* create_instance()
{ return new KnnService("knn_star"); }

void KnnService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    cout << "Service knn received request: " << pWork->body << endl;
    // throw InvalidInput("Service knn test exception.");
    send_response(pWork->conn, ServerType::connection::ok, "Service knn running...\n");
}

void KnnService::handleCommand( std::stringstream &stream )
{
    cout << "Service knn got command: ";

    string cmd;
    while (stream >> cmd)
        cout << cmd << " ";
    cout << endl;
}

bool KnnService::init( int argc, char **argv )
{
    int nInstances = 0;    

    for (const auto &v : this->algServerList()) {
        stringstream stream;
        stream << v.addr << ":" << v.port << flush;
        string addr = std::move(stream.str());
        if (v.nWorkThread < 5) {
            nInstances = v.nWorkThread;
        } else {
            nInstances = v.nWorkThread / 10;
            if (nInstances < 5) 
                nInstances = 5;
            else if (nInstances > 10)
                nInstances = 10;
        } // if
        for (int i = 0; i < nInstances; ++i) {
            auto pClient = boost::make_shared<KnnClient>(v.addr, (uint16_t)(v.port));
            try {
                pClient->start();
            } catch (const std::exception &ex) {
                LOG(ERROR) << "KnnService::init() fail to connect with alg server "
                        << addr << ", " << ex.what();
                continue;
            } // try
            m_mapClientTable[addr].push_back(pClient);
            m_arrClientPool.push_back(pClient);
        } // for
    } // for
}
