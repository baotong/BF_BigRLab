#ifndef _FEATURE_H_
#define _FEATURE_H_

#include <iostream>
#include <memory>
#include <json/json.h>
#include <boost/filesystem.hpp>
#include "FeatureVector_types.h"
#include "CommDef.h"


#define SPACES                " \t\f\r\v\n"
#define VALID_TYPES     {"string", "double", "list_double", "datetime"}


namespace FeatureProject {


class FeatureInfo {
public:
    typedef std::shared_ptr<FeatureInfo>  pointer;

public:
    FeatureInfo() : m_bMulti(false), m_bKeep(true) {}
    FeatureInfo(const std::string &_Name, const std::string &_Type)
            : m_strName(_Name), m_strType(_Type), m_bMulti(false), m_bKeep(true) {}

    std::string& name() { return m_strName; }
    const std::string& name() const { return m_strName; }
    std::string& type() { return m_strType; }
    const std::string& type() const { return m_strType; }
    bool isMulti() const { return m_bMulti; }
    void setMulti(bool multi = true) { m_bMulti = multi; }
    bool isKeep() const { return m_bKeep; }
    void setKeep(bool keep = true) { m_bKeep = keep; }
    std::string& sep() { return m_strSep; }
    const std::string& sep() const { return m_strSep; }

    void fromJson(const Json::Value &jv)
    {
        m_strName = jv["name"].asString();
        m_strType = jv["type"].asString();
        m_strSep = jv["sep"].asString();
        m_bMulti = jv["multi"].asBool();
        auto &jKeep = jv["keep"];
        if (!!jKeep) m_bKeep = jKeep.asBool();
    }

private:
    std::string                 m_strName;
    std::string                 m_strType;
    bool                        m_bMulti;
    bool                        m_bKeep;
    std::string                 m_strSep;    // empty means default SPACE

public:
    friend std::ostream& operator << (std::ostream &os, const FeatureInfo &fi)
    {
        os << "name = " << fi.name() << std::endl;
        os << "type = " << fi.type() << std::endl;
        os << "multi = " << fi.isMulti() << std::endl;
        if (!fi.sep().empty())
            os << "sep = " << fi.sep() << std::endl;
        os << "keep = " << fi.isKeep() << std::endl;
        os << std::endl;
        return os;
    }
};


class FeatureInfoSet {
public:
    void clear()
    {
        m_arrFeatureInfo.clear();
        m_mapFeatureInfo.clear();
    }

    bool add(const FeatureInfo::pointer &pf)
    {
        auto ret = m_mapFeatureInfo.insert(std::make_pair(pf->name(), pf));
        if (!ret.second) return false;
        m_arrFeatureInfo.emplace_back(pf);
        return true;
    }

    bool get(const std::string &name, FeatureInfo &fi)
    {
        auto it = m_mapFeatureInfo.find(name);
        if (it == m_mapFeatureInfo.end()) return false;
        fi = *(it->second);
        return true;
    }

    FeatureInfo::pointer get(const std::string &name) const
    {
        auto it = m_mapFeatureInfo.find(name);
        if (it != m_mapFeatureInfo.end())
            return it->second;
        return nullptr;
    }

    FeatureInfo::pointer operator[](std::size_t pos) const
    { return m_arrFeatureInfo.at(pos); }

    std::vector<FeatureInfo::pointer>& arrFeature()
    { return m_arrFeatureInfo; }
    const std::vector<FeatureInfo::pointer>& arrFeature() const
    { return m_arrFeatureInfo; }

    std::size_t size() const
    { return m_arrFeatureInfo.size(); }

private:
    std::vector<FeatureInfo::pointer>             m_arrFeatureInfo;
    std::map<std::string, FeatureInfo::pointer>   m_mapFeatureInfo;
};


// FeatureVector代表一行记录
class FeatureVectorHandle {
public:
    typedef std::vector<double>             ListDouble;
    typedef std::set<std::string>           StringSet;
    typedef std::map<std::string, double>   FloatDict;

public:
    FeatureVectorHandle(FeatureVector &fv) 
            : m_refFv(fv) {}

    operator FeatureVector& () { return m_refFv; }

    const std::string& id() const { return m_refFv.id; }

    void setId(const std::string &_Id)
    {
        m_refFv.id = _Id;
        m_refFv.__isset.id = true;
    }

    // string feature
    bool getFeature(const std::string &name, StringSet *&pRet)
    {
        auto it = m_refFv.stringFeatures.find(name);
        if (it == m_refFv.stringFeatures.end()) 
            return false;
        pRet = &(it->second);
        return true;
    }

