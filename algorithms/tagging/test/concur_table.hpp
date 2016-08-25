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


class ConcurTable {
public:
    typedef unsigned long                           IdType;
    typedef std::shared_ptr<std::string>            StringPtr;

    struct StrPtrCmp {
        bool operator()(const StringPtr &lhs, const StringPtr &rhs)
        { return (*lhs < *rhs); }
    };

    struct ConcurItem {
        boost::variant<StringPtr, IdType> item;
        double                            weight;
    };

    typedef std::vector<ConcurItem>                            ConcurItemList;
    typedef std::map<StringPtr, ConcurItemList, StrPtrCmp>     TableType;

public:
    void loadFromFile( const std::string &filename );

    std::size_t size() const
    { return m_mapTable.size(); }

private:
    TableType                       m_mapTable;
};

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

        ConcurItemList cList;
        while (stream >> strItem) {
            if (sscanf(strItem.c_str(), "%lu:%lu:%lf", &id, &freq, &weight) != 3)
                continue;
            cList.emplace_back( ConcurItem() );
            cList.back().item = id;
            cList.back().weight = weight;
        } // while

        if (!cList.empty()) {
            vecItems.push_back( pKey );
            auto ret = m_mapTable.insert( std::make_pair(pKey, TableType::mapped_type()) );
            ret.first->second.swap( cList );
        } // if
    } // while

    // DLOG(INFO) << "Totally " << (vecItems.size()-1) << " items loaded.";
    
    for (auto &kv : m_mapTable) {
        for (auto &v : kv.second)
            v.item = vecItems[boost::get<IdType>(v.item)];
    } // for

    // DEBUG
    // for (auto &kv : m_mapTable) {
        // cout << *(kv.first) << "\t";
        // for (auto &v : kv.second)
            // cout << *(boost::get<StringPtr>(v.item)) << ":" << v.weight << " ";
        // cout << endl;
    // } // for
}


#endif

