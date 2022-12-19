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
	: m_diameter(80.)
	, m_focalLength(500.)
	, m_hFlipped(true)
	, m_vFlipped(true)
	, m_equatorial(true)
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

const QMap<int, QString> Telescope::mapping = {
	{0, "name"},
	{1, "diameter"},
	{2, "focalLength"},
	{3, "hFlipped"},
	{4, "vFlipped"},
	{5, "equatorial"}};

QMap<int, QString> Telescope::propertyMap()
{
	return mapping;
}

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

Telescope* Telescope::telescopeFromSettings(QSettings* theSettings, int telescopeIndex)
{
	Telescope* telescope = new Telescope();
	QString prefix = "telescope/" + QVariant(telescopeIndex).toString() + "/";
	
	telescope->setName(theSettings->value(prefix + "name", "").toString());
	telescope->setFocalLength(theSettings->value(prefix + "focalLength", 500.0).toDouble());
	telescope->setDiameter(theSettings->value(prefix + "diameter", 80.0).toDouble());
	telescope->setHFlipped(theSettings->value(prefix + "hFlip", true).toBool());
	telescope->setVFlipped(theSettings->value(prefix + "vFlip", true).toBool());
	telescope->setEquatorial(theSettings->value(prefix + "equatorial", true).toBool());
	return telescope;
}

Telescope* Telescope::telescopeModel()
{
	Telescope* model = new Telescope();
	model->setName("My Telescope");
	return model;
}
