#include "normalize.h"
#include <glog/logging.h>
#include <cassert>
#include "CommDef.h"
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Normalize(name, mgr); }


void Normalize::init(const Json::Value &conf)
{
    using namespace std;

    FeatureTask::init(conf);
    DLOG(INFO) << "Normalize::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "Normalize::init() input file not specified!");
}


void Normalize::run()
{
    DLOG(INFO) << "Normalize::run()";

    LOG(INFO) << "Normalizing data " << m_strInput << " ...";
    doNormalize();
    LOG(INFO) << "Feature vector data has written to " << m_strOutput;
    m_pTaskMgr->setLastOutput(m_strOutput);

    if (autoRemove()) {
        LOG(INFO) << "Removing " << m_strInput;
        remove_file(m_strInput);
    } // if
}


void Normalize::doNormalize()
{
    using namespace std;

    IFvFile ifv(m_strInput);
    OFvFile ofv(m_strOutput);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        for (auto &kv : fv.floatFeatures) {
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
        ofv.writeOne(fv);
    } // while
}


