#ifndef _TOPIC_MODULE_H_
#define _TOPIC_MODULE_H_

#include <iostream>
#include <string>
#include <memory>
#include "Vocab.hpp"
#include "Bigraph.hpp"
#include "Utils.hpp"
#include "warplda.hpp"

class TopicModule {
public:
    typedef std::shared_ptr<TopicModule>    pointer;
    typedef std::vector<std::pair<uint32_t, uint32_t>>  Result;

public:
    TopicModule(const std::string &vocabFile, const std::string &modelFile);

    std::size_t nTopics() const { return m_nTopics; }

    void predict(const std::string &text, Result &result, 
                int nIter, int perplexity, int nSkip);

    void text_to_bin(const std::string &text, 
                    int nSkip,
                    std::ostream &uIdxStream,
                    std::ostream &vIdxStream,
                    std::ostream &uLnkStream,
                    std::ostream &vLnkStream);

    void parse_document(const std::string &line, std::vector<TVID> &v, int nSkip);

private:
    std::shared_ptr<Vocab>   m_pVocab;
    std::shared_ptr<LDA>     m_pLda;
    std::size_t              m_nTopics;
};


#endif

