#ifndef _BUILD_INDEX_H_
#define _BUILD_INDEX_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class BuildIndex : public FeatureTask {
public:
    BuildIndex(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void buildIdx();

private:
    std::string     m_strInfo;
};

#endif /* _BUILD_INDEX_H_ */

