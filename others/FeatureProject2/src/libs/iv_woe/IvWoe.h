#ifndef _IV_WOE_H_
#define _IV_WOE_H_ 

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}

struct ValueCnt {
    ValueCnt() : posCnt_(0), negCnt_(0)
               , woe_(0.0), iv_(0.0) {}
    
    uint32_t posCnt() const { return posCnt_; }
    uint32_t negCnt() const { return negCnt_; }
    void addPos() { ++posCnt_; }
    void addNeg() { ++negCnt_; }

    const double& woe() const { return woe_; }
    const double&  iv() const { return iv_; }

    uint32_t        posCnt_, negCnt_;
    double          woe_, iv_;
};


struct FeatureCnt {
    typedef std::map<std::string, ValueCnt>     ValueCntMap;

    FeatureCnt() : totalPosCnt_(0), totalNegCnt_(0), totalIV_(0.0) {}

    uint32_t totalPosCnt() const { return totalPosCnt_; }
    uint32_t totalNegCnt() const { return totalNegCnt_; }
    void addTotalPos() { ++totalPosCnt_; }
    void addTotalNeg() { ++totalNegCnt_; }
    const double& totalIV() const { return totalIV_; }
    ValueCntMap& values() { return values_; }

    uint32_t        totalPosCnt_, totalNegCnt_;
    double          totalIV_;
    ValueCntMap     values_;
};


struct FeatureCntMgr {
    typedef std::map<std::string, FeatureCnt>   FeatureCntMap;

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
};

#endif /* _IV_WOE_H_ */

