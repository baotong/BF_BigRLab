#ifndef _ARTICLE2VEC_PROXY_H_
#define _ARTICLE2VEC_PROXY_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class Article2vecProxy : public FeatureTask {
public:
    Article2vecProxy(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;
private:
    std::string     m_strMethod, m_strRef;
};

#endif /* _ARTICLE2VEC_PROXY_H_ */

