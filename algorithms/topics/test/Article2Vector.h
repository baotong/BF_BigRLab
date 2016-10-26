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
    Article2Vector() = default;
    explicit Article2Vector(uint32_t _N_Classes) 
            : m_nClasses(_N_Classes) {}

    virtual ~Article2Vector() = default;

    virtual void convert2Vector( const std::vector<std::string> &article, ResultType &result ) = 0;

    uint32_t nClasses() const { return m_nClasses; }

protected:
    uint32_t    m_nClasses;
};


class Article2VectorByWarplda : public Article2Vector {
public:
    // key = word_in_vocab  value = topicid:count ...
    typedef std::map< std::string, std::vector<std::pair<uint32_t, uint32_t>> >     DictType;

public:
    Article2VectorByWarplda(const std::string &modelFile,
                            const std::string &vocabFile)
    { loadData(modelFile, vocabFile); }

    virtual void convert2Vector( const std::vector<std::string> &article, ResultType &result );

private:
    void loadData(const std::string &modelFile,
                  const std::string &vocabFile);

private:
    DictType        m_dictWordTopic;
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

