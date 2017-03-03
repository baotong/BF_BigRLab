#ifndef _WORD_ANN_DB_H_
#define _WORD_ANN_DB_H_

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <memory>
#include <climits>
#include "common.hpp"
#include "alg_common.hpp"
#include "KnnService.h"
#include "annoylib.h"


typedef std::shared_ptr<std::string> StringPtr;

template < typename T >
std::istream& read_into_container( std::istream &is, T &c )
{
    typedef typename T::value_type value_type;
    std::istream_iterator<value_type> beg(is), eof;
    T ret( beg, eof );
    c.swap(ret);
    return is;
}


namespace KNN {

/*
 * 主要功能：
 * 实现 word(string) 到 id(uint32_t) 双向map映射
 * 对 Annoy 主要功能进行封装
 *
 * id 必须从0开始且必须连续，不能有间断，否则会有空记录掺进，影响查询结果
 * TODO 
 * thread safe
 * 泛型化
 */
class WordAnnDB {
public:
    struct StringPtrCmp {
        bool operator() ( const StringPtr &lhs, const StringPtr &rhs ) const
        { return (*lhs < *rhs); }   
    };

    typedef std::map< StringPtr, uint32_t, StringPtrCmp >       WordIdTable;
    typedef std::map< uint32_t, StringPtr >                     IdWordTable;
    typedef AnnoyIndex< uint32_t, float, Angular, RandRandom >  WordAnnIndex;

    static const uint32_t   INVALID_ID = (uint32_t)-1;
    static const uint32_t   ID_WORD_HASH_SIZE = (UCHAR_MAX + 1) * (UCHAR_MAX + 1);

public:
    explicit WordAnnDB( int nFields )
                : m_nFields(nFields)
                , m_AnnIndex(nFields) {}

    /**
     * @brief  将从数据文件读来的行记录添加到数据库中
     *
     * @param line 数据文件行，格式：word val1 val2 ... valn
     *
     * @return  pair.second 插入成功与否，若相同word已存在则插入失败
     *          pair.first 新插入(或已有的，当插入失败时)word的Id
     */
    std::pair<uint32_t, bool> addRecord( const std::string &line );

    // uint32_t size()
    // { return s_nIdIndex; }

    std::size_t size()
    { return m_AnnIndex.get_n_items(); }

    /**
     * @brief  建立Annoy索引，参阅 AnnoyIndex::build 
     *         More trees gives higher precision when querying. After calling ``build``, 
     *         no more items can be added.
     * @param q  number of trees
     */
    void buildIndex( int q )
    { m_AnnIndex.build(q); }

    /**
     * @brief  将建立好的Annoy索引存入文件
     */
    bool saveIndex( const char *filename )
    { return m_AnnIndex.save(filename); }
    /**
     * @brief  从文件中导入AnnoyIndex
     */
    bool loadIndex( const char *filename )
    { return m_AnnIndex.load(filename); }
    /**
     * @brief  将单词到AnnoyIndexID的映射存入文件
     *          格式: word    id
     */
    void saveWordTable( const char *filename );
    /**
     * @brief  从文件中导入单词到ID的映射
     */
    void loadWordTable( const char *filename );

    /**
     * @brief   查询两单词的相似度
     */
    float getDistance( const std::string &s1, const std::string &s2 )
    {
        uint32_t i = getWordId( s1 );
        if (i == INVALID_ID)
            THROW_INVALID_REQUEST("WordAnnDB::getDistance() cannot find "
                    << s1 << " in database!");
        uint32_t j = getWordId( s2 );
        if (j == INVALID_ID)
            THROW_INVALID_REQUEST("WordAnnDB::getDistance() cannot find "
                    << s2 << " in database!");
        return getDistance( i, j );
    }

    float getDistance( uint32_t i, uint32_t j )
    { return m_AnnIndex.get_distance(i, j); }

    void getVector( const std::string &word, std::vector<float> &ret )
    {
        uint32_t i = getWordId( word );
        if (i == INVALID_ID)
            THROW_INVALID_REQUEST("WordAnnDB::getVector() cannot find "
                    << word << " in database!");
        getVector( i, ret );
    }

    void getVector( uint32_t i, std::vector<float> &ret )
    {
        ret.resize( m_nFields );
        m_AnnIndex.get_item(i, &ret[0]);
    }

    /**
     * @brief   查ID对应单词
     */
    bool getWordById( uint32_t id, std::string &ret )
    {
        uint32_t index = id % ID_WORD_HASH_SIZE;
        const IdWordTable &table = m_mapId2Word[ index ];
        auto it = table.find( id );

        if (it != table.end()) {
            ret = *(it->second);
            return true;
        } // if

        return false;
    }

