// vecmath.h
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// Basic templatized vector and matrix math library.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#ifndef _VECMATH_H_
#define _VECMATH_H_

#include <cmath>


template<class T> class Point3;
template<class T> class Matrix4;

template<class T> class Vector3
{
public:
    inline Vector3();
    inline Vector3(const Vector3<T>&);
    inline Vector3(T, T, T);

    inline T &operator[](int) const;
    inline Vector3& operator+=(const Vector3<T>&);
    inline Vector3& operator-=(const Vector3<T>&);
    inline Vector3& operator*=(T);

    inline Vector3 operator-();
    inline Vector3 operator+();
	operator T*(void) {return &x;} // Warning unstable

    inline Vector3 operator*(T);
	inline Vector3 operator/(T);
    inline T dot(const Vector3<T>&);
    void set(T a,T b,T c) {x=a; y=b; z=c;}
	inline Vector3& operator=(T*);

    inline T length() const;
    inline T lengthSquared() const;
    inline void normalize();

	// transform a 3 element column vector by a 4x4 matrix in homogeneous coordinate
	inline Vector3 transfo4d(const Matrix4<T>& m) const;

    T x, y, z;
};

template<class T> class Point3
{
public:
    inline Point3();
    inline Point3(const Point3&);
    inline Point3(T, T, T);

    inline T& operator[](int) const;
    inline Point3& operator+=(const Vector3<T>&);
    inline Point3& operator-=(const Vector3<T>&);
    inline Point3& operator*=(T);

    inline T distanceTo(const Point3&) const;
    inline T distanceToSquared(const Point3&) const;
    inline T distanceFromOrigin() const;
    inline T distanceFromOriginSquared() const;

    T x, y, z;
};


template<class T> class Point2;

template<class T> class Vector2
{
public:
    inline Vector2();
    inline Vector2(T, T);
    inline T& operator[](int) const;
    inline Vector2 operator+(const Vector2<T>&);
    inline Vector2& operator+=(const Vector2<T>&);
    inline Vector2& operator-=(const Vector2<T>&);
    T x, y;
};

template<class T> class Point2
{
public:
    inline Point2();
    inline Point2(T, T);

    T x, y;
};


template<class T> class Vector4
{
 public:
    inline Vector4();
    inline Vector4(T, T, T, T);

    inline T& operator[](int) const;
    inline Vector4& operator+=(const Vector4&);
    inline Vector4& operator-=(const Vector4&);
    inline Vector4& operator*=(T);

    inline Vector4 operator-();
    inline Vector4 operator+();

    T x, y, z, w;
};


template<class T> class Matrix4
{
 public:
    Matrix4();
    Matrix4(const Vector4<T>&, const Vector4<T>&,
            const Vector4<T>&, const Vector4<T>&);
    Matrix4(const Matrix4<T>& m);

    inline const Vector4<T>& operator[](int) const;
    inline Vector4<T> row(int) const;
    inline Vector4<T> column(int) const;

    static Matrix4<T> identity();
    static Matrix4<T> translation(const Point3<T>&);
    static Matrix4<T> translation(const Vector3<T>&);
    static Matrix4<T> rotation(const Vector3<T>&, T);
    static Matrix4<T> xrotation(T);
    static Matrix4<T> yrotation(T);
    static Matrix4<T> zrotation(T);
    static Matrix4<T> scaling(const Vector3<T>&);
    static Matrix4<T> scaling(T);

    void translate(const Point3<T>&);

    Matrix4<T> transpose() const;
    Matrix4<T> inverse() const;

    Vector4<T> r[4];
};


template<class T> class Matrix3
{
 public:
    Matrix3();
    Matrix3(const Vector3<T>&, const Vector3<T>&, const Vector3<T>&);
    template<class U> Matrix3(const Matrix3<U>&);

    static Matrix3<T> xrotation(T);
    static Matrix3<T> yrotation(T);
    static Matrix3<T> zrotation(T);

    inline const Vector3<T>& operator[](int) const;
    inline Vector3<T> row(int) const;
    inline Vector3<T> column(int) const;

    inline Matrix3& operator*=(T);

    static Matrix3<T> identity();

    Matrix3<T> transpose() const;
    Matrix3<T> inverse() const;
    T determinant() const;

    //    template<class U> operator Matrix3<U>() const;

    Vector3<T> r[3];
};


