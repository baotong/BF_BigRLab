#ifndef _ANN_DB_H_
#define _ANN_DB_H_

#include "common.h"
#include "annoylib.h"
#include <vector>
#include <fstream>
#include <glog/logging.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include "jieba.hpp"

class WordAnnDB {
public:
    typedef std::shared_ptr<std::string> StringPtr;

    struct StringPtrCmp {
        bool operator() ( const StringPtr &lhs, const StringPtr &rhs ) const
        { return (*lhs < *rhs); }   
    };

    using Gram = Jieba::Gram;
    using GramPtrCmp = Jieba::GramPtrCmp;

    typedef std::map< Gram::pointer, uint32_t, GramPtrCmp >     WordIdTable;
    typedef std::map< uint32_t, Gram::pointer >                 IdWordTable;
    typedef AnnoyIndex< uint32_t, float, Angular, RandRandom >  WordAnnIndex;

    static const uint32_t   INVALID_ID = (uint32_t)-1;
    static const uint32_t   ID_WORD_HASH_SIZE = (UCHAR_MAX + 1) * (UCHAR_MAX + 1);

public:
    explicit WordAnnDB( int nFields )
                : m_nFields(nFields)
                , m_AnnIndex(nFields) {}


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
    // void saveWordTable( const char *filename );
    /**
     * @brief  从文件中导入单词到ID的映射
     */
    void loadWordTable( const char *filename );

#if 0
    /**
     * @brief   查询两单词的相似度
     */
    float getDistance( const std::string &s1, const std::string &s2 )
    {
        uint32_t i = getWordId( s1 );
        if (i == INVALID_ID)
            THROW_RUNTIME_ERROR("WordAnnDB::getDistance() cannot find "
                    << s1 << " in database!");
        uint32_t j = getWordId( s2 );
        if (j == INVALID_ID)
            THROW_RUNTIME_ERROR("WordAnnDB::getDistance() cannot find "
                    << s2 << " in database!");
        return getDistance( i, j );
    }

    float getDistance( uint32_t i, uint32_t j )
    { return m_AnnIndex.get_distance(i, j); }

    void getVector( const std::string &word, std::vector<float> &ret )
    {
        uint32_t i = getWordId( word );
        if (i == INVALID_ID)
            THROW_RUNTIME_ERROR("WordAnnDB::getVector() cannot find "
                    << word << " in database!");
        getVector( i, ret );
    }

    void getVector( uint32_t i, std::vector<float> &ret )
    {
        ret.resize( m_nFields );
        m_AnnIndex.get_item(i, &ret[0]);
    }
#endif

