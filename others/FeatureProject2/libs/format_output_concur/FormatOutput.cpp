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
    THROW_RUNTIME_ERROR_IF(output().empty(), "FormatOutput::init() output file not specified!");

    fs::path dataPath(m_pTaskMgr->dataDir());
    if (!m_pFeatureInfoSet) {
        string descFile = conf["desc"].asString();
        THROW_RUNTIME_ERROR_IF(descFile.empty(), "No data desc loaded!");
        descFile = (dataPath / descFile).c_str();
        loadDesc(descFile);
    } // if

    m_strConcur = conf["concur_file"].asString();
    THROW_RUNTIME_ERROR_IF(m_strConcur.empty(), "FormatOutput::init() concur_file not specified!");
    m_strConcur = (dataPath / m_strConcur).c_str();

    m_strMark = conf["mark"].asString();
    THROW_RUNTIME_ERROR_IF(m_strMark.empty(), "FormatOutput::init() mark not specified!");
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
    doWork();
    LOG(INFO) << "Result data has written to " << m_strOutput;
    // m_pTaskMgr->setLastOutput(m_strOutput);
}


void FormatOutput::doWork()
{
    using namespace std;

    ofstream ofs(m_strOutput, ios::out | ios::trunc);
    THROW_RUNTIME_ERROR_IF(!ofs, "FormatOutput::doWork() cannot open " << m_strOutput << " for writtingt!");

    std::shared_ptr<FeatureVector> pfv(new FeatureVector);

    IFvFile ifv(m_strInput);
    while (ifv.readOne(*pfv)) {
        for (auto &pfi : m_pFeatureInfoSet->arrFeature())
            printFeature(ofs, *pfv, *pfi);
        ofs << endl;
        pfv.reset(new FeatureVector);
    } // while
}


void FormatOutput::printFeature(std::ostream &os, const FeatureVector &fv, const FeatureInfo &fi)
{
    if (!fi.isKeep()) return;

    auto &probTable = m_ItemProbTable;
    if (fi.type() == "string") {
        const auto it = fv.stringFeatures.find(fi.name());
        if (it == fv.stringFeatures.end())
            return;
        for (const auto &s : it->second) {
            const auto pSubFt = fi.subFeature(s);
            if (!pSubFt) continue;
            auto idx = pSubFt->index();
            const auto it = probTable.find(pSubFt->name());
            if (it == probTable.end()) {
                LOG(ERROR) << "Cannot find " << pSubFt->name() << " in probTable";
                continue;
            } // if
            os << idx << ":" << it->second << " ";
        } // for
    } else if (fi.type() == "double" || fi.type() == "datetime") {
        const auto it = fv.floatFeatures.find(fi.name());
        if (it == fv.floatFeatures.end())
            return;
        for (const auto &kv : it->second) {
            const auto pSubFt = fi.subFeature(kv.first);
            if (!pSubFt) continue;
            auto idx = pSubFt->index();
            os << idx << ":" << kv.second << " ";
        } // for
    } else if (fi.type() == "list_double") {
        const auto it = fv.denseFeatures.find(fi.name());
        if (it == fv.denseFeatures.end())
            return;
        const auto &arr = it->second;
        for (std::size_t i = 0; i < arr.size(); ++i) {
            if (arr[i])
                os << i + fi.startIdx() << ":" << arr[i] << " ";
        } // for i
    } // if
}


void FormatOutput::loadDesc(const std::string &fname)
{
    THROW_RUNTIME_ERROR("FormatOutput::loadDesc() TODO");
}

