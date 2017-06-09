#ifndef _READ_SEP_HPP_
#define _READ_SEP_HPP_

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <CommDef.h>


namespace Utils {

inline
void read_sep(std::string &strSep)
{
    using namespace std;

    if (strSep.empty()) {
        strSep = SPACES;
        return;
    } // if

    vector<string>      items;
    boost::split(items, strSep, boost::is_any_of("|"));

    strSep.clear();
    for (string &s : items) {
        boost::trim(s);
        
        if (s.empty()) continue;
        if (s == "blanks") 
            s = SPACES;
        else if (s == "tab")
            s = "\t";
        else if (s == "space")
            s = " ";
        
        strSep += s;
    } // for
}

} // namespace Utils


#endif /* _READ_SEP_HPP_ */

