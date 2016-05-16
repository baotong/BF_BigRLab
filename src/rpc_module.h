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


template <typename Processor, 
          typename ServerType = TNonblockingServer,
          typename ProtocolFactory = TBinaryProtocolFactory>
class ThriftServer {
public:
    explicit ThriftServer( const boost::shared_ptr<TProcessor> &_Processor,
                           uint16_t _Port,
                           uint32_t _nIoThread = 5, uint32_t _nWorkThread = 100 )
                : m_pProcessor(_Processor)
                , m_nPort(_Port)
                , m_nIoThread(_nIoThread)
                , m_nWorkThread(_nWorkThread)
                , m_pProtocolFactory(boost::make_shared<ProtocolFactory>())
    {
        m_pThrMgr = ThreadManager::newSimpleThreadManager( m_nIoThread + m_nWorkThread );
        m_pThrMgr->threadFactory( boost::make_shared<PlatformThreadFactory>() );
        m_pServer = boost::make_shared<ServerType>(m_pProcessor, m_pProtocolFactory, m_nPort, m_pThrMgr);
        m_pServer->setNumIOThreads( m_nIoThread );
    }

    void start()
    {
        m_pThrMgr->start();
        m_pServer->serve();
    }

private:
    uint16_t                            m_nPort;
    uint32_t                            m_nIoThread;
    uint32_t                            m_nWorkThread;
    boost::shared_ptr<TProcessor>       m_pProcessor; 
    boost::shared_ptr<TProtocolFactory> m_pProtocolFactory;
    boost::shared_ptr<ThreadManager>    m_pThrMgr;
    boost::shared_ptr<ServerType>       m_pServer;
};


} // namespace BigRLab



#endif

