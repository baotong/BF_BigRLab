#ifndef _NGRAM_MODEL_HPP_
#define _NGRAM_MODEL_HPP_

#include <memory>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include "lm/model.hh"


class NGram_Model {
public:
    explicit NGram_Model(const std::string &modelFile)
            : m_pLMmodel(std::make_shared<lm::ngram::Model>(modelFile.c_str())) {}

    template<typename Iter>
    double score(Iter beg, Iter end)
    {
        typedef typename Iter::value_type   value_type;
        return score_impl(beg, end, value_type());
    }

    double score( const std::string &text )
    {
        using namespace std;

        stringstream stream(text);
        vector<string> vec;
        std::copy(istream_iterator<string>(stream),
                  istream_iterator<string>(),
                  back_inserter(vec));
        
        return score(vec.begin(), vec.end());
    }

private:
    template<typename Iter>
    double score_impl(Iter beg, Iter end, const std::string&)
    {
        // DLOG(INFO) << "string version";

        using namespace std;
        using namespace lm::ngram;

        auto &vocab = m_pLMmodel->GetVocabulary();
        lm::FullScoreReturn ret; // score
        Model::State state, out_state;
        double total = 0.0;  

        state = m_pLMmodel->BeginSentenceState();
        for (; beg != end; ++beg) {
            ret = m_pLMmodel->FullScore(state, vocab.Index(*beg), out_state);
            total += ret.prob;
            state = out_state;           
        } // for
        ret = m_pLMmodel->FullScore(state, vocab.EndSentence(), out_state);
        total += ret.prob;

        return total;
    }

    template<typename Iter, typename PtrType>
    double score_impl(Iter beg, Iter end, const PtrType&)
    {
        // DLOG(INFO) << "pointer version";

        using namespace std;
        using namespace lm::ngram;

        auto &vocab = m_pLMmodel->GetVocabulary();
        lm::FullScoreReturn ret; // score
        Model::State state, out_state;
        double total = 0.0;  

        state = m_pLMmodel->BeginSentenceState();
        for (; beg != end; ++beg) {
            ret = m_pLMmodel->FullScore(state, vocab.Index(**beg), out_state);
            total += ret.prob;
            state = out_state;           
        } // for
        ret = m_pLMmodel->FullScore(state, vocab.EndSentence(), out_state);
        total += ret.prob;

        return total;
    }

private:
    std::shared_ptr<lm::ngram::Model>       m_pLMmodel;
};

extern std::unique_ptr<NGram_Model>         g_pLMmodel;

#endif

