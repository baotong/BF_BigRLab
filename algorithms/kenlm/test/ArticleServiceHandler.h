#ifndef _ARTICLE_SERVICE_HANDLER_H_
#define _ARTICLE_SERVICE_HANDLER_H_

#include "ArticleService.h"
#include "AnnDB.hpp"

// AnnDB
typedef uint32_t IdType;
typedef float    ValueType;
extern boost::shared_ptr<WordAnnDB> g_pAnnDB;


namespace Article {

class ArticleServiceHandler : public ArticleServiceIf {
public:
    typedef std::vector<Jieba::Gram>    GramArray;
    typedef std::vector<GramArray>      GramMatrix;
    typedef std::vector<std::vector<std::string>>  StringMatrix;

public:
    virtual void setFilter(const std::string& filter);
    virtual void creativeRoutine(std::vector<Result> & _return, 
            const std::string& input, const int32_t k, const int32_t bSearchK, const int32_t topk);
    virtual void handleRequest(std::string& _return, const std::string& request);
private:
    void knn(const GramArray &arr, StringMatrix &result, int32_t k);
    void beam_search( std::vector<Result> &result, 
                      const StringMatrix &strMat, std::size_t searchK, std::size_t topk );
};

} // namespace Article


#endif

