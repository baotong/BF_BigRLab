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

    m_nWorkers = conf["workers"].asUInt();
    if (!m_nWorkers) {
        m_nWorkers = std::thread::hardware_concurrency();
        LOG(INFO) << "Number of workers not set, auto set to " << m_nWorkers;
    } // if
    DLOG(INFO) << "nWorkers = " << m_nWorkers; 

    uint32_t iBufSz = conf["input_buf"].asUInt();
    if (!iBufSz) iBufSz = 15000;
    m_pInputBuffer.reset(new SharedQueue<InputPtr>(iBufSz));
    DLOG(INFO) << "Input buffer size = " << iBufSz;
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
    m_pFeatureMgr.reset(new FeatureCntMgr);

    m_pThrReader.reset(new std::thread([&, this]{
        size_t      recordCnt = 0;
        auto pInput = std::make_shared<Input>();

        while (ifv.readOne(pInput->fv_)) {
            if (!getline(ifs, pInput->target_)) {
                LOG(ERROR) << "Read target file fail for " << recordCnt+1 << " record!";
                continue;
            } // if

            m_pInputBuffer->push(pInput);

            pInput = std::make_shared<Input>();
            ++recordCnt;
        } // while
    }));

    for (uint32_t i = 0; i < m_nWorkers; ++i) {
        m_ThrWorkers.create_thread([&, this]{
            InputPtr pInput;
            while (pInput = m_pInputBuffer->pop()) {
                FeatureVector &fv = pInput->fv_;
                string &line = pInput->target_;
                bool        targetVal = false;

                boost::trim(line);
                if (!boost::conversion::try_lexical_convert(line, targetVal)) {
                    continue;
                } // if

                for (auto &kv : fv.stringFeatures) {
                    const string &key = kv.first;
                    for (const string &val : kv.second) {
                        m_pFeatureMgr->addRecord(key, val, targetVal);
                    } // for val
                } // for kv
            } // while 
        });
    } // for i

    m_pThrReader->join();
    for (uint32_t i = 0; i < m_nWorkers; ++i)
        m_pInputBuffer->push(nullptr);
    m_ThrWorkers.join_all();
}

