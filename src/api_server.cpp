#include "api_server.h"
#include <glog/logging.h>


namespace BigRLab {

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
    std::cout << "APIServer default constructor" << std::endl;
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

} // namespace BigRLab

