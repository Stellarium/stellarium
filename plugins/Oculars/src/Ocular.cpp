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

Ocular::Ocular()
{
}

Ocular::Ocular(const Ocular* other)
{
	QT_ASSERT(other);
	this->m_appearentFOV = other->appearentFOV();
	this->m_effectiveFocalLength = other->effectiveFocalLength();
	this->m_fieldStop = other->fieldStop();
	this->m_name = other->name();
}

Ocular::~Ocular()
{
}

static Ocular* ocularFromSettings(QSettings* theSettings, QString theGroupName)
{
	Ocular* ocular = new Ocular();
	theSettings->beginGroup(theGroupName);
	
	ocular->setName(theSettings->value("name", "").toString());
	ocular->setAppearentFOV(theSettings->value("afov", "0.0").toDouble());
	ocular->setEffectiveFocalLength(theSettings->value("efl", "0.0").toDouble());
	ocular->setFieldStop(theSettings->value("fieldStop", "0.0").toDouble());
	
	theSettings->endGroup();
	if (!(ocular->appearentFOV() > 0.0 && ocular->effectiveFocalLength() > 0.0)) {
		qWarning() << "WARNING: Invalid data for ocular. Ocular values must be positive. \n"
			<< "\tafov: " << ocular->appearentFOV() << "\n"
			<< "\tefl: " << ocular->effectiveFocalLength() << "\n"
			<< "\tThis ocular will be ignored.";
		delete ocular;
		ocular = NULL;
	}
	
	return ocular;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
double Ocular::actualFOV(Telescope *telescope) const
{
	double actualFOV = 0.0;
	if (fieldStop() > 0.0) {
		actualFOV =  fieldStop() / telescope->focalLength() * 57.3;
	} else {
		//actualFOV = apparent / mag
		actualFOV = appearentFOV() / (telescope->focalLength() / effectiveFocalLength());
	}
	return actualFOV;
}

double Ocular::magnification(Telescope *telescope) const
{
	return telescope->focalLength() / effectiveFocalLength();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
const QString Ocular::name() const
{
	return m_name;
}

void Ocular::setName(QString aName)
{
	m_name = aName;
}

double Ocular::appearentFOV() const
{
	return m_appearentFOV;
}

void Ocular::setAppearentFOV(double fov)
{
	m_appearentFOV = fov;
}

double Ocular::effectiveFocalLength() const
{
	return m_effectiveFocalLength;
}

void Ocular::setEffectiveFocalLength(double fl)
{
	m_effectiveFocalLength = fl;
}

double Ocular::fieldStop() const
{
	return m_fieldStop;
}

void Ocular::setFieldStop(double fs)
{
	m_fieldStop = fs;
}
