#include "alg_mgr.h"
#include "service_manager.h"


namespace BigRLab {

AlgMgrServer::Pointer   g_pAlgMgrServer;

int32_t AlgMgrServiceHandler::addSvr(const std::string& algName, const AlgSvrInfo& svrInfo)
{
    LOG(INFO) << "received addSvr request: name = " << algName << " info = "
        << svrInfo.addr << ":" << svrInfo.port << " maxConcurrency = " << svrInfo.maxConcurrency;

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
        << svrInfo.addr << ":" << svrInfo.port;

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

// void AlgMgrServiceHandler::getAlgSvrList(std::vector<AlgSvrInfo> & _return, const std::string& name)
// {
    // _return.clear();
    // boost::upgrade_lock< AlgSvrTable > sLock(m_mapSvrTable);
    // auto it = m_mapSvrTable.find( name );
    // if ( it == m_mapSvrTable.end() )
        // return;

    // AlgSvrSubTable &subTable = it->second;
    // boost::upgrade_lock< AlgSvrSubTable > sSubLock(subTable);
    // _return.reserve( subTable.size() );
    // for (auto &v : subTable)
        // _return.push_back( v.second->svrInfo );
// }


static boost::shared_ptr<boost::thread> s_pAlgMgrSvrThread;

void start_alg_mgr()
{
    if (g_pAlgMgrServer && g_pAlgMgrServer->isRunning())
        stop_alg_mgr();

    auto thr_func = [&] {
        try {
            boost::shared_ptr< AlgMgrServiceIf > pHandler = boost::make_shared<AlgMgrServiceHandler>();
            // TODO m_nIoThreads && m_nWorkThreads configurable
            g_pAlgMgrServer = boost::make_shared<AlgMgrServer>(pHandler, ALG_MGR_SERV_PORT);
            g_pAlgMgrServer->start();
        } catch (const std::exception &ex) {
            LOG(ERROR) << "Start algmgr server fail!";
            stop_alg_mgr();
        } // try
    };

    s_pAlgMgrSvrThread.reset(new boost::thread(thr_func));
}

void stop_alg_mgr()
{
    try {
        g_pAlgMgrServer->stop();
        if (s_pAlgMgrSvrThread && s_pAlgMgrSvrThread->joinable())
            s_pAlgMgrSvrThread->join();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Stop algmgr server fail!";
    } // try
}

} // namespace BigRLab


