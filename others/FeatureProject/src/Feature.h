#ifndef _FEATURE_H_
#define _FEATURE_H_

#include <iostream>
#include <memory>
#include <limits>
#include <json/json.h>
#include <boost/filesystem.hpp>
#include <example_types.h>
#include <gflags/gflags.h>
#include "CommDef.h"


#define SPACES                " \t\f\r\v\n"
#define VALID_TYPES     {"string", "double", "list_double", "datetime"}


class FeatureInfo {
public:
    typedef std::shared_ptr<FeatureInfo>  pointer;

    struct SubFeatureInfo {
        SubFeatureInfo() : m_nIdx(0)
                , m_fMin(std::numeric_limits<double>::max())
                , m_fMax(std::numeric_limits<double>::min()) {}

        SubFeatureInfo(const std::string &_Name)
                : m_strName(_Name), m_nIdx(0)
                , m_fMin(std::numeric_limits<double>::max())
                , m_fMax(std::numeric_limits<double>::min()) {}

        std::string& name() { return m_strName; }
        const std::string& name() const { return m_strName; }
        uint32_t index() const { return m_nIdx; }
        void setIndex(uint32_t _Idx) { m_nIdx = _Idx; } 
        double minVal() const { return m_fMin; }
        double maxVal() const { return m_fMax; }

        void setMinMax(const double &val)
        {
            m_fMin = val < m_fMin ? val : m_fMin;
            m_fMax = val > m_fMax ? val : m_fMax;
        }

        void toJson(Json::Value &jv, const std::string &type) const
        {
            jv.clear();
            jv["name"] = name();
            jv["index"] = index();
            if (type != "string" && type != "list_double") {
                jv["min"] = minVal();
                jv["max"] = maxVal();
            } // if
        }

        void fromJson(const Json::Value &jv, const std::string &type)
        {
            name() = jv["name"].asString();
            setIndex(jv["index"].asUInt());
            if (type != "string" && type != "list_double") {
                auto &jMin = jv["min"];
                m_fMin = (!jMin ? std::numeric_limits<double>::max() : jMin.asDouble());
                auto &jMax = jv["max"];
                m_fMax = (!jMax ? std::numeric_limits<double>::min() : jMax.asDouble());
            } // if
        }

        std::string     m_strName;
        uint32_t        m_nIdx;
        double          m_fMin, m_fMax;
    };

    typedef std::map<std::string, SubFeatureInfo>   SubFeatureTable;

public:
    FeatureInfo() : m_bMulti(false), m_bKeep(true)
            , m_nIdxStart(0), m_nDenseLen(0) {}
    FeatureInfo(const std::string &_Name, const std::string &_Type)
            : m_strName(_Name), m_strType(_Type), m_bMulti(false), m_bKeep(true) 
            , m_nIdxStart(0), m_nDenseLen(0) {}

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

    void addSubFeature(const std::string &name, bool overwrite = false)
    { 
        auto ret = m_mapSubFeature.insert(std::make_pair(name, SubFeatureInfo(name)));
        if (!ret.second && overwrite)
            ret.first->second = SubFeatureInfo(name);
    }

    void addSubFeature(const SubFeatureInfo &subFt, bool overwrite = false)
    { 
        auto ret = m_mapSubFeature.insert(std::make_pair(subFt.name(), subFt));
        if (!ret.second && overwrite)
            ret.first->second = subFt;
    }

    SubFeatureInfo* subFeature(const std::string &key)
    {
        auto it = m_mapSubFeature.find(key);
        if (it == m_mapSubFeature.end()) 
            return nullptr;
        return &(it->second);
    }

    const SubFeatureInfo* subFeature(const std::string &key) const
    {
        auto it = m_mapSubFeature.find(key);
        if (it == m_mapSubFeature.end()) 
            return nullptr;
        return &(it->second);
    }

    SubFeatureTable& subFeatures() { return m_mapSubFeature; }
    const SubFeatureTable& subFeatures() const { return m_mapSubFeature; }

