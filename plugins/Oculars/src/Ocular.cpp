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

Ocular::Ocular(QSettings* settings)
{
	ocularID = settings->value("id", "0").toInt();
	name = settings->value("name", "").toString();
	appearentFOV = settings->value("afov", "0.0").toDouble();
	effectiveFocalLength = settings->value("efl", "0.0").toDouble();
	fieldStop = settings->value("fieldStop", "0.0").toDouble();
	if (!(appearentFOV > 0.0 && effectiveFocalLength > 0.0))
	{
		qWarning() << "WARNING: Invalid data for ocular. Ocular values must be positive. \n"
			<< "\tafov: " << appearentFOV << "\n"
			<< "\tefl: " << effectiveFocalLength << "\n"
			<< "\tThis ocular will be ignored.";
	}
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
double Ocular::getActualFOV(Telescope *telescope)
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
