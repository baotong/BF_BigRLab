#include "Discrete.h"
#include <glog/logging.h>
#include <cassert>
#include <cmath>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include "CommDef.h"
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Discrete(name, mgr); }


void Discrete::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "Discrete::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "Discrete::init() input file not specified!");

    m_nParts = conf["parts"].asUInt();
    THROW_RUNTIME_ERROR_IF(!m_nParts, "Discrete::init() \"parts\" not specified!");
}


void Discrete::run()
{
    DLOG(INFO) << "Discrete::run()";

    LOG(INFO) << "Discrete for " << m_strInput << " ...";
    countValue();
    // printDictCount();
    getRange();
    genNewFv();
    LOG(INFO) << "New feature vector has written to " << m_strOutput;

    m_pTaskMgr->setLastOutput(m_strOutput);
    if (autoRemove()) {
        LOG(INFO) << "Removing " << m_strInput;
        remove_file(m_strInput);
    } // if
}


void Discrete::countValue()
{
    using namespace std;

    IFvFile     ifv(m_strInput);

    FeatureVector fv;
    m_nSample = 0;
    while (ifv.readOne(fv)) {
        for (auto &kv : fv.floatFeatures) {
            const string &key = kv.first;
            auto &submap = kv.second;
            auto it = submap.begin();
            if (it == submap.end()) 
                continue;
            const double &val = it->second;
            auto ret1 = m_dictCount.insert(std::make_pair(key, 
                                    DictCount::mapped_type()));
            auto &mapCnt = ret1.first->second;
            auto ret2 = mapCnt.insert(std::make_pair(val, 0));
            uint32_t &cnt = ret2.first->second;
            ++cnt;
        } // for kv
        ++m_nSample;
    } // while
}


void Discrete::getRange()
{
    using namespace std;

    size_t nPartSz = (size_t)std::ceil( (double)m_nSample / m_nParts );
    DLOG(INFO) << "m_nSample = " << m_nSample;
    DLOG(INFO) << "m_nParts = " << m_nParts;
    DLOG(INFO) << "nPartSz = " << nPartSz;

    vector<double>      arrWork;
    arrWork.reserve(m_nSample);

    for (auto &kv : m_dictCount) {
        const string &key = kv.first;
        DLOG(INFO) << "key = " << key;
        arrWork.clear();
        for (auto &subkv : kv.second) {
            const double& val = subkv.first;
            const uint32_t& cnt = subkv.second;
            arrWork.insert(arrWork.end(), cnt, val);
        } // subkv

        RangeArray   arrRange(m_nParts);
        for (uint32_t i = 0; i < m_nParts; ++i) {
            size_t idxLow = i * nPartSz;
            size_t idxHigh = (i + 1) * nPartSz;
            // DLOG(INFO) << "idxLow = " << idxLow << ", idxHigh = " << idxHigh;
            assert(idxHigh >= idxLow);
            arrRange[i].first = arrWork[idxLow];
            arrRange[i].second = idxHigh < arrWork.size() 
                            ? arrWork[idxHigh] : INFINITY;
        } // for

        arrRange[0].first = -INFINITY;
        arrRange.back().second = INFINITY;

#ifndef NDEBUG
        for (auto &v : arrRange)
            DLOG(INFO) << boost::format("[%lf,%lf)") % v.first % v.second;
#endif

        // merge
        if (arrRange.size() > 1) {
            for (auto it = arrRange.begin(); (it + 1) != arrRange.end();) {
                if (*it == *(it + 1)) {
                    it = arrRange.erase(it);
                } else {
                    ++it;
                } // if
            } // for it
        } // if

        // set name
        for (auto &v : arrRange)
            v.setName();

#ifndef NDEBUG
        DLOG(INFO) << "After merge range:";
        for (auto &v : arrRange)
            DLOG(INFO) << v.name();
#endif

        auto ret = m_dictRange.insert(std::make_pair(key, FeatureRange::mapped_type()));
        ret.first->second.swap(arrRange);
    } // for kv
}


void Discrete::genNewFv()
{
    using namespace std;

    LOG(INFO) << "Creating new feature vector...";

    IFvFile ifv(m_strInput);
    OFvFile ofv(m_strOutput);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        FeatureVectorHandle hfv(fv);

        for (auto fit = fv.floatFeatures.begin(); fit != fv.floatFeatures.end();) {
            const string &key = fit->first;
            if (fit->second.empty()) {
                LOG(ERROR) << "Empty value for feature " << key;
                fit = fv.floatFeatures.erase(fit);
                continue;
            } // if
            const double &val = fit->second.begin()->second;
            auto it = m_dictRange.find(key);
            if (it == m_dictRange.end()) {
                LOG(ERROR) << "Cannot find feature name " << key << " in dictRange";
                fit = fv.floatFeatures.erase(fit);
                continue;
            } // if
            const RangeArray &arrRange = it->second;
            const string& newVal = range(arrRange, val);
            string newFtName = key + "_sec";
            hfv.setFeature(newFtName, newVal);
            fit = fv.floatFeatures.erase(fit);
        } // for fit

        if (fv.floatFeatures.empty())
            fv.__isset.floatFeatures = false;

        ofv.writeOne(fv);
    } // while
}


void Discrete::printDictCount()
{
    using namespace std;

    for (auto &kv : m_dictCount) {
        const string &key = kv.first;
        cout << key << ":" << endl;
        for (auto &subkv : kv.second)
            cout << "\t" << subkv.first << "\t" << subkv.second << endl;
        cout << endl;
    } // for kv
}