    void setMinMax(const double &val, const std::string &key = "")
    {
        auto ret = m_mapSubFeature.insert(std::make_pair(key, SubFeatureInfo(key)));
        ret.first->second.setMinMax(val);
    }

    std::pair<double, double> minMax(const std::string &key = "")
    {
        auto it = m_mapSubFeature.find(key);
        if (it == m_mapSubFeature.end())
            return std::make_pair(std::numeric_limits<double>::max(), std::numeric_limits<double>::min());
        return std::make_pair(it->second.minVal(), it->second.maxVal());
    }

    void setIndex(uint32_t idx, const std::string &key = "")
    { 
        auto ret = m_mapSubFeature.insert(std::make_pair(key, SubFeatureInfo(key)));
        ret.first->second.setIndex(idx);
    }

    uint32_t index(const std::string &key = "")
    {
        auto it = m_mapSubFeature.find(key);
        THROW_RUNTIME_ERROR_IF(it == m_mapSubFeature.end(),
                "No sub feature " << key << " in feature " << name());
        return it->second.index();
    }

    // for dense feature
    std::string& densePath() { return m_strDenseFilePath; }
    const std::string& densePath() const { return m_strDenseFilePath; }
    uint32_t& startIdx() { return m_nIdxStart; }
    const uint32_t& startIdx() const { return m_nIdxStart; }
    uint32_t& denseLen() { return m_nDenseLen; }
    const uint32_t& denseLen() const { return m_nDenseLen; }

    void toJson(Json::Value &jv) const
    {
        jv.clear();
        jv["name"] = name();
        jv["type"] = type();
        if (isMulti()) jv["multi"] = true;
        if (!isKeep()) jv["keep"] = false;
        if (!sep().empty()) jv["sep"] = sep();
        
        if (type() == "list_double") {
            jv["filepath"] = densePath();
            jv["startIdx"] = startIdx();
            jv["length"] = denseLen();
        } // if

        for (const auto &kv : subFeatures()) {
            auto &back = jv["subfeatures"].append(Json::Value());
            kv.second.toJson(back, type());
        } // for
    }

    void fromJson(const Json::Value &jv)
    {
        name() = jv["name"].asString();
        type() = jv["type"].asString();
        auto &jMulti = jv["multi"];
        if (!!jMulti) setMulti(jMulti.asBool());
        auto &jKeep = jv["keep"];
        if (!!jKeep) setKeep(jKeep.asBool());
        auto &jSep = jv["sep"];
        if (!!jSep) sep() = jSep.asString();

        if (type() == "list_double") {
            densePath() = jv["filepath"].asString();
            startIdx() = jv["startIdx"].asUInt();
            denseLen() = jv["length"].asUInt();
            if (!denseLen())
                parseDense();
        } // if list_double

        // sub features
        for (const auto &jSubFt : jv["subfeatures"]) {
            SubFeatureInfo subFt;
            subFt.fromJson(jSubFt, type());
            addSubFeature(subFt, true);
        } // for
    }

    // for dense feature
    void readDense(std::vector<double> &vec);

private:
    // for dense feature
    void parseDense();

private:
    std::string                 m_strName;
    std::string                 m_strType;
    bool                        m_bMulti;
    bool                        m_bKeep;
    std::string                 m_strSep;    // empty means default SPACE
    SubFeatureTable             m_mapSubFeature;
    // for dense feature (list double)
    std::string                 m_strDenseFilePath;
    boost::filesystem::path     m_pathDense;
    std::shared_ptr<std::istream>   m_pDenseFile;
    uint32_t                    m_nIdxStart, m_nDenseLen;

public:
    friend std::ostream& operator << (std::ostream &os, const FeatureInfo &fi)
    {
        os << "name = " << fi.name() << std::endl;
        os << "type = " << fi.type() << std::endl;
        os << "multi = " << fi.isMulti() << std::endl;
        if (!fi.sep().empty())
            os << "sep = " << fi.sep() << std::endl;
        for (const auto &kv : fi.subFeatures()) {
            os << kv.second.name() << "{" << "index=" << kv.second.index();
            if (fi.type() != "string")
                os << ",min=" << kv.second.minVal() << ",max=" << kv.second.maxVal(); 
            os << "} ";
        } // for
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
    FeatureVectorHandle(FeatureVector &fv, FeatureInfoSet &fiSet) 
            : m_refFv(fv), m_refFiSet(fiSet) {}

    operator FeatureVector& () { return m_refFv; }

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
        auto pfi = m_refFiSet.get(name);
        THROW_RUNTIME_ERROR_IF(!pfi, "No feature info for " << name << " found!");
        auto ret = m_refFv.stringFeatures.insert(std::make_pair(name, StringSet()));
        auto &strSet = ret.first->second;
        strSet.insert(value);
        pfi->addSubFeature(value);
        m_refFv.__isset.stringFeatures = true;
    }