typedef Vector3<float>   Vec3f;
typedef Vector3<float>   vec3_t;
typedef Vector3<double>  Vec3d;
typedef Point3<float>    Point3f;
typedef Point3<double>   Point3d;
typedef Vector2<float>   Vec2f;
typedef Vector2<float>   vec2_t;
typedef Vector2<int>     vec2_i;
typedef Point2<float>    Point2f;
typedef Vector4<float>   Vec4f;
typedef Vector4<double>  Vec4d;
typedef Matrix4<float>   Mat4f;
typedef Matrix4<double>  Mat4d;
typedef Matrix3<float>   Mat3f;
typedef Matrix3<double>  Mat3d;


//**** Vector3 constructors

template<class T> Vector3<T>::Vector3() : x(0), y(0), z(0)
{
}

template<class T> Vector3<T>::Vector3(const Vector3<T>& v) :
    x(v.x), y(v.y), z(v.z)
{
}

template<class T> Vector3<T>::Vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z)
{
}


//**** Vector3 operators

template<class T> T &Vector3<T>::operator[](int n) const
{
    // Not portable--I'll write a new version when I try to compile on a
    // platform where it bombs.
    return ((T*) this)[n];
}

template<class T> Vector3<T>& Vector3<T>::operator=(T* a)
{
    x = a[0]; y = a[1]; z = a[2];
    return *this;
}

template<class T> Vector3<T>& Vector3<T>::operator+=(const Vector3<T>& a)
{
    x += a.x; y += a.y; z += a.z;
    return *this;
}

template<class T> Vector3<T>& Vector3<T>::operator-=(const Vector3<T>& a)
{
    x -= a.x; y -= a.y; z -= a.z;
    return *this;
}

template<class T> Vector3<T>& Vector3<T>::operator*=(T s)
{
    x *= s; y *= s; z *= s;
    return *this;
}

template<class T> Vector3<T> Vector3<T>::operator-()
{
    return Vector3<T>(-x, -y, -z);
}

template<class T> Vector3<T> Vector3<T>::operator+()
{
    return *this;
}

template<class T> Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<class T> Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<class T> Vector3<T> operator*(T s, const Vector3<T>& v)
{
    return Vector3<T>(s * v.x, s * v.y, s * v.z);
}

template<class T> Vector3<T> operator*(const Vector3<T>& v, T s)
{
    return Vector3<T>(s * v.x, s * v.y, s * v.z);
}

template<class T> Vector3<T> operator*(T s)
{
    return Vector3<T>(s * v.x, s * v.y, s * v.z);
}

template<class T> Vector3<T> Vector3<T>::operator/(T s)
{
    T is = (T)1 / s;
    return Vector3<T>(is * x, is * y, is * z);
}

// dot product
template<class T> T operator*(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<class T> T Vector3<T>::dot(const Vector3<T>& b)
{
    return x * b.x + y * b.y + z * b.z;
}

// cross product
template<class T> Vector3<T> operator^(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.y * b.z - a.z * b.y,
                      a.z * b.x - a.x * b.z,
                      a.x * b.y - a.y * b.x);
}

