/*
 * Usage:
 * GLOG_logtostderr=1 /tmp/test -searchk 3 -model text.bin < knn_result.txt
 */
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <cassert>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "lm/model.hh"
#include "trie.hpp"
#include "ngram_model.hpp"

DEFINE_int32(searchk, 0, "beam_search's k");
DEFINE_string(model, "", "filename of kenlm model (binary file)");

namespace {
using namespace std;
static inline
bool check_above_zero(const char* flagname, gflags::int32 value)
{
    if (value <= 0) {
        cerr << "value of " << flagname << " must be greater than 0" << endl;
        return false;
    } // if
    return true;
}
static inline
bool check_not_empty(const char* flagname, const std::string &value) 
{
    if (value.empty()) {
        cerr << "value of " << flagname << " cannot be empty" << endl;
        return false;
    } // if
    return true;
}
static bool validate_searchk(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool searchk_dummy = gflags::RegisterFlagValidator(&FLAGS_searchk, &validate_searchk);
static bool validate_model(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool model_dummy = gflags::RegisterFlagValidator(&FLAGS_model, &validate_model);
} // namespace



class StringTrie : public Trie<std::string> {
public:
    typedef Trie<std::string>       BaseType;
public:
    Node::pointer addText( const std::string &text )
    {
        using namespace std;
        stringstream stream(text);
        return addPath( istream_iterator<string>(stream), istream_iterator<string>() );
    }

    std::pair<Node::pointer, bool>
    lookup( const std::string &text )
    {
        using namespace std;
        stringstream stream(text);
        return BaseType::lookup( istream_iterator<string>(stream), istream_iterator<string>() ); // NOTE!!! BaseType is must
    }
};


template<typename T>
void print_tree_info( Trie<T> &tree )
{
    using namespace std;

    tree.traverse(cout);
    cout << endl;

    cout << "Totally " << tree.countNodes() << " nodes in this tree." << endl;

    for (const auto &v : tree.elemSet())
        cout << *v << ":" << v.use_count() << " ";
    cout << endl;
    cout << "Totally " << tree.elemSet().size() << " elems in this tree." << endl;
}


void test1()
{
    using namespace std;

    StringTrie tree;
    tree.addText( "I am a student" );
    tree.addText( "I am a programmer" );
    tree.addText( "I am working in BlueFocus" );
    tree.addText( "I am working in Microsoft" );
    tree.addText( "I am studying in PKU" );
    tree.addText( "You are a lawyer" );
    tree.addText( "You are a student" );
    tree.addText( "You are studying in PKU" );
    tree.addText( "You are working in Microsoft" );

    auto ptr = tree.addText( "I am studying in CMU" );
    {
        deque<StringTrie::elem_pointer> path;
        ptr->getPath(path);
        cout << "Path is: ";
        for (const auto &v : path)
            cout << *v << " ";
        cout << endl;
    }

    print_tree_info(tree);

    // test lookup
    auto ret = tree.lookup("I am a teacher");
    if (ret.second)
        cout << "Found!" << endl;
    else
        cout << "Not found!" << endl;

    // lookup path
    {
        deque<StringTrie::elem_pointer> path;
        ret.first->getPath( path );
        for (auto &v : path)
            cout << *v << " ";
        cout << endl;
    }
    
    // test removeSelf
    ret.first->removeSelf();
    cout << "After remove node..." << endl;
    print_tree_info(tree);
    cout << endl;
}


std::unique_ptr<NGram_Model>    g_pLMmodel;


void test3()
{
    using namespace std;

    string text = "姑苏 城外 寒山寺";

    cout << g_pLMmodel->score(text) << endl;
    
    using StringPtr = std::shared_ptr<std::string>;

    {
        deque<StringPtr> list;
        stringstream stream(text);
        string tmp;
        while (stream >> tmp)
            list.push_back(std::make_shared<string>(tmp));
        cout << g_pLMmodel->score(list.begin(), list.end()) << endl;
    }
}


std::ostream& operator << (std::ostream &os, const std::deque<StringTrie::elem_pointer> &path)
{
    for (const auto &v : path)
        os << *v << " ";
    return os;
}


typedef std::vector<std::vector<std::string>>  StringMatrix;

void beam_search( const StringMatrix &strMat, std::size_t searchK )
{
    using namespace std;

    struct NodeCmp {
        bool operator()(const StringTrie::Node::pointer &lhs,
                        const StringTrie::Node::pointer &rhs) const
        { return lhs->weight() < rhs->weight(); }
    };

    typedef std::multiset<StringTrie::Node::pointer, NodeCmp>   WorkSet;

    if (strMat.size() <= 1)
        return;

    StringTrie                              trie;
    WorkSet                                 lastLevel, curLevel;
    std::deque<StringTrie::elem_pointer>    path;
    std::vector<std::size_t>                searchkVec;

    searchkVec.resize( strMat.size() );
    for (auto it = searchkVec.rbegin(); it != searchkVec.rend(); ++it)
        *it = searchK * (std::distance(searchkVec.rbegin(), it) + 1);

    // DLOG(INFO) << "searchkVec:";
    // std::copy(searchkVec.begin(), searchkVec.end(), ostream_iterator<size_t>(DLOG(INFO), " "));

    auto rowIt = strMat.begin();
    auto& firstRow = *rowIt++;
    // add first row to tree
    for (auto &v : firstRow) {
        auto ret = trie.addNode( trie.root(), v );
        if (ret.second)
            curLevel.insert(ret.first);
    } // for
    // trie.traverse(cout);
    // cout << endl;
    // cout << curLevel.size() << endl;

    // from 2nd row to end
    for (auto searchkIt = searchkVec.begin()+1; rowIt != strMat.end(); ++rowIt, ++searchkIt) {
        curLevel.swap(lastLevel);
        curLevel.clear();
        // for every word in this row
        for (auto &word : *rowIt) {
            // for every node in last level
            for (auto &parent : lastLevel) {
                auto ret = trie.addNode(parent, word);
                if (ret.second) {
                    auto pNewChild = ret.first;
                    pNewChild->getPath(path);
                    double score = g_pLMmodel->score(path.begin(), path.end());
                    pNewChild->setWeight(score);
                    DLOG(INFO) << score << "\t" << path;
                    // DLOG(INFO) << "searchK = " << *searchkIt;
                    // try to add to cur level
                    if (curLevel.size() < *searchkIt) {
                        curLevel.insert(pNewChild);   
                    } else if (pNewChild->weight() > (*curLevel.begin())->weight()) {
                        auto pRemoved = *curLevel.begin();
                        curLevel.erase(curLevel.begin());
                        pRemoved->removeSelf();
                        curLevel.insert(pNewChild);
                    } else {
                        pNewChild->removeSelf();
                    } // if
                } // if ret.second
            } // for parent
        } // for word
    } // for

    // trie.syncElems();

#ifndef NDEBUG
    cout << "Final results:" << endl;
    for (auto it = curLevel.rbegin(); it != curLevel.rend(); ++it) {
        auto &node = *it;
        node->getPath(path);
        cout << node->weight() << "\t" << path << endl;
    } // for
#endif
}

void test4()
{
    using namespace std;
    
    StringMatrix    strMat;
    string          line;

    while (getline(cin, line)) {
        stringstream stream(line);
        strMat.emplace_back();
        std::copy( istream_iterator<string>(stream),
                   istream_iterator<string>(),
                   back_inserter(strMat.back()) );
    } // while

    // print matrix
    // for (auto &row : strMat) {
        // std::copy( row.begin(), row.end(), ostream_iterator<string>(cout, " ") );
        // cout << endl;
    // } // for

    beam_search(strMat, (size_t)FLAGS_searchk);
}


int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        // test1();
        
        cout << "Initialzing LM model..." << endl;
        g_pLMmodel.reset(new NGram_Model(FLAGS_model));
        cout << "LM model initialize done!" << endl;
        
        test4();

        // test3();
        
        // g_pLMmodel.reset(new lm::ngram::Model("text.bin"));
        // test2();

    } catch (const std::exception &ex) {
        cerr << ex.what() << endl;
        return -1;
    } // try

