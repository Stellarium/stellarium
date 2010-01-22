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

Telescope::Telescope(QSqlRecord record)
{
	telescopeID = record.value("id").toInt();
	name = record.value("name").toString();
	focalLength = record.value("focalLength").toDouble();
	diameter = record.value("diameter").toDouble();
	hFlipped = record.value("hFlip").toBool();
	vFlipped = record.value("vFlip").toBool();
}

Telescope::~Telescope()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
const QString Telescope::getName()
{
	return name;
}

double Telescope::getFocalLength()
{
	return focalLength;
}

int Telescope::getTelescopeID()
{
	return telescopeID;
}

double Telescope::getDiameter()
{
	return diameter;
}

bool Telescope::isHFlipped()
{
	return hFlipped;
}

bool Telescope::isVFlipped()
{
	return vFlipped;
}
