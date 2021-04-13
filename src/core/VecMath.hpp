/*
 *
 * Copyright (C) 2003 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

// Template vector and matrix library.
// Use OpenGL compatible ordering ie. you can pass a matrix or vector to
// OpenGL functions without changes in the ordering

#ifndef VECMATH_HPP
#define VECMATH_HPP


#include <cmath>
#include <limits>
#include <QString>
#include <QMatrix4x4>
#include <QColor>

template<class T> class Vector2;
template<class T> class Vector3;
template<class T> class Vector4;
template<class T> class Matrix4;
template<class T> class Matrix3;

typedef Vector2<double>	Vec2d;
typedef Vector2<float>	Vec2f;
typedef Vector2<int>	Vec2i;

//! @typedef Vec3d
//! A 3d vector of doubles compatible with OpenGL.
typedef Vector3<double>	Vec3d;

//! @typedef Vec3f
//! A 3d vector of floats compatible with OpenGL.
typedef Vector3<float>	Vec3f;

//! @typedef Vec3i
//! A 3d vector of ints compatible with OpenGL.
typedef Vector3<int>	Vec3i;

//! @typedef Vec4d
//! A 4d vector of doubles compatible with OpenGL.
typedef Vector4<double>	Vec4d;

//! @typedef Vec4f
//! A 4d vector of floats compatible with OpenGL.
typedef Vector4<float>	Vec4f;

//! @typedef Vec4i
//! A 4d vector of ints compatible with OpenGL.
typedef Vector4<int>	Vec4i;

//! @typedef Mat4d
//! A 4x4 matrix of doubles compatible with OpenGL.
typedef Matrix4<double>	Mat4d;

//! @typedef Mat4f
//! A 4x4 matrix of floats compatible with OpenGL.
typedef Matrix4<float>	Mat4f;

//! @typedef Mat3d
//! A 3x3 matrix of doubles compatible with OpenGL.
typedef Matrix3<double> Mat3d;

//! @typedef Mat3f
//! A 3x3 matrix of floats compatible with OpenGL.
typedef Matrix3<float> Mat3f;


//! @class Vector2
//! A templatized 2d vector compatible with OpenGL.
//! Use Vec2i for integer and Vec2d or Vec2f typedef for vectors of double and float respectively.
template<class T> class Vector2
{
public:
	//! The vector is not initialized!
	inline Vector2();
	//! Sets all components of the vector to the same value
	inline explicit Vector2(T);
	//! Explicit conversion constructor from an array (copies values)
	//! @warning Does not check array size, make sure it has at least 2 elements
	inline explicit Vector2(const T*);
	//! Explicit conversion constructor from another Vec2 of any type.
	//! Uses default primitive type conversion
	template <class T2> inline explicit Vector2(const Vector2<T2>&);
	inline Vector2(T, T);
	//! Constructor from a comma-separated QString like "2,4" or "2.1,4.2"
	explicit Vector2(QString s);
	//! Constructor from a QStringList like { "2", "4" } or { "2.1", "4.2", "6.3" }
	explicit Vector2(QStringList s);

	//! Assignment from array
	//! @warning Does not check array size, make sure it has at least 2 elements
	inline Vector2<T>& operator=(const T*);
	inline void set(T, T);

	inline bool operator==(const Vector2<T>&) const;
	inline bool operator!=(const Vector2<T>&) const;

	inline const T& operator[](int x) const;
	inline T& operator[](int);
	inline operator const T*() const;
	inline operator T*();

	inline Vector2<T>& operator+=(const Vector2<T>&);
	inline Vector2<T>& operator-=(const Vector2<T>&);
	//! Scalar multiplication
	inline Vector2<T>& operator*=(T);
	//! Component-wise multiplication
	inline Vector2<T>& operator*=(const Vector2<T>&);
	//! Scalar division
	inline Vector2<T>& operator/=(T);
	//! Component-wise division
	inline Vector2<T>& operator/=(const Vector2<T>&);

	inline Vector2<T> operator-(const Vector2<T>&) const;
	inline Vector2<T> operator+(const Vector2<T>&) const;

	inline Vector2<T> operator-() const;
	inline Vector2<T> operator+() const;

	//! Scalar multiplication
	inline Vector2<T> operator*(T) const;
	//! Component-wise multiplication
	inline Vector2<T> operator*(const Vector2<T>&) const;
	//! Scalar division
	inline Vector2<T> operator/(T) const;
	//! Component-wise division
	inline Vector2<T> operator/(const Vector2<T>&) const;

	//! Component-wise minimum determination
	inline Vector2<T> min(const Vector2<T>&) const;
	//! Component-wise maximum determination
	inline Vector2<T> max(const Vector2<T>&) const;
	//! Component-wise clamping to the specified upper and lower bounds
	inline Vector2<T> clamp(const Vector2<T>& low, const Vector2<T>& high) const;

	inline T dot(const Vector2<T>&) const;

	inline T length() const;
	inline T lengthSquared() const;
	inline void normalize();

	//! Formatted string with brackets
	inline QString toString() const {return QString("[%1, %2]").arg(v[0]).arg(v[1]);}
	//! Compact comma-separated string without brackets and spaces.
	//! The result can be restored into a Vector2 by the Vector2(QString s) constructors.
	QString toStr() const;

	T v[2];
};


//! @class Vector3
//! A templatized 3d vector compatible with OpenGL.
//! Use Vec3i for integer and Vec3d or Vec3f typedef for vectors of double and float respectively.
template<class T> class Vector3
{
public:
	//! The vector is not initialized!
	inline Vector3();
	//! Sets all components of the vector to the same value
	inline explicit Vector3(T);
	//! Explicit conversion constructor from an array (copies values)
	//! @warning Does not check array size, make sure it has at least 3 elements
	inline explicit Vector3(const T*);
	inline Vector3(T, T, T);
	//! Constructor from a comma-separated QString like "2,4,6" or "2.1,4.2,6.3"
	explicit Vector3(QString s);
	//! Constructor from a QStringList like { "2", "4", "6" } or { "2.1", "4.2", "6.3" }
	explicit Vector3(QStringList s);
	//! Constructor from a QColor
	explicit Vector3(QColor c);

	//inline Vector3& operator=(const Vector3&);

	//! Assignment from array
	//! @warning Does not check array size, make sure it has at least 2 elements
	inline Vector3& operator=(const T*);
	//template <class T2> inline Vector3& operator=(const Vector3<T2>&);
	inline void set(T, T, T);

	//! Assign from HTML color
	//! The Vec3i type will have values 0...255
	//! Vec3f and Vec3d will have [0...[1
	Vector3 setFromHtmlColor(QString s);

	inline bool operator==(const Vector3<T>&) const;
	inline bool operator!=(const Vector3<T>&) const;
	//! allows for a fuzzy comparison using some epsilon value
	inline bool fuzzyEquals(const Vector3<T>&, T epsilon = std::numeric_limits<T>::epsilon()) const;

	inline T& operator[](int);
	inline const T& operator[](int) const;
	inline operator const T*() const;
	inline operator T*();
	inline const T* data() const {return v;}
	inline T* data() {return v;}

	inline void operator+=(const Vector3<T>&);
	inline void operator-=(const Vector3<T>&);
	inline void operator*=(T);
	inline void operator/=(T);

	inline Vector3 operator-(const Vector3<T>&) const;
	inline Vector3 operator+(const Vector3<T>&) const;

	inline Vector3 operator-() const;
	inline Vector3 operator+() const;

	inline Vector3 operator*(T) const;
	inline Vector3 operator/(T) const;


	inline T dot(const Vector3<T>&) const;
	inline Vector3 operator^(const Vector3<T>&) const;

	// Return latitude in rad
	inline T latitude() const;
	// Return longitude in rad
	inline T longitude() const;

	// Distance in radian between two
	inline T angle(const Vector3<T>&) const;
	inline T angleNormalized(const Vector3<T>&) const;

	inline T length() const;
	inline T lengthSquared() const;
	inline void normalize();

	inline void transfo4d(const Mat4d&);
	inline void transfo4d(const Mat4f&);

	//used for explicit conversion to other precision type
	inline Vec3f toVec3f() const;
	inline Vec3d toVec3d() const;

	//! Formatted string with brackets
	inline QString toString() const {return QString("[%1, %2, %3]").arg(v[0]).arg(v[1]).arg(v[2]);}
	//! Compact comma-separated string without brackets and spaces.
	//! The result can be restored into a Vector2 by the Vector3(QString s) constructors.
	QString toStr() const;
	inline QString toStringLonLat() const {return QString("[") + QString::number(longitude()*180./M_PI, 'g', 12) + "," + QString::number(latitude()*180./M_PI, 'g', 12)+"]";}
	//! Convert a Vec3i/Vec3f/Vec3d to HTML color notation. In case of Vec3i, components are 0...255, else 0...1
	QString toHtmlColor() const;
	//! Convert to a QColor.
	QColor toQColor() const;
	//! Convert to a QVector3D.
	QVector3D toQVector3D() const;

	T v[3];		// The 3 values
};


//! @class Vector4
//! A templatized 4d vector compatible with OpenGL.
//! Use Vec4i for integer and Vec4d or Vec4f typedef for vectors of double and float respectively.
template<class T> class Vector4
{
public:
	//! The vector is not initialized!
	inline Vector4();
	//! Sets all components of the vector to the same value
	inline explicit Vector4(T);
	//! Explicit conversion constructor from an array
	//! @warning Does not check array size, make sure it has at least 4 elements
	inline explicit Vector4(const T*);
	//! Explicit conversion constructor from another Vec4 of any type.
	//! Uses default primitive type conversion
	template <class T2> inline explicit Vector4(const Vector4<T2>&);
	//! Creates an Vector4 with xyz set to the given Vector3, and w set to 1.0
	inline Vector4(const Vector3<T>&);
	//! Creates an Vector4 with xyz set to the given values, and w set to 1.0
	inline Vector4(T, T, T);
	//! Creates an Vector4 with xyz set to the given Vector3, and given last value as w
	inline Vector4(const Vector3<T>&, T);
	inline Vector4(T, T, T, T);
	//! Constructor from a comma-separated QString like "2,4,6,8" or "2.1,4.2,6.3,8.4"
	explicit Vector4(QString s);
	//! Constructor from a QStringList like { "2", "4", "6", "8" } or { "2.1", "4.2", "6.3", "8.4" }
	explicit Vector4(QStringList s);
	//! Constructor from a QColor
	explicit Vector4(QColor c);

	inline Vector4& operator=(const Vector3<T>&);
	inline Vector4& operator=(const T*);
	inline void set(T, T, T, T);

	inline bool operator==(const Vector4<T>&) const;
	inline bool operator!=(const Vector4<T>&) const;

	inline T& operator[](int);
	inline const T& operator[](int) const;
	inline operator T*();
	inline operator const T*() const;

	inline void operator+=(const Vector4<T>&);
	inline void operator-=(const Vector4<T>&);
	inline void operator*=(T);
	inline void operator/=(T);

	inline Vector4 operator-(const Vector4<T>&) const;
	inline Vector4 operator+(const Vector4<T>&) const;

	inline Vector4 operator-() const;
	inline Vector4 operator+() const;

	inline Vector4 operator*(T) const;
	inline Vector4 operator/(T) const;


	inline T dot(const Vector4<T>&) const;

	inline T length() const;
	inline T lengthSquared() const;
	inline void normalize();

	inline void transfo4d(const Mat4d&);

	//! Formatted string with brackets
	inline QString toString() const {return QString("[%1, %2, %3, %4]").arg(v[0]).arg(v[1]).arg(v[2]).arg(v[3]);}
	//! Compact comma-separated string without brackets and spaces.
	//! The result can be restored into a Vector2 by the Vector4(QString s) constructors.
	QString toStr() const;
	//! Convert to a QColor.
	QColor toQColor() const;

	T v[4];		// The 4 values
};

//! @class Matrix3
//! A templatized column-major 3x3 matrix compatible with OpenGL (mostly for NormalMatrix calculation).
//! Use Mat3d or Mat3f typedef for matrices of doubles and floats respectively.
template<class T> class Matrix3
{
public:
	Matrix3();
	Matrix3(T,T,T,T,T,T,T,T,T);
	Matrix3(const T*);
	Matrix3(const Vector3<T>&, const Vector3<T>&, const Vector3<T>&);

	inline Matrix3& operator=(const T*);
	inline void set(T,T,T,T,T,T,T,T,T);

	inline T& operator[](int);
	inline operator T*();
	inline operator const T*() const;

	inline Matrix3 operator-(const Matrix3<T>&) const;
	inline Matrix3 operator+(const Matrix3<T>&) const;
	inline Matrix3 operator*(const Matrix3<T>&) const;

	inline Vector3<T> operator*(const Vector3<T>&) const;

	static Matrix3<T> identity();

	Matrix3<T> transpose() const;
	Matrix3<T> inverse() const;
	//! return trace (sum of diagonal elements).
	inline T trace() const {return r[0]+r[4]+r[8];}
	//! return rotational angle
	inline T angle() const {return acos(0.5*(this->trace()-1.0));}

	inline void print(void) const;
	QString toString(int fieldWidth=0, char format='g', int precision=-1) const {return QString("[[%1, %2, %3], [%4, %5, %6], [%7, %8, %9]]")
				.arg(r[0], fieldWidth, format, precision)
				.arg(r[1], fieldWidth, format, precision)
				.arg(r[2], fieldWidth, format, precision)
				.arg(r[3], fieldWidth, format, precision)
				.arg(r[4], fieldWidth, format, precision)
				.arg(r[5], fieldWidth, format, precision)
				.arg(r[6], fieldWidth, format, precision)
				.arg(r[7], fieldWidth, format, precision)
				.arg(r[8], fieldWidth, format, precision);}

	T r[9];
};

//! @class Matrix4
//! A templatized column-major 4x4 matrix compatible with OpenGL.
//! Use Mat4d or Mat4f typdef for matrices of doubles and floats respectively.
template<class T> class Matrix4
{
public:
	Matrix4();
	Matrix4(T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T);
	Matrix4(const T*);
	Matrix4(const Vector4<T>&, const Vector4<T>&,
			const Vector4<T>&, const Vector4<T>&);

	inline Matrix4& operator=(const T*);
	inline void set(T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T);

	inline T& operator[](int);
	inline operator T*();
	inline operator const T*() const;

	inline Matrix4 operator-(const Matrix4<T>&) const;
	inline Matrix4 operator+(const Matrix4<T>&) const;
	inline Matrix4 operator*(const Matrix4<T>&) const;

	inline Vector3<T> operator*(const Vector3<T>&) const;
	inline Vector3<T> multiplyWithoutTranslation(const Vector3<T>& a) const;
	inline Vector4<T> operator*(const Vector4<T>&) const;

	inline void transfo(Vector3<T>&) const;

	static Matrix4<T> identity();
	static Matrix4<T> translation(const Vector3<T>&);

	//    static Matrix4<T> rotation(const Vector3<T>&);
	static Matrix4<T> rotation(const Vector3<T>&, T);
	static Matrix4<T> xrotation(T);
	static Matrix4<T> yrotation(T);
	static Matrix4<T> zrotation(T);
	static Matrix4<T> scaling(const Vector3<T>&);
	static Matrix4<T> scaling(T);

	Matrix4<T> transpose() const;
	Matrix4<T> inverse() const;

	//Returns the upper 3x3 Matrix of the current 4x4 Matrix
	Matrix3<T> upper3x3() const;
	Matrix3<T> upper3x3Transposed() const;

	inline Vector4<T> getRow(const int row) const;
	inline Vector4<T> getColumn(const int column) const;

	//Converts to QMatix4x4 (for use in OpenGL or other Qt classes)
	inline QMatrix4x4 convertToQMatrix() const;

	inline void print(void) const;
	QString toString(int fieldWidth=0, char format='g', int precision=-1) const {return QString("[[%1, %2, %3, %4], [%5, %6, %7, %8], [%9, %10, %11, %12], [%13, %14, %15, %16]]")
				.arg(r[0], fieldWidth, format, precision)
				.arg(r[1], fieldWidth, format, precision)
				.arg(r[2], fieldWidth, format, precision)
				.arg(r[3], fieldWidth, format, precision)
				.arg(r[4], fieldWidth, format, precision)
				.arg(r[5], fieldWidth, format, precision)
				.arg(r[6], fieldWidth, format, precision)
				.arg(r[7], fieldWidth, format, precision)
				.arg(r[8], fieldWidth, format, precision)
				.arg(r[9], fieldWidth, format, precision)
				.arg(r[10], fieldWidth, format, precision)
				.arg(r[11], fieldWidth, format, precision)
				.arg(r[12], fieldWidth, format, precision)
				.arg(r[13], fieldWidth, format, precision)
				.arg(r[14], fieldWidth, format, precision)
				.arg(r[15], fieldWidth, format, precision);}

	T r[16];
};

//! Serialization routines.
template<class T> QDataStream& operator<<(QDataStream& out, const Vector2<T>& v) {out << v[0] << v[1]; return out;}
template<class T> QDataStream& operator<<(QDataStream& out, const Vector3<T>& v) {out << v[0] << v[1] << v[2]; return out;}
template<class T> QDataStream& operator<<(QDataStream& out, const Vector4<T>& v) {out << v[0] << v[1] << v[2] << v[3]; return out;}
template<class T> QDataStream& operator<<(QDataStream& out, const Matrix4<T>& m) {out << m[0] << m[1] << m[2] << m[3] << m[4] << m[5] << m[6] << m[7] << m[8] << m[9] << m[10] << m[11] << m[12] << m[13] << m[14] << m[15]; return out;}
template<class T> QDataStream& operator<<(QDataStream& out, const Matrix3<T>& m) {out << m[0] << m[1] << m[2] << m[3] << m[4] << m[5] << m[6] << m[7] << m[8]; return out;}

template<class T> QDataStream& operator>>(QDataStream& in, Vector2<T>& v) {in >> v[0] >> v[1]; return in;}
template<class T> QDataStream& operator>>(QDataStream& in, Vector3<T>& v) {in >> v[0] >> v[1] >> v[2]; return in;}
template<class T> QDataStream& operator>>(QDataStream& in, Vector4<T>& v) {in >> v[0] >> v[1] >> v[2] >> v[3]; return in;}
template<class T> QDataStream& operator>>(QDataStream& in, Matrix4<T>& m) {in >> m[0] >> m[1] >> m[2] >> m[3] >> m[4] >> m[5] >> m[6] >> m[7] >> m[8] >> m[9] >> m[10] >> m[11] >> m[12] >> m[13] >> m[14] >> m[15]; return in;}
template<class T> QDataStream& operator>>(QDataStream& in, Matrix3<T>& m) {in >> m[0] >> m[1] >> m[2] >> m[3] >> m[4] >> m[5] >> m[6] >> m[7] >> m[8]; return in;}

//! Fuzzy comparison for two primitive variables
template <class T> inline bool fuzzyEquals(T a, T b, T eps = std::numeric_limits<T>::epsilon())
{
	if(a == b) return true;
	if(((a+eps) < b) || ((a-eps) > b)) return false;
	return true;
}

////////////////////////// Vector2 class methods ///////////////////////////////

template<class T> Vector2<T>::Vector2() {}

template<class T> Vector2<T>::Vector2(T x)
{
        v[0]=x; v[1]=x;
}

template<class T> Vector2<T>::Vector2(const T* x)
{
	v[0]=x[0]; v[1]=x[1];
}

template <class T> template <class T2> Vector2<T>::Vector2(const Vector2<T2>& other)
{
	v[0]=other[0]; v[1]=other[1];
}

template<class T> Vector2<T>::Vector2(T x, T y)
{
	v[0]=x; v[1]=y;
}

template<class T> Vector2<T>& Vector2<T>::operator=(const T* a)
{
	v[0]=a[0]; v[1]=a[1];
	return *this;
}

template<class T> void Vector2<T>::set(T x, T y)
{
	v[0]=x; v[1]=y;
}


template<class T> bool Vector2<T>::operator==(const Vector2<T>& a) const
{
	return (v[0] == a.v[0] && v[1] == a.v[1]);
}

template<class T> bool Vector2<T>::operator!=(const Vector2<T>& a) const
{
	return (v[0] != a.v[0] || v[1] != a.v[1]);
}

template<class T> const T& Vector2<T>::operator[](int x) const
{
	return v[x];
}

template<class T> T& Vector2<T>::operator[](int x)
{
	return v[x];
}

template<class T> Vector2<T>::operator const T*() const
{
	return v;
}

template<class T> Vector2<T>::operator T*()
{
	return v;
}


template<class T> Vector2<T>& Vector2<T>::operator+=(const Vector2<T>& a)
{
	v[0] += a.v[0]; v[1] += a.v[1];
	return *this;
}

template<class T> Vector2<T>& Vector2<T>::operator-=(const Vector2<T>& a)
{
	v[0] -= a.v[0]; v[1] -= a.v[1];
	return *this;
}

template<class T> Vector2<T>& Vector2<T>::operator*=(T s)
{
	v[0] *= s; v[1] *= s;
	return *this;
}

template<class T> Vector2<T>& Vector2<T>::operator*=(const Vector2<T>& b)
{
	v[0] *= b.v[0]; v[1] *= b.v[1];
	return *this;
}

template<class T> Vector2<T>& Vector2<T>::operator/=(T s)
{
	v[0] /= s; v[1] /= s;
	return *this;
}

template<class T> Vector2<T>& Vector2<T>::operator/=(const Vector2<T>& b)
{
	v[0] /= b.v[0]; v[1] /= b.v[1];
	return *this;
}

template<class T> Vector2<T> Vector2<T>::operator-() const
{
	return Vector2<T>(-v[0], -v[1]);
}

template<class T> Vector2<T> Vector2<T>::operator+() const
{
	return *this;
}

template<class T> Vector2<T> Vector2<T>::operator+(const Vector2<T>& b) const
{
	return Vector2<T>(v[0] + b.v[0], v[1] + b.v[1]);
}

template<class T> Vector2<T> Vector2<T>::operator-(const Vector2<T>& b) const
{
	return Vector2<T>(v[0] - b.v[0], v[1] - b.v[1]);
}

template<class T> Vector2<T> Vector2<T>::operator*(T s) const
{
	return Vector2<T>(s * v[0], s * v[1]);
}

template<class T> Vector2<T> Vector2<T>::operator*(const Vector2<T>& b) const
{
	return Vector2<T>(v[0] * b.v[0], v[1] * b.v[1]);
}

template<class T> Vector2<T> Vector2<T>::operator/(T s) const
{
	return Vector2<T>(v[0]/s, v[1]/s);
}

template<class T> Vector2<T> Vector2<T>::operator/(const Vector2<T>& b) const
{
	return Vector2<T>(v[0]/b.v[0], v[1]/b.v[1]);
}

template<class T> Vector2<T> Vector2<T>::min(const Vector2<T>& b) const
{
	return Vector2<T>(std::min(v[0],b.v[0]), std::min(v[1],b.v[1]));
}

template<class T> Vector2<T> Vector2<T>::max(const Vector2<T>& b) const
{
	return Vector2<T>(std::max(v[0],b.v[0]), std::max(v[1],b.v[1]));
}

template<class T> Vector2<T> Vector2<T>::clamp(const Vector2<T>& low, const Vector2<T>& high) const
{
	return this->max(low).min(high);
}

template<class T> T Vector2<T>::dot(const Vector2<T>& b) const
{
	return v[0] * b.v[0] + v[1] * b.v[1];
}


template<class T> T Vector2<T>::length() const
{
	return static_cast<T>(std::sqrt(v[0] * v[0] + v[1] * v[1]));
}

template<class T> T Vector2<T>::lengthSquared() const
{
	return v[0] * v[0] + v[1] * v[1];
}

template<class T> void Vector2<T>::normalize()
{
	T s = static_cast<T>( 1 / std::sqrt(v[0] * v[0] + v[1] * v[1]));
	v[0] *= s;
	v[1] *= s;
}

// template<class T>
// std::ostream& operator<<(std::ostream &o,const Vector2<T> &v) {
//   return o << '[' << v[0] << ',' << v[1] << ']';
// }

////////////////////////// Vector3 class methods ///////////////////////////////

template<class T> Vector3<T>::Vector3() {}

//template<class T> Vector3<T>::Vector3(const Vector3& a)
//{
//	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
//}

//template<class T> template<class T2> Vector3<T>::Vector3(const Vector3<T2>& a)
//{
//	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
//}

template<class T> Vector3<T>::Vector3(T x)
{
	v[0]=x; v[1]=x; v[2]=x;
}

template<class T> Vector3<T>::Vector3(const T* x)
{
	v[0]=x[0]; v[1]=x[1]; v[2]=x[2];
}

template<class T> Vector3<T>::Vector3(T x, T y, T z)
{
	v[0]=x; v[1]=y; v[2]=z;
}

//template<class T> Vector3<T>& Vector3<T>::operator=(const Vector3& a)
//{
//	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
//	return *this;
//}

//template<class T> template <class T2> Vector3<T>& Vector3<T>::operator=(const Vector3<T2>& a)
//{
//	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
//	return *this;
//}

template<class T> Vector3<T>& Vector3<T>::operator=(const T* a)
{
	v[0]=a[0]; v[1]=a[1]; v[2]=a[2];
	return *this;
}

template<class T> void Vector3<T>::set(T x, T y, T z)
{
	v[0]=x; v[1]=y; v[2]=z;
}


template<class T> bool Vector3<T>::operator==(const Vector3<T>& a) const
{
	return (v[0] == a.v[0] && v[1] == a.v[1] && v[2] == a.v[2]);
}

template<class T> bool Vector3<T>::operator!=(const Vector3<T>& a) const
{
	return (v[0] != a.v[0] || v[1] != a.v[1] || v[2] != a.v[2]);
}

template<class T> bool Vector3<T>::fuzzyEquals(const Vector3<T>& a, const T eps) const
{
	return ::fuzzyEquals(v[0], a.v[0], eps) && ::fuzzyEquals(v[1], a.v[1], eps) && ::fuzzyEquals(v[2], a.v[2], eps);
}

template<class T> T& Vector3<T>::operator[](int x)
{
	return v[x];
}

template<class T> const T& Vector3<T>::operator[](int x) const
{
	return v[x];
}

template<class T> Vector3<T>::operator const T*() const
{
	return v;
}

template<class T> Vector3<T>::operator T*()
{
	return v;
}

template<class T> void Vector3<T>::operator+=(const Vector3<T>& a)
{
	v[0] += a.v[0]; v[1] += a.v[1]; v[2] += a.v[2];
}

template<class T> void Vector3<T>::operator-=(const Vector3<T>& a)
{
	v[0] -= a.v[0]; v[1] -= a.v[1]; v[2] -= a.v[2];
}

template<class T> void Vector3<T>::operator*=(T s)
{
	v[0] *= s; v[1] *= s; v[2] *= s;
}

template<class T> void Vector3<T>::operator/=(T s)
{
	v[0] /= s; v[1] /= s; v[2] /= s;
}

template<class T> Vector3<T> Vector3<T>::operator-() const
{
	return Vector3<T>(-v[0], -v[1], -v[2]);
}

template<class T> Vector3<T> Vector3<T>::operator+() const
{
	return *this;
}

template<class T> Vector3<T> Vector3<T>::operator+(const Vector3<T>& b) const
{
	return Vector3<T>(v[0] + b.v[0], v[1] + b.v[1], v[2] + b.v[2]);
}

template<class T> Vector3<T> Vector3<T>::operator-(const Vector3<T>& b) const
{
	return Vector3<T>(v[0] - b.v[0], v[1] - b.v[1], v[2] - b.v[2]);
}

template<class T> Vector3<T> Vector3<T>::operator*(T s) const
{
	return Vector3<T>(s * v[0], s * v[1], s * v[2]);
}

template<class T> Vector3<T> Vector3<T>::operator/(T s) const
{
	return Vector3<T>(v[0]/s, v[1]/s, v[2]/s);
}


template<class T> T Vector3<T>::dot(const Vector3<T>& b) const
{
	return v[0] * b.v[0] + v[1] * b.v[1] + v[2] * b.v[2];
}


// cross product
template<class T> Vector3<T> Vector3<T>::operator^(const Vector3<T>& b) const
{
	return Vector3<T>(v[1] * b.v[2] - v[2] * b.v[1],
			  v[2] * b.v[0] - v[0] * b.v[2],
			  v[0] * b.v[1] - v[1] * b.v[0]);
}

// Angle in radian between two vectors
template<class T> T Vector3<T>::angle(const Vector3<T>& b) const
{
	const T cosAngle = dot(b)/std::sqrt(lengthSquared()*b.lengthSquared());
	return cosAngle>=1 ? 0 : (cosAngle<=-1 ? M_PI : std::acos(cosAngle));
}

// Angle in radian between two normalized vectors
template<class T> T Vector3<T>::angleNormalized(const Vector3<T>& b) const
{
	const T cosAngle = dot(b);
	return cosAngle>=1 ? 0 : (cosAngle<=-1 ? M_PI : std::acos(cosAngle));
}

template<class T> T Vector3<T>::length() const
{
	return static_cast<T>(std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}

template<class T> T Vector3<T>::lengthSquared() const
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

template<class T> void Vector3<T>::normalize()
{
	T s = static_cast<T>(1. / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}

template<class T> void Vector3<T>::transfo4d(const Mat4d& m)
{
	const T v0 = v[0];
	const T v1 = v[1];
	v[0]=m.r[0]*v0 + m.r[4]*v1 + m.r[8]*v[2] + m.r[12];
	v[1]=m.r[1]*v0 + m.r[5]*v1 +  m.r[9]*v[2] + m.r[13];
	v[2]=m.r[2]*v0 + m.r[6]*v1 + m.r[10]*v[2] + m.r[14];
}

template<class T> void Vector3<T>::transfo4d(const Mat4f& m)
{
	const T v0 = v[0];
	const T v1 = v[1];
	v[0]=m.r[0]*v0 + m.r[4]*v1 + m.r[8]*v[2] + m.r[12];
	v[1]=m.r[1]*v0 + m.r[5]*v1 +  m.r[9]*v[2] + m.r[13];
	v[2]=m.r[2]*v0 + m.r[6]*v1 + m.r[10]*v[2] + m.r[14];
}

template<class T> Vec3f Vector3<T>::toVec3f() const
{
	return Vec3f(v[0],v[1],v[2]);
}

template<class T> Vec3d Vector3<T>::toVec3d() const
{
	return Vec3d(v[0],v[1],v[2]);
}

// Return latitude in rad
template<class T> T Vector3<T>::latitude() const
{
	return std::asin(v[2]/length());
}

// Return longitude in rad
template<class T> T Vector3<T>::longitude() const
{
	return std::atan2(v[1],v[0]);
}


////////////////////////// Vector4 class methods ///////////////////////////////

template<class T> Vector4<T>::Vector4() {}

template<class T> Vector4<T>::Vector4(T x)
{
	v[0]=x; v[1]=x; v[2]=x; v[3]=x;
}

template<class T> Vector4<T>::Vector4(const T* x)
{
	v[0]=x[0]; v[1]=x[1]; v[2]=x[2]; v[3]=x[3];
}

template <class T> template <class T2> Vector4<T>::Vector4(const Vector4<T2>& other)
{
	v[0]=other[0]; v[1]=other[1]; v[2]=other[2]; v[3]=other[3];
}

template<class T> Vector4<T>::Vector4(const Vector3<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2]; v[3]=1;
}

template<class T> Vector4<T>::Vector4(const Vector3<T>& a, const T w)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2]; v[3]=w;
}

template<class T> Vector4<T>::Vector4(T x, T y, T z)
{
	v[0]=x; v[1]=y; v[2]=z; v[3]=1;
}

template<class T> Vector4<T>::Vector4(T x, T y, T z, T a)
{
	v[0]=x; v[1]=y; v[2]=z; v[3]=a;
}

template<class T> Vector4<T>& Vector4<T>::operator=(const Vector3<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2]; v[3]=1;
	return *this;
}

template<class T> Vector4<T>& Vector4<T>::operator=(const T* a)
{
	v[0]=a[0]; v[1]=a[1]; v[2]=a[2]; v[3]=a[3];
	return *this;
}

template<class T> void Vector4<T>::set(T x, T y, T z, T a)
{
	v[0]=x; v[1]=y; v[2]=z; v[3]=a;
}

template<class T> bool Vector4<T>::operator==(const Vector4<T>& a) const
{
	return (v[0] == a.v[0] && v[1] == a.v[1] && v[2] == a.v[2] && v[3] == a.v[3]);
}

template<class T> bool Vector4<T>::operator!=(const Vector4<T>& a) const
{
	return (v[0] != a.v[0] || v[1] != a.v[1] || v[2] != a.v[2] || v[3] != a.v[3]);
}

template<class T> T& Vector4<T>::operator[](int x)
{
	return v[x];
}

template<class T> const T& Vector4<T>::operator[](int x) const
{
	return v[x];
}

template<class T> Vector4<T>::operator T*()
{
	return v;
}

template<class T> Vector4<T>::operator const T*() const
{
	return v;
}

template<class T> void Vector4<T>::operator+=(const Vector4<T>& a)
{
	v[0] += a.v[0]; v[1] += a.v[1]; v[2] += a.v[2]; v[3] += a.v[3];
}

template<class T> void Vector4<T>::operator-=(const Vector4<T>& a)
{
	v[0] -= a.v[0]; v[1] -= a.v[1]; v[2] -= a.v[2]; v[3] -= a.v[3];
}

template<class T> void Vector4<T>::operator*=(T s)
{
	v[0] *= s; v[1] *= s; v[2] *= s; v[3] *= s;
}

template<class T> void Vector4<T>::operator/=(T s)
{
	v[0] /= s; v[1] /= s; v[2] /= s; v[3] /= s;
}

template<class T> Vector4<T> Vector4<T>::operator-() const
{
	return Vector4<T>(-v[0], -v[1], -v[2], -v[3]);
}

template<class T> Vector4<T> Vector4<T>::operator+() const
{
	return *this;
}

template<class T> Vector4<T> Vector4<T>::operator+(const Vector4<T>& b) const
{
	return Vector4<T>(v[0] + b.v[0], v[1] + b.v[1], v[2] + b.v[2], v[3] + b.v[3]);
}

template<class T> Vector4<T> Vector4<T>::operator-(const Vector4<T>& b) const
{
	return Vector4<T>(v[0] - b.v[0], v[1] - b.v[1], v[2] - b.v[2], v[3] - b.v[3]);
}

template<class T> Vector4<T> Vector4<T>::operator*(T s) const
{
	return Vector4<T>(s * v[0], s * v[1], s * v[2], s * v[3]);
}

template<class T> Vector4<T> Vector4<T>::operator/(T s) const
{
	return Vector4<T>(v[0]/s, v[1]/s, v[2]/s, v[3]/s);
}

template<class T> T Vector4<T>::dot(const Vector4<T>& b) const
{
	return v[0] * b.v[0] + v[1] * b.v[1] + v[2] * b.v[2] + v[3] * b.v[3];
}

template<class T> T Vector4<T>::length() const
{
	return static_cast<T>(std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]));
}

template<class T> T Vector4<T>::lengthSquared() const
{
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
}

template<class T> void Vector4<T>::normalize()
{
	T s = static_cast<T>(1. / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]));
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
	v[3] *= s;
}

template<class T> void Vector4<T>::transfo4d(const Mat4d& m)
{
	(*this)=m*(*this);
}
/*
template<class T>
std::ostream& operator<<(std::ostream &o,const Vector4<T> &v) {
  return o << '[' << v[0] << ',' << v[1] << ',' << v[2] << ',' << v[3] << ']';
}*/

