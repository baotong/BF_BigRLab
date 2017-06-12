#include "FormatOutput.h"
#include <glog/logging.h>
#include <cassert>
#include <fstream>
#include <boost/filesystem.hpp>
#include "CommDef.h"
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new FormatOutput(name, mgr); }


void FormatOutput::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "FormatOutput::init()";

    THROW_RUNTIME_ERROR_IF(input().empty(), "FormatOutput::init() input file not specified!");

    m_strInfo = conf["info"].asString();
    THROW_RUNTIME_ERROR_IF(m_strInfo.empty(), "Normalize::init() info file not specified!");
    fs::path dataPath(m_pTaskMgr->dataDir());
    m_strInfo = (dataPath / m_strInfo).c_str();
}


void FormatOutput::run()
{
    DLOG(INFO) << "FormatOutput::run()";

    LOG(INFO) << "Formatting data " << m_strInput << " ...";
    m_pFtIdx = load_info(m_strInfo);
    doWork();
    LOG(INFO) << "Result data has written to " << m_strOutput;
    // m_pTaskMgr->setLastOutput(m_strOutput);

    if (autoRemove()) {
        LOG(INFO) << "Removing " << m_strInput;
        remove_file(m_strInput);
    } // if
}


void FormatOutput::doWork()
{
    using namespace std;

    ofstream ofs(m_strOutput, ios::out | ios::trunc);
    THROW_RUNTIME_ERROR_IF(!ofs, "FormatOutput::doWork() cannot open " << m_strOutput << " for writtingt!");

    FeatureIndexHandle hFi(*m_pFtIdx);

    FeatureVector fv;
    IFvFile ifv(m_strInput);
    while (ifv.readOne(fv)) {
        // print stringFeatures
        int32_t idx = 0;
        for (auto &kv : fv.stringFeatures) {
            const string &key = kv.first;
            for (auto &val : kv.second) {
                if (!hFi.getInfo(key, val, idx)) {
                    LOG(WARNING) << "FormatOutput cannot find info of string feature " << key << ":" << val;
                    continue;
                } // if
                ofs << (uint32_t)idx << ":" << "1 ";
            } // for val
        } // for kv

        // print floatFeatures
        FloatInfo *pFInfo = NULL;
        for (auto &kv : fv.floatFeatures) {
            const string &key = kv.first;
            for (auto &subkv : kv.second) {
                const string &subkey = subkv.first;
                const double &val = subkv.second;
                if (!hFi.getInfo(key, subkey, pFInfo)) {
                    LOG(WARNING) << "FormatOutput cannot find info of float feature " << key << ":" << subkey;
                    continue;
                } // if
                ofs << (uint32_t)(pFInfo->index) << ":" << val << " ";
            } // for subkv
        } // for kv

        // print denseFeatures
        DenseInfo *pDInfo = NULL;
        for (auto &kv : fv.denseFeatures) {
            const string &key = kv.first;
            const auto &vec = kv.second;
            if (!hFi.getInfo(key, pDInfo)) {
                LOG(WARNING) << "FormatOutput cannot find info of dense feature " << key;
                continue;
            } // if
            uint32_t startIdx = (uint32_t)(pDInfo->startIdx); 
            for (uint32_t i = 0; i < vec.size(); ++i) {
                if (vec[i])
                    ofs << (startIdx + i) << ":" << vec[i] << " ";
            } // for i
        } // for kv
        ofs << endl;
    } // while
}


