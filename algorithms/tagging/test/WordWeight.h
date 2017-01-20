#ifndef _WORD_WEIGHT_H_
#define _WORD_WEIGHT_H_

#include <string>
#include <map>


struct WordWeight {
    WordWeight() : weight(0.0), count(0) {}

    double          weight;
    std::size_t     count;
};


typedef std::map<std::string, WordWeight>   WordWeightTable;
extern WordWeightTable          g_WordWgtTable;


#endif

