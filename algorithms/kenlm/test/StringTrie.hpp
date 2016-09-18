#ifndef _STRING_TRIE_HPP_
#define _STRING_TRIE_HPP_

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

inline
std::ostream& operator << (std::ostream &os, const std::deque<StringTrie::elem_pointer> &path)
{
    for (const auto &v : path)
        os << *v << " ";
    return os;
}

#endif

