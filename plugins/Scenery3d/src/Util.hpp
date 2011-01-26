#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <string>
#include <vector>

//! Split string into list of strings.
//! Example: splitStr("Hello World!", " ") results in {"Hello", "World!"}.
//! @param line String to split.
//! @param delim Delimiter.
//! @return List of strings.
extern std::vector<std::string> splitStr(const std::string& line, char delim);
//! Convert string into floating point number.
//! Throws a std::string in case of failure.
//! @param str String to convert.
//! @return Floating point number.
extern float parseFloat(const std::string& str);
//! Convert string into integer.
//! Throws a std::string in case of failure.
//! @param str String to convert.
//! @return Integer.
extern unsigned int parseInt(const std::string& str);
//! Removes whitespace from the end of a string.
//! @param source Reference to string. It is modified in place.
extern void trim_right(std::string& source);
//! Removes specific characters from the end of a string.
//! @param source Reference to string. It is modified in place.
//! @param t Character sequence; characters to remove from source.
extern void trim_right(std::string& source, const std::string& t);

#endif