    // set string feature value
    void setFeature(const std::string &name, const std::string &value)
    {
        auto pfi = m_refFiSet.get(name);
        THROW_RUNTIME_ERROR_IF(!pfi, "No feature info for " << name << " found!");
        auto ret = m_refFv.stringFeatures.insert(std::make_pair(name, StringSet()));
        auto &strSet = ret.first->second;
        strSet.clear();
        strSet.insert(value);
        pfi->addSubFeature(value);
        m_refFv.__isset.stringFeatures = true;
    }

    // set/add float feature
    void setFeature(const double &val, const std::string &name, const std::string &subName = "")
    {
        auto pfi = m_refFiSet.get(name);
        THROW_RUNTIME_ERROR_IF(!pfi, "No feature info for " << name << " found!");
        auto ret = m_refFv.floatFeatures.insert(std::make_pair(name, FloatDict()));
        auto &fDict = ret.first->second;
        fDict[subName] = val;
        pfi->setMinMax(val, subName);
        m_refFv.__isset.floatFeatures = true;
    }

    // set list double feature
    void setFeature(const std::string &name, ListDouble &val)
    {
        auto pfi = m_refFiSet.get(name);
        THROW_RUNTIME_ERROR_IF(!pfi, "No feature info for " << name << " found!");
        auto ret = m_refFv.denseFeatures.insert(std::make_pair(name, ListDouble()));
        auto &arr = ret.first->second;
        arr.swap(val);
        m_refFv.__isset.denseFeatures = true;
    }

private:
    FeatureVector&      m_refFv;
    FeatureInfoSet&     m_refFiSet;
};


class ExampleHandle {
public:
    ExampleHandle(Example &exp) : m_refExp(exp) {}

    operator Example& () { return m_refExp; }

    void addVector(FeatureVector &&fv)
    {
        m_refExp.example.emplace_back(std::move(fv));
        m_refExp.__isset.example = true;
    }

    void clearVector()
    {
        m_refExp.example.clear();
        m_refExp.__isset.example = false;
    }

    void setContext(FeatureVector &&ctx)
    {
        m_refExp.context = std::move(ctx);
        m_refExp.__isset.context = true;
    }

    void setMeta(const std::string &key, const std::string &value)
    {
        m_refExp.metadata[key] = value;
        m_refExp.__isset.metadata = true;
    }

    bool getMeta(const std::string &key, std::string &value)
    {
        auto it = m_refExp.metadata.find(key);
        if (it == m_refExp.metadata.end())
            return false;
        value = it->second;
        return true;
    }

private:
    Example&    m_refExp;
};


DECLARE_string(conf);
extern FeatureInfoSet                       g_ftInfoSet;
extern std::string                          g_strSep;

extern void load_feature_info(const std::string &fname, FeatureInfoSet &fiSet);
extern void update_feature_info(const std::string &fname, const FeatureInfoSet &fiSet);
extern void load_data(const std::string &ifname, const std::string &ofname, FeatureInfoSet &fiSet);


#endif /* ifndef _FEATURE_H_ */



