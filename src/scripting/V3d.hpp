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

#ifdef ENABLE_SCRIPT_QML

//! V3d is a (currently still quite kludgy) glue class to allow some interaction between Vec3d and the scripting system. Vec3d is not scriptable,
//! but an intermediate V3d allows calling slots which require a Vec3d argument by calling like
//! @code
//! StelMovementMgr.setViewDirectionJ2000(new V3d(0., 1., 0.).toVec3d());
//! @endcode
//! Similar to the direct use of Vec3d in the older (QtScript-based) solution of Stellarium 0.19-0.22, V3d also has an aspect of RGB colors:
//! You can initialize it with a string that encodes HTML color (e.g., "#aaff33")
//!
//! @todo This class would probably not be required if QVector3D or self-made classes that don't derive from QObject could be made scriptable
//! like in the old QtScript module. See https://forum.qt.io/topic/24368/custom-c-types-and-qjsengine for a very early discussion.

class V3d: public QObject
{
	Q_OBJECT
public:
	V3d() = default; // TODO: Make sure this (only) initializes m_x=m_y=m_z=0;
	V3d(const V3d &other); // copy constructor
	V3d & operator =(const V3d &v);
	//! The usual constructor to create a 3-dimensional vector that can be manipulated in JavaScript
	Q_INVOKABLE V3d(const double x, const double y, const double z): m_x(x), m_y(y), m_z(z){};
	Q_INVOKABLE V3d(QString &hexColor);
	//! Create a 3-dimensional vector from a Vec3d that can later be manipulated in JavaScript, e.g.
	//! @code
	//! myVec3d=new V3d(StelMovementMgr.getViewDirectionJ2000());
	//! @endcode
	Q_INVOKABLE V3d(const Vec3d &vec);
	//! Create a 3-dimensional vector from a Vec3f that can later be manipulated in JavaScript, e.g.
	Q_INVOKABLE V3d(const Vec3f &vec);
	//! Create a 3-dimensional vector from a Vec3i that can later be manipulated in JavaScript, e.g.
	Q_INVOKABLE V3d(const Vec3i &vec);

public slots:
	static V3d fromVec3d(const Vec3d &vec){return V3d(vec[0], vec[1], vec[2]);}
	Vec3d toVec3d() const {return Vec3d(m_x, m_y, m_z);}
	double r() const {return m_x;}
	double g() const {return m_y;}
	double b() const {return m_z;}
	double x() const {return m_x;}
	double y() const {return m_y;}
	double z() const {return m_z;}
	void setR(double r) {m_x = r;}
	void setG(double g) {m_y = g;}
	void setB(double b) {m_z = b;}
	void setX(double x) {m_x = x;}
	void setY(double y) {m_y = y;}
	void setZ(double z) {m_z = z;}
	inline QString toString() const {return QString("[%1, %2, %3]").arg(m_x).arg(m_y).arg(m_z);}
private:
	double m_x, m_y, m_z;
};
Q_DECLARE_METATYPE(V3d)
#endif

#endif // V3D_HPP

