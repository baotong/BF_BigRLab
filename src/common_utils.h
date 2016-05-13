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
#include <boost/date_time.hpp>
#include <glog/logging.h>

#define THIS_THREAD_ID        boost::this_thread::get_id()
#define SLEEP_SECONDS(x)      boost::this_thread::sleep_for(boost::chrono::seconds(x))
#define SLEEP_MILLISECONDS(x) boost::this_thread::sleep_for(boost::chrono::milliseconds(x))
#define SPACES                " \t\f\r\v\n"

namespace BigRLab {

template < typename T >
class SharedQueue : private std::deque<T> {
    typedef typename std::deque<T>   BaseType;
public:
    void push( const T &elem )
    {
        boost::unique_lock<boost::mutex> lk(lock);

        this->push_back( elem );

        lk.unlock();
        condRd.notify_one();
    }

    T pop()
    {
        boost::unique_lock<boost::mutex> lk(lock);
        
        while( this->empty() )
            condRd.wait( lk );

        T retval = this->front();
        this->pop_front();

        return retval;
    }

    void clear()
    {
        boost::unique_lock<boost::mutex> lk(lock);
        BaseType::clear();
    }

protected:
    boost::mutex                  lock;
    boost::condition_variable     condRd;
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
void throw_runtime_error( const std::ostream &os )
{
    using namespace std;
    const stringstream &_str = dynamic_cast<const stringstream&>(os);
    throw runtime_error(_str.str());
}

inline
void throw_runtime_error( const std::string &msg )
{
    using namespace std;
    throw runtime_error(msg);
}


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

