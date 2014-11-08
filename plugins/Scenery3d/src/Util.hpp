#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <string>
#include <vector>
#include <sstream>

#include <QString>
#include <QDebug>
#include <QTime>

#include "VecMath.hpp"

//TODO FS: ditch most of these functions and replace them with QString's versions

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
//! Generates a normal Matrix from the given 3x3 view matrix
//! @param mat 3x3 view matrix, will be overwritten to normal matrix
extern void makeNormalMatrix(std::vector<float>& mat);
//! Returns the current time string as char array
extern const char* getTime();
//! Parses a texture string ready for loading
extern void parseTextureString(std::string in, std::string& out);
//! Casts a Vec3d to Vec3f;
extern Vec3f vecdToFloat(Vec3d v);
extern Vec3d vecfToDouble(Vec3f v);
//! Parses a int to string
extern std::string toString(int i);
//! Compares two vertices to eachother
extern bool CompareVerts(const Vec3f &v1, const Vec3f &v2);
//! Compares two floats to eachother
extern bool CompareFloats(float c1, float c2);
#endif

