#include <string>
#include <sstream>
#include <iterator>
#include <glog/logging.h>
#include "trie.hpp"


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
        return BaseType::lookup( istream_iterator<string>(stream), istream_iterator<string>() );
    }
};


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

    tree.traverse(cout);
    cout << endl;

    cout << "Totally " << tree.countNodes() << " nodes in this tree." << endl;

    for (const auto &v : tree.elemSet())
        cout << *v << " ";
    cout << endl;
    cout << "Totally " << tree.elemSet().size() << " elems in this tree." << endl;

    // check level nodes
    StringTrie::NodeMatrix &levels = tree.levelNodes();
    for (size_t i = 0; i < levels.size(); ++i) {
        cout << i+1 << ": ";
        for (auto &v : levels[i])
            cout << v->data() << " ";
        cout << endl;
    } // for

    // test lookup
    auto ret = tree.lookup("she is a lawyer");
    if (ret.second)
        cout << "Found!" << endl;
    else
        cout << "Not found!" << endl;

    {
        deque<StringTrie::elem_pointer> path;
        ret.first->getPath( path );
        for (auto &v : path)
            cout << *v << " ";
        cout << endl;
    }
}


int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);

    try {
        test1();

    } catch (const std::exception &ex) {
        cerr << ex.what() << endl;
        return -1;
    } // try

    return 0;
}

