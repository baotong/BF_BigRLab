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

