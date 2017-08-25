#include "Article2Vector.h"
#include "common.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <set>
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
        vector<float> vec;
        vec.reserve(m_nClasses);
        stream >> word;
        if (bad_stream(stream)) {
            LOG(ERROR) << "bad stream error when reading line " << lineno
                    << ": " << line;
            continue;
        } // if
        copy(istream_iterator<float>(stream), istream_iterator<float>(), 
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

    typedef boost::tuple<double&, float&> IterType;

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
        vector<float> &wordVec = it->second;
        // Test::print_container(wordVec);
        BOOST_FOREACH( IterType v, boost::combine(sumVec, wordVec) )
            v.get<0>() += v.get<1>();
    } // while

    if (!count)
        return;

    std::for_each(sumVec.begin(), sumVec.end(), 
            [&](double &v){ v /= (double)count; });

    BOOST_FOREACH( IterType v, boost::combine(sumVec, result) )
        v.get<1>() = (float)(v.get<0>());
}

void Article2VectorByCluster::convert2Vector( const std::vector<std::string> &article, 
            ResultType &result )
{
    using namespace std;

    typedef boost::tuple<double&, float&> IterType;

    result.clear();
    result.resize(m_nClasses, 0.0);

    vector<double> workVec( m_nClasses, 0.0 );
    set<string>    wordSet;
    string         word;
    double         maxCount = 0.0;

    for (const auto& word : article) {
        auto ret = wordSet.insert(word);    // 生成无重复的词语集合
        // 跳过重复单词
        if (!ret.second)
            continue;

        auto it = m_mapDict.find(word);
        if (it == m_mapDict.end()) {
            // DLOG(INFO) << "no word " << word << " found in cluster table.";
            continue;
        } // if
        uint32_t id = it->second;       // 找出该词所属的classid
        workVec[id] += 1.0;             // 对应classid下标元素++
        maxCount = workVec[id] > maxCount ? workVec[id] : maxCount;
    } // while

    if (maxCount < 1.0)
        return;

    // DLOG(INFO) << "Before normalization:";
    // Test::print_non_zero_vector(cout, workVec);
    // 归一化
    std::for_each(workVec.begin(), workVec.end(), 
            [&](double &v){ v /= maxCount; });
    // DLOG(INFO) << "After normalization:";
    // Test::print_non_zero_vector(cout, workVec);

    BOOST_FOREACH( IterType v, boost::combine(workVec, result) )
        v.get<1>() = (float)(v.get<0>());
}
