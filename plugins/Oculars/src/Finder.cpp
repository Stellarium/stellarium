/*
 * Copyright (C) 2021 Georg Zotti
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

#include "Finder.hpp"

Finder::Finder()
	: //m_binoculars(false),
	  m_permanentCrosshair(false),
	  m_trueFOV(5.0),
	  m_magnification(5.0),
	  m_aperture(25.0),
	  m_hFlipped(false),
	  m_vFlipped(false),
	  m_equatorial(false)
{
}

Finder::Finder(const QObject& other)
	: //m_binoculars(other.property("binoculars").toBool()),
	  m_permanentCrosshair(other.property("permanentCrosshair").toBool()),
	  m_trueFOV(other.property("trueFOV").toDouble()),
	  m_magnification(other.property("magnification").toDouble()),
	  m_aperture(other.property("aperture").toDouble()),
	  m_hFlipped(other.property("hFlipped").toBool()),
	  m_vFlipped(other.property("vFlipped").toBool()),
	  m_equatorial(other.property("equatorial").toBool()),
	  m_name(other.property("name").toString())
{
}

Finder::~Finder()
{
}

static QMap<int, QString> mapping;
QMap<int, QString> Finder::propertyMap(void)
{
	if(mapping.isEmpty()) {
		mapping = QMap<int, QString>();
		mapping[0] = "name";
		mapping[1] = "trueFOV";
		mapping[2] = "magnification";
		mapping[3] = "aperture";
		mapping[4] = "permanentCrosshair";
		mapping[5] = "hFlipped";
		mapping[6] = "vFlipped";
		mapping[7] = "equatorial";
		mapping[8] = "reticlePath";
	}
	return mapping;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Accessors & Mutators
#endif
/* ********************************************************************* */
QString Finder::name(void) const
{
	return m_name;
}

void Finder::setName(const QString aName)
{
	m_name = aName;
}

double Finder::trueFOV(void) const
{
	return m_trueFOV;
}

void Finder::setTrueFOV(const double fov)
{
	m_trueFOV = fov;
}

double Finder::magnification(void) const
{
	return m_magnification;
}

void Finder::setMagnification(const double mag)
{
	m_magnification = mag;
}

double Finder::aperture(void) const
{
	return m_aperture;
}

void Finder::setAperture(const double ap)
{
	m_aperture = ap;
}

//bool Finder::isBinoculars(void) const
//{
//	return m_binoculars;
//}
//
//void Finder::setBinoculars(const bool flag)
//{
//	m_binoculars = flag;
//}

bool Finder::hasPermanentCrosshair(void) const
{
	return m_permanentCrosshair;
}

void Finder::setPermanentCrosshair(const bool flag)
{
	m_permanentCrosshair = flag;
}

QString Finder::reticlePath(void) const
{
	return m_reticlePath;
}
void Finder::setReticlePath(const QString path)
{
	m_reticlePath = path;
}

bool Finder::isHFlipped() const
{
	return m_hFlipped;
}

void Finder::setHFlipped(bool flipped)
{
	m_hFlipped = flipped;
}

bool Finder::isVFlipped() const
{
	return m_vFlipped;
}

void Finder::setVFlipped(bool flipped)
{
	m_vFlipped = flipped;
}

bool Finder::isEquatorial() const
{
	return m_equatorial;
}

void Finder::setEquatorial(bool eq)
{
	m_equatorial = eq;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

Finder * Finder::finderFromSettings(const QSettings *theSettings, const int finderIndex)
{
	qDebug() << "finderFromSettings()...";
	Finder* finder = new Finder();
	QString prefix = "finder/" + QVariant(finderIndex).toString() + "/";

	finder->setName(theSettings->value(prefix + "name", "nameless").toString());
	finder->setTrueFOV(theSettings->value(prefix + "tfov", 5.0).toDouble());
	finder->setMagnification(theSettings->value(prefix + "mag", 5.0).toDouble());
	finder->setAperture(theSettings->value(prefix + "aperture", 24.0).toDouble());
	//finder->setBinoculars(theSettings->value(prefix + "binoculars", "false").toBool());
	finder->setPermanentCrosshair(theSettings->value(prefix + "permanentCrosshair", "false").toBool());
	finder->setReticlePath(theSettings->value(prefix + "reticlePath", "").toString());
	finder->setHFlipped(theSettings->value(prefix + "hFlip").toBool());
	finder->setVFlipped(theSettings->value(prefix + "vFlip").toBool());
	finder->setEquatorial(theSettings->value(prefix + "equatorial").toBool());

	if (!(finder->trueFOV() > 0.0 && finder->magnification() > 0.0)) {
		qWarning() << "WARNING: Invalid data for finder " << finderIndex << ". Finder values must be positive. \n"
		<< "\ttfov: " << finder->trueFOV() << "\n"
		<< "\tmag: " << finder->magnification() << "x" << finder->aperture() << "\n"
		<< "\tThis finder will be ignored.";
		delete finder;
		finder = Q_NULLPTR;
	}
	else qDebug() << "Adding Finder: " << finder->name() << " (" << finder->magnification() << "x" << finder->aperture() << ") TFOV: " << finder->trueFOV() << "°";
	qDebug() << "finderFromSettings()...returning.";

	return finder;
}

void Finder::writeToSettings(QSettings * settings, const int index)
{
	QString prefix = "ocular/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "tfov", this->trueFOV());
	settings->setValue(prefix + "mag", this->magnification());
	settings->setValue(prefix + "aperture", this->aperture());
	//settings->setValue(prefix + "binoculars", this->isBinoculars());
	settings->setValue(prefix + "permanentCrosshair", this->hasPermanentCrosshair());
	settings->setValue(prefix + "reticlePath", this->reticlePath());
	settings->setValue(prefix + "hFlip", this->isHFlipped());
	settings->setValue(prefix + "vFlip", this->isVFlipped());
	settings->setValue(prefix + "equatorial", this->isEquatorial());

}

Finder * Finder::finderModel(void)
{
	qDebug() << "finderModel()";
	Finder* model = new Finder();
	model->setName("My Finder");
	model->setTrueFOV(5);
	model->setMagnification(5);
	model->setAperture(24);
	//model->setBinoculars(false);
	model->setPermanentCrosshair(false);
	model->setReticlePath("");
	model->setHFlipped(false);
	model->setVFlipped(false);
	model->setEquatorial(false);
	qDebug() << "finderModel() exiting";

	return model;
}
