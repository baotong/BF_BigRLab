#include "SemanticTree.h"
#include "CommDef.h"
#include <iostream>
#include <fstream>
#include <json/json.h>
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>


void SemanticTreeNode::print(std::ostream &os, std::size_t level) const
{
    for (std::size_t i = 0; i < level; ++i)
        os << "\t";
    os << domain() << ": [";
    if (!m_setConcepts.empty()) {
        auto cit = m_setConcepts.begin();
        auto cLast = m_setConcepts.end();
        --cLast;
        for (; cit != cLast; ++cit)
            os << *cit << ", ";
        os << *cLast;
    } // if
    os << "]" << std::endl;

    for (const auto &kv : m_mapChildren) {
        kv.second->print(os, level + 1);
    } // for
}

void SemanticTreeSet::loadFromJson(const std::string &fname)
{
    using namespace std;

    DLOG(INFO) << "SemanticTreeSet::loadFromJson() " << fname;

    Json::Value         jsTreeSet;

    // read json file
    {
        ifstream ifs(fname, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs, "SemanticTreeSet::loadFromJson() cannot open " << fname << " for reading!");

        ostringstream oss;
        oss << ifs.rdbuf() << flush;

        Json::Reader    reader;
        THROW_RUNTIME_ERROR_IF(!reader.parse(oss.str(), jsTreeSet),
                "SemanticTreeSet::loadFromJson() Invalid json format!");
    } // read json file

    for (Json::ArrayIndex i = 0; i < jsTreeSet.size(); ++i) {
        const auto &jsNode = jsTreeSet[i];
        string strDomain = jsNode["domain"].asString();
        boost::trim(strDomain);
        if (strDomain.empty()) {
            LOG(ERROR) << "Error in reading tree node " << (i+1) << ": domain cannot be empty!";
            continue;
        } // if
        DLOG(INFO) << "Reading tree node " << (i + 1) << ": " << strDomain;
        // create new node or find existing
        auto pNode = std::make_shared<SemanticTreeNode>(strDomain);
        pNode = addNode(pNode);
        // read concepts
        const auto& jsConcepts = jsNode["concepts"];
        if (!!jsConcepts) {
            for (const auto &jsVal : jsConcepts)
                pNode->addConcept(jsVal.asString());
        } // if
        // read response
        const auto& jsResp = jsNode["response"];
        if (!!jsResp) {
            for (const auto &jsVal : jsResp)
                pNode->addResponse(jsVal.asString());
        } // if
        // children
        const auto& jsChildren = jsNode["children"];
        if (!!jsChildren) {
            for (const auto &jsVal : jsChildren) {
                auto pChild = std::make_shared<SemanticTreeNode>(jsVal.asString());
                pChild = addNode(pChild);
                pNode->addChild(pChild);
            } // for
        } // if
    } // for i

    // keep only roots
    DLOG(INFO) << m_mapNodeTable.size();
    for (auto it = m_mapNodeTable.begin(); it != m_mapNodeTable.end();) {
        auto &pNode = it->second;
        if (pNode->parent()) {
            it = m_mapNodeTable.erase(it);
        } else {
            ++it;
        } // if
    } // for it
    DLOG(INFO) << m_mapNodeTable.size();
}


void SemanticTreeSet::print(std::ostream &os) const
{
    for (const auto &kv : m_mapNodeTable)
        kv.second->print(os, 0);
}


