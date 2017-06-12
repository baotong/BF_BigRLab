#ifndef _STR2TIME_HPP_
#define _STR2TIME_HPP_ 

#include <ctime>

namespace Utils {

inline
bool str2time(const std::string &s, std::time_t &tt, 
              const std::string &format = "%Y-%m-%d %H:%M:%S")
{
    std::tm tm;
    char *success = ::strptime(s.c_str(), format.c_str(), &tm);
    if (!success) return false;
    tt = std::mktime(&tm);
    return true;
}

} // namespace Utils


#endif /* _STR2TIME_HPP_ */
