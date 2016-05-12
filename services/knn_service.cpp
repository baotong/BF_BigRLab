#include "knn_service.h"

using namespace BigRLab;
using namespace std;

Service* create_instance()
{ return new KnnService; }

void KnnService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    cout << "Service knn received request: " << pWork->body << endl;
    pWork->conn->write("Service knn running...\n");
}

void KnnService::handleCommand( std::stringstream &stream )
{
    cout << "Service knn got command: ";

    string cmd;
    while (stream >> cmd)
        cout << cmd << " ";
    cout << endl;
}


