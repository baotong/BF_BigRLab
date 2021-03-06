#ifndef _ARTICLE_SERVICE_HANDLER_H_
#define _ARTICLE_SERVICE_HANDLER_H_

#include <gflags/gflags.h>
#include "ArticleService.h"
#include "jieba.hpp"
#include "AnnDB.hpp"
#include "concur_table.hpp"
#include "ClusterPredict.h"
#include "Article2Vector.h"
#include "WordClusterDB.h"

DECLARE_double(threshold);

// Article2Vector
extern Article2Vector::pointer                 g_pVecConverter;

// AnnDB
typedef uint32_t IdType;
typedef float    ValueType;
// typedef AnnDB<IdType, ValueType>    AnnDbType;
// extern boost::shared_ptr<WordAnnDB> g_pAnnDB;
extern ClusterPredict::pointer          g_pClusterPredict;
extern WordClusterDB::pointer           g_pWordClusterDB;

// concur table
extern boost::shared_ptr<ConcurTable>          g_pConcurTable;
// tagset
extern std::set<std::string>   g_TagSet;

namespace Article {

class ArticleServiceHandler : public ArticleServiceIf {
public:
    enum TaggingMethod {
        CONCUR, KNN
    };

public:
    virtual void setFilter(const std::string& filter);
    virtual void tagging(std::vector<TagResult> & _return, const std::string& text, 
            const int32_t method, const int32_t k1, const int32_t k2, const int32_t searchK, const int32_t topk);
    virtual void handleRequest(std::string& _return, const std::string& request);
private:
    void wordSegment(std::vector<std::string> & _return, const std::string& sentence);
    void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k);

    void do_tagging_concur(std::vector<TagResult> & _return, const std::string& text, 
            const int32_t k1, const int32_t k2);
    void do_tagging_knn(std::vector<TagResult> & _return, const std::string& text, 
            const int32_t k1, const int32_t k2, const int32_t searchK);
};

} // namespace Article


#endif

