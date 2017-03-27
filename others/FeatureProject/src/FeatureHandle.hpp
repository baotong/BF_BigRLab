#ifndef _FEATURE_HANDLE_HPP_
#define _FEATURE_HANDLE_HPP_ 

#include <example_types.h>

// FeatureVector代表一行记录
class FeatureVectorHandle {
public:
    typedef std::vector<double>             ListDouble;
    typedef std::set<std::string>           StringSet;
    typedef std::map<std::string, double>   FloatDict;

public:
    FeatureVectorHandle(FeatureVector &fv) : m_refFv(fv) {}

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
        auto ret = m_refFv.stringFeatures.insert(std::make_pair(name, StringSet()));
        auto &strSet = ret.first->second;
        strSet.insert(value);
        m_refFv.__isset.stringFeatures = true;
    }

    // set feature value
    void setFeature(const std::string &name, const std::string &value)
    {
        auto ret = m_refFv.stringFeatures.insert(std::make_pair(name, StringSet()));
        auto &strSet = ret.first->second;
        strSet.clear();
        strSet.insert(value);
        m_refFv.__isset.stringFeatures = true;
    }

    // set float feature
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


#endif /* ifndef _FEATURE_OP_H_ */