    // double feature
    bool getFeature(const std::string &name, FloatDict *&pRet)
    {
        auto it = m_refFv.floatFeatures.find(name);
        if (it == m_refFv.floatFeatures.end()) 
            return false;
        pRet = &(it->second);
        return true;
    }

    // list double feature
    bool getFeature(const std::string &name, ListDouble *&pRet)
    {
        auto it = m_refFv.denseFeatures.find(name);
        if (it == m_refFv.denseFeatures.end()) 
            return false;
        pRet = &(it->second);
        return true;
    }

    // 单值string feature
    bool getFeatureValue(const std::string &name, std::string &value)
    {
        StringSet *pStrSet = NULL;
        bool ret = getFeature(name, pStrSet);
        if (!ret || pStrSet->empty()) return false;
        value = *(pStrSet->begin());
        return true;
    }

    // 查看 string feature 是否存在 0 1 值类型
    bool hasFeatureValue(const std::string &name, const std::string &subName)
    {
        StringSet *pStrSet = NULL;
        bool ret = getFeature(name, pStrSet);
        if (!ret) return false;
        return pStrSet->count(subName);
    }

    // get double value
    bool getFeatureValue(double &value, const std::string &name, const std::string &subName = "")
    {
        FloatDict *pfDict = NULL;
        bool ret = getFeature(name, pfDict);
        if (!ret) return false;
        auto it = pfDict->find(subName);
        if (it == pfDict->end()) return false;
        value = it->second;
        return true;
    }

    // get list double value
    bool getFeatureValue(const std::string &name, ListDouble *&pRet)
    { return getFeature(name, pRet); }

    // add string feature value
    void addFeature(const std::string &name, const std::string &value)
    {
        auto ret = m_refFv.stringFeatures.insert(std::make_pair(name, StringSet()));
        auto &strSet = ret.first->second;
        strSet.insert(value);
        m_refFv.__isset.stringFeatures = true;
    }

    // set string feature value
    void setFeature(const std::string &name, const std::string &value)
    {
        auto ret = m_refFv.stringFeatures.insert(std::make_pair(name, StringSet()));
        auto &strSet = ret.first->second;
        strSet.clear();
        strSet.insert(value);
        m_refFv.__isset.stringFeatures = true;
    }

    // set/add float feature
    void setFeature(const double &val, const std::string &name, const std::string &subName = "")
    {
        auto ret = m_refFv.floatFeatures.insert(std::make_pair(name, FloatDict()));
        auto &fDict = ret.first->second;
        fDict[subName] = val;
        m_refFv.__isset.floatFeatures = true;
    }

    // set list double feature
    void setFeature(const std::string &name, ListDouble &val)
    {
        auto ret = m_refFv.denseFeatures.insert(std::make_pair(name, ListDouble()));
        auto &arr = ret.first->second;
        arr.swap(val);
        m_refFv.__isset.denseFeatures = true;
    }

private:
    FeatureVector&      m_refFv;
};


class FeatureIndexHandle {
public:
    typedef std::map<std::string, std::map<std::string, int32_t> >      StringIndexMap;
    typedef std::map<std::string, std::map<std::string, FloatInfo> >    FloatInfoMap;
    typedef std::map<std::string, DenseInfo>                            DenseInfoMap;

public:
    FeatureIndexHandle( FeatureIndex &fi )
            : m_refFi(fi) {}

    operator FeatureIndex& () { return m_refFi; }

    void setStringIndices( StringIndexMap &sIdx )
    {
        m_refFi.stringIndices.swap(sIdx);
        m_refFi.__isset.stringIndices = true;
    }

    StringIndexMap& stringIndices() 
    { return m_refFi.stringIndices; }

    void setFloatInfo( FloatInfoMap &fiMap )
    {
        m_refFi.floatInfo.swap(fiMap);
        m_refFi.__isset.floatInfo = true;
    }

    FloatInfoMap& floatInfo()
    { return m_refFi.floatInfo; }

    void setDenseInfo( DenseInfoMap &diMap )
    {
        m_refFi.denseInfo.swap(diMap);
        m_refFi.__isset.denseInfo = true;
    }

    DenseInfoMap& denseInfo()
    { return m_refFi.denseInfo; }

private:
    FeatureIndex&    m_refFi;
};



} // namespace FeatureProject


#endif /* ifndef _FEATURE_H_ */



