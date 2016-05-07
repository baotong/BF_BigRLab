#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

#include <string>
#include <map>
#include <set>
#include <memory>
#include <sstream>
#include <functional>

namespace BigRLab {


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

    static const char *SPACES = " \t\f\r\v\n";

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


typedef std::map< std::string, std::set<std::string> > PropertyTable;

extern void parse_config_file( const char *filename, PropertyTable &propTable );

} // namespace BigRLab



#endif

