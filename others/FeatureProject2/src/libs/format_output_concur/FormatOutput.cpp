#include "FormatOutput.h"
#include <glog/logging.h>
#include <cassert>
#include <fstream>
#include <boost/filesystem.hpp>
#include "CommDef.h"
#include "FvFile.h"
#include "concur_table.hpp"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new FormatOutput(name, mgr); }


void FormatOutput::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "FormatOutput::init()";

    THROW_RUNTIME_ERROR_IF(input().empty(), "FormatOutput::init() input file not specified!");

    fs::path dataPath(m_pTaskMgr->dataDir());

    m_strConcur = conf["concur"].asString();
    THROW_RUNTIME_ERROR_IF(m_strConcur.empty(), "FormatOutput::init() concur_file not specified!");
    m_strConcur = (dataPath / m_strConcur).c_str();

    m_strMark = conf["mark"].asString();
    THROW_RUNTIME_ERROR_IF(m_strMark.empty(), "FormatOutput::init() mark not specified!");

    m_strInfo = conf["info"].asString();
    THROW_RUNTIME_ERROR_IF(m_strInfo.empty(), "Normalize::init() info file not specified!");
    m_strInfo = (dataPath / m_strInfo).c_str();
}


void FormatOutput::run()
{
    DLOG(INFO) << "FormatOutput::run()";

    LOG(INFO) << "Loading concur table...";
    auto pConcurTable = std::make_shared<ConcurTable>();
    pConcurTable->loadFromFile(m_strConcur);
    pConcurTable->getItemProb(m_ItemProbTable, m_strMark);
    pConcurTable.reset();

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
        auto &probTable = m_ItemProbTable;
        for (auto &kv : fv.stringFeatures) {
            const string &key = kv.first;
            for (auto &val : kv.second) {
                if (!hFi.getInfo(key, val, idx)) {
                    LOG(WARNING) << "FormatOutput cannot find info of string feature " << key << ":" << val;
                    continue;
                } // if
                const auto it = probTable.find(val);
                if (it == probTable.end()) {
                    LOG(WARNING) << "FormatOutput cannot find " << val << " in probTable";
                    continue;
                } // if
                ofs << (uint32_t)idx << ":" << it->second << " ";
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


