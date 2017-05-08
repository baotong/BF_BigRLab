#ifndef _RAW2FV_H_
#define _RAW2FV_H_

#include "FeatureTask.h"

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class Raw2Fv : public FeatureTask {
public:
    Raw2Fv(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}
    ~Raw2Fv() override;

    void init(const Json::Value &conf) override;
    void run() override;
};


#endif /* _RAW2FV_H_ */

