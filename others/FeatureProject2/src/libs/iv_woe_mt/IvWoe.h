#ifndef _IV_WOE_H_
#define _IV_WOE_H_ 

#include "FeatureTask.h"
#include "shared_queue.hpp"
#include <atomic>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>


using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


template <typename K, typename V>
class SafeMap : public std::map<K, V>
              , public boost::shared_lockable_adapter<boost::shared_mutex> 
{
public:
    typedef typename std::map<K, V>     BaseType;
    typedef SafeMap<K, V>               ThisType;

public:
    // 有copy constructor，必须实现默认constructor
    SafeMap() = default;

    // mutex is nocopyable, 所以必须实现 copy constructor
    SafeMap(const SafeMap<K, V> &other)
    {
        boost::unique_lock<ThisType> wLock(*this);
        // 在模板中，必须显示调用基类函数
        BaseType::clear();
        BaseType::insert(other.begin(), other.end());
    }

    std::pair<typename BaseType::iterator, bool> 
    insert(const typename BaseType::value_type &v)
    {
        boost::unique_lock<ThisType> wLock(*this);
        return BaseType::insert(v);
    }
};


struct ValueCnt {
    ValueCnt() : posCnt_(0), negCnt_(0)
               , woe_(0.0), iv_(0.0) {}

    // atomic is nocopyable, copy constructor must be provided
    ValueCnt(const ValueCnt &rhs)
            : posCnt_(rhs.posCnt()), negCnt_(rhs.negCnt())
            , woe_(rhs.woe()), iv_(rhs.iv()) {}
    
    uint32_t posCnt() const { return posCnt_; }
    uint32_t negCnt() const { return negCnt_; }
    void addPos() { ++posCnt_; }
    void addNeg() { ++negCnt_; }

    const double& woe() const { return woe_; }
    const double&  iv() const { return iv_; }

    std::atomic_uint        posCnt_, negCnt_;
    double                  woe_, iv_;
};


struct FeatureCnt {
    typedef SafeMap<std::string, ValueCnt>      ValueCntMap;

    FeatureCnt() : totalPosCnt_(0), totalNegCnt_(0), totalIV_(0.0) {}

    FeatureCnt(const FeatureCnt &rhs)
            : totalPosCnt_(rhs.totalPosCnt()), totalNegCnt_(rhs.totalNegCnt())
            , totalIV_(rhs.totalIV()), values_(rhs.values()) {}

    uint32_t totalPosCnt() const { return totalPosCnt_; }
    uint32_t totalNegCnt() const { return totalNegCnt_; }
    void addTotalPos() { ++totalPosCnt_; }
    void addTotalNeg() { ++totalNegCnt_; }
    const double& totalIV() const { return totalIV_; }
    ValueCntMap& values() { return values_; }
    const ValueCntMap& values() const { return values_; }

    std::atomic_uint        totalPosCnt_, totalNegCnt_;
    double                  totalIV_;
    ValueCntMap             values_;
};


struct FeatureCntMgr {
    typedef SafeMap<std::string, FeatureCnt>   FeatureCntMap;

    FeatureCntMap& featureCnt() { return featureCntMap_; }

    void addRecord(const std::string &key, const std::string &value, bool pos)
    {
        auto ret1 = featureCntMap_.insert(std::make_pair(key, FeatureCnt()));
        auto &featureCnt = ret1.first->second;
        auto ret2 = featureCnt.values().insert(std::make_pair(value, ValueCnt()));
        auto &valueCnt = ret2.first->second;
        if (pos) {
            valueCnt.addPos(); featureCnt.addTotalPos();
        } else {
            valueCnt.addNeg(); featureCnt.addTotalNeg();
        } // if pos
    }

    FeatureCntMap       featureCntMap_;
};



class IvWoe : public FeatureTask {
public:
    struct Input {
        FeatureVector   fv_;
        std::string     target_;
    };
    typedef std::shared_ptr<Input>  InputPtr;

public:
    IvWoe(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void loadData();
    void caculateIV();
    void writeResult();

private:
    std::string                         m_strTargetFile;
    std::shared_ptr<FeatureCntMgr>      m_pFeatureMgr;
    std::unique_ptr<SharedQueue<InputPtr> >     m_pInputBuffer;
    std::unique_ptr<std::thread>                m_pThrReader;
    boost::thread_group                         m_ThrWorkers;
    uint32_t                                    m_nWorkers;
};

#endif /* _IV_WOE_H_ */

