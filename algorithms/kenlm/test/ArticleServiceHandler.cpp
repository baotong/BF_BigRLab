#include "ArticleServiceHandler.h"
#include "jieba.hpp"
#include <glog/logging.h>
#include <json/json.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>

#define TIMEOUT     60000   // 1min

#define ON_FINISH_CLASS(name, deleter) \
    std::unique_ptr<void, std::function<void(void*)> > \
        name((void*)-1, [&, this](void*) deleter )

namespace Article {

using namespace std;

void ArticleServiceHandler::setFilter( const std::string &strFilter )
{
    THROW_INVALID_REQUEST("not implemented");
}

void ArticleServiceHandler::wordSegment(std::vector<std::string> & _return, const std::string& sentence)
{
    // if (sentence.empty())
        // THROW_INVALID_REQUEST("Input sentence cannot be empty!");

    // Jieba::pointer pJieba;
    // if (!g_JiebaPool.timed_pop(pJieba, TIMEOUT))
        // THROW_INVALID_REQUEST("No available Jieba object!");

    // ON_FINISH_CLASS(pCleanup, {g_JiebaPool.push(pJieba);});

    // try {
        // pJieba->wordSegment(sentence, _return);
    // } catch (const std::exception &ex) {
        // LOG(ERROR) << "Jieba wordSegment error: " << ex.what();
        // THROW_INVALID_REQUEST("Jieba wordSegment error: " << ex.what());
    // } // try
}

void ArticleServiceHandler::keyword(std::vector<KeywordResult> & _return, 
            const std::string& sentence, const int32_t k)
{
}

void ArticleServiceHandler::tagging(std::vector<TagResult> & _return, const std::string& text, 
        const int32_t method, const int32_t k1, const int32_t k2, const int32_t searchK, const int32_t topk)
{
}


void ArticleServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
}

} // namespace Article

