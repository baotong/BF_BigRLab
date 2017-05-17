#ifndef _MERGE_DENSE_H_
#define _MERGE_DENSE_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class MergeDense : public FeatureTask {
public:
    MergeDense(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void mergeWithId();
    void mergeWithoutId();

private:
    std::string         m_strDense, m_strFeature;
};

#endif /* _MERGE_DENSE_H_ */

