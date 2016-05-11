#ifndef _API_SERVER_H_
#define _API_SERVER_H_

#include "service_manager.h"
#include <boost/network/include/http/server.hpp>
#include <boost/network/uri.hpp>
#include <boost/asio.hpp>

namespace BigRLab {

using ThreadPool = boost::network::utils::thread_pool;
using ThreadGroup = boost::thread_group;
typedef boost::shared_ptr<ThreadGroup>                   ThreadGroupPtr;
typedef boost::shared_ptr<boost::asio::io_service>       IoServicePtr;
typedef boost::shared_ptr<boost::asio::io_service::work> IoServiceWorkPtr;

struct APIServerHandler;
typedef boost::network::http::async_server<APIServerHandler> ServerType;

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

    IoServicePtr ioService()
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

    IoServicePtr        m_pIoService;
    ThreadGroupPtr      m_pIoThrgrp;
};

struct WorkItem : boost::enable_shared_from_this<WorkItem> {
    WorkItem(const std::string &_Src, const std::string &_Dest, std::size_t nRead) 
            : source(_Src)
            , dest(_Dest)
            , left2Read(nRead) 
    { body.reserve(nRead); }

    void readBody( const ServerType::connection_ptr &conn );

    void handleRead(ServerType::connection::input_range range, 
            boost::system::error_code error, std::size_t size, 
            ServerType::connection_ptr conn);

    std::string     source;
    std::string     dest;
    std::string     body;
    std::size_t     left2Read;
};

typedef boost::shared_ptr<WorkItem>   WorkItemPtr;


typedef SharedQueue<WorkItemPtr>    WorkQueue;

extern boost::shared_ptr<WorkQueue>   g_pWorkQueue;

} // namespace BigRLab


#endif

