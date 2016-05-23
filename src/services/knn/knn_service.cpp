#include "knn_service.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <glog/logging.h>

using namespace BigRLab;
using namespace std;

Service* create_instance()
{ return new KnnService("knn_star"); }

void KnnService::QueryWork::run()
{
    LOG(INFO) << "Service " << srvName << " querying \"" << iter->first << "\"";

    auto pClient = idleClients->getIdleClient();
    if (!pClient) {
        LOG(ERROR) << "Service " << srvName << " query \"" 
                << iter->first << "\" fail! no client obj available.";
        return;
    } // if

    try {
        pClient->client()->queryByItem( iter->second, iter->first, k );
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Service " << srvName << " query \"" 
                << iter->first << "\" fail! " << ex.what();
    } // try

    idleClients->putBack( pClient );
    ++*counter;
    condCounter->notify_all();
}

/**
 * @brief 
 * service knn items k item1 item2 ... itemn
 * service knn file k input output
 */
void KnnService::handleCommand( std::stringstream &stream )
{
    using namespace std;

    string cmd;
    stream >> cmd;

    if (cmd.empty())
        ERR_RET("Service " << name() << ": command cannot be empty!");

    if ("items" == cmd) {
        int k;
        stream >> k;
        if (bad_stream(stream))
            ERR_RET("Service " << name() << ": read k value fail!");

        QuerySet querySet;
        string item;
        atomic_size_t counter;
        boost::mutex                  mtx;
        boost::condition_variable     cond;
        counter = 0;
        while (stream >> item) {
            auto ret = querySet.insert( std::make_pair(item, QuerySet::mapped_type()) );
            WorkItemBasePtr pQueryWork = boost::make_shared<QueryWork>( ret.first, k, &counter, 
                    &cond, &m_queIdleClients, this->name().c_str() );
            WorkManager::getInstance()->addWork( pQueryWork );
        } // while

        if (querySet.empty())
            ERR_RET("Service " << name() << ": item list cannot be empty!");

        boost::unique_lock<boost::mutex> lock(mtx);
        if (!cond.wait_for( lock, boost::chrono::milliseconds(2 * TIMEOUT), 
                    [&]()->bool {return counter >= querySet.size();} )) {
            cout << "Wait timeout, " << " result may be incomplete." << endl;
        } // if

        // print the result
        for (const auto &v : querySet) {
            cout << "Query results for item \"" << v.first << "\":" << endl;
            for (const auto &sub : v.second)
                cout << sub.item << "\t\t" << sub.weight << endl;
        } // for
        cout << endl;
    } // if


    // cout << "Service knn got command: ";

    // string cmd;
    // while (stream >> cmd)
        // cout << cmd << " ";
    // cout << endl;
}

void KnnService::handleRequest(const BigRLab::WorkItemPtr &pWork)
{
    cout << "Service knn received request: " << pWork->body << endl;
    // throw InvalidInput("Service knn test exception.");
    send_response(pWork->conn, ServerType::connection::ok, "Service knn running...\n");
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
            m_queIdleClients.push_back(pClient);
        } // for
    } // for

    // shuffle
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_queIdleClients.begin(), m_queIdleClients.end(), g);

    LOG(INFO) << "Totally " << m_queIdleClients.size() << " client instances for service " << this->name();

    return true;
}

