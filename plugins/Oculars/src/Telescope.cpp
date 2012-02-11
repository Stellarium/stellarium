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
{
}

Telescope::Telescope(const QObject& other)
{
	this->m_diameter = other.property("diameter").toDouble();
	this->m_focalLength = other.property("focalLength").toDouble();
	this->m_hFlipped = other.property("hFlipped").toBool();
	this->m_vFlipped = other.property("vFlipped").toBool();
	this->m_name = other.property("name").toString();
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
	return model;
}
