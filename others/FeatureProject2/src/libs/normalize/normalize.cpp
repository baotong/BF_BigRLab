#include "normalize.h"
#include <glog/logging.h>
#include <cassert>
#include <boost/filesystem.hpp>
#include "CommDef.h"
#include "FvFile.h"
#include "ThriftFile.hpp"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Normalize(name, mgr); }


void Normalize::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);
    DLOG(INFO) << "Normalize::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "Normalize::init() input file not specified!");

    m_strInfo = conf["info"].asString();
    THROW_RUNTIME_ERROR_IF(m_strInfo.empty(), "Normalize::init() info file not specified!");
    fs::path dataPath(m_pTaskMgr->dataDir());
    m_strInfo = (dataPath / m_strInfo).c_str();
}


void Normalize::run()
{
    DLOG(INFO) << "Normalize::run()";

    LOG(INFO) << "Normalizing data " << m_strInput << " ...";
    m_pFtIdx = load_info(m_strInfo);
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

    FeatureIndexHandle hFi(*m_pFtIdx);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        for (auto &kv : fv.floatFeatures) {
            const string &key = kv.first;
            for (auto &subkv : kv.second) {
                const string &subkey = subkv.first;
                FloatInfo *pInfo = NULL;
                if (!hFi.getInfo(key, subkey, pInfo)) {
                    LOG(WARNING) << "Normalize cannot find info of " << key << ":" << subkey;
                    continue;
                } // if
                double &val = subkv.second;
                double range = pInfo->maxVal - pInfo->minVal;
                if (range == 0.0) {
                    val = 0.0;
                } else {
                    val -= pInfo->minVal;
                    val /= range;
                    if (val < 0.0) val = 0.0;
                    else if (val > 1.0) val = 1.0;
                } // if range
            } // for subkv
        } // for kv
        ofv.writeOne(fv);
    } // while
}


