#ifndef _ARTICLE_SERVICE_HANDLER_H_
#define _ARTICLE_SERVICE_HANDLER_H_

#include "ArticleService.h"
#include "Article2Vector.h"
#include "AnnDB.hpp"

// Article2Vector
extern Article2Vector::pointer                 g_pVecConverter;

// AnnDB
typedef uint32_t IdType;
typedef float    ValueType;
typedef AnnDB<IdType, ValueType>    AnnDbType;
extern boost::shared_ptr<AnnDbType> g_pAnnDB;

namespace Article {

class ArticleServiceHandler : public ArticleServiceIf {
public:
    virtual void setFilter(const std::string& filter);
    virtual void wordSegment(std::vector<std::string> & _return, const std::string& sentence);
    virtual void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k);
    virtual void toVector(std::vector<double> & _return, const std::string& sentence);
    virtual void knn(std::vector<KnnResult> & _return, const std::string& sentence, 
            const int32_t n, const int32_t searchK);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace Article


#endif

