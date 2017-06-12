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
private:
    std::string     m_strConcur, m_strMark, m_strInfo;
    std::map<std::string, double>   m_ItemProbTable;
    std::shared_ptr<FeatureIndex>   m_pFtIdx;
};


#endif /* _FORMAT_OUTPUT_H_ */

