#include "knn_service.h"

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


