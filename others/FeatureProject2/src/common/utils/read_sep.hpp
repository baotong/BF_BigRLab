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

    if (strSep.empty() || strSep == "blanks") {
        strSep = SPACES;
    } else if (strSep == "tab") {
        strSep = "\t";
    } else if (strSep == "space") {
        strSep = " ";
    } // if
}

} // namespace Utils


#endif /* _READ_SEP_HPP_ */