    return 0;
}






#if 0
std::unique_ptr<lm::ngram::Model>       g_pLMmodel;

double get_score( const std::string &text)
{
    using namespace std;
    using namespace lm::ngram;

    auto &vocab = g_pLMmodel->GetVocabulary();
    lm::FullScoreReturn ret; // score
    Model::State state, out_state;
    double total = 0.0;
    vector<string> vec;

    stringstream stream(text);
    std::copy( istream_iterator<string>(stream), istream_iterator<string>(),
                back_inserter(vec) );

    state = g_pLMmodel->BeginSentenceState();
    for (auto &v : vec) {
        ret = g_pLMmodel->FullScore(state, vocab.Index(v), out_state);
        total += ret.prob;
        state = out_state;
    } // for
    ret = g_pLMmodel->FullScore(state, vocab.EndSentence(), out_state);
    total += ret.prob;

    return total;
}

void test2()
{
    using namespace std;

    string line;
    double score = 0.0;
    while (getline(cin, line)) {
        score = get_score(line);
        cout << score << "\t" << line << endl;
    } // while
}
#endif

#if 0
// official example
#include "lm/model.hh"
#include <iostream>
#include <string>
#include <memory>
int main() {
  using namespace lm::ngram;
  // std::unique_ptr<Model> pModel(new Model("text.bin"));
  // Model *pModel = new Model("text.bin");
  Model model("text.bin");
  State state(model.BeginSentenceState()), out_state;
  const Vocabulary &vocab = model.GetVocabulary();
  std::string word;
  while (std::cin >> word) {
    std::cout << model.Score(state, vocab.Index(word), out_state) << '\n';
    state = out_state;
  }
}
#endif


