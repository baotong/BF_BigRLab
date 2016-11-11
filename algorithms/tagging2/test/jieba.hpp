#ifndef _JIEBA_HPP_
#define _JIEBA_HPP_

#include "shared_queue.h"
// #include "common.h"
#include "cppjieba/Jieba.hpp"
#include "cppjieba/KeywordExtractor.hpp"
#include <boost/make_shared.hpp>
#include <string>
#include <cstring>
#include <sstream>
#include <set>
#include <algorithm>
#include <iterator>

class Jieba {
public:
    typedef boost::shared_ptr<Jieba>    pointer;
    typedef std::set<std::string>       FilterSet;
    typedef std::vector<std::string>    WordVector;
    typedef std::vector< std::pair<std::string, std::string> > TagResult;
    typedef std::vector<cppjieba::KeywordExtractor::Word>      KeywordResult;

public:
    explicit Jieba(const std::string &dict_path, 
                   const std::string &hmm_path,
                   const std::string &user_dict_path,
                   const std::string &idf_path,
                   const std::string &stop_word_path)
        : DICT_PATH(dict_path)
        , HMM_PATH(hmm_path)
        , USER_DICT_PATH(user_dict_path)
        , IDF_PATH(idf_path)
        , STOP_WORD_PATH(stop_word_path)
    {
        m_pJieba = boost::make_shared<cppjieba::Jieba>(DICT_PATH.c_str(), HMM_PATH.c_str(), USER_DICT_PATH.c_str());
        m_pExtractor = boost::make_shared<cppjieba::KeywordExtractor>(*m_pJieba, IDF_PATH.c_str(), STOP_WORD_PATH.c_str());
    }

    void setFilter( const std::string &strFilter )
    {
        if (strFilter.empty())
            return;

        FilterSet filter; 
        char *cstrFilter = const_cast<char*>(strFilter.c_str());
        for (char *p = strtok(cstrFilter, ";" SPACES); p; p = strtok(NULL, ";" SPACES))
            filter.insert(p);

        setFilter( filter );
    }

    void setFilter( FilterSet &filter )
    { m_setFilter.swap(filter); }

    void tagging( const std::string &content, TagResult &result )
    {
        TagResult _Result;
        m_pJieba->Tag(content, _Result);

        result.clear();
        result.reserve( _Result.size() );
        for (auto &v : _Result) {
            if (!m_setFilter.count(v.second)) {
                result.push_back( TagResult::value_type() );
                result.back().first.swap(v.first);
                result.back().second.swap(v.second);
            } // if
        } // for

        result.shrink_to_fit();
    }

    void wordSegment( const std::string &content, WordVector &result )
    {
        TagResult tagResult;
        tagging(content, tagResult);

        result.resize( tagResult.size() );
        for (std::size_t i = 0; i < result.size(); ++i)
            result[i].swap( tagResult[i].first );
    }

    void keywordExtract( const std::string &content, KeywordResult &result, const std::size_t topk )
    { m_pExtractor->Extract(content, result, topk); }

protected:
    const std::string   DICT_PATH; 
    const std::string   HMM_PATH; 
    const std::string   USER_DICT_PATH; 
    const std::string   IDF_PATH; 
    const std::string   STOP_WORD_PATH; 
    FilterSet                                     m_setFilter;
    boost::shared_ptr<cppjieba::Jieba>            m_pJieba;
    boost::shared_ptr<cppjieba::KeywordExtractor> m_pExtractor;
};

// extern BigRLab::SharedQueue<Jieba::pointer>      g_JiebaPool;
extern Jieba::pointer g_pJieba;

#endif

