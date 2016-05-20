#include "rpc_module.h"
#include "AlgMgrService.h"
#include <map>
#include <string>
#include <ctime>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <glog/logging.h>


namespace BigRLab {

class AlgMgrServiceHandler : virtual public AlgMgrServiceIf {
public:
    struct AlgSvrRecord {
        AlgSvrRecord( const AlgSvrInfo &_SvrInfo )
                    : svrInfo(_SvrInfo)
                    , lastAlive(time(0))
        {}

        const std::string& addr() const
        { return svrInfo.addr; }

        uint16_t port() const
        { return (uint16_t)(svrInfo.port); }

        AlgSvrInfo  svrInfo;
        time_t      lastAlive;
    };

    typedef boost::shared_ptr<AlgSvrRecord>     AlgSvrRecordPtr;

    struct AlgSvrSubTable : std::multimap<std::string, AlgSvrRecordPtr>
                          , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

    // { algname : {addr : Record} }
    struct AlgSvrTable : std::map<std::string, AlgSvrSubTable>
                       , boost::upgrade_lockable_adapter<boost::shared_mutex>
    {};

public:
    // for alg servers
    // virtual int16_t availablePort()
    // { return (int16_t)(++s_nNextPort); }

    virtual int32_t addSvr(const std::string& algName, const AlgSvrInfo& svrInfo);
    virtual void rmSvr(const std::string& algName, const AlgSvrInfo& svrInfo);
    virtual void informAlive(const std::string& algName, const AlgSvrInfo& svrInfo); // TODO timer

    // for api server
    virtual void getAlgSvrList(std::vector<AlgSvrInfo> & _return, const std::string& name);

private:
    // static const uint16_t       START_PORT = 9020;
    // static std::atomic_ushort   s_nNextPort;

