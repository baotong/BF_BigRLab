#ifndef _SEMANTIC_TREE_H_
#define _SEMANTIC_TREE_H_

#include <string>
#include <memory>
#include <set>
#include <map>


class SemanticTreeNode : public std::enable_shared_from_this<SemanticTreeNode> {
public:
    typedef std::shared_ptr<SemanticTreeNode>   pointer;
    typedef std::weak_ptr<SemanticTreeNode>     wpointer;
    typedef std::set<std::string>               StringSet;
    typedef std::map<std::string, pointer>      ChildrenSet;

public:
    explicit SemanticTreeNode(const std::string &_Domain) 
                : m_strDomain(_Domain) {}

    const std::string& domain() const
    { return m_strDomain; }

    void addConcept(const std::string &_Concept)
    { m_setConcepts.insert(_Concept); }
    bool hasConcept(const std::string &_Concept)
    { return m_setConcepts.count(_Concept); }
    const StringSet& concepts() const
    { return m_setConcepts; }

    void addResponse(const std::string &res)
    { m_setResponse.insert(res); }

    void addChild(const pointer &pNode)     // NOTE!!! 设一个node只有唯一的parent
    { 
        m_mapChildren[pNode->domain()] = pNode; 
        pNode->setParent(shared_from_this());
    }

    const ChildrenSet& children() const { return m_mapChildren; }

    bool getChild(const std::string &_Domain, pointer &pChild)
    {
        auto it = m_mapChildren.find(_Domain);
        if (it == m_mapChildren.end())
            return false;
        pChild = it->second;
        return true;
    }

    void setParent(const pointer &pParent)
    { m_wpParent = pParent; }

    pointer parent() const
    { return m_wpParent.lock(); }

    std::size_t count() const
    {
        std::size_t ret = 1;
        for (const auto &kv : children())
            ret += kv.second->count();
        return ret;
    }

    void print(std::ostream &os, std::size_t level) const;

private:
    std::string     m_strDomain;
    StringSet       m_setConcepts, m_setResponse;
    ChildrenSet     m_mapChildren;
    wpointer        m_wpParent;
};


class SemanticTreeSet {
public:
    typedef SemanticTreeNode::pointer             NodePointer;
    typedef std::map<std::string, NodePointer>    NodeTable;

public:
    void loadFromJson(const std::string &fname);

    NodePointer addNode(const NodePointer &pNode)
    {
        auto ret = m_mapNodeTable.insert(std::make_pair(pNode->domain(), pNode));
        return ret.first->second;
    }

    NodeTable& trees()
    { return m_mapNodeTable; }

    void print(std::ostream &os) const;

private:
    NodeTable       m_mapNodeTable;
};


#endif /* _SEMANTIC_TREE_H_ */

