#include "Article2Vector.h"
// #include "common.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <set>
#include <limits>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <glog/logging.h>


void Article2VectorByWordVec::loadDict(const char *filename)
{
    using namespace std;

    ifstream ifs(filename, ios::in);

    if (!ifs)
        THROW_RUNTIME_ERROR("Article2VectorByWordVec::loadDict() cannot open " << filename);

    string line, word;
    size_t lineno = 0;
    while (getline(ifs, line)) {
        ++lineno;
        stringstream stream(line);
        vector<double> vec;
        vec.reserve(m_nClasses);
        stream >> word;
        if (bad_stream(stream)) {
            LOG(ERROR) << "bad stream error when reading line " << lineno
                    << ": " << line;
            continue;
        } // if
        copy(istream_iterator<double>(stream), istream_iterator<double>(), 
                    back_inserter(vec));
        if (vec.size() != m_nClasses) {
            LOG(ERROR) << "Invalid vector len when reading line " << lineno
                    << ": " << line;
            continue;
        } // if
        auto ret = m_mapDict.insert(std::make_pair(word, WordVecTable::mapped_type()));
        if (!ret.second) {
            LOG(ERROR) << "Duplicate word " << word << " when reading line " << lineno;
            continue;
        } // if
        ret.first->second.swap(vec);
    } // while
}


void Article2VectorByCluster::loadDict(const char *filename)
{
    using namespace std;
    
    ifstream ifs(filename, ios::in);

    if (!ifs)
        THROW_RUNTIME_ERROR("Article2VectorByCluster::loadDict() cannot open " << filename);

    string line, word;
    uint32_t clusterId;
    size_t lineno = 0;
    //?? 第一行是合法的吗？
    while (getline(ifs, line)) {
        ++lineno;
        stringstream stream(line);
        stream >> word >> clusterId;
        if (clusterId >= m_nClasses) {
            LOG(ERROR) << clusterId << " read in line:" << lineno << " is not valid";
            continue;
        } // if
        auto ret = m_mapDict.insert(std::make_pair(word, clusterId));
        if (!ret.second) {
            LOG(ERROR) << "Duplicate word " << word << " when reading line " << lineno;
            continue;
        } // if
    } // while
}


void Article2VectorByWordVec::convert2Vector( const std::vector<std::string> &article, 
            ResultType &result )
{
    using namespace std;

    typedef boost::tuple<double&, double&> IterType;

    result.clear();
    result.resize(m_nClasses, 0.0);

    vector<double> sumVec( m_nClasses );
    string word;

    size_t count = 0;
    for (const auto& word : article) {
        auto it = m_mapDict.find(word);
        if (it == m_mapDict.end()) {
            // DLOG(INFO) << "no word " << word << " found in wordvec table.";
            continue;
        } // if
        ++count;
        vector<double> &wordVec = it->second;
        // Test::print_container(wordVec);
        BOOST_FOREACH( IterType v, boost::combine(sumVec, wordVec) )
            v.get<0>() += v.get<1>();
    } // while

    if (!count)
        return;

    std::for_each(sumVec.begin(), sumVec.end(), 
            [&](double &v){ v /= (double)count; });

    result.swap(sumVec);
    // BOOST_FOREACH( IterType v, boost::combine(sumVec, result) )
        // v.get<1>() = (float)(v.get<0>());
}

void Article2VectorByCluster::convert2Vector( const std::vector<std::string> &article, 
            ResultType &result )
{
    using namespace std;

    // typedef boost::tuple<double&, double&> IterType;

    result.clear();
    result.resize(m_nClasses, 0.0);

    vector<double> workVec( m_nClasses, 0.0 );
    set<string>    wordSet;
    string         word;
    double         maxCount = 0.0;

    for (const auto& word : article) {
        auto ret = wordSet.insert(word);
        // 跳过重复单词
        if (!ret.second)
            continue;

        auto it = m_mapDict.find(word);
        if (it == m_mapDict.end()) {
            // DLOG(INFO) << "no word " << word << " found in cluster table.";
            continue;
        } // if
        uint32_t id = it->second;
        workVec[id] += 1.0;
        maxCount = workVec[id] > maxCount ? workVec[id] : maxCount;
    } // while

    if (maxCount < 1.0)
        return;

    // DLOG(INFO) << "Before normalization:";
    // Test::print_non_zero_vector(cout, workVec);
    std::for_each(workVec.begin(), workVec.end(), 
            [&](double &v){ v /= maxCount; });
    // DLOG(INFO) << "After normalization:";
    // Test::print_non_zero_vector(cout, workVec);

    result.swap(workVec);
    // BOOST_FOREACH( IterType v, boost::combine(workVec, result) )
        // v.get<1>() = (float)(v.get<0>());
}


void Article2VectorByWarplda::loadData(const std::string &modelFile,
                                       const std::string &vocabFile)
{
    using namespace std;

    ifstream fModel(modelFile, ios::in);
    ifstream fVocab(vocabFile, ios::in);

    string lineModel, word;
    // read nTopics
    {
        uint32_t nWords, nTopics;
        double    alpha, beta;
        getline(fModel, lineModel);
        stringstream stream(lineModel);
        stream >> nWords >> nTopics >> alpha >> beta;
        if (stream.fail() || stream.bad())
            THROW_RUNTIME_ERROR("Bad model file " << modelFile);
        m_nClasses = nTopics;
    }

    uint32_t id, count;
    string item;
    while (getline(fModel, lineModel) && getline(fVocab, word)) {
        boost::trim(word);
        auto ret = m_dictWordTopic.insert(std::make_pair(word, DictType::mapped_type()));
        auto it = ret.first;
        
        stringstream stream(lineModel);
        stream >> item;     // skip first col
        if (stream.fail() || stream.bad())
            continue;
        while (stream >> item) {
            if (sscanf(item.c_str(), "%u:%u", &id, &count) != 2)
               continue; 
            if (id >= m_nClasses)
                THROW_RUNTIME_ERROR("Found invalid topicid when loading model!");
            it->second.push_back(std::make_pair(id, count));
        } // while

        if (it->second.empty())
            m_dictWordTopic.erase(it);
    } // while

    if (getline(fModel, lineModel) || getline(fVocab, word))
        THROW_RUNTIME_ERROR("Model file and vocab file records mismatch!");

    // test
    // for (auto &kv : m_dictWordTopic) {
        // cout << kv.first << "\t";
        // for (auto &item : kv.second)
            // cout << item.first << ":" << item.second << " ";
        // cout << endl;
    // } // for
}


void Article2VectorByWarplda::convert2Vector( const std::vector<std::string> &article, ResultType &result )
{
    result.assign(m_nClasses, 0.0);

    for (auto &s : article) {
        auto it = m_dictWordTopic.find(s);
        if (it == m_dictWordTopic.end())
            continue;
        for (auto &item : it->second)
            result[item.first] += item.second;
    } // for

    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();

    for (auto &v : result) {
        if (v < min) min = v;
        if (v > max) max = v;
    } // for

    double base = max - min;
    for (auto &v : result) {
        v -= min;
        v /= base;
    } // for
}
