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

#include <QDebug>
#include <QVariant>

#include <cmath>

V3d::V3d(const V3d &other) : QObject() // copy constructor
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

QString V3d::toHex() const
{
	return QString("#%1%2%3")
		.arg(qMin(255, int(m_x * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(m_y * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(m_z * 255)), 2, 16, QChar('0'));
}

/////////////////////
V3f::V3f(const V3f &other) : QObject() // copy constructor
{
	m_x=other.x();
	m_y=other.y();
	m_z=other.z();
}

V3f & V3f::operator =(const V3f &v)
{
	m_x=v.x();
	m_y=v.y();
	m_z=v.z();
	return *this;
}

///////////////////////
#ifdef ENABLE_SCRIPT_QML

Color::Color(const Color &other) : QObject() // copy constructor
{
	m_r=other.r();
	m_g=other.g();
	m_b=other.b();
}

Color & Color::operator =(const Color &v)
{
	m_r=v.r();
	m_g=v.g();
	m_b=v.b();
	return *this;
}

Color::Color(QString hexColor) : QObject()
{
	QColor qcol = QColor( hexColor );
	if( qcol.isValid() )
	{
		m_r = qcol.redF();
		m_g = qcol.greenF();
		m_b = qcol.blueF();
	}
	else
	{
		qWarning() << "Bad color string: " << hexColor;
		m_r=m_g=m_b=0.;
	}
}

QString Color::toHex() const
{
	return QString("#%1%2%3")
		.arg(qMin(255, int(m_r * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(m_g * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(m_b * 255)), 2, 16, QChar('0'));
}
#endif
