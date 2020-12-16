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

#include "Ocular.hpp"
#include "Telescope.hpp"
#include "Lens.hpp"

Ocular::Ocular()
	: m_binoculars(false),
	  m_permanentCrosshair(false),
	  m_apparentFOV(0.0),
	  m_effectiveFocalLength(0.0),
	  m_fieldStop(0.0)
{
}

Ocular::Ocular(const QObject& other)
	: m_binoculars(other.property("binoculars").toBool()),
	  m_permanentCrosshair(other.property("permanentCrosshair").toBool()),
	  m_apparentFOV(other.property("apparentFOV").toDouble()),
	  m_effectiveFocalLength(other.property("effectiveFocalLength").toDouble()),
	  m_fieldStop(other.property("fieldStop").toDouble()),
	  m_name(other.property("name").toString()),
	  m_reticlePath(other.property("reticlePath").toString())
{
}

Ocular::~Ocular()
{
}

static QMap<int, QString> mapping;
QMap<int, QString> Ocular::propertyMap(void)
{
	if(mapping.isEmpty()) {
		mapping = QMap<int, QString>();
		mapping[0] = "name";
		mapping[1] = "apparentFOV";
		mapping[2] = "effectiveFocalLength";
		mapping[3] = "fieldStop";
		mapping[4] = "binoculars";
		mapping[5] = "permanentCrosshair";
		mapping[6] = "reticlePath";
	}
	return mapping;
}


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
double Ocular::actualFOV(const Telescope * telescope, const Lens * lens) const
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double actualFOV = 0.0;
	if (m_binoculars) {
		actualFOV = apparentFOV();
	} else if (fieldStop() > 0.0) {
		actualFOV =  fieldStop() / (telescope->focalLength() * lens_multipler) * 57.3;
	} else {
		//actualFOV = apparent / mag
		actualFOV = apparentFOV() / (telescope->focalLength() * lens_multipler / effectiveFocalLength());
	}
	return actualFOV;
}

double Ocular::magnification(const Telescope * telescope, const Lens * lens) const
{
	double magnifiction = 0.0;
	if (m_binoculars) {
		magnifiction = effectiveFocalLength();
	} else {
		const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
		magnifiction = telescope->focalLength() * lens_multipler / effectiveFocalLength();
	}
	return magnifiction;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
QString Ocular::name(void) const
{
	return m_name;
}

void Ocular::setName(const QString aName)
{
	m_name = aName;
}

double Ocular::apparentFOV(void) const
{
	return m_apparentFOV;
}

void Ocular::setApparentFOV(const double fov)
{
	m_apparentFOV = fov;
}

double Ocular::effectiveFocalLength(void) const
{
	return m_effectiveFocalLength;
}

void Ocular::setEffectiveFocalLength(const double fl)
{
	m_effectiveFocalLength = fl;
}

double Ocular::fieldStop(void) const
{
	return m_fieldStop;
}

void Ocular::setFieldStop(const double fs)
{
	m_fieldStop = fs;
}

bool Ocular::isBinoculars(void) const
{
	return m_binoculars;
}

void Ocular::setBinoculars(const bool flag)
{
	m_binoculars = flag;
}

bool Ocular::hasPermanentCrosshair(void) const
{
	return m_permanentCrosshair;
}

void Ocular::setPermanentCrosshair(const bool flag)
{
	m_permanentCrosshair = flag;
}

QString Ocular::reticlePath(void) const
{
	return m_reticlePath;
}
void Ocular::setReticlePath(const QString path)
{
	m_reticlePath = path;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

Ocular * Ocular::ocularFromSettings(const QSettings *theSettings, const int ocularIndex)
{
	Ocular* ocular = new Ocular();
	QString prefix = "ocular/" + QVariant(ocularIndex).toString() + "/";

	ocular->setName(theSettings->value(prefix + "name", "").toString());
	ocular->setApparentFOV(theSettings->value(prefix + "afov", 0.0).toDouble());
	ocular->setEffectiveFocalLength(theSettings->value(prefix + "efl", 0.0).toDouble());
	ocular->setFieldStop(theSettings->value(prefix + "fieldStop", 0.0).toDouble());
	ocular->setBinoculars(theSettings->value(prefix + "binoculars", "false").toBool());
	ocular->setPermanentCrosshair(theSettings->value(prefix + "permanentCrosshair", "false").toBool());
	ocular->setReticlePath(theSettings->value(prefix + "reticlePath", "").toString());

	if (!(ocular->apparentFOV() > 0.0 && ocular->effectiveFocalLength() > 0.0)) {
		qWarning() << "WARNING: Invalid data for ocular. Ocular values must be positive. \n"
		<< "\tafov: " << ocular->apparentFOV() << "\n"
		<< "\tefl: " << ocular->effectiveFocalLength() << "\n"
		<< "\tThis ocular will be ignored.";
		delete ocular;
		ocular = Q_NULLPTR;
	}
	
	return ocular;
}

void Ocular::writeToSettings(QSettings * settings, const int index)
{
	QString prefix = "ocular/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "afov", this->apparentFOV());
	settings->setValue(prefix + "efl", this->effectiveFocalLength());
	settings->setValue(prefix + "fieldStop", this->fieldStop());
	settings->setValue(prefix + "binoculars", this->isBinoculars());
	settings->setValue(prefix + "permanentCrosshair", this->hasPermanentCrosshair());
	settings->setValue(prefix + "reticlePath", this->reticlePath());
}

Ocular * Ocular::ocularModel(void)
{
	Ocular* model = new Ocular();
	model->setName("My Ocular");
	model->setApparentFOV(68);
	model->setEffectiveFocalLength(32);
	model->setFieldStop(0);
	model->setBinoculars(false);
	model->setPermanentCrosshair(false);
	model->setReticlePath("");
	return model;
}
