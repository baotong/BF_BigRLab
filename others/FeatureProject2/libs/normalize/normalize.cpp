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

    FeatureTask::init(conf);

    DLOG(INFO) << "Normalize::init()";
    fs::path dataPath(m_pTaskMgr->dataDir());

    THROW_RUNTIME_ERROR_IF(input().empty(), "Normalize::init() input file not specified!");
    m_strInput = (dataPath / m_strInput).c_str();

    THROW_RUNTIME_ERROR_IF(output().empty(), "Normalize::init() output file not specified!");
    m_strOutput = (dataPath / m_strOutput).c_str();
}


void Normalize::run()
{
    DLOG(INFO) << "Normalize::run()";

    LOG(INFO) << "Feature vector data has written to " << m_strOutput;
    m_pTaskMgr->setLastOutput(m_strOutput);
}



