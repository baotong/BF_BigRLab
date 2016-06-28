#include "Article2Vector.h"
#include "common.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iterator>
#include <algorithm>
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