    AlgSvrTable     m_mapSvrTable;
};

// std::atomic_ushort AlgSvrServiceHandler::s_nNextPort = START_PORT;


int32_t AlgMgrServiceHandler::addSvr(const std::string& algName, const AlgSvrInfo& svrInfo)
{
    LOG(INFO) << "received addSvr request: name = " << algName << " info = "
        << svrInfo.addr << ":" << svrInfo.port << " nWorkThread = " << svrInfo.nWorkThread;

    boost::upgrade_lock< AlgSvrTable > sLock(m_mapSvrTable);
    auto it = m_mapSvrTable.find( algName );
    // algName not exist
    if ( it == m_mapSvrTable.end() ) {
        boost::upgrade_to_unique_lock< AlgSvrTable > uLock(sLock);
        m_mapSvrTable[algName].insert( std::make_pair(svrInfo.addr,
                            boost::make_shared<AlgSvrRecord>(svrInfo)) );
        // m_mapSvrTable[algName].insert( it, std::make_pair(svrInfo.addr,
                            // boost::make_shared<AlgSvrRecord>(svrInfo)) );
        return SUCCESS;
    } // if

    // else
    AlgSvrSubTable &subTable = it->second;
    boost::upgrade_lock< AlgSvrSubTable > sSubLock(subTable);
    auto range = subTable.equal_range( svrInfo.addr );
    // addr not exist
    if (range.first == range.second) {
        boost::upgrade_to_unique_lock< AlgSvrSubTable > uSubLock(sSubLock);
        subTable.insert( std::make_pair(svrInfo.addr,
                            boost::make_shared<AlgSvrRecord>(svrInfo)) );
        // subTable.insert( range.second, std::make_pair(svrInfo.addr,
                            // boost::make_shared<AlgSvrRecord>(svrInfo)) );
        return SUCCESS;
    } // if

    // else
    for (auto sit = range.first; sit != range.second; ++sit) {
        if (sit->second->port() == svrInfo.port)
            return ALREADY_EXIST;
    } // for

    boost::upgrade_to_unique_lock< AlgSvrSubTable > uSubLock(sSubLock);
    subTable.insert( std::make_pair(svrInfo.addr,
                boost::make_shared<AlgSvrRecord>(svrInfo)) );
    // subTable.insert( range.second, std::make_pair(svrInfo.addr,
                // boost::make_shared<AlgSvrRecord>(svrInfo)) );
    
    return SUCCESS;
}

void AlgMgrServiceHandler::rmSvr(const std::string& algName, const AlgSvrInfo& svrInfo)
{
    LOG(INFO) << "received rmSvr request: name = " << algName << " info = "
        << svrInfo.addr << ":" << svrInfo.port << " nWorkThread = " << svrInfo.nWorkThread;

    boost::upgrade_lock< AlgSvrTable > sLock(m_mapSvrTable);
    auto it = m_mapSvrTable.find( algName );
    if ( it == m_mapSvrTable.end() )
        return;

    AlgSvrSubTable &subTable = it->second;
    boost::upgrade_lock< AlgSvrSubTable > sSubLock(subTable);
    auto range = subTable.equal_range( svrInfo.addr );
    if (range.first == range.second)
        return;

    for (auto sit = range.first; sit != range.second;) {
        if (sit->second->port() == svrInfo.port) {
            boost::upgrade_to_unique_lock< AlgSvrSubTable > uSubLock(sSubLock);
            sit = subTable.erase(sit);
        } else {
            ++sit;
        } // if
    } // for
}

void AlgMgrServiceHandler::informAlive(const std::string& algName, const AlgSvrInfo& svrInfo)
{
    boost::upgrade_lock< AlgSvrTable > sLock(m_mapSvrTable);
    auto it = m_mapSvrTable.find( algName );
    if ( it == m_mapSvrTable.end() )
        return;

    AlgSvrSubTable &subTable = it->second;
    boost::upgrade_lock< AlgSvrSubTable > sSubLock(subTable);
    auto range = subTable.equal_range( svrInfo.addr );
    if (range.first == range.second)
        return;

    for (auto sit = range.first; sit != range.second; ++sit) {
        if (sit->second->port() == svrInfo.port)
            sit->second->lastAlive = time(0);
    } // for

    return;
}

void AlgMgrServiceHandler::getAlgSvrList(std::vector<AlgSvrInfo> & _return, const std::string& name)
{
    _return.clear();
    boost::upgrade_lock< AlgSvrTable > sLock(m_mapSvrTable);
    auto it = m_mapSvrTable.find( name );
    if ( it == m_mapSvrTable.end() )
        return;

    AlgSvrSubTable &subTable = it->second;
    boost::upgrade_lock< AlgSvrSubTable > sSubLock(subTable);
    _return.reserve( subTable.size() );
    for (auto &v : subTable)
        _return.push_back( v.second->svrInfo );
}


typedef ThriftServer< AlgMgrServiceIf, AlgMgrServiceProcessor > AlgMgrServer;

} // namespace BigRLab


// global vars
static boost::shared_ptr< BigRLab::AlgMgrServer >   g_pServer;
static boost::asio::io_service                 g_io_service;

static
void finish()
{
    g_pServer->stop();
}


int main( int argc, char **argv )
{
    using namespace BigRLab;
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);

        // install signal handler
        auto pIoServiceWork = boost::make_shared< boost::asio::io_service::work >(std::ref(g_io_service));
        boost::asio::signal_set signals(g_io_service, SIGINT, SIGTERM);
        signals.async_wait( [](const boost::system::error_code& error, int signal)
                { finish(); } );

        auto io_service_thr = boost::thread( [&]{ g_io_service.run(); } );

        boost::shared_ptr< AlgMgrServiceIf > pHandler = boost::make_shared<AlgMgrServiceHandler>();
        g_pServer = boost::make_shared<AlgMgrServer>(pHandler, 9001);
        g_pServer->start();

        pIoServiceWork.reset();
        g_io_service.stop();
        if (io_service_thr.joinable())
            io_service_thr.join();
        cout << "AlgMgr server Done!" << endl;

    } catch (const std::exception &ex) {
        LOG(ERROR) << "Exception caught by main " << ex.what();
        exit(-1); 
    } // try

    return 0;
}

