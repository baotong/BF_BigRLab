#ifndef _BUILD_INDEX_H_
#define _BUILD_INDEX_H_

#include "FeatureTask.h"

using namespace FeatureProject;

extern "C" {
    extern FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr);
}


// NOTE!!! 不支持多值
class Discrete : public FeatureTask {
public:
    struct RangeType : std::pair<double, double> {
        typedef std::pair<double, double>   BaseType;

        void setName()
        {
            std::ostringstream oss;
            if (first == second)
                oss << "[" << first << "]";
            else
                oss << "[" << first << "," << second << ")";
            oss.flush();
            name_ = oss.str();
        }

        const std::string& name() const
        { return name_; }

        std::string     name_;
    };

    typedef std::map<std::string, std::map<double, uint32_t> >      DictCount;
    typedef std::vector<RangeType>                                  RangeArray;
    typedef std::map<std::string, RangeArray>                       FeatureRange;

public:
    Discrete(const std::string &name, FeatureTaskMgr *mgr)
            : FeatureTask(name, mgr), m_nParts(0), m_nSample(0) {}

    void init(const Json::Value &conf) override;
    void run() override;

private:
    void countValue();
    void getRange();
    void genNewFv();
    void printDictCount();

    static const std::string& range(const RangeArray &arrRange, const double &val)
    {
        for (const auto &r : arrRange) {
            if (r.first == r.second) {
                if (val == r.first)
                    return r.name();
            } else {
                if (val >= r.first && val < r.second)
                    return r.name();
            } // if
        } // for
        THROW_RUNTIME_ERROR("No valid range found for value: " << val);
    }

private:
    DictCount       m_dictCount;
    FeatureRange    m_dictRange;
    uint32_t        m_nParts;
    std::size_t     m_nSample;
};

#endif /* _BUILD_INDEX_H_ */

