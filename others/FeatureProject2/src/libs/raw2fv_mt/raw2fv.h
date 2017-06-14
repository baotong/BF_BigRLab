#ifndef _RAW2FV_H_
#define _RAW2FV_H_

#include "shared_queue.hpp"
#include "FeatureTask.h"
#include <boost/thread.hpp>


using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


template <typename T>
class SortedOutputBuffer {
public:
    typedef std::map<std::size_t, T>    ContainerType;

public:
    SortedOutputBuffer(const std::size_t _MaxSize, const std::size_t _HashSzie) 
            : m_nMaxSize(_MaxSize), m_nHashSize(_HashSzie), m_nSize(0) 
    { m_arrData.resize(m_nHashSize); }

    std::size_t maxSize() const { return m_nMaxSize; }
    std::size_t size() const { return m_nSize; }
    bool full() const { return size() >= maxSize(); }

    void put(const std::size_t no, const T &data)
    {
        std::size_t idx = no % m_nHashSize;
        std::unique_lock<std::mutex>    lck(m_Lock);
        m_CondFull.wait(lck, [&, this]()->bool{
            return !full();
        });
        m_arrData[idx][no] = data;
        ++m_nSize;
    }

    bool pop(const std::size_t no, T &data)
    {
        std::size_t idx = no % m_nHashSize;
        auto &sub = m_arrData[idx];
        std::unique_lock<std::mutex>    lck(m_Lock);
        auto it = sub.begin();
        if (it == sub.end() || it->first != no) 
            return false;
        data = it->second;
        sub.erase(it);
        --m_nSize;
        lck.unlock();
        m_CondFull.notify_all();
        return true;
    }

private:
    const std::size_t                       m_nMaxSize, m_nHashSize;
    std::size_t                             m_nSize;
    std::vector<ContainerType>              m_arrData;
    std::mutex                              m_Lock;
    std::condition_variable                 m_CondFull;
};



class Raw2Fv : public FeatureTask {
public:
    typedef std::pair<std::size_t, std::string>     InputType;
    typedef std::shared_ptr<InputType>              InputPtr;
    typedef std::shared_ptr<FeatureVector>          FvPtr;

public:
    Raw2Fv(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr), m_nWorkers(0) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void loadDesc();
    void loadDataWithId();
    void loadDataWithoutId();
    // void genIdx();
    // void writeDesc();
    bool read_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    bool read_string_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    bool read_double_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    bool read_datetime_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);

private:
    std::string         m_strDesc, m_strNewDesc, m_strSep;
    uint32_t            m_nWorkers;
    std::unique_ptr<FeatureInfoSet>             m_pFeatureInfoSet;
    std::unique_ptr<SharedQueue<InputPtr> >     m_pInputBuffer;
    std::unique_ptr<SortedOutputBuffer<FvPtr> > m_pOutputBuffer;
    std::unique_ptr<std::thread>                m_pThrWriter, m_pThrReader;
    boost::thread_group                         m_ThrWorkers;
};


#endif /* _RAW2FV_H_ */



#if 0
// for keeping pointers
template <typename T>
class SeqBuffer {
public:
    explicit SeqBuffer(const std::size_t _MaxSize) 
            : m_nMaxSize(_MaxSize), m_nSize(0)
    {
        m_arrData.assign(m_nMaxSize, T());
        m_arrLck.resize(m_nMaxSize);
        m_arrCond.resize(m_nMaxSize);
    }

    std::size_t maxSize() const { return m_nMaxSize; }
    std::size_t size() const { return m_nSize; }

    void put(const std::size_t no, const T &data)
    {
        std::size_t idx = no % maxSize();
        boost::unique_lock<boost::mutex> lk(m_arrLck[idx]);
        m_arrCond[idx].wait(lk, [&, this]()->bool{
            return !m_arrData[idx];
        });
        m_arrData[no % maxSize()] = data;
        ++m_nSize;
    }

private:
    const std::size_t                       m_nMaxSize;
    std::atomic_size_t                      m_nSize;
    std::vector<T>                          m_arrData;
    std::vector<boost::mutex>               m_arrLck;
    std::vector<boost::condition_variable>  m_arrCond;
};
#endif

