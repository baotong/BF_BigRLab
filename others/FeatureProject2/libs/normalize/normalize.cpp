#include "normalize.h"
#include <glog/logging.h>
#include <cassert>
#include <boost/filesystem.hpp>
#include "CommDef.h"
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Normalize(name, mgr); }


void Normalize::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "Normalize::init()";

    THROW_RUNTIME_ERROR_IF(input().empty(), "Normalize::init() input file not specified!");
    THROW_RUNTIME_ERROR_IF(output().empty(), "Normalize::init() output file not specified!");

    fs::path dataPath(m_pTaskMgr->dataDir());
    if (!m_pFeatureInfoSet) {
        string descFile = conf["desc"].asString();
        THROW_RUNTIME_ERROR_IF(descFile.empty(), "No data desc loaded!");
        descFile = (dataPath / descFile).c_str();
        loadDesc(descFile);
    } // if
}


void Normalize::run()
{
    DLOG(INFO) << "Normalize::run()";

    LOG(INFO) << "Normalizing data " << m_strInput << " ...";
    doNormalize();
    LOG(INFO) << "Feature vector data has written to " << m_strOutput;
    m_pTaskMgr->setLastOutput(m_strOutput);
}


void Normalize::doNormalize()
{
    using namespace std;

    std::shared_ptr<FeatureVector> pfv(new FeatureVector);

    IFvFile ifv(m_strInput);
    OFvFile ofv(m_strOutput);

    while (ifv.readOne(*pfv)) {
        for (auto &kv : pfv->floatFeatures) {
            const string &ftName = kv.first;
            auto pFtInfo = m_pFeatureInfoSet->get(ftName);
            THROW_RUNTIME_ERROR_IF(!pFtInfo,
                    "do_normalize() data inconsistency!, no feature info \"" << ftName << "\" found!");
            THROW_RUNTIME_ERROR_IF(kv.second.empty(), "do_normalize() invalid data!");
            for (auto &subkv : kv.second) {
                const string &subFtName = subkv.first;
                auto minmax = pFtInfo->minMax(subFtName);
                double range = minmax.second - minmax.first;
                double &val = subkv.second;
                if (range == 0.0) {
                    val = 0.0;
                } else {
                    val -= minmax.first;
                    val /= range;
                    if (val < 0.0) val = 0.0;
                    else if (val > 1.0) val = 1.0;
                } // if range
            } // for subkv
        } // for kv
        ofv.writeOne(*pfv);
        pfv.reset(new FeatureVector);
    } // while
}


void Normalize::loadDesc(const std::string &fname)
{
    THROW_RUNTIME_ERROR("Normalize::loadDesc() TODO");
}

