#ifndef _WORD_CLUSTER_DB_H_
#define _WORD_CLUSTER_DB_H_

#include <string>
#include <map>
#include <limits>
#include <boost/shared_ptr.hpp>


class WordClusterDB {
public:
    typedef boost::shared_ptr<WordClusterDB>                pointer;

    typedef std::map<uint32_t, double>                      ClusterIdProbability;
    typedef std::map<std::string, ClusterIdProbability>     WordClusterTable;

public:
    explicit WordClusterDB(const std::string &fname);

    void loadFromFile(const std::string &fname);

    std::pair<double, bool> query(const std::string &word, uint32_t cid);

private:
    WordClusterTable::iterator addWord(const std::string &word);

private:
    WordClusterTable    m_mapWordCluster[UCHAR_MAX + 1][UCHAR_MAX + 1];
};


#endif

