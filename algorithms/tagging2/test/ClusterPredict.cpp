#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <algorithm>
#include <boost/format.hpp>
#include <glog/logging.h>
#include "common.hpp"
#include "ClusterPredict.h"


void ClusterPredict::loadModel(const std::string &modelFile,
               std::function<void(std::string&, uint32_t, uint32_t, uint32_t)> processor)
{
    using namespace std;

    if (modelFile.empty())
        THROW_RUNTIME_ERROR("ClusterPredict model filename cannot be empty!");

    ifstream ifs(modelFile, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("ClusterPredict cannot open " << modelFile << " for reading!");

    string line;
    
    // skip 1st line
    if (!getline(ifs, line))
        THROW_RUNTIME_ERROR("ClusterPredict bad model file!");

    // read nClusters
    if (!getline(ifs, line))
        THROW_RUNTIME_ERROR("ClusterPredict bad model file!");
    {
        uint32_t k = 0;
        stringstream ss(line);
        ss >> k;
        if (ss.fail() || ss.bad() || k <= 0)
            THROW_RUNTIME_ERROR("ClusterPredict fail to read nClusters!");
        m_nClusters = k;
    }

    // read nFeatures
    if (!getline(ifs, line))
        THROW_RUNTIME_ERROR("ClusterPredict bad model file!");
    {
        uint32_t nf = 0;
        stringstream ss(line);
        ss >> nf;
        if (ss.fail() || ss.bad() || nf <= 0)
            THROW_RUNTIME_ERROR("ClusterPredict fail to read nFeatures!");
        m_nFeatures = nf;
    }

    for (uint32_t i = 0; i != m_nClusters; ++i) {
        if (!getline(ifs, line))
            THROW_RUNTIME_ERROR("ClusterPredict error while reading cluster matrix lineno = " << i);
        processor(line, i, m_nFeatures, m_nClusters);
    } // for
}


ClusterPredictManual::ClusterPredictManual(const std::string &modelFile)
{
    loadModel(modelFile, std::bind(&ClusterPredictManual::parseLine, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}


void ClusterPredictManual::parseLine(std::string &text, uint32_t cId, 
                                    uint32_t nFeatures, uint32_t nClusters)
{
    using namespace std;

    if (m_matClusters.empty())
        m_matClusters.resize(nClusters);

    auto &vec = m_matClusters[cId];
    vec.resize(nFeatures, 0.0);

    string item;
    uint32_t idx = 0;   // starts from 1
    double val = 0.0;
    stringstream ss(text);
    while (ss >> item) {
        if (sscanf(item.c_str(), "%u:%lf", &idx, &val) != 2 || idx < 1 || idx > nFeatures)
            continue;
        vec[idx-1] = val;
    } // while
}


uint32_t ClusterPredictManual::predict(const std::vector<double> &vec)
{
    using namespace std;

    // DLOG(INFO) << "vec.size = " << vec.size() << " nFeatures = " << nFeatures();

    if (vec.size() != nFeatures())
        THROW_RUNTIME_ERROR("Requested nFeatures not same as model nFeatures!");

    vector<double> dist(nClusters());

#pragma omp parallel for
    for (uint32_t i = 0; i < nClusters(); ++i)
        dist[i] = getDist(vec, m_matClusters[i]);

    // DEBUG
    // cout << "Distances: ";
    // for (auto &v : dist)
        // cout << boost::format("%10.5lf") % v;
    // cout << endl;

    auto itMin = std::min_element(dist.begin(), dist.end());
    return (uint32_t)(std::distance(dist.begin(), itMin));
}


ClusterPredictKnn::ClusterPredictKnn(const std::string &idxFile, uint32_t _N_Features)
        : m_AnnDB(_N_Features)
{
    m_nFeatures = _N_Features;
    loadAnnDB(idxFile);
    m_nClusters = (uint32_t)m_AnnDB.size();
}


void ClusterPredictKnn::loadAnnDB(const std::string &idxFile)
{
    using namespace std;

    // check file
    {
        THROW_RUNTIME_ERROR_IF(idxFile.empty(), "ClusterPredictKnn idx file not set!");
        ifstream ifs(idxFile, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs, "ClusterPredictKnn cannot open idx file " 
                                << idxFile << " for reading!");
    }

    m_AnnDB.loadIndex(idxFile.c_str());
}


uint32_t ClusterPredictKnn::predict(const std::vector<double> &vec)
{
    using namespace std;

    // LOG(INFO) << "ClusterPredictKnn::predict()";

    if (vec.size() != nFeatures())
        THROW_RUNTIME_ERROR("Requested nFeatures not same as model nFeatures!");

    // uint32_t cid = (uint32_t)-1;
    // float dist = 0.0;
    vector<float> v(vec.begin(), vec.end());
    vector<uint32_t> cid;
    vector<float> dist;
    m_AnnDB.rawIndex().get_nns_by_vector(&v[0], 1, (size_t)-1, &cid, &dist);

    THROW_RUNTIME_ERROR_IF(cid.empty(), "knn error!");

    return cid[0];
}


