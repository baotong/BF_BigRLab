#ifndef _NORMALIZE_H_
#define _NORMALIZE_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class Normalize : public FeatureTask {
public:
    Normalize(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;
private:
    void doNormalize();
    void loadDesc(const std::string &fname);
};

#endif /* _NORMALIZE_H_ */

