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

#include "Ocular.hpp"
#include "Telescope.hpp"

#include <QDebug>
#include <QSettings>

Ocular::Ocular(QSqlRecord record)
{
	ocularID = record.value("id").toInt();
	name = record.value("name").toString();
	appearentFOV = record.value("afov").toDouble();
	effectiveFocalLength = record.value("efl").toDouble();
	fieldStop = record.value("fieldStop").toDouble();
}

Ocular::~Ocular()
{
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
double Ocular::getAcutalFOV(Telescope *telescope)
{
	double actualFOV = 0.0;
	if (fieldStop > 0.0) {
		actualFOV =  fieldStop / telescope->getFocalLength() * 57.3;
	} else {
		//actualFOV = apparent / mag
		actualFOV = appearentFOV / (telescope->getFocalLength() / effectiveFocalLength);
	}
	return actualFOV;
}

double Ocular::getMagnification(Telescope *telescope)
{
	return telescope->getFocalLength() / effectiveFocalLength;
}

double Ocular::getExitCircle(Telescope *telescope)
{
	return telescope->getDiameter() * effectiveFocalLength / telescope->getFocalLength();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
const QString Ocular::getName()
{
	return name;
}

int Ocular::getOcularID()
{
	return ocularID;
}

double Ocular::getAppearentFOV()
{
	return appearentFOV;
}

double Ocular::getEffectiveFocalLength()
{
	return effectiveFocalLength;
}

double Ocular::getFieldStop()
{
	return fieldStop;
}
