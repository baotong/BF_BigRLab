#ifndef _FEATURE_INFO_H_
#define _FEATURE_INFO_H_

#include <string>
#include <set>
#include <memory>


struct FeatureInfo {
    typedef std::shared_ptr<FeatureInfo>  pointer;

    FeatureInfo() : multi(false) {}
    FeatureInfo(const std::string &_Name, const std::string &_Type)
            : name(_Name), type(_Type), multi(false) {}

    void addValue(const std::string &v)
    { values.insert(v); }

    std::string                 name;
    std::string                 type;
    bool                        multi;
    std::string                 sep;    // empty means default SPACE
    std::set<std::string>       values;
};


#endif

