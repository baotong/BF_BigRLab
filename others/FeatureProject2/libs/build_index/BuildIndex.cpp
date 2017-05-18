#include "BuildIndex.h"
#include <glog/logging.h>
#include <cassert>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "CommDef.h"
#include "FvFile.h"
#include "ThriftFile.hpp"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new BuildIndex(name, mgr); }


void BuildIndex::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "BuildIndex::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "BuildIndex::init() input file not specified!");
}


void BuildIndex::run()
{
    DLOG(INFO) << "BuildIndex::run()";

    LOG(INFO) << "Building index for " << m_strInput << " ...";
    buildIdx();
    LOG(INFO) << "Index file has written to " << m_strOutput;
}


struct StringIndexMapHandle {
    StringIndexMapHandle(FeatureIndexHandle::StringIndexMap& _Data)
            : ref_(_Data) {}

    int32_t& add(const std::string &key, const std::string &subKey)
    {
        assert(key != "" && subKey != "");
        auto ret1 = ref_.insert(std::make_pair(key, FeatureIndexHandle::StringIndexMap::mapped_type()));
        auto& subMap = ret1.first->second;
        auto ret2 = subMap.insert(std::make_pair(subKey, 0));
        return ret2.first->second;
    }

    FeatureIndexHandle::StringIndexMap&      ref_;
};


struct FloatInfoMapHandle {
    FloatInfoMapHandle(FeatureIndexHandle::FloatInfoMap& _Data)
            : ref_(_Data) {}

    FloatInfo& add(const std::string &key, const std::string &subKey)
    {
        auto ret1 = ref_.insert(std::make_pair(key, FeatureIndexHandle::FloatInfoMap::mapped_type()));
        auto& subMap = ret1.first->second;
        auto ret2 = subMap.insert(std::make_pair(subKey, 
                    FeatureIndexHandle::FloatInfoMap::mapped_type::mapped_type()));
        return ret2.first->second;
    }

    FeatureIndexHandle::FloatInfoMap&        ref_;
};


struct DenseInfoMapHandle {
    DenseInfoMapHandle(FeatureIndexHandle::DenseInfoMap &_Data)
            : ref_(_Data) {}

    DenseInfo& add(const std::string &key)
    {
        auto ret = ref_.insert(std::make_pair(key, FeatureIndexHandle::DenseInfoMap::mapped_type()));
        return ret.first->second;
    }

    FeatureIndexHandle::DenseInfoMap&        ref_;
};


void BuildIndex::buildIdx()
{
    using namespace std;

    FeatureIndexHandle::StringIndexMap      stringIndices;
    FeatureIndexHandle::FloatInfoMap        floatInfo;
    FeatureIndexHandle::DenseInfoMap        denseInfo;

    IFvFile     ifv(m_strInput);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        // FeatureVectorHandle     hfv(fv);
        
        // stringFeatures
        StringIndexMapHandle    hStringIndices(stringIndices);
        for (auto &kv : fv.stringFeatures) {
            const string &key = kv.first;
            for (auto &val : kv.second)
                hStringIndices.add(key, val);
        } // for kv

        // floatFeatures
        FloatInfoMapHandle      hFloatInfo(floatInfo);
        for (auto &kv : fv.floatFeatures) {
            const string &key = kv.first;
            for (auto &subkv : kv.second) {
                const string &subKey = subkv.first;
                const double &val = subkv.second;
                auto &fInfo = hFloatInfo.add(key, subKey);
                assert(fInfo.index == 0);
                // set min & max
                fInfo.minVal = val < fInfo.minVal ? val : fInfo.minVal;
                fInfo.maxVal = val > fInfo.maxVal ? val : fInfo.maxVal;
            } // for subkv
        } // for kv

        // denseFeatures
        DenseInfoMapHandle hDenseInfo(denseInfo);
        for (auto &kv : fv.denseFeatures) {
            const string &key = kv.first;
            auto denseLen = kv.second.size();
            auto &dInfo = hDenseInfo.add(key);
            // get len
            dInfo.len = (int32_t)(denseLen > (size_t)(dInfo.len) ? denseLen : dInfo.len);
        } // for kv
    } // while

    uint32_t idx = 0;
    // assign index to stringFeatures
    for (auto &kv : stringIndices) {
        for (auto &subkv : kv.second)
            subkv.second = idx++;
    } // for kv
    // assign index to floatFeatures
    for (auto &kv : floatInfo) {
        for (auto &subkv : kv.second)
            subkv.second.index = idx++;
    } // for kv
    // assign index to denseFeatures
    for (auto &kv : denseInfo) {
        auto &dInfo = kv.second;
        dInfo.startIdx = idx;
        idx += dInfo.len;
    } // for kv

    FeatureIndex        ftIdx;
    FeatureIndexHandle  hFtIdx(ftIdx);
    hFtIdx.setStringIndices(stringIndices);
    hFtIdx.setFloatInfo(floatInfo);
    hFtIdx.setDenseInfo(denseInfo);

    // ofstream ofs(m_strOutput, ios::out);
    // THROW_RUNTIME_ERROR_IF(!ofs, "BuildIndex cannot open " << m_strOutput << " for writting!");
    // ofs << ftIdx << flush;
    
    OThriftFile<FeatureIndex>    ofi(m_strOutput);
    ofi.writeOne(ftIdx);
}


