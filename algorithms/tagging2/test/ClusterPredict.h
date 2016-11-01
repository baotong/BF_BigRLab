#ifndef _CLUSTER_PREDICT_H_
#define _CLUSTER_PREDICT_H_

#include <string>
#include <vector>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <cmath>


class ClusterPredict {
public:
    typedef boost::shared_ptr<ClusterPredict>     pointer;

public:
    ClusterPredict() : m_nClusters(0), m_nFeatures(0) {}

    virtual ~ClusterPredict() = default;

    // processor(line, clusterId, nFeatures, nClusters)
    void loadModel(const std::string &modelFile, 
                   std::function<void(std::string&, uint32_t, uint32_t, uint32_t)> processor);

    virtual uint32_t predict(const std::vector<double> &vec) = 0;

    uint32_t nClusters() const { return m_nClusters; }
    uint32_t nFeatures() const { return m_nFeatures; }

protected:
    uint32_t    m_nClusters, m_nFeatures;
};


class ClusterPredictManual : public ClusterPredict {
public:
    typedef std::vector< std::vector<double> >  Matrix;
public:   
    ClusterPredictManual(const std::string &modelFile);
    virtual uint32_t predict(const std::vector<double> &vec) override;

private:
    void parseLine(std::string &text, uint32_t cId, uint32_t nFeatures, uint32_t nClusters);

    static inline
    double getDist(const std::vector<double> &v1, const std::vector<double> &v2)
    {
        double result = 0.0;
        auto it1 = v1.begin(), it2 = v2.begin();
        for (; it1 != v1.end(); ++it1, ++it2) {
            double diff = *it1 - *it2;
            result += diff * diff;
        } // for
        return std::sqrt(result);
    }

private:
    Matrix  m_matClusters;
};


#endif

