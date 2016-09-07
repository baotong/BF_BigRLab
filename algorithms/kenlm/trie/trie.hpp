#ifndef _TRIE_HPP_
#define _TRIE_HPP_

#include <memory>
#include <set>
#include <cassert>


template<typename T>
class Trie {
public:
    typedef T                                  elem_type;
    typedef typename std::shared_ptr<T>        elem_pointer;

    struct ElemPtrCmp {
        constexpr bool operator() (const elem_pointer &lhs, const elem_pointer &rhs) const
        { return *lhs < *rhs; }
    };

    struct Node : std::enable_shared_from_this<Node> {
        typedef typename std::shared_ptr<Node>     pointer;
        typedef typename std::weak_ptr<Node>       weak_pointer;

        struct PointerCmp {
            constexpr bool operator() (const pointer &lhs, const pointer &rhs) const
            { return lhs->data() < rhs->data(); }
        };

        typedef typename std::set<pointer, PointerCmp> ChildrenSet;

        Node() = default;
        Node( const elem_pointer &_Data ) : m_pData(_Data) {}

        T& data() { return *m_pData; }
        const T& data() const { return *m_pData; }

        void setParent(const pointer &_Parent)
        { m_wpParent = _Parent; }
        pointer parent() const
        { return m_wpParent.lock(); }

        std::pair<typename ChildrenSet::iterator, bool>     // NOTE!!! typname must
        addChild(const pointer &_Child)
        { assert(_Child->m_pData); return m_setChildren.insert(_Child); }

        pointer child( const pointer &p )
        {
            auto it = m_setChildren.find(p);
            return (it == m_setChildren.end() ? pointer() : *it);
        }

        pointer child( const elem_pointer &p )
        {
            Node node(p);
            pointer pNode(&node, [](Node*){/* Do nothing */});
            return child(pNode);
        }

        pointer child( const elem_type& data )
        {
            elem_pointer pElem(
                const_cast<elem_type*>(&data),
                [](elem_type*){/* Do nothing */}
            );
            return child(pElem);
        }

        ChildrenSet& children() { return m_setChildren; }

        bool removeChild( const pointer &pChild )
        { return m_setChildren.erase( pChild ); }

        bool removeSelf()
        {
            pointer pParent = parent();
            if (pParent)
                pParent->removeChild( this->shared_from_this() ); // NOTE!!! this must,  error: there are no arguments to ‘shared_from_this’ that depend on a template parameter, so a declaration of ‘shared_from_this’ must be available
        }

        void setWeight(const double &w) { m_fWeight = w; }
        double weight() const { return m_fWeight; }

        elem_pointer    m_pData;
        double          m_fWeight;
        ChildrenSet     m_setChildren;
        weak_pointer    m_wpParent;
    }; // struct Node

public:
    Trie()
    {
    }

    virtual ~Trie() = default;

protected:
    typename Node::pointer                      m_pRoot;
    typename std::set<elem_pointer, ElemPtrCmp> m_setElems;
};


#endif

