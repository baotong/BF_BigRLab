#ifndef _DB_HPP_
#define _DB_HPP_

#include <map>
#include <string>
#include <memory>
#include <ctime>
#include <climits>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>


class DB {
public:
    struct Item {
        typedef std::shared_ptr<Item>   pointer;

        Item(const std::string &_Data)
            : data(_Data), value(0.0), createTime(std::time(0)) {}

        void setValue(const double &v) { value = v; }

        std::string   data;
        double        value;
        time_t        createTime;
    };

    // typedef std::map<std::string, Item::pointer>    DbMap;
    struct DbMap : std::map<std::string, Item::pointer>
                 , boost::upgrade_lockable_adapter<boost::shared_mutex> {};

public:

private:
    DbMap   m_mapTable[CHAR_MAX][CHAR_MAX];
};

#endif

