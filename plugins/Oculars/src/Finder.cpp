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
	: //m_binoculars(true),
	  m_magnification(5.0),
	  m_aperture(24.0),
	  m_trueFOV(8.0),
	  m_hFlipped(false),
	  m_vFlipped(false),
	  m_equatorial(false),
	m_permanentCrosshair(false)
{
}

Finder::Finder(const QObject& other)
	: //m_binoculars(other.property("binoculars").toBool()),
	  m_name(other.property("name").toString()),
	  m_magnification(other.property("magnification").toDouble()),
	  m_aperture(other.property("aperture").toDouble()),
	  m_trueFOV(other.property("trueFOV").toDouble()),
	  m_hFlipped(other.property("hFlipped").toBool()),
	  m_vFlipped(other.property("vFlipped").toBool()),
	  m_equatorial(other.property("equatorial").toBool()),
	  m_permanentCrosshair(other.property("permanentCrosshair").toBool()),
	  m_reticlePath(other.property("reticlePath").toString())
{
}

Finder::~Finder()
{
}

static const QMap<int, QString> mapping = {
	{0, "name"		 },
	{1, "trueFOV"		 },
	{2, "magnification"	 },
	{3, "aperture"		 },
	{4, "hFlipped"		 },
	{5, "vFlipped"		 },
	{6, "equatorial"	 },
	{7, "permanentCrosshair" },
	{8, "reticlePath"	 }};

QMap<int, QString> Finder::propertyMap(void)
{
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

Finder * Finder::finderFromSettings(const QSettings *settings, const int finderIndex)
{
	qDebug() << "finderFromSettings()...";
	Finder* finder = new Finder();
	QString prefix = "finder/" + QVariant(finderIndex).toString() + "/";

	finder->setName(settings->value(prefix + "name", "nameless").toString());
	finder->setTrueFOV(settings->value(prefix + "tfov", 8.0).toDouble());
	finder->setMagnification(settings->value(prefix + "mag", 5.0).toDouble());
	finder->setAperture(settings->value(prefix + "aperture", 24.0).toDouble());
	//finder->setBinoculars(theSettings->value(prefix + "binoculars", false).toBool());
	finder->setHFlipped(settings->value(prefix + "hFlip", false).toBool());
	finder->setVFlipped(settings->value(prefix + "vFlip", false).toBool());
	finder->setEquatorial(settings->value(prefix + "equatorial", false).toBool());
	finder->setPermanentCrosshair(settings->value(prefix + "permanentCrosshair", false).toBool());
	finder->setReticlePath(settings->value(prefix + "reticlePath", "").toString());

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
	QString prefix = "finder/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "tfov", this->trueFOV());
	settings->setValue(prefix + "mag", this->magnification());
	settings->setValue(prefix + "aperture", this->aperture());
	//settings->setValue(prefix + "binoculars", this->isBinoculars());
	settings->setValue(prefix + "hFlip", this->isHFlipped());
	settings->setValue(prefix + "vFlip", this->isVFlipped());
	settings->setValue(prefix + "equatorial", this->isEquatorial());
	settings->setValue(prefix + "permanentCrosshair", this->hasPermanentCrosshair());
	settings->setValue(prefix + "reticlePath", this->reticlePath());
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
	model->setHFlipped(false);
	model->setVFlipped(false);
	model->setEquatorial(false);
	model->setPermanentCrosshair(false);
	model->setReticlePath("");
	qDebug() << "finderModel() exiting";

	return model;
}
