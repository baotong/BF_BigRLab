#include <sstream>
#include <fstream>
#include "common.hpp"
#include "TopicModule.h"

TopicModule::TopicModule(const std::string &vocabFile, const std::string &modelFile)
{
    using namespace std;

    m_pVocab.reset(new Vocab);
    m_pVocab->load(vocabFile);

    // load model
    {
        size_t nWords = 0, nTopics = 0, alpha = 0.0, beta = 0.0;
        ifstream ifs(modelFile, ios::in);
        ifs >> nWords >> nTopics >> alpha >> beta;
        if (!nTopics || ifs.fail() || ifs.bad())
            THROW_RUNTIME_ERROR("Bad model file!");
        m_nTopics = nTopics;
    }
    m_pLda.reset(new WarpLDA<1>());
    m_pLda->loadModel(modelFile);
}

void TopicModule::predict(const std::string &text, Result &result, 
                int nIter, int perplexity, int nSkip)
{
    using namespace std;

    result.clear();
    stringstream uIdxStream(ios::in | ios::out | ios::binary), 
                 vIdxStream(ios::in | ios::out | ios::binary), 
                 uLnkStream(ios::in | ios::out | ios::binary), 
                 vLnkStream(ios::in | ios::out | ios::binary);
    text_to_bin(text, nSkip, uIdxStream, vIdxStream, uLnkStream, vLnkStream);
    m_pLda->loadBinary(uIdxStream, vIdxStream, uLnkStream, vLnkStream);
    m_pLda->inference(nIter, perplexity);
    m_pLda->storeZ(result);
}

void TopicModule::text_to_bin(const std::string &text, 
                int nSkip,
                std::ostream &uIdxStream,
                std::ostream &vIdxStream,
                std::ostream &uLnkStream,
                std::ostream &vLnkStream)
{
    int doc_id = 0;
    std::vector<std::pair<TUID, TVID>>  edge_list;   // TUID TVID all uint32_t
    std::vector<TVID>                   vlist;

    parse_document(text, vlist, nSkip);
    for (auto word_id : vlist)
        edge_list.emplace_back(doc_id, word_id);
    ++doc_id;

    // Shuffle tokens
    std::vector<TVID> new_vid(m_pVocab->nWords());
    for (unsigned i = 0; i < new_vid.size(); ++i)
        new_vid[i] = i;
    m_pVocab->RearrangeId(new_vid.data()); // TODO NOTE!!! useful? 要改变vocab的, not thread-safe

    for (auto &e : edge_list)
        e.second = new_vid[e.second];

    Bigraph::Generate(uIdxStream, vIdxStream, 
            uLnkStream, vLnkStream, edge_list, m_pVocab->nWords());      // 生成输出文件
}

void TopicModule::parse_document(const std::string &line, std::vector<TVID> &v, int nSkip)
{
    std::istringstream sin(line);
    std::string w;
    v.clear();
    for (int i = 0; sin >> w; i++) {
        if (i >= nSkip) {
            int vid = m_pVocab->getIdByWord(w);
            if (vid != -1) v.push_back(vid);
        } // if
    } // for
}

