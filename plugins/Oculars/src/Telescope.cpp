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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Telescope.hpp"

#include <QDebug>
#include <QSettings>

Telescope::Telescope()
{
}

Telescope::~Telescope()
{
}

static Telescope* telescopeFromSettings(QSettings* theSettings, QString theGroupName)
{
	Telescope* telescope = new Telescope();
	theSettings->beginGroup(theGroupName);

	telescope->setName(theSettings->value("name", "").toString());
	telescope->setFocalLength(theSettings->value("focalLength", "0").toDouble());
	telescope->setDiameter(theSettings->value("diameter", "0").toDouble());
	telescope->setHFlipped(theSettings->value("hFlip").toBool());
	telescope->setVFlipped(theSettings->value("vFlip").toBool());
	
	theSettings->endGroup();
	return telescope;
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