    /**
     * @brief   查ID对应单词
     */
    bool getWordById( uint32_t id, Gram &ret )
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
                    std::vector<Gram> &result, std::vector<float> &distances,
                    size_t search_k = (size_t)-1 )
    {
        result.clear(); distances.clear();

        uint32_t id = getWordId( word );
        if (id == INVALID_ID)
            return;
        
        std::vector<uint32_t> resultIds;
        kNN_By_Id( id, n, resultIds, distances, search_k );
        result.resize( resultIds.size() );

        for (std::size_t i = 0; i != resultIds.size(); ++i) {
            getWordById( resultIds[i], result[i] );
            // LOG_IF(WARNING, !ret) << "cannot find word with id " << resultIds[i];
        } // for
    }

    void kNN_By_Gram(const Gram &gram, size_t n, 
                    std::vector<Gram> &result, std::vector<float> &distances,
                    size_t search_k = (size_t)-1 )
    {
        result.clear(); distances.clear();

        uint32_t id = getGramId( gram );
        if (id == INVALID_ID)
            return;
        
        std::vector<uint32_t> resultIds;
        kNN_By_Id( id, n, resultIds, distances, search_k );
        result.resize( resultIds.size() );

        for (std::size_t i = 0; i != resultIds.size(); ++i) {
            getWordById( resultIds[i], result[i] );
            // LOG_IF(WARNING, !ret) << "cannot find word with id " << resultIds[i];
        } // for

        auto it1 = result.begin();
        auto it2 = distances.begin();
        for (; it1 != result.end();) {
            if (it1->type != gram.type || it1->word == gram.word) {
                it1 = result.erase(it1);
                it2 = distances.erase(it2);
            } else {
                ++it1; ++it2;
            } // if
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
        if (v.size() != m_nFields)
            THROW_RUNTIME_ERROR("WordAnnDB::kNN_By_Vector() input vector size invalid!");
        result.clear(); distances.clear();
        m_AnnIndex.get_nns_by_vector( &v[0], n, search_k, &result, &distances );
    }

    void kNN_By_Vector(const std::vector<float> &v, size_t n, 
                    std::vector<Gram> &result, std::vector<float> &distances,
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
        auto pWord = std::make_shared<Gram>(word);
        const char *p = pWord->word.c_str();
        uint8_t i = (uint8_t)(*p++);
        uint8_t j = (uint8_t)(*p);
        const WordIdTable &table = m_mapWord2Id[i][j];
        auto it = table.find( pWord );
        return (it == table.end() ? INVALID_ID : it->second);
    }

    uint32_t getGramId( const Gram &gram )
    {
        Gram &_gram = const_cast<Gram&>(gram);
        Gram::pointer pWord(&_gram, [](Gram*){});
        const char *p = pWord->word.c_str();
        uint8_t i = (uint8_t)(*p++);
        uint8_t j = (uint8_t)(*p);
        const WordIdTable &table = m_mapWord2Id[i][j];
        auto it = table.find( pWord );
        return (it == table.end() ? INVALID_ID : it->second);
    }

private:
    std::pair<uint32_t, bool> insert2WordIdTable( const Gram::pointer &pWord, uint32_t id )
    {
        const char *p = pWord->word.c_str();
        uint8_t i = (uint8_t)(*p++);
        uint8_t j = (uint8_t)(*p);
        WordIdTable &table = m_mapWord2Id[i][j];
        auto result = table.insert( std::make_pair(pWord, id) );
        std::pair<uint32_t, bool> ret;
        ret.second = result.second;
        ret.first = (result.first)->second;
        return ret;
    }

    void insert2IdWordTable( uint32_t id, const Gram::pointer &pWord )
    {
        uint32_t index = id % ID_WORD_HASH_SIZE;
        IdWordTable &table = m_mapId2Word[ index ];
        table[id] = pWord;
    }

    // 只插入word不建立Annoy index，用于从文件读入 word table.
    std::pair<uint32_t, bool> addWord( const std::string &word, uint32_t id )
    {
        auto pWord = std::make_shared<Gram>(word);

        std::pair<uint32_t, bool> ret = insert2WordIdTable( pWord, id );
        if (ret.second) {
            insert2IdWordTable( id, pWord );
        } // if

        // DLOG_IF(INFO, !ret.second) << word << " already exists!";

        return ret;
    }

private:
    WordIdTable         m_mapWord2Id[UCHAR_MAX + 1][UCHAR_MAX + 1];  // 取单词前2个字符做hash
    IdWordTable         m_mapId2Word[ID_WORD_HASH_SIZE];
    WordAnnIndex        m_AnnIndex;
    int                 m_nFields;
};

