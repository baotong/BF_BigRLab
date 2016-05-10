#include "api_server.h"
#include <glog/logging.h>


namespace BigRLab {

std::unique_ptr<WorkQueue>   g_pWorkQueue;


APIServer::APIServer( const ServerType::options &_Opts,
        const IoServicePtr &_pIoSrv, 
        const ThreadGroupPtr &_pThrgrp ) 
      : m_pServer(NULL)
      , m_bRunning(false)
      , m_nPort( DEFAULT_PORT )
      , m_nIoThreads( DEFAULT_N_IO_THREADS )
      , m_nWorkThreads( DEFAULT_N_WORK_THREADS )
      , m_Options(std::move(_Opts))
      , m_pIoService(_pIoSrv)
      , m_pIoThrgrp(_pThrgrp)
{ 
    options().address("0.0.0.0").reuse_address(true)
        .io_service(m_pIoService); 
}

APIServer::APIServer( const ServerType::options &_Opts,
                      const IoServicePtr &_pIoSrv, 
                      const ThreadGroupPtr &_pThrgrp, 
                      const char *confFileName ) 
                : APIServer(_Opts, _pIoSrv, _pThrgrp)
{
    using namespace std;

    parse_config_file( confFileName, m_mapProperties );

    for (auto &v : m_mapProperties) {
        if ("port" == v.first) {
            if (!(read_from_string(*(v.second.begin()), m_nPort))) {
                cout << "APIServer Invalid value " << *(v.second.begin()) 
                        << " for port number, set to default." << endl;
                m_nPort = DEFAULT_PORT;
            } // if
        } else if ("n_io_threads" == v.first) {
            if (!(read_from_string(*(v.second.begin()), m_nIoThreads))) {
                cout << "APIServer Invalid value " << *(v.second.begin()) 
                        << " for n_io_threads, set to default." << endl;
                m_nIoThreads = DEFAULT_N_IO_THREADS;
            } // if
        } else if ("n_work_threads" == v.first) {
            if (!(read_from_string(*(v.second.begin()), m_nWorkThreads))) {
                cout << "APIServer Invalid value " << *(v.second.begin()) 
                        << " for n_work_threads, set to default." << endl;
                m_nWorkThreads = DEFAULT_N_WORK_THREADS;
            } // if
        } // if
    } // for

    options().port(to_string(m_nPort))
        .thread_pool(std::make_shared<ThreadPool>(m_nIoThreads, m_pIoService, m_pIoThrgrp));
}

void APIServer::run()
{
    using namespace std;

    if ( m_bRunning ) {
        cerr << "APIServer already running!" << endl;
        return;
    } // if

    m_bRunning = true;

    m_pServer = new ServerType(m_Options);
    m_pServer->run();
}

void APIServer::stop()
{
    m_bRunning = false;
    m_pServer->stop();   // 是异步的 async_stop
    // 不能在这里join这些 io thread 需要io_service结束后
    // m_pIoThrgrp->join_all();
}

void APIServerHandler::operator()(const ServerType::request& req,
        ServerType::connection_ptr conn) 
{
    using namespace std;

    cout << "Received client request from " << req.source << endl; // source 已经包含port
    cout << "destination = " << req.destination << endl;  // 去除了URL port
    cout << "method = " << req.method << endl;
    cout << "http version = " << (uint32_t)(req.http_version_major) 
        << "." << (uint32_t)(req.http_version_minor) << endl;
    cout << "headers:" << endl;
    for (const auto &header : req.headers)
        cout << header.name << " = " << header.value << endl;

    SLEEP_SECONDS(1);

    stringstream os;
    os << "Hello, from server thread " << THIS_THREAD_ID << endl << flush;
    conn->write(os.str());
}


} // namespace BigRLab

