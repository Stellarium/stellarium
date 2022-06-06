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

#include "V3d.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QVariant>

#include <cmath>


#ifdef ENABLE_SCRIPT_QML
#include <QJSEngine>

V3d::V3d(const V3d &other) // copy constructor
{
	m_x=other.x();
	m_y=other.y();
	m_z=other.z();
}

V3d & V3d::operator =(const V3d &v)
{
	m_x=v.x();
	m_y=v.y();
	m_z=v.z();
	return *this;
}

V3d::V3d(QString &hexColor)
{
	QColor qcol = QColor( hexColor );
	if( qcol.isValid() )
	{
		m_x = qcol.redF();
		m_y = qcol.greenF();
		m_z = qcol.blueF();
	}
	else
	{
		qWarning() << "Bad color string: " << hexColor;
		m_x=m_y=m_z=0.;
	}
}

V3d::V3d(const Vec3d &vec) : m_x(vec[0]), m_y(vec[1]), m_z(vec[2]){}
V3d::V3d(const Vec3f &vec) : m_x(double(vec[0])), m_y(double(vec[1])), m_z(double(vec[2])){}
V3d::V3d(const Vec3i &vec) : m_x(double(vec[0])), m_y(double(vec[1])), m_z(double(vec[2])){}

#endif
