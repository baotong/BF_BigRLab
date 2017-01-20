#ifndef _CONCUR_TABLE_HPP_
#define _CONCUR_TABLE_HPP_

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <memory>
#include <boost/variant.hpp>
#include "WordWeight.h"


class ConcurTable {
public:
    typedef unsigned long                           IdType;
    typedef std::shared_ptr<std::string>            StringPtr;

    struct StrPtrCmp {
        bool operator()(const StringPtr &lhs, const StringPtr &rhs) const
        { return (*lhs < *rhs); }
    };

    struct ConcurItem {
        boost::variant<StringPtr, IdType> item;
        double                            weight;
    };

    typedef std::vector<ConcurItem>                            ConcurItemList;
    typedef std::map<StringPtr, ConcurItemList, StrPtrCmp>     TableType;

    TableType& data()
    { return m_mapTable; }

    ConcurItemList& lookup( const std::string &key )
    {
        // NOTE!!! lookup pointer in this way
        std::string &_key = const_cast<std::string&>(key);
        StringPtr pKey(&_key, [](std::string*){ /* DLOG(INFO) << "fake delete"; */ });
        auto it = m_mapTable.find( pKey );
        if (it == m_mapTable.end())
            return m_vecEmptyList;
        return it->second;
    }

public:
    void loadFromFile( const std::string &filename );
    void loadFromFile( const std::string &filename, const std::set<std::string> &tagSet );

    std::size_t size() const
    { return m_mapTable.size(); }

private:
    TableType                       m_mapTable;
    ConcurItemList                  m_vecEmptyList;
};

