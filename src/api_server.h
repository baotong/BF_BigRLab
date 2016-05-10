#ifndef _API_SERVER_H_
#define _API_SERVER_H_

#include "service_manager.h"
#include <boost/network/utils/thread_group.hpp>
#include <boost/network/include/http/server.hpp>
#include <boost/network/uri.hpp>
#include <asio.hpp>

namespace BigRLab {

using ThreadPool = boost::network::utils::thread_pool;
using ThreadGroup = boost::network::utils::thread_group;
typedef std::shared_ptr<ThreadGroup>            ThreadGroupPtr;
typedef std::shared_ptr<asio::io_service>       IoServicePtr;
typedef std::shared_ptr<asio::io_service::work> IoServiceWorkPtr;

struct APIServerHandler;
typedef boost::network::http::server<APIServerHandler> ServerType;

struct APIServerHandler {
    void operator()(const ServerType::request& req,
                    ServerType::connection_ptr conn); 
};

// 正确的结束方式: gracefully cleanly terminate
/*
 * 1. server->stop();
 * 2. io_service->stop();       g_pWork.reset(); g_pIoService->stop();
 * 3. threadgroup->join_all()
 */
class APIServer {
public:
    static const uint32_t       DEFAULT_N_IO_THREADS = 5;
    static const uint32_t       DEFAULT_N_WORK_THREADS = 100;
    static const uint16_t       DEFAULT_PORT = 9000;
public:
    explicit APIServer( const ServerType::options &_Opts,
                        const IoServicePtr &_pIoSrv, 
                        const ThreadGroupPtr &_pThrgrp );

    explicit APIServer( const ServerType::options &_Opts,
                        const IoServicePtr &_pIoSrv, 
                        const ThreadGroupPtr &_pThrgrp, 
                        const char *confFileName );

    ~APIServer()
    {
        stop();
        delete m_pServer;
        m_pServer = NULL;
    }

    // void setHandler( APIServerHandler *_pH )
    // { m_pHandler = _pH; }
    
    void run();
    void stop();

    std::string toString() const
    {
        using namespace std;

        stringstream os;

        os << "APIServer: " << endl;
        os << "port: " << m_nPort << endl;
        os << "n_io_threads: " << m_nIoThreads << endl;
        os << "n_work_threads: " << m_nWorkThreads << endl;

        return os.str();
    }

    std::shared_ptr<asio::io_service> ioService()
    { return m_pIoService; }

    ServerType::options& options()
    { return m_Options; }

    bool isRunning() const
    { return m_bRunning; }

private:
    PropertyTable       m_mapProperties;
    uint16_t            m_nPort;
    uint32_t            m_nIoThreads;
    uint32_t            m_nWorkThreads;
    bool                m_bRunning;
    ServerType*         m_pServer;
    ServerType::options m_Options;

    std::shared_ptr<asio::io_service>           m_pIoService;
    std::shared_ptr<ThreadGroup>                m_pIoThrgrp;
};

} // namespace BigRLab


#endif

