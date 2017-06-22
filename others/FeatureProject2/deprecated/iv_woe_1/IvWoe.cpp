#include "IvWoe.h"
#include <glog/logging.h>
#include <cassert>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new IvWoe(name, mgr); }


void IvWoe::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "IvWoe::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "IvWoe::init() input file not specified!");

    m_strTargetFile = conf["target"].asString();
    THROW_RUNTIME_ERROR_IF(m_strTargetFile.empty(),
            "IvWoe::init() target file not specified!");

    fs::path dataPath(m_pTaskMgr->dataDir());
    m_strTargetFile = (dataPath / m_strTargetFile).c_str();
}


void IvWoe::run()
{
    DLOG(INFO) << "IvWoe::run()";

    LOG(INFO) << "Loading data " << m_strInput << " ...";
    loadData();
    LOG(INFO) << "Caculating IV and WOE...";
    caculateIV();
    LOG(INFO) << "Writing result to file...";
    writeResult();
    LOG(INFO) << "Results has written to " << m_strOutput;
}


void IvWoe::writeResult()
{
    using namespace std;

#if 0
    // dump FeatureCntMgr for debug
    for (auto &kv : m_pFeatureMgr->featureCnt()) {
        const string &key = kv.first;
        auto &featureCnt = kv.second;
        double sumIV = 0.0;
        cout << key << "\t" << featureCnt.totalPosCnt() << "\t" << featureCnt.totalNegCnt() 
                << "\t" << featureCnt.totalIV() << endl;
        for (auto &subkv : featureCnt.values()) {
            const string &val = subkv.first;
            auto &valCnt = subkv.second;
            cout << val << "\t" << valCnt.posCnt() << "\t" << valCnt.negCnt() 
                    << "\t" << valCnt.woe() << "\t" << valCnt.iv() << endl;
            sumIV += valCnt.iv();
        } // for subkv
        assert(sumIV == featureCnt.totalIV());
        cout << endl;
    } // for kv
#endif

    ofstream ofs(m_strOutput, ios::out);
    THROW_RUNTIME_ERROR_IF(!ofs, "IvWoe cannot open " << m_strOutput << " for writting!");

    typedef FeatureCntMgr::FeatureCntMap::value_type    FeatureCntMapValue;
    typedef FeatureCnt::ValueCntMap::value_type         ValueCntMapValue;
    
    // sort featureCnt
    boost::ptr_vector<FeatureCntMapValue, boost::view_clone_allocator> 
            pArrFeatureCnt(m_pFeatureMgr->featureCnt().begin(), m_pFeatureMgr->featureCnt().end());
    pArrFeatureCnt.sort([](const FeatureCntMapValue &lhs, const FeatureCntMapValue &rhs)->bool {
        return lhs.second.totalIV() > rhs.second.totalIV();
    });

    for (auto &kv : pArrFeatureCnt) {
        const string &key = kv.first;
        auto &featureCnt = kv.second;
        ofs << "FeatureName: " << key << "\tTotalIV: " << featureCnt.totalIV() << endl;

        boost::ptr_vector<ValueCntMapValue, boost::view_clone_allocator> 
                pArrValueCnt(featureCnt.values().begin(), featureCnt.values().end());
        pArrValueCnt.sort([](const ValueCntMapValue &lhs, const ValueCntMapValue &rhs)->bool {
            return lhs.second.iv() > rhs.second.iv();
        });

        for (auto &subkv : pArrValueCnt) {
            const string &val = subkv.first;
            auto &valCnt = subkv.second;
            ofs << "value: " << val << "\tWOE: " << valCnt.woe() << "\tIV: " << valCnt.iv() << endl;
        } // for subkv

        ofs << endl;
    } // for kv
}


void IvWoe::caculateIV()
{
    // for every feature value
    for (auto &kv : m_pFeatureMgr->featureCnt()) {
        auto &featureCnt = kv.second;
        uint32_t nTotalGood = featureCnt.totalPosCnt();
        uint32_t nTotalBad  = featureCnt.totalNegCnt();
        for (auto &subkv : featureCnt.values()) {
            auto &valueCnt = subkv.second;
            uint32_t nGood = valueCnt.posCnt();
            uint32_t nBad  = valueCnt.negCnt();
            double dGood = (double)(nGood + 1) / (nTotalGood + 1);
            double dBad  = (double)(nBad  + 1) / (nTotalBad  + 1);
            // double dGood = (double)(nGood) / (nTotalGood);
            // double dBad  = (double)(nBad) / (nTotalBad);
            valueCnt.woe_ = std::log(dGood / dBad);
            valueCnt.iv_  = valueCnt.woe_ * (dGood - dBad);
            featureCnt.totalIV_ += valueCnt.iv_;
        } // for subkv
    } // for kv

}


void IvWoe::loadData()
{
    using namespace std;

    ifstream ifs(m_strTargetFile, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "IvWoe::loadData() cannot open " 
            << m_strTargetFile << " for reading!");

    IFvFile     ifv(m_strInput);
    string      line;
    bool        targetVal = false;
    size_t      recordCnt = 0;
    FeatureVector fv;

    m_pFeatureMgr.reset(new FeatureCntMgr);

    while (ifv.readOne(fv)) {
        if (!getline(ifs, line)) {
            LOG(ERROR) << "Read target file fail for " << recordCnt+1 << " record!";
            continue;
        } // if
        boost::trim(line);
        if (!boost::conversion::try_lexical_convert(line, targetVal)) {
            LOG(ERROR) << "Read target value fail for " << recordCnt+1 << " record! line = " << line;
            continue;
        } // if

        for (auto &kv : fv.stringFeatures) {
            const string &key = kv.first;
            for (const string &val : kv.second) {
                m_pFeatureMgr->addRecord(key, val, targetVal);
            } // for val
        } // for kv

        ++recordCnt;
    } // while

#if 0
    // dump FeatureCntMgr for debug
    for (auto &kv : m_pFeatureMgr->featureCnt()) {
        uint32_t    sum = 0;
        const string &key = kv.first;
        auto &featureCnt = kv.second;
        cout << key << "\t" << featureCnt.totalPosCnt() << "\t" << featureCnt.totalNegCnt() << endl;
        assert(featureCnt.totalPosCnt() + featureCnt.totalNegCnt() == recordCnt);
        for (auto &subkv : featureCnt.values()) {
            const string &val = subkv.first;
            auto &valCnt = subkv.second;
            cout << val << "\t" << valCnt.posCnt() << "\t" << valCnt.negCnt() << endl;
            sum += valCnt.posCnt(); sum += valCnt.negCnt();
        } // for subkv
        cout << endl;
        assert(sum == recordCnt);
    } // for kv
#endif
}