////////////////////////// Matrix3 class methods ///////////////////////////////

template<class T> Matrix3<T>::Matrix3() {}

template<class T> Matrix3<T>::Matrix3(const T* m)
{
	memcpy(r,m,sizeof(T)*9);
}

template<class T> Matrix3<T>::Matrix3(const Vector3<T>& v0, const Vector3<T>& v1, const Vector3<T>& v2)
{
	r[0] = v0.v[0]; r[3] = v1.v[0]; r[6] = v2.v[0];
	r[1] = v0.v[1]; r[4] = v1.v[1]; r[7] = v2.v[1];
	r[2] = v0.v[2]; r[5] = v1.v[2]; r[8] = v2.v[2];
}

template<class T> Matrix3<T>::Matrix3(T a, T b, T c, T d, T e, T f, T g, T h, T i)
{
	r[0] = a; r[3] = d; r[6] = g;
	r[1] = b; r[4] = e; r[7] = h;
	r[2] = c; r[5] = f; r[8] = i;
}

template<class T> void Matrix3<T>::set(T a, T b, T c, T d, T e, T f, T g, T h, T i)
{
	r[0] = a; r[3] = d; r[6] = g;
	r[1] = b; r[4] = e; r[7] = h;
	r[2] = c; r[5] = f; r[8] = i;
}

