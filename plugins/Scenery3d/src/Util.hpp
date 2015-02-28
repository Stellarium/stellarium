#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <string>
#include <vector>
#include <sstream>

#include <QString>
#include <QDebug>
#include <QTime>
#include <QMatrix4x4>

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

//! Like StelUtils::strToVec3d, but with 4 components
inline Vec4d strToVec4d(const QString& str)
{
	QStringList l = str.split(",");

	if(l.size()<4)
		return Vec4d(0.0,0.0,0.0,0.0);

	return Vec4d(l[0].toDouble(), l[1].toDouble(), l[2].toDouble(), l[3].toDouble());
}

inline QString vec4dToStr(const Vec4d& vec)
{
	return QString("%1,%2,%3,%4")
			.arg(vec[0],0,'f',10)
			.arg(vec[1],0,'f',10)
			.arg(vec[2],0,'f',10)
			.arg(vec[3],0,'f',10);
}

inline QString vec3fToStr(const Vec3f& vec)
{
	return QString("%1,%2,%3")
			.arg(vec[0],0,'f',6)
			.arg(vec[1],0,'f',6)
			.arg(vec[2],0,'f',6);
}

//! Provide Qt 3x3 matrix-vector multiplication, which does not exist for some reason
inline QVector3D operator*(const QMatrix3x3& mat, const QVector3D& vec)
{
	float x,y,z;
	x =	vec.x() * mat(0,0) +
		vec.y() * mat(0,1) +
		vec.z() * mat(0,2);
	y =	vec.x() * mat(1,0) +
		vec.y() * mat(1,1) +
		vec.z() * mat(1,2);
	z =	vec.x() * mat(2,0) +
		vec.y() * mat(2,1) +
		vec.z() * mat(2,2);
	return QVector3D(x,y,z);
}

template<typename T>
inline QMatrix4x4 convertToQMatrix(const Matrix4<T>& mat)
{
	return QMatrix4x4( mat.r[0], mat.r[4], mat.r[8],mat.r[12],
			   mat.r[1], mat.r[5], mat.r[9],mat.r[13],
			   mat.r[2], mat.r[6],mat.r[10],mat.r[14],
			   mat.r[3], mat.r[7],mat.r[11],mat.r[15] );
}


#endif

