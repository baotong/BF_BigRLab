#ifndef _ALG_COMMON_HPP_
#define _ALG_COMMON_HPP_


#define THROW_INVALID_REQUEST(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        AlgCommon::InvalidRequest __invalid_request_err; \
        __invalid_request_err.reason = std::move(__err_stream.str()); \
        throw __invalid_request_err; \
    } while (0)


#define THROW_INVALID_REQUEST_IF(cond, args) \
    do { \
        if (cond) THROW_INVALID_REQUEST(args); \
    } while (0)

#endif

