#include <glog/logging.h>
#include "raw2fv.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Raw2Fv(name, mgr); }


Raw2Fv::~Raw2Fv()
{
    DLOG(INFO) << "Raw2Fv destructor";
}


void Raw2Fv::init(const Json::Value &conf)
{
    FeatureTask::init(conf);
    DLOG(INFO) << "Raw2Fv::init()";
}


void Raw2Fv::run()
{
    DLOG(INFO) << "Raw2Fv::run()";
}

