/*
 *
 * Copyright (C) 2003 Fabien Chéreau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Template vector and matrix library.
// Use OpenGL compatible ordering ie. you can pass a matrix or vector to
// openGL functions without changes in the ordering

#ifndef _VECMATH_H_
#define _VECMATH_H_

#include <cmath>
#include <string.h>
#include <stdio.h>

template<class T> class Vector2;
template<class T> class Vector3;
template<class T> class Vector4;
template<class T> class Matrix4;

typedef Vector2<float>	Vec2f;
typedef Vector2<int>	Vec2i;

typedef Vector3<float>	Vec3f;
typedef Vector3<double>	Vec3d;

typedef Vector4<double>	Vec4d;
typedef Vector4<float>	Vec4f;
typedef Vector4<int>	Vec4i;

typedef Matrix4<float>	Mat4f;
typedef Matrix4<double>	Mat4d;

template<class T> class Vector2
{
public:
    inline Vector2();
    inline Vector2(const Vector2<T>&);
    inline Vector2(T, T);

	inline Vector2& operator=(const Vector2<T>&);
	inline Vector2& operator=(const T*);
    inline void set(T, T);

	inline bool operator==(const Vector2<T>&) const;
	inline bool operator!=(const Vector2<T>&) const;

	inline const T& operator[](int x) const;
    inline T& operator[](int);
	inline operator const T*() const;
	inline operator T*();

    inline Vector2& operator+=(const Vector2<T>&);
    inline Vector2& operator-=(const Vector2<T>&);
    inline Vector2& operator*=(T);
	inline Vector2& operator/=(T);

    inline Vector2 operator-(const Vector2<T>&) const;
    inline Vector2 operator+(const Vector2<T>&) const;

	inline Vector2 Vector2<T>::operator-() const;
	inline Vector2 Vector2<T>::operator+() const;

	inline Vector2 operator*(T) const;
	inline Vector2 operator/(T) const;


    inline T dot(const Vector2<T>&) const;

    inline T length() const;
    inline T lengthSquared() const;
    inline void normalize();

    T v[2];
};


template<class T> class Vector3
{
public:
    inline Vector3();
    inline Vector3(const Vector3&);
	template <class T2> inline Vector3(const Vector3<T2>&);
    inline Vector3(T, T, T);

	inline Vector3& operator=(const Vector3&);
	inline Vector3& operator=(const T*);
	template <class T2> inline Vector3& operator=(const Vector3<T2>&);
    inline void set(T, T, T);

	inline bool operator==(const Vector3<T>&) const;
	inline bool operator!=(const Vector3<T>&) const;

    inline T& operator[](int);
    inline const T& operator[](int) const;
	inline operator const T*() const;
	inline operator T*();

    inline Vector3& operator+=(const Vector3<T>&);
    inline Vector3& operator-=(const Vector3<T>&);
    inline Vector3& operator*=(T);
	inline Vector3& operator/=(T);

    inline Vector3 operator-(const Vector3<T>&) const;
    inline Vector3 operator+(const Vector3<T>&) const;

	inline Vector3 Vector3<T>::operator-() const;
	inline Vector3 Vector3<T>::operator+() const;

	inline Vector3 operator*(T) const;
	inline Vector3 operator/(T) const;


    inline T dot(const Vector3<T>&) const;
	inline Vector3 operator^(const Vector3<T>&) const;

    inline T length() const;
    inline T lengthSquared() const;
    inline void normalize();

	inline void transfo4d(const Mat4d&);
	inline void transfo4d(const Mat4f&);
	T v[3];		// The 3 values
};


template<class T> class Vector4
{
public:
    inline Vector4();
    inline Vector4(const Vector4<T>&);
	inline Vector4(const Vector3<T>&);
    inline Vector4(T, T, T, T);

	inline Vector4& operator=(const Vector4<T>&);
	inline Vector4& operator=(const T*);
    inline void set(T, T, T, T);

	inline bool operator==(const Vector4<T>&) const;
	inline bool operator!=(const Vector4<T>&) const;

    inline T& operator[](int);
    inline const T& operator[](int) const;
	inline operator T*();
	inline operator const T*() const;

    inline Vector4& operator+=(const Vector4<T>&);
    inline Vector4& operator-=(const Vector4<T>&);
    inline Vector4& operator*=(T);
	inline Vector4& operator/=(T);

    inline Vector4 operator-(const Vector4<T>&) const;
    inline Vector4 operator+(const Vector4<T>&) const;

	inline Vector4 Vector4<T>::operator-() const;
	inline Vector4 Vector4<T>::operator+() const;

	inline Vector4 operator*(T) const;
	inline Vector4 operator/(T) const;


    inline T dot(const Vector4<T>&) const;

    inline T length() const;
    inline T lengthSquared() const;
    inline void normalize();

	inline void transfo4d(const Mat4d&);

	T v[4];		// The 4 values
};


// Column-major matrix compatible with openGL.
template<class T> class Matrix4
{
 public:
    Matrix4();
    Matrix4(const Matrix4<T>& m);
	Matrix4(T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T);
	Matrix4(const T*);

	inline Matrix4& operator=(const Matrix4<T>&);
	inline Matrix4& operator=(const T*);
	inline void set(T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T);

    inline T* operator[](int);
	inline operator T*();
	inline operator const T*() const;

    inline Matrix4 operator-(const Matrix4<T>&) const;
    inline Matrix4 operator+(const Matrix4<T>&) const;
    inline Matrix4 operator*(const Matrix4<T>&) const;

    inline Vector3<T> operator*(const Vector3<T>&) const;
	inline Vector4<T> operator*(const Vector4<T>&) const;

    static Matrix4<T> identity();
    static Matrix4<T> translation(const Vector3<T>&);

	static Matrix4<T> rotation(const Vector3<T>&);
    static Matrix4<T> xrotation(T);
    static Matrix4<T> yrotation(T);
    static Matrix4<T> zrotation(T);
    static Matrix4<T> scaling(const Vector3<T>&);
    static Matrix4<T> scaling(T);

    Matrix4<T> transpose() const;
    Matrix4<T> inverse() const;

	inline void print(void) const;

    T r[16];
};



////////////////////////// Vector2 class methods ///////////////////////////////

template<class T> Vector2<T>::Vector2()
{
	v[0]=0; v[1]=0;
}

template<class T> Vector2<T>::Vector2(const Vector2<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1];
}

template<class T> Vector2<T>::Vector2(T x, T y)
{
	v[0]=x; v[1]=y;
}

template<class T> Vector2<T>& Vector2<T>::operator=(const Vector2<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1];
    return *this;
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

template<class T> Vector2<T> Vector2<T>::operator/(T s) const
{
    return Vector2<T>(v[0]/s, v[1]/s);
}


template<class T> T Vector2<T>::dot(const Vector2<T>& b) const
{
    return v[0] * b.v[0] + v[1] * b.v[1];
}


template<class T> T Vector2<T>::length() const
{
    return (T) sqrt(v[0] * v[0] + v[1] * v[1]);
}

template<class T> T Vector2<T>::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1];
}

template<class T> void Vector2<T>::normalize()
{
    T s = (T) 1 / sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] *= s;
    v[1] *= s;
}


////////////////////////// Vector3 class methods ///////////////////////////////

template<class T> Vector3<T>::Vector3()
{
	v[0]=0; v[1]=0; v[2]=0;
}

template<class T> Vector3<T>::Vector3(const Vector3& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
}

template<class T> template<class T2> Vector3<T>::Vector3(const Vector3<T2>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
}

template<class T> Vector3<T>::Vector3(T x, T y, T z)
{
	v[0]=x; v[1]=y; v[2]=z;
}

template<class T> Vector3<T>& Vector3<T>::operator=(const Vector3& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
    return *this;
}

template<class T> template <class T2> Vector3<T>& Vector3<T>::operator=(const Vector3<T2>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2];
    return *this;
}

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

template<class T> Vector3<T>& Vector3<T>::operator+=(const Vector3<T>& a)
{
    v[0] += a.v[0]; v[1] += a.v[1]; v[2] += a.v[2];
    return *this;
}

template<class T> Vector3<T>& Vector3<T>::operator-=(const Vector3<T>& a)
{
    v[0] -= a.v[0]; v[1] -= a.v[1]; v[2] -= a.v[2];
    return *this;
}

template<class T> Vector3<T>& Vector3<T>::operator*=(T s)
{
    v[0] *= s; v[1] *= s; v[2] *= s;
    return *this;
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


template<class T> Vector3<T> Vector3<T>::operator^(const Vector3<T>& b) const
{
    return Vector3<T>(v[1] * b.v[2] - v[2] * b.v[1],
                      v[2] * b.v[0] - v[0] * b.v[2],
                      v[0] * b.v[1] - v[1] * b.v[0]);
}


template<class T> T Vector3<T>::length() const
{
    return (T) sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

template<class T> T Vector3<T>::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

template<class T> void Vector3<T>::normalize()
{
    register T s = (T) (1. / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

template<class T> void Vector3<T>::transfo4d(const Mat4d& m)
{
	(*this)=m*(*this);
}

template<class T> void Vector3<T>::transfo4d(const Mat4f& m)
{
	(*this)=m*(*this);
}

////////////////////////// Vector4 class methods ///////////////////////////////

template<class T> Vector4<T>::Vector4()
{
	v[0]=0; v[1]=0; v[2]=0; v[3]=0;
}

template<class T> Vector4<T>::Vector4(const Vector4<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2]; v[3]=a.v[3];
}

template<class T> Vector4<T>::Vector4(const Vector3<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2]; v[3]=1;
}

template<class T> Vector4<T>::Vector4(T x, T y, T z, T a = 1)
{
	v[0]=x; v[1]=y; v[2]=z; v[3]=a;
}

template<class T> Vector4<T>& Vector4<T>::operator=(const Vector4<T>& a)
{
	v[0]=a.v[0]; v[1]=a.v[1]; v[2]=a.v[2]; v[3]=a.v[3];
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

template<class T> Vector4<T>& Vector4<T>::operator+=(const Vector4<T>& a)
{
    v[0] += a.v[0]; v[1] += a.v[1]; v[2] += a.v[2]; v[3] += a.v[3];
    return *this;
}

template<class T> Vector4<T>& Vector4<T>::operator-=(const Vector4<T>& a)
{
    v[0] -= a.v[0]; v[1] -= a.v[1]; v[2] -= a.v[2]; v[3] -= a/v[3];
    return *this;
}

template<class T> Vector4<T>& Vector4<T>::operator*=(T s)
{
    v[0] *= s; v[1] *= s; v[2] *= s; v[3] *= s;
    return *this;
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
    return (T) sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
}

template<class T> T Vector4<T>::lengthSquared() const
{
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
}

template<class T> void Vector4<T>::normalize()
{
    T s = (T) (1. / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]));
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
	v[3] *= s;
}

template<class T> void Vector4<T>::transfo4d(const Mat4d& m)
{
	(*this)=m*(*this);
}


////////////////////////// Matrix4 class methods ///////////////////////////////

template<class T> Matrix4<T>::Matrix4()
{
	r[0]=0; r[1]=0; r[2]=0; r[3]=0; r[4]=0; r[5]=0; r[6]=0; r[7]=0;
	r[8]=0; r[9]=0; r[10]=0; r[11]=0; r[12]=0; r[13]=0; r[14]=0; r[15]=0;
}

template<class T> Matrix4<T>::Matrix4(const Matrix4<T>& m)
{
	memcpy(r,m.r,sizeof(m.r));
}

template<class T> Matrix4<T>::Matrix4(const T* m)
{
	memcpy(r,m,sizeof(T)*16);
}

template<class T> Matrix4<T>& Matrix4<T>::operator=(const Matrix4<T>& m)
{
	memcpy(r,m.r,sizeof(m.r));
	return (*this);
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

template<class T> T* Matrix4<T>::operator[](int n)
{
    return &(r[n*4]);
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
    return Matrix4<T>(	1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 1, 0,
						0, 0, 0, 1  );
}


template<class T> Matrix4<T> Matrix4<T>::translation(const Vector3<T>& a)
{
    return Matrix4<T>(	1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 1, 0,
						a.v[0], a.v[1], a.v[2], 1);
}

template<class T> Matrix4<T> Matrix4<T>::rotation(const Vector3<T>& a)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);
	T d = 1-c;
	 return Matrix4<T>(	a.v[0]*a.v[0]*d+c, a.v[1]*a.v[0]*d+a.v[2]*s, a.v[0]*a.v[2]*d-a.v[1]*s, 0,
						a.v[0]*a.v[1]*d-a.v[2]*s, a.v[1]*a.v[1]*d+c, a.v[1]*a.v[2]*d+a.v[0]*s, 0,
						a.v[0]*a.v[2]*d+a.v[1]*s, a.v[1]*a.v[2]*d-a.v[0]*s, a.v[2]*a.v[2]*d+c, 0,
						0,0,0,1	);
}

template<class T> Matrix4<T> Matrix4<T>::xrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix4<T>(1, 0, 0, 0,
                      0, c, s, 0,
                      0,-s, c, 0,
                      0, 0, 0, 1 );
}


template<class T> Matrix4<T> Matrix4<T>::yrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix4<T>( c, 0,-s, 0,
                       0, 1, 0, 0,
                       s, 0, c, 0,
                       0, 0, 0, 1 );
}


template<class T> Matrix4<T> Matrix4<T>::zrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix4<T>(c, s, 0, 0,
                     -s, c, 0, 0,
                      0, 0, 1, 0,
                      0, 0, 0, 1 );
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
    return Vector3<T>(	r[0]*a.v[0] + r[4]*a.v[1] +  r[8]*a.v[2] + r[12],
						r[1]*a.v[0] + r[5]*a.v[1] +  r[9]*a.v[2] + r[13],
						r[2]*a.v[0] + r[6]*a.v[1] + r[10]*a.v[2] + r[14] );
}

// multiply column vector by a 4x4 matrix in homogeneous coordinate (considere a[3]=1)
template<class T> Vector4<T> Matrix4<T>::operator*(const Vector4<T>& a) const
{
    return Vector4<T>(	r[0]*a.v[0] + r[4]*a.v[1] +  r[8]*a.v[2] + r[12]*a.v[3],
						r[1]*a.v[0] + r[5]*a.v[1] +  r[9]*a.v[2] + r[13]*a.v[3],
						r[2]*a.v[0] + r[6]*a.v[1] + r[10]*a.v[2] + r[14]*a.v[3] );
}

template<class T> Matrix4<T> Matrix4<T>::transpose() const
{
    return Matrix4<T>(	r[0], r[4], r[8],  r[12],
						r[1], r[5], r[9],  r[13],
						r[2], r[6], r[10], r[14],
						r[3], r[7], r[11], r[15]);
}

template<class T> Matrix4<T> Matrix4<T>::operator*(const Matrix4<T>& a) const
{
#define MATMUL(R, C) (r[R] * a.r[C] + r[R+4] * a.r[C+1] + r[R+8] * a.r[C+2] + r[R+12] * a.r[C+3])
    return Matrix4<T>(	MATMUL(0,0), MATMUL(1,0), MATMUL(2,0), MATMUL(3,0),
						MATMUL(0,4), MATMUL(1,4), MATMUL(2,4), MATMUL(3,4),
						MATMUL(0,8), MATMUL(1,8), MATMUL(2,8), MATMUL(3,8),
						MATMUL(0,12), MATMUL(1,12), MATMUL(2,12), MATMUL(3,12) );
#undef MATMUL
}


template<class T> Matrix4<T> Matrix4<T>::operator+(const Matrix4<T>& a) const
{
    return Matrix4<T>(	r[0]+a.r[0], r[1]+a.r[1], r[2]+a.r[2], r[3]+a.r[3],
						r[4]+a.r[4], r[5]+a.r[5], r[6]+a.r[6], r[7]+a.r[7],
						r[8]+a.r[8], r[9]+a.r[9], r[10]+a.r[10], r[11]+a.r[11],
						r[12]+a.r[12], r[13]+a.r[13], r[14]+a.r[14], r[15]+a.r[15] );
}

template<class T> Matrix4<T> Matrix4<T>::operator-(const Matrix4<T>& a) const
{
    return Matrix4<T>(	r[0]-a.r[0], r[1]-a.r[1], r[2]-a.r[2], r[3]-a.r[3],
						r[4]-a.r[4], r[5]-a.r[5], r[6]-a.r[6], r[7]-a.r[7],
						r[8]-a.r[8], r[9]-a.r[9], r[10]-a.r[10], r[11]-a.r[11],
						r[12]-a.r[12], r[13]-a.r[13], r[14]-a.r[14], r[15]-a.r[15] );
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


template<class T> void Matrix4<T>::print(void) const
{
	printf("[%.2lf %.2lf %.2lf %.2lf]\n[%.2lf %.2lf %.2lf %.2lf]\n[%.2lf %.2lf %.2lf %.2lf]\n[%.2lf %.2lf %.2lf %.2lf]\n",
	r[0],r[4],r[8],r[12],r[1],r[5],r[9],r[13],r[2],r[6],r[10],r[14],r[3],r[7],r[11],r[15]);
}

#endif // _VECMATH_H_
