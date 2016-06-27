#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

#include "shared_queue.h"
#include <string>
#include <map>
#include <set>
#include <deque>
#include <sstream>
#include <functional>
#include <stdexcept>
#include <boost/smart_ptr.hpp>
#include <glog/logging.h>

#define THIS_THREAD_ID        boost::this_thread::get_id()
#define SLEEP_SECONDS(x)      boost::this_thread::sleep_for(boost::chrono::seconds(x))
#define SLEEP_MILLISECONDS(x) boost::this_thread::sleep_for(boost::chrono::milliseconds(x))
#define SPACES                " \t\f\r\v\n"

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define ERR_RET(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return; \
    } while (0)

#define ERR_RET_VAL(val, args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return val; \
    } while (0)

#define COND_RET(cond, args) \
    do { \
        if (cond) ERR_RET(args); \
    } while (0)

#define COND_RET_VAL(cond, val, args) \
    do { \
        if (cond) ERR_RET_VAL(val, args); \
    } while (0)

#define WRITE_LINE(args) \
    do { \
        stringstream __write_line_stream; \
        __write_line_stream << args << flush; \
        g_pWriter->writeLine( __write_line_stream.str() ); \
    } while (0)

namespace BigRLab {

template < typename T >
std::string to_string( const T &value )
{
    std::stringstream os;
    os << value << std::flush;
    return os.str();
}

template< typename StreamType >
bool bad_stream( const StreamType &stream )
{ return (stream.fail() || stream.bad()); }

class Writer {
public:
    typedef boost::shared_ptr<Writer> pointer;
public:    
    virtual bool readLine(std::string &line) = 0;
    virtual void writeLine(const std::string &msg) = 0;
};

extern Writer::pointer      g_pWriter;

} // namespace BigRLab



#endif

