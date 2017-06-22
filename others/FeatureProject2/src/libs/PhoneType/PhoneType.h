#ifndef _PHONE_TYPE_H_
#define _PHONE_TYPE_H_ 

#include "FeatureTask.h"


using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class PhoneType : public FeatureTask {
public:
    typedef std::set<std::string>       StringSet;

public:
    PhoneType(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void doWork();

private:
    const StringSet m_setKnownTypes = {"APPLE", "OPPO", "XIAOMI", "UNCATEGORIED", "HUAWEI", "VIVO", "SAMSUNG", "MEIZU", "MOTO", "LETV"};
};

#endif /* _PHONE_TYPE_H_ */

