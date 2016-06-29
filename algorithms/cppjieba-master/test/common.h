#ifndef _COMMON_H_
#define _COMMON_H_

#define SLEEP_MILLISECONDS(x) boost::this_thread::sleep_for(boost::chrono::milliseconds(x))

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define THROW_INVALID_REQUEST(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        InvalidRequest __invalid_request_err; \
        __invalid_request_err.reason = std::move(__err_stream.str()); \
        throw __invalid_request_err; \
    } while (0)

#define ERR_RET_VAL(val, args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return val; \
    } while (0)

#define COND_RET_VAL(cond, val, args) \
    do { \
        if (cond) ERR_RET_VAL(val, args); \
    } while (0)

#define SPACES " \t\f\r\v\n"

template< typename StreamType >
bool bad_stream( const StreamType &stream )
{ return (stream.fail() || stream.bad()); }

#endif

