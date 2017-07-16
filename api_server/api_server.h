#ifndef _API_SERVER_H_
#define _API_SERVER_H_

#include "common_utils.h"
#include "work_mgr.h"
#include "rpc_module.h"
#include "AlgMgrService.h"
#include <boost/network/include/http/server.hpp>
#include <boost/network/uri.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <glog/logging.h>

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
    enum StatusCode {
        OK = 0,
        BAD_REQUEST = -1,
        SERVICE_UNAVAILABLE = -2
    };

public:
    explicit APIServer( uint16_t _SvrPort,
            uint32_t _nIoThreads, uint32_t _nWorkThreads,
            const ServerType::options &_Opts,
            const IoServicePtr &_pIoSrv,
            const ThreadGroupPtr &_pThrgrp );

    // explicit APIServer( const ServerType::options &_Opts,
                        // const IoServicePtr &_pIoSrv, 
                        // const ThreadGroupPtr &_pThrgrp, 
                        // const char *confFileName );

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

    uint32_t nWorkThreads() const
    { return m_nWorkThreads; }

    // AlgMgrClient::Pointer algMgrClient() const
    // { return m_pAlgMgrClient; }

private:
    ServerType*         m_pServer;
    bool                m_bRunning;
    uint16_t            m_nPort;
    uint32_t            m_nIoThreads;
    uint32_t            m_nWorkThreads;
    ServerType::options m_Options;

    IoServicePtr        m_pIoService;
    ThreadGroupPtr      m_pIoThrgrp;
};

extern boost::shared_ptr<APIServer>       g_pApiServer;

struct WorkItem : WorkItemBase
                , boost::enable_shared_from_this<WorkItem> {
    WorkItem(const ServerType::request &_Req, 
             const ServerType::connection_ptr &_Conn, 
             std::size_t nRead) 
                : req(_Req)
                , conn(_Conn)
                , left2Read(nRead) 
    { body.reserve(nRead); }

    virtual void readBody( const ServerType::connection_ptr &conn );

    void handleRead(ServerType::connection::input_range range, 
            boost::system::error_code error, std::size_t size, 
            ServerType::connection_ptr conn);

    virtual void run(); 

    ServerType::request        req;
    ServerType::connection_ptr conn;
    std::string     body;
    std::size_t     left2Read;
};

typedef boost::shared_ptr<WorkItem>   WorkItemPtr;


struct WorkItemCmd : WorkItem {

    static const int TIMEOUT = 10000;

    typedef boost::shared_ptr<WorkItemCmd>  pointer;

    WorkItemCmd(const ServerType::request &_Req, 
             const ServerType::connection_ptr &_Conn, 
             std::size_t nRead) : WorkItem(_Req, _Conn, nRead) {}

    virtual void readBody( const ServerType::connection_ptr &conn );
};

extern boost::shared_ptr< SharedQueue<WorkItemCmd::pointer> >    g_pCmdQue;

class HttpWriter : public Writer {
public:  
    virtual bool readLine( std::string &line );
    virtual void writeLine( const std::string &msg );
private:
    WorkItemCmd::pointer    m_pWorkCmd;
};


inline
void send_response(const ServerType::connection_ptr &conn,
                   ServerType::connection::status_t status,
                   const std::string &content = "")
{
    conn->set_status(status);
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n";     // No reason field like "OK"
    oss << "Content-Length: " << content.length() << "\r\n";
    oss << "Content-Type: application/json\r\n\r\n";
    oss << content << std::flush;
    conn->write(oss.str());
}

#define RESPONSE_MSG(conn, args) \
    do { \
        std::stringstream __response_stream; \
        __response_stream << args << std::endl << std::flush; \
        send_response(conn, ServerType::connection::ok, __response_stream.str()); \
    } while (0)

inline
void response_json_string(const ServerType::connection_ptr &conn,
                    ServerType::connection::status_t connStatus,
                    int statCode,
                    const std::string &key,
                    const std::string &value)
{
    Json::Value root;
    root["status"] = statCode;
    root[key] = value;

    Json::FastWriter writer;  
    std::string strResp = writer.write(root);
    send_response(conn, connStatus, strResp);
}

#define RESPONSE_ERROR(conn, connStatus, errCode, args) \
    do { \
        std::stringstream __response_stream; \
        __response_stream << args << std::flush; \
        response_json_string(conn, connStatus, errCode, "errmsg", __response_stream.str()); \
    } while (0)

extern uint16_t g_nApiSvrPort;

} // namespace BigRLab


#endif

/*
 * enum status_t {
 *     ok = 200
 *     , created = 201
 *     , accepted = 202
 *     , no_content = 204
 *     , multiple_choices = 300
 *     , moved_permanently = 301
 *     , moved_temporarily = 302
 *     , not_modified = 304
 *     , bad_request = 400
 *     , unauthorized = 401
 *     , forbidden = 403
 *     , not_found = 404
 *     , not_supported = 405
 *     , not_acceptable = 406
 *     , internal_server_error = 500
 *     , not_implemented = 501
 *     , bad_gateway = 502
 *     , service_unavailable = 503
 * };
 */
