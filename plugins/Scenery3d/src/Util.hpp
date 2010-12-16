#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <string>
#include <vector>

extern std::vector<std::string> splitStr(const std::string& line, char delim);
extern float parseFloat(const std::string& str);
extern unsigned int parseInt(const std::string& str);
extern void trim_right(std::string& source);
extern void trim_right(std::string& source, const std::string& t);

#endif

