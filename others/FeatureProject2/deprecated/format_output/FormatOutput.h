#ifndef _FORMAT_OUTPUT_H_
#define _FORMAT_OUTPUT_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class FormatOutput : public FeatureTask {
public:
    FormatOutput(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;
private:
    void doWork();
    void printFeature(std::ostream &os, const FeatureVector &fv, const FeatureInfo &fi);
    void loadDesc(const std::string &fname);
};


#endif /* _FORMAT_OUTPUT_H_ */

