/*
 * Copyright (C) 2009 Timothy Reaves
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

#include "Telescope.hpp"

#include <QDebug>
#include <QSettings>

Telescope::Telescope()
	: m_diameter(0.)
	, m_focalLength(0.)
	, m_hFlipped(false)
	, m_vFlipped(false)
	, m_equatorial(false)
{
}

Telescope::Telescope(const QObject& other)
	: m_name(other.property("name").toString())
	, m_diameter(other.property("diameter").toDouble())
	, m_focalLength(other.property("focalLength").toDouble())
	, m_hFlipped(other.property("hFlipped").toBool())
	, m_vFlipped(other.property("vFlipped").toBool())
	, m_equatorial(other.property("equatorial").toBool())
{
}

Telescope::~Telescope()
{
}

static QMap<int, QString> mapping;
QMap<int, QString> Telescope::propertyMap()
{
	if(mapping.isEmpty()) {
		mapping = QMap<int, QString>();
		mapping[0] = "name";
		mapping[1] = "diameter";
		mapping[2] = "focalLength";
		mapping[3] = "hFlipped";
		mapping[4] = "vFlipped";
		mapping[5] = "equatorial";
	}
	return mapping;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
const QString Telescope::name() const
{
	return m_name;
}

void Telescope::setName(QString theValue)
{
	m_name = theValue;
}

double Telescope::focalLength() const
{
	return m_focalLength;
}

void Telescope::setFocalLength(double theValue)
{
	m_focalLength = theValue;
}

double Telescope::diameter() const
{
	return m_diameter;
}

void Telescope::setDiameter(double theValue)
{
	m_diameter = theValue;
}

bool Telescope::isHFlipped() const
{
	return m_hFlipped;
}

void Telescope::setHFlipped(bool flipped)
{
	m_hFlipped = flipped;
}

bool Telescope::isVFlipped() const
{
	return m_vFlipped;
}

void Telescope::setVFlipped(bool flipped)
{
	m_vFlipped = flipped;
}

bool Telescope::isEquatorial() const
{
	return m_equatorial;
}

void Telescope::setEquatorial(bool eq)
{
	m_equatorial = eq;
}

void Telescope::writeToSettings(QSettings * settings, const int index)
{
	QString prefix = "telescope/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "focalLength", this->focalLength());
	settings->setValue(prefix + "diameter", this->diameter());
	settings->setValue(prefix + "hFlip", this->isHFlipped());
	settings->setValue(prefix + "vFlip", this->isVFlipped());
	settings->setValue(prefix + "equatorial", this->isEquatorial());
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

Telescope* Telescope::telescopeFromSettings(QSettings* theSettings, int telescopeIndex)
{
	Telescope* telescope = new Telescope();
	QString prefix = "telescope/" + QVariant(telescopeIndex).toString() + "/";
	
	telescope->setName(theSettings->value(prefix + "name", "").toString());
	telescope->setFocalLength(theSettings->value(prefix + "focalLength", "0").toDouble());
	telescope->setDiameter(theSettings->value(prefix + "diameter", "0").toDouble());
	telescope->setHFlipped(theSettings->value(prefix + "hFlip").toBool());
	telescope->setVFlipped(theSettings->value(prefix + "vFlip").toBool());
	telescope->setEquatorial(theSettings->value(prefix + "equatorial").toBool());
	return telescope;
}
Telescope* Telescope::telescopeModel()
{
	Telescope* model = new Telescope();
	model->setName("My Telescope");
	model->setDiameter(80);
	model->setFocalLength(500);
	model->setHFlipped(true);
	model->setVFlipped(true);
	model->setEquatorial(true);
	return model;
}
