#include <fstream>
#include <sstream>
#include <iostream>
#include <glog/logging.h>
#include "common.hpp"
#include "WordClusterDB.h"


WordClusterDB::WordClusterDB(const std::string &fname)
{ 
    loadFromFile(fname); 

    // DEBUG
    // using namespace std;
    // size_t sz = 0;
    // for (size_t i = 0; i <= UCHAR_MAX; ++i)
        // for (size_t j = 0; j <= UCHAR_MAX; ++j)
            // sz += m_mapWordCluster[i][j].size();
    // cout << "WordClusterDB::size = " << sz << endl;
}


WordClusterDB::WordClusterTable::iterator 
WordClusterDB::addWord(const std::string &word)
{
    const char *p = word.c_str();
    uint8_t i = (uint8_t)(*p++);
    uint8_t j = (uint8_t)(*p);
    WordClusterTable &table = m_mapWordCluster[i][j];
    auto ret = table.insert(std::make_pair(word, WordClusterTable::mapped_type()));
    return ret.first;
}


void WordClusterDB::loadFromFile(const std::string &fname)
{
    using namespace std;

    if (fname.empty())
        THROW_RUNTIME_ERROR("WordClusterDB load file name cannot be empty!");

    ifstream ifs(fname, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("WordClusterDB cannot open " << fname << " for reading!");

    string line, word, item;
    uint32_t cid = 0;
    double val = 0.0;
    while (getline(ifs, line)) {
        istringstream ss(line);
        ss >> word;
        // DLOG(INFO) << "Adding word " << word;
        if (ss.fail() || ss.bad())
            continue;
        auto it = addWord(word);
        while (ss >> item) {
            if (sscanf(item.c_str(), "%u:%lf", &cid, &val) != 2)
                continue;
            it->second.insert(std::make_pair(cid, val));
        } // while
    } // while
}


std::pair<double, bool> WordClusterDB::query(const std::string &word, uint32_t cid)
{
    const char *p = word.c_str();
    uint8_t i = (uint8_t)(*p++);
    uint8_t j = (uint8_t)(*p);
    WordClusterTable &table = m_mapWordCluster[i][j];

    auto it1 = table.find(word);
    if (it1 == table.end())
        return std::make_pair(0.0, false);

    auto it2 = it1->second.find(cid);
    if (it2 == it1->second.end())
        return std::make_pair(0.0, false);

    return std::make_pair(it2->second, true);
}


