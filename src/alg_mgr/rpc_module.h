#ifndef _RPC_MODULE_H_
#define _RPC_MODULE_H_

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <boost/make_shared.hpp>

namespace BigRLab {

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;


template <typename Handler,
          typename Processor, 
          typename ServerType = TNonblockingServer,
          typename ProtocolFactory = TBinaryProtocolFactory>
class ThriftServer {
public:
    typedef typename boost::shared_ptr< ThriftServer<Handler, 
                Processor, ServerType, ProtocolFactory> >  Pointer;

public:
    explicit ThriftServer( const boost::shared_ptr<Handler> &_Handler,
                           uint16_t _Port,
                           uint32_t _nIoThread = 5, uint32_t _nWorkThread = 100 )
                : m_nPort(_Port)
                , m_nIoThread(_nIoThread)
                , m_nWorkThread(_nWorkThread)
                , m_bRunning(false)
    {
        m_pThrMgr = ThreadManager::newSimpleThreadManager( m_nIoThread + m_nWorkThread );
        m_pThrMgr->threadFactory( boost::make_shared<PlatformThreadFactory>() );
        boost::shared_ptr<TProtocolFactory> pProtocolFactory = boost::make_shared<ProtocolFactory>();
        boost::shared_ptr<TProcessor> pProcessor = boost::make_shared<Processor>(_Handler);
        m_pServer = boost::make_shared<ServerType>(pProcessor, pProtocolFactory, m_nPort, m_pThrMgr);
        m_pServer->setNumIOThreads( m_nIoThread );
    }

    ~ThriftServer()
    { stop(); }

    void start()
    {
        if (!m_bRunning) {
            m_bRunning = true;
            m_pThrMgr->start();
            m_pServer->serve();
        } // if
    }

    void stop()
    {
        if (m_bRunning) {
            m_bRunning = false;
            m_pServer->stop();
        } // if
    }

private:
    uint16_t                            m_nPort;
    uint32_t                            m_nIoThread;
    uint32_t                            m_nWorkThread;
    bool                                m_bRunning;
    boost::shared_ptr<ThreadManager>    m_pThrMgr;
    boost::shared_ptr<ServerType>       m_pServer;
};


template <typename ClientType, 
          typename Transport = TFramedTransport,
          typename Protocol = TBinaryProtocol>
class ThriftClient {
public:
    explicit ThriftClient( const std::string &_SvrAddr, uint16_t _SvrPort )
                : m_strSvrAddr(_SvrAddr)
                , m_nSvrPort(_SvrPort)
                , m_bRunning(false)
    {
        auto pSocket = boost::make_shared<TSocket>(_SvrAddr, _SvrPort);
        m_pTransport = boost::make_shared<Transport>(pSocket);
        boost::shared_ptr<TProtocol> pProtocol = boost::make_shared<Protocol>(m_pTransport);
        m_pClient = boost::make_shared<ClientType>(pProtocol);
    }

    ~ThriftClient()
    { stop(); }

    void start()
    { 
        if (!m_bRunning) {
            m_pTransport->open(); 
            m_bRunning = true;
        } // if
    }

    void stop()
    {
        if (m_bRunning) {
            m_pTransport->close();
            m_bRunning = false;
        }
    }

private:
    std::string                         m_strSvrAddr;
    uint16_t                            m_nSvrPort;
    bool                                m_bRunning;
    boost::shared_ptr<Transport>        m_pTransport;
    boost::shared_ptr<ClientType>       m_pClient;
};


} // namespace BigRLab



#endif

