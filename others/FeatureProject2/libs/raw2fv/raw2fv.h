#ifndef _RAW2FV_H_
#define _RAW2FV_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


class DenseInfo {
public:
    typedef std::shared_ptr<DenseInfo>  pointer;
public:
    DenseInfo(FeatureInfo &fi, const std::string &dataDir, bool _HasId);
    uint32_t getDenseLen(const char *fname);
    bool readData();
    std::string& id() { return m_strId; }
    std::vector<double>& data() { return m_vecData; }
private:
    bool readDataNoId();
    bool readDataId();
private:
    std::shared_ptr<std::istream>   m_pDenseFile;
    uint32_t                        m_nLen;
    std::string                     m_strId;
    std::vector<double>             m_vecData;
    bool                            m_bHasId;
};


class Raw2Fv : public FeatureTask {
public:
    Raw2Fv(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr), m_bHasId(false) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void loadDesc();
    void loadDataWithId();
    void loadDataWithoutId();
    void genIdx();
    void writeDesc();
    void read_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_string_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_double_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_datetime_feature(FeatureVector &fv, std::string &strField, 
                FeatureInfo &ftInfo, const std::size_t lineno);
    void read_list_double_feature_id(FeatureVector &fv, 
                FeatureInfo &fi, const std::size_t lineno, DenseInfo &denseInfo);
    void read_list_double_feature(FeatureVector &fv, 
                FeatureInfo &fi, const std::size_t lineno, DenseInfo &denseInfo);

private:
    std::string         m_strDesc, m_strNewDesc, m_strSep;
    bool                m_bHasId;
    std::vector<DenseInfo::pointer>     m_arrDenseInfo;
};


#endif /* _RAW2FV_H_ */

