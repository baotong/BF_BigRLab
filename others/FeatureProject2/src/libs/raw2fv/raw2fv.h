#ifndef _RAW2FV_H_
#define _RAW2FV_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class Raw2Fv : public FeatureTask {
public:
    Raw2Fv(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void loadDesc();
    void loadDataWithId();
    void loadDataWithoutId();
    // void genIdx();
    // void writeDesc();
    bool read_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    bool read_string_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    bool read_double_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    bool read_datetime_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    // void read_list_double_feature_id(FeatureVector &fv, 
                // FeatureInfo &fi, const std::size_t lineno, DenseInfo &denseInfo);
    // void read_list_double_feature(FeatureVector &fv, 
                // FeatureInfo &fi, const std::size_t lineno, DenseInfo &denseInfo);

private:
    std::string         m_strDesc, m_strNewDesc, m_strSep;
    std::shared_ptr<FeatureInfoSet>     m_pFeatureInfoSet;
    // std::vector<DenseInfo::pointer>     m_arrDenseInfo;
};


#endif /* _RAW2FV_H_ */