template<class T> bool operator==(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

template<class T> bool operator!=(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

template<class T> T dot(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<class T> Vector3<T> cross(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.y * b.z - a.z * b.y,
                      a.z * b.x - a.x * b.z,
                      a.x * b.y - a.y * b.x);
}

template<class T> T Vector3<T>::length() const
{
    return (T) sqrt(x * x + y * y + z * z);
}

template<class T> T Vector3<T>::lengthSquared() const
{
    return x * x + y * y + z * z;
}

template<class T> void Vector3<T>::normalize()
{
    T s = 1 / (T) sqrt(x * x + y * y + z * z);
    x *= s;
    y *= s;
    z *= s;
}


//**** Point3 constructors

template<class T> Point3<T>::Point3() : x(0), y(0), z(0)
{
}

template<class T> Point3<T>::Point3(const Point3<T>& p) :
    x(p.x), y(p.y), z(p.z)
{
}

template<class T> Point3<T>::Point3(T _x, T _y, T _z) : x(_x), y(_y), z(_z)
{
}


//**** Point3 operators

template<class T> T& Point3<T>::operator[](int n) const
{
    // Not portable--I'll write a new version when I try to compile on a
    // platform where it bombs.
    return ((T*) this)[n];
}

template<class T> Vector3<T> operator-(const Point3<T>& a, const Point3<T>& b)
{
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<class T> Point3<T>& Point3<T>::operator+=(const Vector3<T>& v)
{
    x += v.x; y += v.y; z += v.z;
    return *this;
}

template<class T> Point3<T>& Point3<T>::operator-=(const Vector3<T>& v)
{
    x -= v.x; y -= v.y; z -= v.z;
    return *this;
}

template<class T> Point3<T>& Point3<T>::operator*=(T s)
{
    x *= s; y *= s; z *= s;
    return *this;
}

template<class T> bool operator==(const Point3<T>& a, const Point3<T>& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

template<class T> bool operator!=(const Point3<T>& a, const Point3<T>& b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

template<class T> Point3<T> operator+(const Point3<T>& p, const Vector3<T>& v)
{
    return Point3<T>(p.x + v.x, p.y + v.y, p.z + v.z);
}

template<class T> Point3<T> operator-(const Point3<T>& p, const Vector3<T>& v)
{
    return Point3<T>(p.x - v.x, p.y - v.y, p.z - v.z);
}

// Naughty naughty . . .  remove these since they aren't proper
// point methods--changing the implicit homogenous coordinate isn't
// allowed.
template<class T> Point3<T> operator*(const Point3<T>& p, T s)
{
    return Point3<T>(p.x * s, p.y * s, p.z * s);
}

template<class T> Point3<T> operator*(T s, const Point3<T>& p)
{
    return Point3<T>(p.x * s, p.y * s, p.z * s);
}


//**** Point3 methods

template<class T> T Point3<T>::distanceTo(const Point3& p) const
{
    return (T) sqrt((p.x - x) * (p.x - x) +
                    (p.y - y) * (p.y - y) +
                    (p.z - z) * (p.z - z));
}

template<class T> T Point3<T>::distanceToSquared(const Point3& p) const
{
    return ((p.x - x) * (p.x - x) +
            (p.y - y) * (p.y - y) +
            (p.z - z) * (p.z - z));
}

template<class T> T Point3<T>::distanceFromOrigin() const
{
    return (T) sqrt(x * x + y * y + z * z);
}

template<class T> T Point3<T>::distanceFromOriginSquared() const
{
    return x * x + y * y + z * z;
}



//**** Vector2 constructors
template<class T> Vector2<T>::Vector2() : x(0), y(0)
{
}

template<class T> Vector2<T>::Vector2(T _x, T _y) : x(_x), y(_y)
{
}


//**** Vector2 operators
template<class T> bool operator==(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x == b.x && a.y == b.y;
}

template<class T> bool operator!=(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x != b.x || a.y != b.y;
}

template<class T> T operator+(const Vector2<T>& b)
{
    return Vector2<T>(x+b.x, y+b.y);
}

template<class T> Vector2<T>& Vector2<T>::operator+=(const Vector2<T>& a)
{
    x += a.x; y += a.y;
    return *this;
}

template<class T> Vector2<T>& Vector2<T>::operator-=(const Vector2<T>& a)
{
    x -= a.x; y -= a.y;
    return *this;
}
//**** Point2 constructors

template<class T> Point2<T>::Point2() : x(0), y(0)
{
}

template<class T> Point2<T>::Point2(T _x, T _y) : x(_x), y(_y)
{
}


//**** Point2 operators

template<class T> bool operator==(const Point2<T>& a, const Point2<T>& b)
{
    return a.x == b.x && a.y == b.y;
}

template<class T> bool operator!=(const Point2<T>& a, const Point2<T>& b)
{
    return a.x != b.x || a.y != b.y;
}


//**** Vector4 constructors

template<class T> Vector4<T>::Vector4() : x(0), y(0), z(0), w(0)
{
}

template<class T> Vector4<T>::Vector4(T _x, T _y, T _z, T _w) :
    x(_x), y(_y), z(_z), w(_w)
{
}


//**** Vector4 operators

template<class T> T& Vector4<T>::operator[](int n) const
{
    // Not portable--I'll write a new version when I try to compile on a
    // platform where it bombs.
    return ((T*) this)[n];
}

template<class T> bool operator==(const Vector4<T>& a, const Vector4<T>& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

template<class T> bool operator!=(const Vector4<T>& a, const Vector4<T>& b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
}

template<class T> Vector4<T>& Vector4<T>::operator+=(const Vector4<T>& a)
{
    x += a.x; y += a.y; z += a.z; w += a.w;
    return *this;
}

template<class T> Vector4<T>& Vector4<T>::operator-=(const Vector4<T>& a)
{
    x -= a.x; y -= a.y; z -= a.z; w -= a.w;
    return *this;
}

template<class T> Vector4<T>& Vector4<T>::operator*=(T s)
{
    x *= s; y *= s; z *= s; w *= s;
    return *this;
}

template<class T> Vector4<T> Vector4<T>::operator-()
{
    return Vector4<T>(-x, -y, -z, -w);
}

template<class T> Vector4<T> Vector4<T>::operator+()
{
    return *this;
}

template<class T> Vector4<T> operator+(const Vector4<T>& a, const Vector4<T>& b)
{
    return Vector4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

template<class T> Vector4<T> operator-(const Vector4<T>& a, const Vector4<T>& b)
{
    return Vector4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

template<class T> Vector4<T> operator*(T s, const Vector4<T>& v)
{
    return Vector4<T>(s * v.x, s * v.y, s * v.z, s * v.w);
}

template<class T> Vector4<T> operator*(const Vector4<T>& v, T s)
{
    return Vector4<T>(s * v.x, s * v.y, s * v.z, s * v.w);
}

// dot product
template<class T> T operator*(const Vector4<T>& a, const Vector4<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template<class T> T dot(const Vector4<T>& a, const Vector4<T>& b)
{
    return a * b;
}



//**** Matrix3 constructors

template<class T> Matrix3<T>::Matrix3()
{
    r[0] = Vector3<T>(0, 0, 0);
    r[1] = Vector3<T>(0, 0, 0);
    r[2] = Vector3<T>(0, 0, 0);
}


template<class T> Matrix3<T>::Matrix3(const Vector3<T>& r0,
                                      const Vector3<T>& r1,
                                      const Vector3<T>& r2)
{
    r[0] = r0;
    r[1] = r1;
    r[2] = r2;
}


#if 0
template<class T, class U> Matrix3<T>::Matrix3(const Matrix3<U>& m)
{
#if 0
    r[0] = m.r[0];
    r[1] = m.r[1];
    r[2] = m.r[2];
#endif
    r[0].x = m.r[0].x; r[0].y = m.r[0].y; r[0].z = m.r[0].z;
    r[1].x = m.r[1].x; r[1].y = m.r[1].y; r[1].z = m.r[1].z;
    r[2].x = m.r[2].x; r[2].y = m.r[2].y; r[2].z = m.r[2].z;
}
#endif


//**** Matrix3 operators

template<class T> const Vector3<T>& Matrix3<T>::operator[](int n) const
{
    //    return const_cast<Vector3<T>&>(r[n]);
    return r[n];
}

template<class T> Vector3<T> Matrix3<T>::row(int n) const
{
    return r[n];
}

template<class T> Vector3<T> Matrix3<T>::column(int n) const
{
    return Vector3<T>(r[0][n], r[1][n], r[2][n]);
}

template<class T> Matrix3<T>& Matrix3<T>::operator*=(T s)
{
    r[0] *= s;
    r[1] *= s;
    r[2] *= s;
    return *this;
}


// pre-multiply column vector by a 3x3 matrix
template<class T> Vector3<T> operator*(const Matrix3<T>& m, const Vector3<T>& v)
{
    return Vector3<T>(m.r[0].x * v.x + m.r[0].y * v.y + m.r[0].z * v.z,
		      m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z,
		      m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z);
}


// post-multiply row vector by a 3x3 matrix
template<class T> Vector3<T> operator*(const Vector3<T>& v, const Matrix3<T>& m)
{
    return Vector3<T>(m.r[0].x * v.x + m.r[1].x * v.y + m.r[2].x * v.z,
                      m.r[0].y * v.x + m.r[1].y * v.y + m.r[2].y * v.z,
                      m.r[0].z * v.x + m.r[1].z * v.y + m.r[2].z * v.z);
}


// pre-multiply column point by a 3x3 matrix
template<class T> Point3<T> operator*(const Matrix3<T>& m, const Point3<T>& p)
{
    return Point3<T>(m.r[0].x * p.x + m.r[0].y * p.y + m.r[0].z * p.z,
                     m.r[1].x * p.x + m.r[1].y * p.y + m.r[1].z * p.z,
                     m.r[2].x * p.x + m.r[2].y * p.y + m.r[2].z * p.z);
}


// post-multiply row point by a 3x3 matrix
template<class T> Point3<T> operator*(const Point3<T>& p, const Matrix3<T>& m)
{
    return Point3<T>(m.r[0].x * p.x + m.r[1].x * p.y + m.r[2].x * p.z,
		     m.r[0].y * p.x + m.r[1].y * p.y + m.r[2].y * p.z,
		     m.r[0].z * p.x + m.r[1].z * p.y + m.r[2].z * p.z);
}


template<class T> Matrix3<T> operator*(const Matrix3<T>& a,
                                       const Matrix3<T>& b)
{
#define MATMUL(R, C) (a[R].x * b[0].C + a[R].y * b[1].C + a[R].z * b[2].C)
    return Matrix3<T>(Vector3<T>(MATMUL(0, x), MATMUL(0, y), MATMUL(0, z)),
                      Vector3<T>(MATMUL(1, x), MATMUL(1, y), MATMUL(1, z)),
                      Vector3<T>(MATMUL(2, x), MATMUL(2, y), MATMUL(2, z)));
#undef MATMUL
}


template<class T> Matrix3<T> operator+(const Matrix3<T>& a,
                                       const Matrix3<T>& b)
{
    return Matrix3<T>(a.r[0] + b.r[0],
                      a.r[1] + b.r[1],
                      a.r[2] + b.r[2]);
}


template<class T> Matrix3<T> Matrix3<T>::identity()
{
    return Matrix3<T>(Vector3<T>(1, 0, 0),
                      Vector3<T>(0, 1, 0),
                      Vector3<T>(0, 0, 1));
}


template<class T> Matrix3<T> Matrix3<T>::transpose() const
{
    return Matrix3<T>(Vector3<T>(r[0].x, r[1].x, r[2].x),
                      Vector3<T>(r[0].y, r[1].y, r[2].y),
                      Vector3<T>(r[0].z, r[1].z, r[2].z));
}


template<class T> T det2x2(T a, T b, T c, T d)
{
    return a * d - b * c;
}

template<class T> T Matrix3<T>::determinant() const
{
    return (r[0].x * r[1].y * r[2].z +
            r[0].y * r[1].z * r[2].x +
            r[0].z * r[1].x * r[2].y -
            r[0].z * r[1].y * r[2].x -
            r[0].x * r[1].z * r[2].y -
            r[0].y * r[1].x * r[2].z);
}


template<class T> Matrix3<T> Matrix3<T>::inverse() const
{
    Matrix3<T> adjoint;

    // Just use Cramer's rule for now . . .
    adjoint.r[0].x =  det2x2(r[1].y, r[1].z, r[2].y, r[2].z);
    adjoint.r[0].y = -det2x2(r[1].x, r[1].z, r[2].x, r[2].z);
    adjoint.r[0].z =  det2x2(r[1].x, r[1].y, r[2].x, r[2].y);
    adjoint.r[1].x = -det2x2(r[0].y, r[0].z, r[2].y, r[2].z);
    adjoint.r[1].y =  det2x2(r[0].x, r[0].z, r[2].x, r[2].z);
    adjoint.r[1].z = -det2x2(r[0].x, r[0].y, r[2].x, r[2].y);
    adjoint.r[2].x =  det2x2(r[0].y, r[0].z, r[1].y, r[1].z);
    adjoint.r[2].y = -det2x2(r[0].x, r[0].z, r[1].x, r[1].z);
    adjoint.r[2].z =  det2x2(r[0].x, r[0].y, r[1].x, r[1].y);
    adjoint *= 1 / determinant();

    return adjoint;
}


template<class T> Matrix3<T> Matrix3<T>::xrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix3<T>(Vector3<T>(1, 0, 0),
                      Vector3<T>(0, c, -s),
                      Vector3<T>(0, s, c));
}


template<class T> Matrix3<T> Matrix3<T>::yrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix3<T>(Vector3<T>(c, 0, s),
                      Vector3<T>(0, 1, 0),
                      Vector3<T>(-s, 0, c));
}


template<class T> Matrix3<T> Matrix3<T>::zrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix3<T>(Vector3<T>(c, -s, 0),
                      Vector3<T>(s, c, 0),
                      Vector3<T>(0, 0, 1));
}


/***********************************************
 **
 **  Matrix4 methods
 **
 ***********************************************/

template<class T> Matrix4<T>::Matrix4()
{
    r[0] = Vector4<T>(0, 0, 0, 0);
    r[1] = Vector4<T>(0, 0, 0, 0);
    r[2] = Vector4<T>(0, 0, 0, 0);
    r[3] = Vector4<T>(0, 0, 0, 0);
}


template<class T> Matrix4<T>::Matrix4(const Vector4<T>& v0,
                                      const Vector4<T>& v1,
                                      const Vector4<T>& v2,
                                      const Vector4<T>& v3)
{
    r[0] = v0;
    r[1] = v1;
    r[2] = v2;
    r[3] = v3;
}


template<class T> Matrix4<T>::Matrix4(const Matrix4<T>& m)
{
    r[0] = m.r[0];
    r[1] = m.r[1];
    r[2] = m.r[2];
    r[3] = m.r[3];
}


template<class T> const Vector4<T>& Matrix4<T>::operator[](int n) const
{
    return r[n];
    //    return const_cast<Vector4<T>&>(r[n]);
}

template<class T> Vector4<T> Matrix4<T>::row(int n) const
{
    return r[n];
}

template<class T> Vector4<T> Matrix4<T>::column(int n) const
{
    return Vector4<T>(r[0][n], r[1][n], r[2][n], r[3][n]);
}


template<class T> Matrix4<T> Matrix4<T>::identity()
{
    return Matrix4<T>(Vector4<T>(1, 0, 0, 0),
                      Vector4<T>(0, 1, 0, 0),
                      Vector4<T>(0, 0, 1, 0),
                      Vector4<T>(0, 0, 0, 1));
}


template<class T> Matrix4<T> Matrix4<T>::translation(const Point3<T>& p)
{
    return Matrix4<T>(Vector4<T>(1, 0, 0, 0),
                      Vector4<T>(0, 1, 0, 0),
                      Vector4<T>(0, 0, 1, 0),
                      Vector4<T>(p.x, p.y, p.z, 1));
}


template<class T> Matrix4<T> Matrix4<T>::translation(const Vector3<T>& v)
{
    return Matrix4<T>(Vector4<T>(1, 0, 0, 0),
                      Vector4<T>(0, 1, 0, 0),
                      Vector4<T>(0, 0, 1, 0),
                      Vector4<T>(v.x, v.y, v.z, 1));
}


template<class T> void Matrix4<T>::translate(const Point3<T>& p)
{
    r[3].x += p.x;
    r[3].y += p.y;
    r[3].z += p.z;
}


template<class T> Matrix4<T> Matrix4<T>::rotation(const Vector3<T>& axis,
                                                  T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);
    T t = 1 - c;

    return Matrix4<T>(Vector4<T>(t * axis.x * axis.x + c,
                                 t * axis.x * axis.y - s * axis.z,
                                 t * axis.x * axis.z + s * axis.y,
                                 0),
                      Vector4<T>(t * axis.x * axis.y + s * axis.z,
                                 t * axis.y * axis.y + c,
                                 t * axis.y * axis.z - s * axis.x,
                                 0),
                      Vector4<T>(t * axis.x * axis.z - s * axis.y,
                                 t * axis.y * axis.z + s * axis.x,
                                 t * axis.z * axis.z + c,
                                 0),
                      Vector4<T>(0, 0, 0, 1));
}


template<class T> Matrix4<T> Matrix4<T>::xrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix4<T>(Vector4<T>(1, 0, 0, 0),
                      Vector4<T>(0, c, -s, 0),
                      Vector4<T>(0, s, c, 0),
                      Vector4<T>(0, 0, 0, 1));
}


template<class T> Matrix4<T> Matrix4<T>::yrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix4<T>(Vector4<T>(c, 0, s, 0),
                      Vector4<T>(0, 1, 0, 0),
                      Vector4<T>(-s, 0, c, 0),
                      Vector4<T>(0, 0, 0, 1));
}


template<class T> Matrix4<T> Matrix4<T>::zrotation(T angle)
{
    T c = (T) cos(angle);
    T s = (T) sin(angle);

    return Matrix4<T>(Vector4<T>(c, -s, 0, 0),
                      Vector4<T>(s, c, 0, 0),
                      Vector4<T>(0, 0, 1, 0),
                      Vector4<T>(0, 0, 0, 1));
}


template<class T> Matrix4<T> Matrix4<T>::scaling(const Vector3<T>& scale)
{
    return Matrix4<T>(Vector4<T>(scale.x, 0, 0, 0),
                      Vector4<T>(0, scale.y, 0, 0),
                      Vector4<T>(0, 0, scale.z, 0),
                      Vector4<T>(0, 0, 0, 1));
}


template<class T> Matrix4<T> Matrix4<T>::scaling(T scale)
{
    return scaling(Vector3<T>(scale, scale, scale));
}


// multiply column vector by a 4x4 matrix
template<class T> Vector3<T> operator*(const Matrix4<T>& m, const Vector3<T>& v)
{
    return Vector3<T>(m.r[0].x * v.x + m.r[0].y * v.y + m.r[0].z * v.z,
		      m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z,
		      m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z);
}

// transform a 3 element column vector by a 4x4 matrix in homogeneous coordinate
template<class T> Vector3<T> transfo4d(const Matrix4<T>& m)
{
    return Vector3<T>(m.r[0].x * x + m.r[0].y * y + m.r[0].z * z + m.r[0].w,
		      m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z + m.r[1].w,
		      m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z + m.r[2].w);
}

// multiply row vector by a 4x4 matrix
template<class T> Vector3<T> operator*(const Vector3<T>& v, const Matrix4<T>& m)
{
    return Vector3<T>(m.r[0].x * v.x + m.r[1].x * v.y + m.r[2].x * v.z,
                      m.r[0].y * v.x + m.r[1].y * v.y + m.r[2].y * v.z,
                      m.r[0].z * v.x + m.r[1].z * v.y + m.r[2].z * v.z);
}

// multiply column point by a 4x4 matrix; no projection is performed
template<class T> Point3<T> operator*(const Matrix4<T>& m, const Point3<T>& p)
{
    return Point3<T>(m.r[0].x * p.x + m.r[0].y * p.y + m.r[0].z * p.z + m.r[0].w,
                     m.r[1].x * p.x + m.r[1].y * p.y + m.r[1].z * p.z + m.r[1].w,
                     m.r[2].x * p.x + m.r[2].y * p.y + m.r[2].z * p.z + m.r[2].w);
}

// multiply row point by a 4x4 matrix; no projection is performed
template<class T> Point3<T> operator*(const Point3<T>& p, const Matrix4<T>& m)
{
    return Point3<T>(m.r[0].x * p.x + m.r[1].x * p.y + m.r[2].x * p.z + m.r[3].x,
		     m.r[0].y * p.x + m.r[1].y * p.y + m.r[2].y * p.z + m.r[3].y,
		     m.r[0].z * p.x + m.r[1].z * p.y + m.r[2].z * p.z + m.r[3].z);
}

// multiply column vector by a 4x4 matrix
template<class T> Vector4<T> operator*(const Matrix4<T>& m, const Vector4<T>& v)
{
    return Vector4<T>(m.r[0].x * v.x + m.r[0].y * v.y + m.r[0].z * v.z + m.r[0].w * v.w,
		      m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z + m.r[1].w * v.w,
		      m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z + m.r[2].w * v.w,
		      m.r[3].x * v.x + m.r[3].y * v.y + m.r[3].z * v.z + m.r[3].w * v.w);
}

// multiply row vector by a 4x4 matrix
template<class T> Vector4<T> operator*(const Vector4<T>& v, const Matrix4<T>& m)
{
    return Vector4<T>(m.r[0].x * v.x + m.r[1].x * v.y + m.r[2].x * v.z + m.r[3].x * v.w,
                      m.r[0].y * v.x + m.r[1].y * v.y + m.r[2].y * v.z + m.r[3].y * v.w,
                      m.r[0].z * v.x + m.r[1].z * v.y + m.r[2].z * v.z + m.r[3].z * v.w,
                      m.r[0].w * v.x + m.r[1].w * v.y + m.r[2].w * v.z + m.r[3].w * v.w);
}



template<class T> Matrix4<T> Matrix4<T>::transpose() const
{
    return Matrix4<T>(Vector4<T>(r[0].x, r[1].x, r[2].x, r[3].x),
                      Vector4<T>(r[0].y, r[1].y, r[2].y, r[3].y),
                      Vector4<T>(r[0].z, r[1].z, r[2].z, r[3].z),
                      Vector4<T>(r[0].w, r[1].w, r[2].w, r[3].w));
}


template<class T> Matrix4<T> operator*(const Matrix4<T>& a,
                                       const Matrix4<T>& b)
{
#define MATMUL(R, C) (a[R].x * b[0].C + a[R].y * b[1].C + a[R].z * b[2].C + a[R].w * b[3].C)
    return Matrix4<T>(Vector4<T>(MATMUL(0, x), MATMUL(0, y), MATMUL(0, z), MATMUL(0, w)),
                      Vector4<T>(MATMUL(1, x), MATMUL(1, y), MATMUL(1, z), MATMUL(1, w)),
                      Vector4<T>(MATMUL(2, x), MATMUL(2, y), MATMUL(2, z), MATMUL(2, w)),
                      Vector4<T>(MATMUL(3, x), MATMUL(3, y), MATMUL(3, z), MATMUL(3, w)));
#undef MATMUL
}


template<class T> Matrix4<T> operator+(const Matrix4<T>& a, const Matrix4<T>& b)
{
    return Matrix4<T>(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]);
}


// Compute inverse using Gauss-Jordan elimination; caller is responsible
// for ensuring that the matrix isn't singular.
template<class T> Matrix4<T> Matrix4<T>::inverse() const
{
    Matrix4<T> a(*this);
    Matrix4<T> b(Matrix4<T>::identity());
    int i, j;
    int p;

    for (j = 0; j < 4; j++)
    {
        p = j;
        for (i = j + 1; i < 4; i++)
        {
            if (fabs(a.r[i][j]) > fabs(a.r[p][j]))
                p = i;
        }

        // Swap rows p and j
        Vector4<T> t = a.r[p];
        a.r[p] = a.r[j];
        a.r[j] = t;

        t = b.r[p];
        b.r[p] = b.r[j];
        b.r[j] = t;

        T s = a.r[j][j];  // if s == 0, the matrix is singular
        a.r[j] *= (1.0f / s);
        b.r[j] *= (1.0f / s);

        // Eliminate off-diagonal elements
        for (i = 0; i < 4; i++)
        {
            if (i != j)
            {
                b.r[i] -= a.r[i][j] * b.r[j];
                a.r[i] -= a.r[i][j] * a.r[j];
            }
        }
    }

    return b;
}


#endif // _VECMATH_H_