template<class T> T& Matrix3<T>::operator[](int n)
{
	return r[n];
}

template<class T> Matrix3<T>::operator T*()
{
	return r;
}

template<class T> Matrix3<T>::operator const T*() const
{
	return r;
}

template<class T> Matrix3<T> Matrix3<T>::identity()
{
	return Matrix3<T>(1, 0, 0,
			  0, 1, 0,
			  0, 0, 1);
}

// multiply column vector by a 3x3 matrix
template<class T> Vector3<T> Matrix3<T>::operator*(const Vector3<T>& a) const
{
	return Vector3<T>(r[0]*a.v[0] + r[3]*a.v[1] + r[6]*a.v[2],
			  r[1]*a.v[0] + r[4]*a.v[1] + r[7]*a.v[2],
			  r[2]*a.v[0] + r[5]*a.v[1] + r[8]*a.v[2]);
}

template<class T> Matrix3<T> Matrix3<T>::transpose() const
{
	return Matrix3<T>(r[0], r[3], r[6],
			  r[1], r[4], r[7],
			  r[2], r[5], r[8]);
}

template<class T> Matrix3<T> Matrix3<T>::operator*(const Matrix3<T>& a) const
{
#define MATMUL(R, C) (r[R] * a.r[C] + r[R+3] * a.r[C+1] + r[R+6] * a.r[C+2])
	return Matrix3<T>(MATMUL(0,0), MATMUL(1,0), MATMUL(2,0),
			  MATMUL(0,3), MATMUL(1,3), MATMUL(2,3),
			  MATMUL(0,6), MATMUL(1,6), MATMUL(2,6));
#undef MATMUL
}


