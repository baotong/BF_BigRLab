#include "alg_mgr.h"
#include "service_manager.h"


namespace BigRLab {

AlgMgrServer::Pointer                   g_pAlgMgrServer;
boost::shared_ptr<AlgMgrServiceHandler> g_pAlgMgrHandler;
uint16_t                                g_nAlgMgrPort = 0;

int32_t AlgMgrServiceHandler::addSvr(const std::string& algName, const AlgSvrInfo& svrInfo)
{
    int ret = SUCCESS;

    // DLOG(INFO) << "received addSvr request: name = " << algName << " info = " << svrInfo;

    boost::upgrade_lock< AlgSvrTable > sLock(m_mapSvrTable);
    auto it = m_mapSvrTable.find( algName );
    // algName not exist
    if ( it == m_mapSvrTable.end() ) {
        ret = ServiceManager::getInstance()->addAlgServer( algName, svrInfo ); // ★
        if (SUCCESS == ret) {
            boost::upgrade_to_unique_lock< AlgSvrTable > uLock(sLock);
            m_mapSvrTable[algName].insert( std::make_pair(svrInfo.addr,
                        boost::make_shared<AlgSvrRecord>(svrInfo)) );
        } // if
        return ret;
    } // if

    // else
    AlgSvrSubTable &subTable = it->second;
    boost::upgrade_lock< AlgSvrSubTable > sSubLock(subTable);
    auto range = subTable.equal_range( svrInfo.addr );
    // addr not exist
    if (range.first == range.second) {
        ret = ServiceManager::getInstance()->addAlgServer( algName, svrInfo );
        if (SUCCESS == ret) {
            boost::upgrade_to_unique_lock< AlgSvrSubTable > uSubLock(sSubLock);
            subTable.insert( std::make_pair(svrInfo.addr,
                        boost::make_shared<AlgSvrRecord>(svrInfo)) );
        } // if
        return ret;
    } // if

    // else
    for (auto sit = range.first; sit != range.second; ++sit) {
        if (sit->second->port() == svrInfo.port)
            return ALREADY_EXIST;
    } // for
    
    ret = ServiceManager::getInstance()->addAlgServer( algName, svrInfo );
    if (SUCCESS == ret) {
        boost::upgrade_to_unique_lock< AlgSvrSubTable > uSubLock(sSubLock);
        subTable.insert( std::make_pair(svrInfo.addr,
                    boost::make_shared<AlgSvrRecord>(svrInfo)) );
    } // if

    return ret;
}

void AlgMgrServiceHandler::rmSvr(const std::string& algName, const AlgSvrInfo& svrInfo)
{
    DLOG(INFO) << "received rmSvr request: name = " << algName << " info = " << svrInfo;

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

    ServiceManager::getInstance()->rmAlgServer( algName, svrInfo );     // ★
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


static boost::shared_ptr<boost::thread> s_pAlgMgrSvrThread;

void start_alg_mgr()
{
    if (g_pAlgMgrServer && g_pAlgMgrServer->isRunning())
        return;

    auto thr_func = [&] {
        try {
            g_pAlgMgrHandler = boost::make_shared<AlgMgrServiceHandler>();
            // 1 io threads and 3 work threads
            g_pAlgMgrServer = boost::make_shared<AlgMgrServer>(
                    boost::static_pointer_cast<AlgMgrServiceIf>(g_pAlgMgrHandler), g_nAlgMgrPort, 2, 3);
            g_pAlgMgrServer->start();
        } catch (const std::exception &ex) {
            LOG(ERROR) << "Start algmgr server fail! " << ex.what(); 
            stop_alg_mgr();
        } // try
    };

    s_pAlgMgrSvrThread.reset(new boost::thread(thr_func));
}

void stop_alg_mgr()
{
    if (!g_pAlgMgrServer)
        return;

    try {
        g_pAlgMgrServer->stop();
        if (s_pAlgMgrSvrThread && s_pAlgMgrSvrThread->joinable())
            s_pAlgMgrSvrThread->join();
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Stop algmgr server fail! " << ex.what(); 
    } // try
}

} // namespace BigRLab