inline
void ConcurTable::loadFromFile( const std::string &filename, const std::set<std::string> &tagSet )
{
    using namespace std;

    // RawTable                        m_mapRawTable;
    std::vector<StringPtr>          vecItems;

    vecItems.resize(1);
    m_mapTable.clear();

    ifstream ifs(filename, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("ConcurTable::loadFromFile() cannot read file " << filename);

    string line, strItem;
    IdType id = 0, freq = 0;
    double weight = 0.0;
    // 读入concur table
    while (getline(ifs, line)) {
        stringstream stream(line);
        auto pKey = std::make_shared<std::string>();
        stream >> *pKey;
        auto colon = pKey->rfind(':');
        if (colon == string::npos || colon == 0)
            continue;
        pKey->erase(colon);
        pKey->shrink_to_fit();
        // DLOG(INFO) << "pKey = " << *pKey;

        ConcurItemList cList;
        while (stream >> strItem) {
            if (sscanf(strItem.c_str(), "%lu:%lu:%lf", &id, &freq, &weight) != 3)
                continue;
            cList.emplace_back( ConcurItem{id, weight} );
        } // while

        // assert( !cList.empty() );

        vecItems.push_back( pKey );
        auto ret = m_mapTable.insert( std::make_pair(pKey, TableType::mapped_type()) );
        ret.first->second.swap( cList );
    } // while

    // DLOG(INFO) << "Totally " << (vecItems.size()-1) << " items loaded.";
    
    // 将concur table 中的id转换为word
    for (auto mit = m_mapTable.begin(); mit != m_mapTable.end();) {
        auto &cList = mit->second;
        for (auto it = cList.begin(); it != cList.end(); ++it) {
            auto idx = boost::get<IdType>(it->item);
            assert( idx != 0 && idx < vecItems.size() );
            auto pStr = vecItems[idx];
            it->item = pStr;
        } // for it

        if (cList.empty())
            m_mapTable.erase(mit++);
        else
            ++mit;
    } // for mit

    // 添加到g_WordWgtTable中
    for (auto mit = m_mapTable.begin(); mit != m_mapTable.end();) {
        string &key = *(mit->first);
        // 如果key未在tagSet中出现，就略过
        if (!tagSet.count(key)) {
            mit = m_mapTable.erase(mit);
        } else {
            auto &cList = mit->second;
            auto ret = g_WordWgtTable.insert(std::make_pair(key, WordWeightTable::mapped_type()));
            ret.first->second.weight = cList.begin()->weight;
            ret.first->second.count = 1;
            for (auto it = cList.begin() + 1; it != cList.end(); ++it) {
                auto& word = *(boost::get<StringPtr>(it->item));
                // 只对tagSet未出现的 concur word 使用平均weight
                if (!tagSet.count(word)) {
                    auto cret = g_WordWgtTable.insert(std::make_pair(word, WordWeightTable::mapped_type()));
                    cret.first->second.weight += it->weight;
                    ++(ret.first->second.count);
                } // if
            } // for it
            ++mit;
        } // if
    } // for mit

    // 归一化 g_WordWgtTable
    for (auto &kv : g_WordWgtTable) {
        if (kv.second.count > 1)
            kv.second.weight /= (double)(kv.second.count);
    } // for

    // DEBUG
#if 0
    for (auto &kv : m_mapTable) {
        // cout << *(kv.first) << "\t";
        DLOG(INFO) << "keyword = " << *(kv.first);
        auto &cList = kv.second;
        if (cList.size() > 1) {
            for (auto it = cList.begin(); it != cList.end()-1; ++it)
                if (it->weight < (it+1)->weight)
                { DLOG(INFO) << "found inconsistent record!"; break; }
        } // if
        // for (auto &v : kv.second)
            // cout << *(boost::get<StringPtr>(v.item)) << ":" << v.weight << " ";
        // cout << endl;
    } // for
#endif
}

inline
void ConcurTable::loadFromFile( const std::string &filename )
{
    using namespace std;

    // RawTable                        m_mapRawTable;
    std::vector<StringPtr>          vecItems;

    vecItems.resize(1);
    m_mapTable.clear();

    ifstream ifs(filename, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("ConcurTable::loadFromFile() cannot read file " << filename);

    string line, strItem;
    IdType id = 0, freq = 0;
    double weight = 0.0;
    while (getline(ifs, line)) {
        stringstream stream(line);
        auto pKey = std::make_shared<std::string>();
        stream >> *pKey;
        auto colon = pKey->rfind(':');
        if (colon == string::npos || colon == 0)
            continue;
        pKey->erase(colon);
        // DLOG(INFO) << "pKey = " << *pKey;

        ConcurItemList cList;
        while (stream >> strItem) {
            if (sscanf(strItem.c_str(), "%lu:%lu:%lf", &id, &freq, &weight) != 3)
                continue;
            cList.emplace_back( ConcurItem{id, weight} );
            // cList.back().item = id;
            // cList.back().weight = weight;
        } // while

        // assert( !cList.empty() );

        vecItems.push_back( pKey );
        auto ret = m_mapTable.insert( std::make_pair(pKey, TableType::mapped_type()) );
        ret.first->second.swap( cList );
    } // while

    // DLOG(INFO) << "Totally " << (vecItems.size()-1) << " items loaded.";
    
    for (auto &kv : m_mapTable) {
        // DLOG(INFO) << "keyword = " << *(kv.first);
        for (auto &v : kv.second) {
            auto idx = boost::get<IdType>(v.item);
            // DLOG(INFO) << "idx = " << idx;
            assert( idx != 0 && idx < vecItems.size() );
            v.item = vecItems[idx];
            // v.item = vecItems[boost::get<IdType>(v.item)];
        } // for
    } // for

    // DEBUG
#if 0
    for (auto &kv : m_mapTable) {
        // cout << *(kv.first) << "\t";
        DLOG(INFO) << "keyword = " << *(kv.first);
        auto &cList = kv.second;
        if (cList.size() > 1) {
            for (auto it = cList.begin(); it != cList.end()-1; ++it)
                if (it->weight < (it+1)->weight)
                { DLOG(INFO) << "found inconsistent record!"; break; }
        } // if
        // for (auto &v : kv.second)
            // cout << *(boost::get<StringPtr>(v.item)) << ":" << v.weight << " ";
        // cout << endl;
    } // for
#endif
}


#endif

