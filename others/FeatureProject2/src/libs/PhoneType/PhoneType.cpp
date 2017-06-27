#include "PhoneType.h"
#include <glog/logging.h>
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new PhoneType(name, mgr); }


void PhoneType::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "PhoneType::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "PhoneType::init() input file not specified!");
}


void PhoneType::run()
{
    DLOG(INFO) << "PhoneType::run()";
    doWork();
    LOG(INFO) << "Results has written to " << m_strOutput;
    m_pTaskMgr->setLastOutput(m_strOutput);

    if (autoRemove()) {
        LOG(INFO) << "Removing " << m_strInput;
        remove_file(m_strInput);
    } // if
}


void PhoneType::doWork()
{
    using namespace std;

    LOG(INFO) << "Processing input file: " << m_strInput;

    IFvFile ifv(m_strInput);
    OFvFile ofv(m_strOutput);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        auto ret = fv.stringFeatures.emplace(std::make_pair("phonetype", StringSet()));
        auto& valSet = ret.first->second;
        if (valSet.empty()) {
            valSet.emplace("UNKNOWN");
        } else {
            const string &oldVal = *(valSet.begin());
            if (!m_setKnownTypes.count(oldVal)) {
                string newVal = oldVal.empty() ? "UNKNOWN" : "OTHER";
                valSet.erase(oldVal);
                valSet.emplace(newVal);
            } // if
        } // if
        fv.__isset.stringFeatures = true;
        ofv.writeOne(fv);
    } // while
}
