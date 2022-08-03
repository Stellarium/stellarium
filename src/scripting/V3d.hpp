/*
 * Stellarium
 * Copyright (C) 2022 Georg Zotti
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

#ifndef V3D_HPP
#define V3D_HPP

#include <QObject>

#include "VecMath.hpp"

//! V3d is a glue class to allow some interaction between Vec3d and the scripting system.
//! Vec3f and Vec3d are not scriptable with the QJSEngine, but an intermediate V3d allows calling slots
//! which require a Vec3f or Vec3d argument by calling like
//! @code
//! StelMovementMgr.setViewDirectionJ2000(new V3d(0., 1., 0.).toVec3d());
//! @endcode
//! This is especually helpful if you need to manipulate the V3d's components first. Else you can also just use
//! @code
//! StelMovementMgr.setViewDirectionJ2000(core.vec3d(0., 1., 0.));
//! @endcode


// We need separate auxiliary classes Vec3f and Color which have constructors without same-numbered overrides!

class V3d: public QObject
{
	Q_OBJECT
public:
	V3d() = default; // TODO: Make sure this (only) initializes m_x=m_y=m_z=0;
	V3d(const V3d &other); // copy constructor
	V3d & operator =(const V3d &v);
	//! The usual constructor to create a 3-dimensional vector that can be manipulated in JavaScript
	Q_INVOKABLE V3d(const double x, const double y, const double z): QObject(), m_x(x), m_y(y), m_z(z){};
	//Q_INVOKABLE V3d(QString &hexColor);
	//! Create a 3-dimensional vector from a Vec3d that can later be manipulated in JavaScript, e.g.
	//! @code
	//! myVec3d=new V3d(StelMovementMgr.getViewDirectionJ2000());
	//! @endcode
	Q_INVOKABLE V3d(const Vec3d &vec): QObject(), m_x(vec[0]), m_y(vec[1]), m_z(vec[2]){};

public slots:
	Q_INVOKABLE static V3d fromVec3d(const Vec3d &vec){return V3d(vec[0], vec[1], vec[2]);}
	Q_INVOKABLE static V3d fromVec3f(const Vec3f &vec){return V3d(double(vec[0]), double(vec[1]), double(vec[2]));}
	//! @return a Vec3d object which is not directly accessible by scripts but can be used as arguments to other calls.
	Q_INVOKABLE Vec3d toVec3d() const {return Vec3d(m_x, m_y, m_z);}
	//! @return a Vec3f object which is not directly accessible by scripts but can be used as arguments to other calls.
	Q_INVOKABLE Vec3f toVec3f() const {return Vec3f(float(m_x), float(m_y), float(m_z));}
	//! @return the R or X component
	Q_INVOKABLE double r() const {return m_x;}
	//! @return the G or Y component
	Q_INVOKABLE double g() const {return m_y;}
	//! @return the B or Z component
	Q_INVOKABLE double b() const {return m_z;}
	//! @return the R or X component
	Q_INVOKABLE double x() const {return m_x;}
	//! @return the G or Y component
	Q_INVOKABLE double y() const {return m_y;}
	//! @return the B or Z component
	Q_INVOKABLE double z() const {return m_z;}
	//! Sets the R or X component
	Q_INVOKABLE void setR(double r) {m_x = r;}
	//! Sets the G or Y component
	Q_INVOKABLE void setG(double g) {m_y = g;}
	//! Sets the B or Z component
	Q_INVOKABLE void setB(double b) {m_z = b;}
	//! Sets the R or X component
	Q_INVOKABLE void setX(double x) {m_x = x;}
	//! Sets the G or Y component
	Q_INVOKABLE void setY(double y) {m_y = y;}
	//! Sets the B or Z component
	Q_INVOKABLE void setZ(double z) {m_z = z;}
	//! Formats a comma-separated string in angle brackets
	Q_INVOKABLE inline QString toString() const {return QString("[%1, %2, %3]").arg(m_x).arg(m_y).arg(m_z);}
	//! Formats a comma-separated string of named rgb components in angle brackets
	Q_INVOKABLE inline QString toRGBString() const {return QString("[r:%1, g:%2, b:%3]").arg(m_x).arg(m_y).arg(m_z);}
	//! Formats a comma-separated string in angle brackets
	Q_INVOKABLE QString toHex() const;
private:
	double m_x, m_y, m_z;
};

//! V3f is a glue class to allow some interaction between Vec3f and the scripting system.
//! Vec3f is not scriptable with the QJSEngine, but this intermediate V3f allows calling slots
//! which require a Vec3f argument in a way similar to V3d.

class V3f: public QObject
{
	Q_OBJECT
public:
	V3f() = default; // TODO: Make sure this (only) initializes m_x=m_y=m_z=0;
	V3f(const V3f &other); // copy constructor
	V3f & operator =(const V3f &v);
	//! The usual constructor to create a 3-dimensional vector that can be manipulated in JavaScript
	Q_INVOKABLE V3f(const float x, const float y, const float z): QObject(), m_x(x), m_y(y), m_z(z){};
	//! Create a 3-dimensional vector from a Vec3f that can later be manipulated in JavaScript, e.g.
	//! @code
	//! myVec3f=new V3f(StelMovementMgr.getViewDirectionJ2000());
	//! @endcode
	Q_INVOKABLE V3f(const Vec3f &vec): QObject(), m_x(vec[0]), m_y(vec[1]), m_z(vec[2]){};

public slots:
	Q_INVOKABLE static V3f fromVec3f(const Vec3f &vec){return V3f(vec[0], vec[1], vec[2]);}
	//! @return a Vec3d object which is not directly accessible by scripts but can be used as arguments to other calls.
	Q_INVOKABLE Vec3d toVec3f() const {return Vec3d(m_x, m_y, m_z);}
	//! @return a Vec3f object which is not directly accessible by scripts but can be used as arguments to other calls.
	Q_INVOKABLE Vec3f toVec3d() const {return Vec3f(double(m_x), double(m_y), double(m_z));}
	//! @return the X component
	Q_INVOKABLE double x() const {return m_x;}
	//! @return the Y component
	Q_INVOKABLE double y() const {return m_y;}
	//! @return the Z component
	Q_INVOKABLE double z() const {return m_z;}
	//! Sets the X component
	Q_INVOKABLE void setX(double x) {m_x = x;}
	//! Sets the Y component
	Q_INVOKABLE void setY(double y) {m_y = y;}
	//! Sets the Z component
	Q_INVOKABLE void setZ(double z) {m_z = z;}
	//! Formats a comma-separated string in angle brackets
	Q_INVOKABLE inline QString toString() const {return QString("[%1, %2, %3]").arg(m_x).arg(m_y).arg(m_z);}
private:
	float m_x, m_y, m_z;
};

//! Similar to the direct use of Vec3f in the older (QtScript-based) solution of Stellarium 0.19-0.22, a class similar to V3f can be used for RGB colors.
//! You can initialize it with a string that encodes HTML color (e.g., "#aaff33")
//!
//! @todo This class would probably not be required if QVector3D or self-made classes that don't derive from QObject could be made scriptable
//! like in the old QtScript module. See https://forum.qt.io/topic/24368/custom-c-types-and-qjsengine for a very early discussion.

class Color: public QObject
{
	Q_OBJECT
public:
	Color() = default; // TODO: Make sure this (only) initializes m_x=m_y=m_z=0;
	Color(const Color &other); // copy constructor
	Color & operator =(const Color &v);
	//! The usual constructor to create a 3-dimensional vector that can be manipulated in JavaScript
	Q_INVOKABLE Color(const double x, const double y, const double z): QObject(), m_r(x), m_g(y), m_b(z){};
	Q_INVOKABLE Color(QString &hexColor);
	// Does not work - ambiguous!
	//Q_INVOKABLE Color(const Vec3f &vec): QObject(), m_r(vec[0]), m_g(vec[1]), m_b(vec[2]){};

public slots:
	Q_INVOKABLE static Color fromVec3d(const Vec3d &vec){return Color(vec[0], vec[1], vec[2]);}
	Q_INVOKABLE static Color fromVec3f(const Vec3f &vec){return Color(double(vec[0]), double(vec[1]), double(vec[2]));}
	//! @return a Vec3d object which is not directly accessible by scripts but can be used as arguments to other calls.
	Q_INVOKABLE Vec3d toVec3d() const {return Vec3d(m_r, m_g, m_b);}
	//! @return a Vec3f object which is not directly accessible by scripts but can be used as arguments to other calls.
	Q_INVOKABLE Vec3f toVec3f() const {return Vec3f(float(m_r), float(m_g), float(m_b));}
	//! @return the R component
	Q_INVOKABLE double r() const {return m_r;}
	//! @return the G component
	Q_INVOKABLE double g() const {return m_g;}
	//! @return the B component
	Q_INVOKABLE double b() const {return m_b;}
	//! Sets the R component
	Q_INVOKABLE void setR(double r) {m_r = r;}
	//! Sets the G component
	Q_INVOKABLE void setG(double g) {m_g = g;}
	//! Sets the B component
	Q_INVOKABLE void setB(double b) {m_b = b;}
	//! Formats a comma-separated string of named rgb components in angle brackets
	Q_INVOKABLE inline QString toString() const {return QString("[r:%1, g:%2, b:%3]").arg(m_r).arg(m_g).arg(m_b);}
	//! Formats a comma-separated string in angle brackets
	Q_INVOKABLE QString toHex() const;
private:
	double m_r, m_g, m_b;
};
Q_DECLARE_METATYPE(V3d)
Q_DECLARE_METATYPE(V3f)
Q_DECLARE_METATYPE(Color)

#endif // V3D_HPP
