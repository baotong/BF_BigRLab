#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <string>
#include <sstream>
#include <memory>
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

#define THROW_RUNTIME_ERROR_IF(cond, args) \
    do { \
        if (cond) THROW_RUNTIME_ERROR(args); \
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

#define RET_MSG_IF(cond, args) \
    do { \
        if (cond) RET_MSG(args); \
    } while (0)

#define RET_MSG_VAL_IF(cond, val, args) \
    do { \
        if (cond) RET_MSG_VAL(val, args); \
    } while (0)

#define ON_FINISH(name, deleter) \
    std::unique_ptr<void, std::function<void(void*)> > \
        name((void*)-1, [&](void*) deleter )

#define ON_FINISH_CLASS(name, deleter) \
    std::unique_ptr<void, std::function<void(void*)> > \
        name((void*)-1, [&, this](void*) deleter )

#define ON_FINISH_TYPE(type, name, ptr, deleter) \
    std::unique_ptr<type, std::function<void(type*)> > \
        name((ptr), [&](type* pArg) deleter )

#define RUN_COMMAND(x) \
    do { \
        std::stringstream __cmd_stream; \
        __cmd_stream << x; \
        __cmd_stream.flush(); \
        std::string __cmd_str = __cmd_stream.str(); \
        if (system(__cmd_str.c_str())) \
            THROW_RUNTIME_ERROR("Run command \"" << __cmd_str << "\" fail!"); \
    } while (0)

template< typename StreamType >
bool bad_stream( StreamType &stream )
{ return (stream.fail() || stream.bad()); }

#endif

