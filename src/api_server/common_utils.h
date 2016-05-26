#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

#include <string>
#include <map>
#include <set>
#include <deque>
#include <sstream>
#include <functional>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono.hpp>
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

namespace BigRLab {

template < typename T >
class SharedQueue : public std::deque<T> {
    typedef typename std::deque<T>   BaseType;
public:
    SharedQueue( std::size_t _MaxSize = UINT_MAX ) 
                : maxSize(_MaxSize) {}

    bool full() const { return this->size() >= maxSize; }

    void push( const T& elem )
    {
        boost::unique_lock<boost::mutex> lk(lock);

        while ( this->full() )
            condWr.wait( lk );

        this->push_back( elem );

        lk.unlock();
        condRd.notify_one();
    }

    void push( T&& elem )
    {
        boost::unique_lock<boost::mutex> lk(lock);

        while ( this->full() )
            condWr.wait( lk );

        this->push_back( std::move(elem) );

        lk.unlock();
        condRd.notify_one();
    }

    void pop( T& retval )
    {
        boost::unique_lock<boost::mutex> lk(lock);
        
        while ( this->empty() )
            condRd.wait( lk );

        retval = std::move(this->front());
        this->pop_front();

        lk.unlock();
        condWr.notify_one();
    }

    T pop()
    {
        T retval;
        this->pop( retval );
        return retval;
    }

    bool timed_push( const T& elem, std::size_t timeout )
    {
        boost::unique_lock<boost::mutex> lk(lock);

        if (!condWr.wait_for(lk, boost::chrono::milliseconds(timeout),
                    [this]()->bool {return !this->full();}))
            return false;

        this->push_back( elem );

        lk.unlock();
        condRd.notify_one();

        return true;
    }

    bool timed_push( T&& elem, std::size_t timeout )
    {
        boost::unique_lock<boost::mutex> lk(lock);

        if (!condWr.wait_for(lk, boost::chrono::milliseconds(timeout),
                    [this]()->bool {return !this->full();}))
            return false;

        this->push_back( std::move(elem) );

        lk.unlock();
        condRd.notify_one();

        return true;
    }

    bool timed_pop( T& retval, std::size_t timeout )
    {
        boost::unique_lock<boost::mutex> lk(lock);
        
        //!! 这里的pred条件和while正好相反，相当于until某条件
        if (!condRd.wait_for(lk, boost::chrono::milliseconds(timeout),
                  [this]()->bool {return !this->empty();}))
            return false;

        retval = std::move(this->front());
        this->pop_front();

        lk.unlock();
        condWr.notify_one();

        return true;
    }

    void clear()
    {
        boost::unique_lock<boost::mutex> lk(lock);
        BaseType::clear();
    }

    boost::mutex& mutex()
    { return std::ref(lock); }

protected:
    const std::size_t             maxSize;
    boost::mutex                  lock;
    boost::condition_variable     condRd;
    boost::condition_variable     condWr;
};

struct InvalidInput : std::exception {
    explicit InvalidInput( const std::string &what )
            : whatString("InvalidInput: ") 
    { whatString.append(what); }

    explicit InvalidInput( const std::ostream &os )
            : whatString("InvalidInput: ") 
    { 
        using namespace std;
        const stringstream &_str = dynamic_cast<const stringstream&>(os);
        whatString.append(_str.str()); 
    }

    explicit InvalidInput( const std::string &inputStr,
                                    const std::string &desc )
            : whatString("InvalidInput: input string \"")
    { whatString.append( inputStr ).append( "\" is not valid! " ).append(desc); }

    virtual const char* what() const throw()
    { return whatString.c_str(); }

    std::string     whatString;
};


inline
std::string& strip_string( std::string &s )
{
    using namespace std;

    // static const char *SPACES = " \t\f\r\v\n";

    string::size_type pEnd = s.find_last_not_of( SPACES );
    if (string::npos != pEnd) {
        ++pEnd;
    } else {
        s.clear();
        return s;
    } // if

    string::size_type pStart = s.find_first_not_of( SPACES );
    s = s.substr(pStart, pEnd - pStart);

    return s;
}

template < typename T >
bool read_from_string( const std::string &s, T &value )
{
    std::stringstream str(s);
    str >> value;
    bool ret = (str.good() || str.eof());
    if ( !ret )
        value = T();
    return ret;
}

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

typedef std::map< std::string, std::set<std::string> > PropertyTable;

extern void parse_config_file( const char *filename, PropertyTable &propTable );

} // namespace BigRLab



#endif

