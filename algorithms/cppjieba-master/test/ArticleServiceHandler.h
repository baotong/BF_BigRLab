#ifndef _ARTICLE_SERVICE_HANDLER_H_
#define _ARTICLE_SERVICE_HANDLER_H_

#include "ArticleService.h"

namespace Article {

class ArticleServiceHandler : public ArticleServiceIf {
public:
    virtual void setFilter(const std::string& filter);
    virtual void wordSegment(std::vector<std::string> & _return, const std::string& sentence);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace Article


#endif

