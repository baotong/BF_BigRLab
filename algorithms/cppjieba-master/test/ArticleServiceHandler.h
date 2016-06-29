#ifndef _ARTICLE_SERVICE_HANDLER_H_
#define _ARTICLE_SERVICE_HANDLER_H_

#include "ArticleService.h"
#include "Article2Vector.h"

extern Article2Vector::pointer                g_pWordVecConverter;
extern Article2Vector::pointer                g_pClusterIdConverter;

namespace Article {

class ArticleServiceHandler : public ArticleServiceIf {
public:
    virtual void setFilter(const std::string& filter);
    virtual void wordSegment(std::vector<std::string> & _return, const std::string& sentence);
    virtual void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k);
    virtual void toVector(std::vector<double> & _return, const std::string& sentence, const VectorMethod method);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace Article


#endif