inline
void WordAnnDB::loadWordTable( const char *filename )
{
    using namespace std;

    ifstream ifs( filename, ios::in );

    if (!ifs)
        THROW_RUNTIME_ERROR("WordAnnDB::saveWordTable() cannot open " << filename << " for reading!");

    string line, word;
    uint32_t id = 0;
    while (getline(ifs, line)) {
        stringstream str(line);
        str >> word >> id;
        // DLOG_IF(ERROR, bad_stream(str)) << "read error in line: " << line;
        if (bad_stream(str))
            continue;
        addWord( word, id );
    } // while

    boost::ptr_vector<IdWordTable::value_type, boost::view_clone_allocator> ptrArray;
    for (auto &table : m_mapId2Word)
        ptrArray.insert(ptrArray.end(), table.begin(), table.end());

    // DLOG(INFO) << "Getting words type...";

#pragma omp parallel for
    for (size_t i = 0; i < ptrArray.size(); ++i) {
        Jieba::TagResult tagResult;
        g_pJieba->tagging( ptrArray[i].second->word, tagResult );
        if (!tagResult.empty())
            ptrArray[i].second->type = tagResult[0].second;
    } // for

    // DLOG(INFO) << "Load word table done!";

    // DEBUG
    // for (size_t i = 0; i < UCHAR_MAX+1; ++i) {
        // for (size_t j = 0; j < UCHAR_MAX+1; ++j) {
            // auto &table = m_mapWord2Id[i][j];
            // for (auto &v : table)
                // cout << v.first->word << "\t" << v.first->type << endl;
        // } // for j
    // } // for
}



#if 0
/**
 * @brief 
 *
 * @tparam IdType   unsigned integer
 * @tparam DataType float
 */
template <typename IdType, typename DataType>
class AnnDB {
public:
    typedef AnnoyIndex< IdType, DataType, Angular, RandRandom > WordAnnIndex;
    static const IdType                                         INVALID_ID = (IdType)-1;

public:
    explicit AnnDB( int nFields )
                : m_nFields(nFields)
                , m_AnnIndex(nFields) {}

    virtual ~AnnDB() = default;

    /**
     * @brief  建立Annoy索引，参阅 AnnoyIndex::build 
     *         More trees gives higher precision when querying. After calling ``build``, 
     *         no more items can be added.
     * @param q  number of trees
     */
    void buildIndex( int q )
    { 
        // DLOG(INFO) << "AnnDB::buildIndex() q = " << q;
        m_AnnIndex.build(q); 
    }

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

    DataType getDistance( IdType i, IdType j )
    { return m_AnnIndex.get_distance(i, j); }

    void getVector( IdType i, std::vector<DataType> &ret )
    {
        ret.resize( m_nFields );
        m_AnnIndex.get_item(i, &ret[0]);
    }

    void kNN_By_Id(IdType id, size_t n, 
                    std::vector<IdType> &result, std::vector<DataType> &distances,
                    size_t search_k = (size_t)-1 )
    {
        // DLOG(INFO) << "AnnDB::kNN_By_Id() id = " << id << ", n = " << n
                // << ", search_k = " << search_k;
        result.clear(); distances.clear();
        // wrapped func use push_back
        // 返回结果已按升序排列
        m_AnnIndex.get_nns_by_item( id, n, search_k, &result, &distances );
    }

    void kNN_By_Vector(const std::vector<DataType> &v, size_t n, 
                    std::vector<IdType> &result, std::vector<DataType> &distances,
                    size_t search_k = (size_t)-1 )
    {
        // DLOG(INFO) << "AnnDB::kNN_By_Vector()  n = " << n
                // << ", search_k = " << search_k;
        if (v.size() != m_nFields)
            THROW_RUNTIME_ERROR("AnnDB::kNN_By_Vector() input vector size " << v.size()
                    << " not equal to predefined size " << m_nFields);
        result.clear(); distances.clear();
        m_AnnIndex.get_nns_by_vector( &v[0], n, search_k, &result, &distances );
    }

    void addItem( const std::vector<DataType> &v )
    {
        if (v.size() != m_nFields)
            THROW_RUNTIME_ERROR("AnnDB::addItem() input vector size " << v.size()
                    << " not equal to predefined size " << m_nFields);
        IdType id = m_AnnIndex.get_n_items();
        // DLOG(INFO) << "addItem id = " << id;
        m_AnnIndex.add_item( id, &v[0] );
    }

    std::size_t size()
    { return m_AnnIndex.get_n_items(); }

protected:
    WordAnnIndex        m_AnnIndex;
    int                 m_nFields;
};
#endif


#endif

