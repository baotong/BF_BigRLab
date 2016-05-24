#ifndef _WORK_MGR_H_
#define _WORK_MGR_H_

#include "common_utils.h"

namespace BigRLab {

struct WorkItemBase {
    virtual ~WorkItemBase() = default;
    virtual void run() = 0;
};

typedef boost::shared_ptr<WorkItemBase>   WorkItemBasePtr;

class WorkManager {
public:
    typedef typename boost::shared_ptr< WorkManager >    Pointer;

public:
    explicit WorkManager( std::size_t _nWorker )
            : m_nWorkThreads(_nWorker) {}

    void start()
    {
        for (std::size_t i = 0; i < m_nWorkThreads; ++i)
            m_Thrgrp.create_thread( 
                    std::bind(&WorkManager::run, this) );
    }

    void stop()
    {
        for (std::size_t i = 0; i < 2 * m_nWorkThreads; ++i)
            m_WorkQueue.push( WorkItemBasePtr() );
        m_Thrgrp.join_all();
        m_WorkQueue.clear();
    }

    void addWork( const WorkItemBasePtr &pWork )
    { m_WorkQueue.push(pWork); }

    template <typename U>
    void addWork( const U &p )
    { 
        WorkItemBasePtr pWork = boost::dynamic_pointer_cast<WorkItemBase>(p);
        if (!pWork)
            throw std::bad_cast();
        m_WorkQueue.push(pWork); 
    }

private:
    WorkManager(const WorkManager&) = delete;
    WorkManager(WorkManager&&) = delete;
    WorkManager& operator = (const WorkManager&) = delete;
    WorkManager& operator = (WorkManager&&) = delete;

    void run()
    {
        while (true) {
            auto pWork = m_WorkQueue.pop();
            // 空指针表示结束工作线程
            if (!pWork)
                return;
            try {
                pWork->run();
            } catch (const std::exception &ex) {
                LOG(ERROR) << "Run service exception " << ex.what();
            } // try
        } // while
    }

private:
    SharedQueue<WorkItemBasePtr>    m_WorkQueue;
    boost::thread_group     m_Thrgrp;
    std::size_t             m_nWorkThreads;
};

extern boost::shared_ptr< WorkManager >   g_pWorkMgr;


} // namespace BigRLab

#endif