    /**
     * @brief 找出单词word的n个最相似单词
     *
     * @param word
     * @param n
     * @param result      n个最相似单词，包括所查询的word本身
     * @param distances   word和他们的相似度, 已经按升序排序
     * @param search_k    参见Annoy文档
     */
    void kNN_By_Word(const std::string &word, size_t n, 
                    std::vector<std::string> &result, std::vector<float> &distances,
                    size_t search_k = (size_t)-1 )
    {
        uint32_t id = getWordId( word );
        if (id == INVALID_ID)
            THROW_INVALID_REQUEST("WordAnnDB::kNN_By_Word() cannot find "
                    << word << " in database!");
        
        std::vector<uint32_t> resultIds;
        kNN_By_Id( id, n, resultIds, distances, search_k );
        result.resize( resultIds.size() );

        for (std::size_t i = 0; i != resultIds.size(); ++i) {
            getWordById( resultIds[i], result[i] );
            // LOG_IF(WARNING, !ret) << "cannot find word with id " << resultIds[i];
        } // for
    }

    void kNN_By_Id(uint32_t id, size_t n, 
                    std::vector<uint32_t> &result, std::vector<float> &distances,
                    size_t search_k = (size_t)-1 )
    {
        result.clear(); distances.clear();
        // wrapped func use push_back
        // 返回结果已按升序排列
        m_AnnIndex.get_nns_by_item( id, n, search_k, &result, &distances );
    }

    void kNN_By_Vector(const std::vector<float> &v, size_t n, 
                    std::vector<uint32_t> &result, std::vector<float> &distances,
                    size_t search_k = (size_t)-1 )
    {
        if (v.size() != (std::size_t)m_nFields)
            THROW_INVALID_REQUEST("WordAnnDB::kNN_By_Vector() input vector size invalid!");
        result.clear(); distances.clear();
        m_AnnIndex.get_nns_by_vector( &v[0], n, search_k, &result, &distances );
    }

    void kNN_By_Vector(const std::vector<float> &v, size_t n, 
                    std::vector<std::string> &result, std::vector<float> &distances,
                    size_t search_k = (size_t)-1 )
    {
        vector<uint32_t>    resultIds;

        kNN_By_Vector( v, n, resultIds, distances, search_k );

        result.resize( resultIds.size() );
        for (std::size_t i = 0; i != resultIds.size(); ++i) {
            getWordById( resultIds[i], result[i] );
        } // for
    }

    /**
     * @brief   查单词对应ID
     */
    uint32_t getWordId( const std::string &word )
    {
        StringPtr pWord = std::make_shared<std::string>(word);
        const char *p = pWord->c_str();
        uint8_t i = (uint8_t)(*p++);
        uint8_t j = (uint8_t)(*p);
        const WordIdTable &table = m_mapWord2Id[i][j];
        auto it = table.find( pWord );
        return (it == table.end() ? INVALID_ID : it->second);
    }

private:
    std::pair<uint32_t, bool> insert2WordIdTable( const StringPtr &pWord, uint32_t id )
    {
        const char *p = pWord->c_str();
        uint8_t i = (uint8_t)(*p++);
        uint8_t j = (uint8_t)(*p);
        WordIdTable &table = m_mapWord2Id[i][j];
        auto result = table.insert( std::make_pair(pWord, id) );
        std::pair<uint32_t, bool> ret;
        ret.second = result.second;
        ret.first = (result.first)->second;
        return ret;
    }

    void insert2IdWordTable( uint32_t id, const StringPtr &pWord )
    {
        uint32_t index = id % ID_WORD_HASH_SIZE;
        IdWordTable &table = m_mapId2Word[ index ];
        table[id] = pWord;
    }

    // 只插入word不建立Annoy index，用于从文件读入 word table.
    std::pair<uint32_t, bool> addWord( const std::string &word, uint32_t id )
    {
        StringPtr pWord = std::make_shared<std::string>(word);

        std::pair<uint32_t, bool> ret = insert2WordIdTable( pWord, id );
        if (ret.second) {
            insert2IdWordTable( id, pWord );
        } // if

        return ret;
    }

private:
    WordIdTable         m_mapWord2Id[UCHAR_MAX + 1][UCHAR_MAX + 1];  // 取单词前2个字符做hash
    IdWordTable         m_mapId2Word[ID_WORD_HASH_SIZE];
    int                 m_nFields;
    WordAnnIndex        m_AnnIndex;

    static uint32_t     s_nIdIndex;
};


} // namespace KNN

#endif

