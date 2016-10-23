#ifndef _TOPIC_SERVICE_HANDLER_H_
#define _TOPIC_SERVICE_HANDLER_H_

#include "TopicService.h"
#include "TopicModule.h"
#include "lock_free_queue.hpp"

namespace Topics {

class TopicServiceHandler : public TopicServiceIf {
public:
    virtual void predictTopic(std::vector<Result> & _return, const std::string& text, const int32_t topk, 
                const int32_t nIter, const int32_t perplexity, const int32_t nSkip) override;
    virtual void handleRequest(std::string& _return, const std::string& request) override;
};

} // namespace Topics

extern std::unique_ptr< LockFreeQueue<TopicModule*> >  g_queTopicModules;

#endif