template<class T> Matrix3<T> Matrix3<T>::operator+(const Matrix3<T>& a) const
{
	return Matrix3<T>(r[0]+a.r[0], r[1]+a.r[1], r[2]+a.r[2],
			  r[3]+a.r[3], r[4]+a.r[4], r[5]+a.r[5],
			  r[6]+a.r[6], r[7]+a.r[7], r[8]+a.r[8]);
}

template<class T> Matrix3<T> Matrix3<T>::operator-(const Matrix3<T>& a) const
{
	return Matrix3<T>(r[0]-a.r[0], r[1]-a.r[1], r[2]-a.r[2],
			  r[3]-a.r[3], r[4]-a.r[4], r[5]-a.r[5],
			  r[6]-a.r[6], r[7]-a.r[7], r[8]-a.r[8]);
}

/*
 * Code taken from 4x4 version below
 * Added by Andrei Borza
 */
template<class T> Matrix3<T> Matrix3<T>::inverse() const
{
	const T * m = r;
	T out[9];

	/* NB. OpenGL Matrices are COLUMN major. */
	#define SWAP_ROWS(a, b) { T *_tmp = a; (a)=(b); (b)=_tmp; }
	#define MAT(m,r,c) (m)[(c)*3+(r)]

	T wtmp[3][6];
	T m0, m1, m2, s;
	T *r0, *r1, *r2;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2];

	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1), r0[2] = MAT(m, 0, 2),
	r0[3] = 1.0, r0[4] = r0[5] = 0.0,
	r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1), r1[2] = MAT(m, 1, 2),
	r1[4] = 1.0, r1[3] = r1[5] = 0.0,
	r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1), r2[2] = MAT(m, 2, 2),
	r2[5] = 1.0, r2[3] = r2[4] = 0.0;

	/* choose pivot - or die */
	if (fabs(r2[0]) > fabs(r1[0]))
		SWAP_ROWS(r2, r1);
	if (fabs(r1[0]) > fabs(r0[0]))
		SWAP_ROWS(r1, r0);
	if (0.0 == r0[0])
		return Matrix3<T>();

	/* eliminate first variable     */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	s = r0[3];
	if (s != 0.0)
	{
		r1[3] -= m1 * s;
		r2[3] -= m2 * s;
	}
	s = r0[4];
	if (s != 0.0)
	{
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
	}
	s = r0[5];
	if (s != 0.0)
	{
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
	}

	/* choose pivot - or die */
	if (fabs(r2[1]) > fabs(r1[1]))
		SWAP_ROWS(r2, r1);
	if (0.0 == r1[1])
		return Matrix3<T>();

	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	r2[2] -= m2 * r1[2];
	s = r1[3];
	if (0.0 != s)
	{
		r2[3] -= m2 * s;
	}
	s = r1[4];
	if (0.0 != s)
	{
		r2[4] -= m2 * s;
	}
	s = r1[5];
	if (0.0 != s)
	{
		r2[5] -= m2 * s;
	}

	s = 1.0 / r2[2];		/* now back substitute row 2 */
	r2[3] *= s;
	r2[4] *= s;
	r2[5] *= s;

	m1 = r1[2];			/* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[3] = s * (r1[3] - r2[3] * m1), r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1);
	m0 = r0[2];
	r0[3] -= r2[3] * m0, r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0;

	m0 = r0[1];			/* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[3] = s * (r0[3] - r1[3] * m0), r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0);

	MAT(out, 0, 0) = r0[3]; MAT(out, 0, 1) = r0[4]; MAT(out, 0, 2) = r0[5];
	MAT(out, 1, 0) = r1[3]; MAT(out, 1, 1) = r1[4]; MAT(out, 1, 2) = r1[5];
	MAT(out, 2, 0) = r2[3]; MAT(out, 2, 1) = r2[4]; MAT(out, 2, 2) = r2[5];

	return Matrix3<T>(out);

	#undef MAT
	#undef SWAP_ROWS
}

template<class T> void Matrix3<T>::print(void) const
{
        printf("[%5.2lf %5.2lf %5.2lf]\n"
               "[%5.2lf %5.2lf %5.2lf]\n"
               "[%5.2lf %5.2lf %5.2lf]\n",
               r[0],r[3],r[6],
               r[1],r[4],r[7],
               r[2],r[5],r[8]);
}

////////////////////////// Matrix4 class methods ///////////////////////////////

template<class T> Matrix4<T>::Matrix4() {}

template<class T> Matrix4<T>::Matrix4(const T* m)
{
	memcpy(r,m,sizeof(T)*16);
}

template<class T> Matrix4<T>::Matrix4(const Vector4<T>& v0,
				      const Vector4<T>& v1,
				      const Vector4<T>& v2,
				      const Vector4<T>& v3)
{
	r[0] = v0.v[0];
	r[1] = v0.v[1];
	r[2] = v0.v[2];
	r[3] = v0.v[3];
	r[4] = v1.v[0];
	r[5] = v1.v[1];
	r[6] = v1.v[2];
	r[7] = v1.v[3];
	r[8] = v2.v[0];
	r[9] = v2.v[1];
	r[10] = v2.v[2];
	r[11] = v2.v[3];
	r[12] = v3.v[0];
	r[13] = v3.v[1];
	r[14] = v3.v[2];
	r[15] = v3.v[3];
}


template<class T> Matrix4<T>::Matrix4(T a, T b, T c, T d, T e, T f, T g, T h, T i, T j, T k, T l, T m, T n, T o, T p)
{
	r[0]=a; r[1]=b; r[2]=c; r[3]=d; r[4]=e; r[5]=f; r[6]=g; r[7]=h;
	r[8]=i; r[9]=j; r[10]=k; r[11]=l; r[12]=m; r[13]=n; r[14]=o; r[15]=p;
}

template<class T> void Matrix4<T>::set(T a, T b, T c, T d, T e, T f, T g, T h, T i, T j, T k, T l, T m, T n, T o, T p)
{
	r[0]=a; r[1]=b; r[2]=c; r[3]=d; r[4]=e; r[5]=f; r[6]=g; r[7]=h;
	r[8]=i; r[9]=j; r[10]=k; r[11]=l; r[12]=m; r[13]=n; r[14]=o; r[15]=p;
}

template<class T> T& Matrix4<T>::operator[](int n)
{
	return r[n];
}

template<class T> Matrix4<T>::operator T*()
{
	return r;
}

template<class T> Matrix4<T>::operator const T*() const
{
	return r;
}

template<class T> Matrix4<T> Matrix4<T>::identity()
{
	return Matrix4<T>(1, 0, 0, 0,
			  0, 1, 0, 0,
			  0, 0, 1, 0,
			  0, 0, 0, 1);
}


template<class T> Matrix4<T> Matrix4<T>::translation(const Vector3<T>& a)
{
	return Matrix4<T>(1     , 0     , 0     , 0,
			  0     , 1     , 0     , 0,
			  0     , 0     , 1     , 0,
			  a.v[0], a.v[1], a.v[2], 1);
}


template<class T> Matrix4<T> Matrix4<T>::rotation(const Vector3<T>& axis, T angle)
{
	Vector3<T> a(axis);
	a.normalize();
	const T c = static_cast<T>(cos(angle));
	const T s = static_cast<T>(sin(angle));
	const T d = 1-c;
	return Matrix4<T>(a[0]*a[0]*d+c     , a[1]*a[0]*d+a[2]*s, a[0]*a[2]*d-a[1]*s, 0,
			  a[0]*a[1]*d-a[2]*s, a[1]*a[1]*d+c     , a[1]*a[2]*d+a[0]*s, 0,
			  a[0]*a[2]*d+a[1]*s, a[1]*a[2]*d-a[0]*s, a[2]*a[2]*d+c     , 0,
			  0                 ,                  0,                  0, 1);
}

template<class T> Matrix4<T> Matrix4<T>::xrotation(T angle)
{
	T c = static_cast<T>(cos(angle));
	T s = static_cast<T>(sin(angle));

	return Matrix4<T>(1, 0, 0, 0,
			  0, c, s, 0,
			  0,-s, c, 0,
			  0, 0, 0, 1);
}


template<class T> Matrix4<T> Matrix4<T>::yrotation(T angle)
{
	T c = static_cast<T>(cos(angle));
	T s = static_cast<T>(sin(angle));

	return Matrix4<T>(c, 0,-s, 0,
			  0, 1, 0, 0,
			  s, 0, c, 0,
			  0, 0, 0, 1);
}


template<class T> Matrix4<T> Matrix4<T>::zrotation(T angle)
{
	T c = static_cast<T>(cos(angle));
	T s = static_cast<T>(sin(angle));

	return Matrix4<T>(c , s, 0, 0,
			  -s, c, 0, 0,
			  0 , 0, 1, 0,
			  0 , 0, 0, 1);
}


template<class T> Matrix4<T> Matrix4<T>::scaling(const Vector3<T>& s)
{
	return Matrix4<T>(s[0], 0  , 0  , 0,
			  0   ,s[1], 0  , 0,
			  0   , 0  ,s[2], 0,
			  0   , 0  , 0  , 1);
}


template<class T> Matrix4<T> Matrix4<T>::scaling(T scale)
{
	return scaling(Vector3<T>(scale, scale, scale));
}

// multiply column vector by a 4x4 matrix in homogeneous coordinate (use a[3]=1)
template<class T> Vector3<T> Matrix4<T>::operator*(const Vector3<T>& a) const
{
	return Vector3<T>(r[0]*a.v[0] + r[4]*a.v[1] +  r[8]*a.v[2] + r[12],
			  r[1]*a.v[0] + r[5]*a.v[1] +  r[9]*a.v[2] + r[13],
			  r[2]*a.v[0] + r[6]*a.v[1] + r[10]*a.v[2] + r[14]);
}

template<class T> Vector3<T> Matrix4<T>::multiplyWithoutTranslation(const Vector3<T>& a) const
{
	return Vector3<T>(r[0]*a.v[0] + r[4]*a.v[1] +  r[8]*a.v[2],
			  r[1]*a.v[0] + r[5]*a.v[1] +  r[9]*a.v[2],
			  r[2]*a.v[0] + r[6]*a.v[1] + r[10]*a.v[2]);
}

// multiply column vector by a 4x4 matrix in homogeneous coordinate (considere a[3]=1)
template<class T> Vector4<T> Matrix4<T>::operator*(const Vector4<T>& a) const
{
	return Vector4<T>(r[0]*a.v[0] + r[4]*a.v[1] +  r[8]*a.v[2] + r[12]*a.v[3],
			  r[1]*a.v[0] + r[5]*a.v[1] +  r[9]*a.v[2] + r[13]*a.v[3],
			  r[2]*a.v[0] + r[6]*a.v[1] + r[10]*a.v[2] + r[14]*a.v[3]);
}

template<class T> void Matrix4<T>::transfo(Vector3<T>& a) const
{
	a.set(r[0]*a.v[0] + r[4]*a.v[1] +  r[8]*a.v[2] + r[12],
	      r[1]*a.v[0] + r[5]*a.v[1] +  r[9]*a.v[2] + r[13],
	      r[2]*a.v[0] + r[6]*a.v[1] + r[10]*a.v[2] + r[14]);
}

template<class T> Matrix4<T> Matrix4<T>::transpose() const
{
	return Matrix4<T>(r[0], r[4], r[8],  r[12],
			  r[1], r[5], r[9],  r[13],
			  r[2], r[6], r[10], r[14],
			  r[3], r[7], r[11], r[15]);
}

template<class T> Matrix4<T> Matrix4<T>::operator*(const Matrix4<T>& a) const
{
#define MATMUL(R, C) (r[R] * a.r[C] + r[R+4] * a.r[C+1] + r[R+8] * a.r[C+2] + r[R+12] * a.r[C+3])
	return Matrix4<T>(MATMUL(0,0) , MATMUL(1,0) , MATMUL(2,0) , MATMUL(3,0) ,
			  MATMUL(0,4) , MATMUL(1,4) , MATMUL(2,4) , MATMUL(3,4) ,
			  MATMUL(0,8) , MATMUL(1,8) , MATMUL(2,8) , MATMUL(3,8) ,
			  MATMUL(0,12), MATMUL(1,12), MATMUL(2,12), MATMUL(3,12));
#undef MATMUL
}


template<class T> Matrix4<T> Matrix4<T>::operator+(const Matrix4<T>& a) const
{
	return Matrix4<T>(r[0]+a.r[0]  , r[1]+a.r[1]  , r[2]+a.r[2]  , r[3]+a.r[3]  ,
			  r[4]+a.r[4]  , r[5]+a.r[5]  , r[6]+a.r[6]  , r[7]+a.r[7]  ,
			  r[8]+a.r[8]  , r[9]+a.r[9]  , r[10]+a.r[10], r[11]+a.r[11],
			  r[12]+a.r[12], r[13]+a.r[13], r[14]+a.r[14], r[15]+a.r[15]);
}

template<class T> Matrix4<T> Matrix4<T>::operator-(const Matrix4<T>& a) const
{
	return Matrix4<T>(r[0]-a.r[0]  , r[1]-a.r[1]  , r[2]-a.r[2]  , r[3]-a.r[3]  ,
			  r[4]-a.r[4]  , r[5]-a.r[5]  , r[6]-a.r[6]  , r[7]-a.r[7]  ,
			  r[8]-a.r[8]  , r[9]-a.r[9]  , r[10]-a.r[10], r[11]-a.r[11],
			  r[12]-a.r[12], r[13]-a.r[13], r[14]-a.r[14], r[15]-a.r[15]);
}

/*
 * Code ripped from the GLU library
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return zero matrix on failure (singular matrix)
 */
