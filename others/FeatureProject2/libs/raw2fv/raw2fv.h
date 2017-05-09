#ifndef _RAW2FV_H_
#define _RAW2FV_H_

#include "FeatureTask.h"

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class DenseInfo {
public:
    typedef std::shared_ptr<DenseInfo>  pointer;
public:
    DenseInfo(const FeatureInfo &fi, const std::string &dataDir);
    bool readData();
    bool readDataId();
private:
    std::shared_ptr<std::istream>   m_pDenseFile;
    uint32_t                        m_nLen;
    std::string                     m_strId;
    std::vector<double>             m_vecData;
};


class Raw2Fv : public FeatureTask {
public:
    Raw2Fv(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr), m_bHasId(false), m_bSetGlobalDesc(false) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void loadDesc();
    void loadDataWithId();
    void loadDataWithoutId();
    void read_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_string_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_double_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_datetime_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_list_double_feature_id(FeatureVector &fv, 
                FeatureInfo &fi, const std::size_t lineno);
    void parseDense(FeatureInfo &fi);
    bool readDense(std::vector<double> &vec, FeatureInfo &fi);
    bool readDenseId(std::string &id, std::vector<double> &vec, FeatureInfo &fi);

private:
    std::string         m_strDesc, m_strNewDesc, m_strSep;
    bool                m_bHasId, m_bSetGlobalDesc;
    std::shared_ptr<FeatureInfoSet>     m_pFeatureInfoSet;
};


#endif /* _RAW2FV_H_ */

