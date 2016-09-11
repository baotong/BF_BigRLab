#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <cassert>
#include <glog/logging.h>
#include "lm/model.hh"
#include "trie.hpp"
#include "ngram_model.hpp"

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



int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);

    try {
        // test1();
        
        cout << "Initialzing LM model..." << endl;
        g_pLMmodel.reset(new NGram_Model("text.bin"));
        cout << "LM model initialize done!" << endl;
        test3();
        
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