template<class T> Matrix4<T> Matrix4<T>::inverse() const
{
	const T * m = r;
	T out[16];

	/* NB. OpenGL Matrices are COLUMN major. */
	#define SWAP_ROWS(a, b) { T *_tmp = a; (a)=(b); (b)=_tmp; }
	#define MAT(m,r,c) (m)[(c)*4+(r)]

	T wtmp[4][8];
	T m0, m1, m2, m3, s;
	T *r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
	r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
	r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
	r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
	r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
	r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
	r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
	r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
	r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
	r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
	r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
	r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	/* choose pivot - or die */
	if (fabs(r3[0]) > fabs(r2[0]))
		SWAP_ROWS(r3, r2);
	if (fabs(r2[0]) > fabs(r1[0]))
		SWAP_ROWS(r2, r1);
	if (fabs(r1[0]) > fabs(r0[0]))
		SWAP_ROWS(r1, r0);
	if (0.0 == r0[0])
		return Matrix4<T>();

	/* eliminate first variable     */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (fabs(r3[1]) > fabs(r2[1]))
		SWAP_ROWS(r3, r2);
	if (fabs(r2[1]) > fabs(r1[1]))
		SWAP_ROWS(r2, r1);
	if (0.0 == r1[1])
		return Matrix4<T>();

	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (fabs(r3[2]) > fabs(r2[2]))
		SWAP_ROWS(r3, r2);
	if (0.0 == r2[2])
		return Matrix4<T>();

	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

	/* last check */
	if (0.0 == r3[3])
		return Matrix4<T>();

	s = 1.0 / r3[3];		/* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;

	m2 = r2[3];			/* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];			/* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];			/* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];

	return Matrix4<T>(out);

	#undef MAT
	#undef SWAP_ROWS
}


template<class T> Matrix3<T> Matrix4<T>::upper3x3() const
{
    return Matrix3<T>(r[0], r[1], r[2],
		      r[4], r[5], r[6],
		      r[8], r[9], r[10]);
}
template<class T> Matrix3<T> Matrix4<T>::upper3x3Transposed() const
{
    return Matrix3<T>(r[0], r[4], r[8],
                      r[1], r[5], r[9],
                      r[2], r[6], r[10]);
}
template<class T> Vector4<T> Matrix4<T>::getRow(const int row) const
{
	return Vector4<T>(r[0 + row], r[4 + row], r[8 + row], r[12 + row]);
}

template<class T> Vector4<T> Matrix4<T>::getColumn(const int column) const
{
	return Vector4<T>(r[0 + column * 4], r[1 + column * 4], r[2 + column * 4], r[3 + column * 4]);
}

template<class T> QMatrix4x4 Matrix4<T>::convertToQMatrix() const
{
	return QMatrix4x4( r[0], r[4], r[8],r[12],
			   r[1], r[5], r[9],r[13],
			   r[2], r[6],r[10],r[14],
			   r[3], r[7],r[11],r[15] );
}

template<class T> void Matrix4<T>::print(void) const
{
	printf("[%5.2lf %5.2lf %5.2lf %17.12le]\n"
		   "[%5.2lf %5.2lf %5.2lf %17.12le]\n"
		   "[%5.2lf %5.2lf %5.2lf %17.12le]\n"
		   "[%5.2lf %5.2lf %5.2lf %17.12le]\n\n",
	r[0],r[4],r[8],r[12],
	r[1],r[5],r[9],r[13],
	r[2],r[6],r[10],r[14],
	r[3],r[7],r[11],r[15]);
}


template<class T> inline
T operator*(const Vector2<T>&a,const Vector2<T>&b)
{
	return a.v[0] * b.v[0] + a.v[1] * b.v[1];
}

template<class T> inline
T operator*(const Vector3<T>&a,const Vector3<T>&b)
{
	return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2];
}

template<class T> inline
T operator*(const Vector4<T>&a,const Vector4<T>&b)
{
	return a.v[0]*b.v[0] + a.v[1]*b.v[1] + a.v[2]*b.v[2] + a.v[3]*b.v[3];
}

template<class T> inline
Vector2<T> operator*(T s,const Vector2<T>&v)
{
	return Vector2<T>(s*v[0],s*v[1]);
}

template<class T> inline
Vector3<T> operator*(T s,const Vector3<T>&v)
{
	return Vector3<T>(s*v[0],s*v[1],s*v[2]);
}

template<class T> inline
Vector4<T> operator*(T s,const Vector4<T>&v)
{
	return Vector4<T>(s*v[0],s*v[1],s*v[2],s*v[3]);
}

//Make Qt handle the classes as primitive type. This optimizes performance with Qt's container classes
Q_DECLARE_TYPEINFO(Vec2d, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec2f, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec2i, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec3d, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec3f, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec3i, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec4d, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec4f, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Vec4i, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Mat4d, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Mat4f, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Mat3d, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Mat3f, Q_PRIMITIVE_TYPE);

//Declare meta-type information so that Vec/Mat classes can be used in QVariant
//They are registered (qRegisterMetaType) in StelCore::registerMathMetaTypes, called in constructor
Q_DECLARE_METATYPE(Vec2d)
Q_DECLARE_METATYPE(Vec2f)
Q_DECLARE_METATYPE(Vec2i)
Q_DECLARE_METATYPE(Vec3d)
Q_DECLARE_METATYPE(Vec3f)
Q_DECLARE_METATYPE(Vec3i)
Q_DECLARE_METATYPE(Vec4d)
Q_DECLARE_METATYPE(Vec4f)
Q_DECLARE_METATYPE(Vec4i)
Q_DECLARE_METATYPE(Mat4d)
Q_DECLARE_METATYPE(Mat4f)
Q_DECLARE_METATYPE(Mat3d)
Q_DECLARE_METATYPE(Mat3f)

template <class T>
QDebug operator<<(QDebug debug, const Vector2<T> &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '[' << c[0] << ", " << c[1] << ']';

    return debug;
}

template <class T>
QDebug operator<<(QDebug debug, const Vector3<T> &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '[' << c[0] << ", " << c[1] << ", " << c[2] << ']';

    return debug;
}

template <class T>
QDebug operator<<(QDebug debug, const Vector4<T> &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '[' << c[0] << ", " << c[1] << ", " << c[2] << ", " << c[3] << ']';

    return debug;
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

#endif // VECMATH_HPP
