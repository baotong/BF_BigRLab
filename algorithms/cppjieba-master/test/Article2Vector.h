#ifndef _ARTICLE_2_VECTOR_H_
#define _ARTICLE_2_VECTOR_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <map>

class Article2Vector {
public:
    // enum MethodType {
        // UNSPECIFIED,
        // WORD_VECTOR,
        // WORD_CLUSTER,
    // };

    typedef boost::shared_ptr<Article2Vector>   pointer;
    typedef std::vector<float>                  ResultType;

public:
    explicit Article2Vector(uint32_t _N_Classes) 
            : m_nClasses(_N_Classes) {}

    virtual ~Article2Vector() = default;

    virtual void convert2Vector( const std::vector<std::string> &article, ResultType &result ) = 0;

protected:
    uint32_t    m_nClasses;
};


class Article2VectorByWordVec : public Article2Vector {
public:
    typedef std::map< std::string, std::vector<float> >     WordVecTable;

public:
    Article2VectorByWordVec(uint32_t _N_Classes, const char *dictFile)
            : Article2Vector(_N_Classes)
    { loadDict(dictFile); }

    virtual void convert2Vector( const std::vector<std::string> &article, ResultType &result );

private:
    void loadDict(const char *filename);

private:
    WordVecTable    m_mapDict;
};


class Article2VectorByCluster : public Article2Vector {
public:
    typedef std::map< std::string, uint32_t >               WordClusterTable;

public:
    Article2VectorByCluster(uint32_t _N_Classes, const char *dictFile)
            : Article2Vector(_N_Classes)
    { loadDict(dictFile); }

    virtual void convert2Vector( const std::vector<std::string> &article, ResultType &result );

private:
    void loadDict(const char *filename);

private:
    WordClusterTable    m_mapDict;
};


#endif

