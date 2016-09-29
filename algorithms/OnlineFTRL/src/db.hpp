#ifndef _DB_HPP_
#define _DB_HPP_

#include <map>
#include <string>
#include <memory>
#include <ctime>
#include <climits>
#include <deque>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/chrono.hpp>


class DB {
public:
    constexpr static int LOCK_TIMEOUT = 10;     // 10s

    struct Item {
        typedef std::shared_ptr<Item>   pointer;

        Item(const std::string &_Data)
            : data(_Data), value(0.0), createTime(std::time(0)) {}

        void setValue(const double &v) { value = v; }

        std::string   data;
        double        value;
        time_t        createTime;
    };

    struct DbMap : std::map<std::string, Item::pointer>
                 , boost::upgrade_lockable_adapter<boost::shared_mutex> {};

    struct ValueArray : std::deque<Item::pointer>
                      , boost::upgrade_lockable_adapter<boost::shared_mutex> {};

public:
    DB(const std::size_t &_DataLife) : m_nDataLifeTime(_DataLife) {}

    bool add(const std::string &id, const std::string &data)
    {
        auto pItem = std::make_shared<Item>(data);
        auto &table = m_mapTable[id[0]][id[1]];
        boost::unique_lock<DbMap> lock(table);
        auto ret = table.insert(std::make_pair(id, pItem));
        return ret.second;
    }

    bool setValue(const std::string &id, const double &v)
    {
        auto &table = m_mapTable[id[0]][id[1]];
        boost::unique_lock<DbMap> lockTable(table, boost::defer_lock);
        lockTable.try_lock_for(boost::chrono::seconds(LOCK_TIMEOUT));
        if (lockTable.owns_lock()) {
            auto it = table.find(id);
            if (it == table.end())
                return false;
            boost::unique_lock<ValueArray> lockArr(m_arrHasValue, boost::defer_lock);
            lockArr.try_lock_for(boost::chrono::seconds(LOCK_TIMEOUT));
            if (lockArr.owns_lock()) {
                Item::pointer pItem = it->second;
                table.erase(it);
                pItem->setValue(v);
                m_arrHasValue.push_back(pItem);
                return true;
            } // if
        } // if
        return false;
    }

    std::size_t clearOutDated()
    {
        std::size_t count = 0;
        for (int i = 0; i <= CHAR_MAX; ++i) {
            for (int j = 0; j <= CHAR_MAX; ++j) {
                auto &table = m_mapTable[i][j];
                boost::unique_lock<DbMap> lock(table);
                for (auto it = table.begin(); it != table.end();) {
                    double duration = std::difftime(std::time(0), it->second->createTime);
                    if (duration > m_nDataLifeTime || duration < 0) {
                        it = table.erase(it);
                        ++count;
                    } else {
                        ++it;
                    } // if
                } // for it
                lock.unlock();
            } // for j
        } // for i
        return count;
    }

    ValueArray& hasValues()
    { return m_arrHasValue; }

private:
    DbMap                m_mapTable[CHAR_MAX+1][CHAR_MAX+1];
    ValueArray           m_arrHasValue;
    const std::size_t    m_nDataLifeTime;
    // std::atomic_size_t      m_nSize;
};

#endif

