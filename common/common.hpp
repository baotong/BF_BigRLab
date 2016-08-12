#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <string>
#include <sstream>
#include <stdexcept>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>


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

#define RET_MSG(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return; \
    } while (0)

#define RET_MSG_VAL(val, args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return val; \
    } while (0)

#define COND_RET_MSG(cond, args) \
    do { \
        if (cond) RET_MSG(args); \
    } while (0)

#define COND_RET_MSG_VAL(cond, val, args) \
    do { \
        if (cond) RET_MSG_VAL(val, args); \
    } while (0)

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

#endif

