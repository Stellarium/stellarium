/*
 * Copyright (C) 2010 Timothy Reaves
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

#include "CCD.hpp"
#include "Lens.hpp"
#include "Telescope.hpp"

#include <QDebug>
#include <QMetaProperty>
#include <QSettings>
#include <QtMath>

#define RADIAN_TO_DEGREES 57.2957795131

CCD::CCD(QObject * parent)
	: QObject(parent)
{
}

/* ****************************************************************************************************************** */
// MARK: - Instance Methods
/* ****************************************************************************************************************** */
void CCD::initFromSettings(QSettings * settings, int ccdIndex)
{
	QString prefix = "ccd/" + QVariant(ccdIndex).toString() + "/";
	this->setName(settings->value(prefix + "name", "").toString());
	this->setResolutionX(settings->value(prefix + "resolutionX", 0).toInt());
	this->setResolutionY(settings->value(prefix + "resolutionY", 0).toInt());
	this->setChipWidth(settings->value(prefix + "chip_width", 0.0).toDouble());
	this->setChipHeight(settings->value(prefix + "chip_height", 0.0).toDouble());
	this->setRotationAngle(settings->value(prefix + "chip_rot_angle", 0.0).toDouble());
	this->setBinningX(settings->value(prefix + "binningX", 1).toInt());
	this->setBinningY(settings->value(prefix + "binningY", 1).toInt());
	this->setHasOAG(settings->value(prefix + "has_oag", "false").toBool());
	this->setPrismHeight(settings->value(prefix + "prism_height", 0.0).toDouble());
	this->setPrismWidth(settings->value(prefix + "prism_width", 0.0).toDouble());
	this->setPrismDistance(settings->value(prefix + "prism_distance", 0.0).toDouble());
	this->setPrismPosAngle(settings->value(prefix + "prism_pos_angle", 0.0).toDouble());
}

auto CCD::name() const -> QString
{
	return m_name;
}

void CCD::setName(QString name)
{
	m_name = name;
}

auto CCD::resolutionX() const -> int
{
	return m_resolutionX;
}

void CCD::setResolutionX(int resolution)
{
	m_resolutionX = resolution;
}

auto CCD::resolutionY() const -> int
{
	return m_resolutionY;
}

void CCD::setResolutionY(int resolution)
{
	m_resolutionY = resolution;
}

auto CCD::chipWidth() const -> double
{
	return m_chipWidth;
}

void CCD::setChipWidth(double width)
{
	m_chipWidth = width;
}

auto CCD::chipHeight() const -> double
{
	return m_chipHeight;
}

void CCD::setChipHeight(double height)
{
	m_chipHeight = height;
}

void CCD::setRotationAngle(double angle)
{
	m_rotationAngle = angle;
}

auto CCD::rotationAngle() const -> double
{
	return m_rotationAngle;
}

auto CCD::hasOAG() const -> bool
{
	return m_oagEquipped;
}

void CCD::setHasOAG(bool oag)
{
	m_oagEquipped = oag;
}

auto CCD::prismHeight() const -> double
{
	return m_oagPrismHeight;
}

void CCD::setPrismHeight(double height)
{
	m_oagPrismHeight = height;
}

auto CCD::prismWidth() const -> double
{
	return m_oagPrismWidth;
}

void CCD::setPrismWidth(double width)
{
	m_oagPrismWidth = width;
}

auto CCD::prismDistance() const -> double
{
	return m_oagPrismDistance;
}

void CCD::setPrismDistance(double distance)
{
	m_oagPrismDistance = distance;
}

void CCD::setPrismPosAngle(double angle)
{
	m_oagPrismPositionAngle = angle;
}

auto CCD::prismPosAngle() const -> double
{
	return m_oagPrismPositionAngle;
}

auto CCD::binningX() const -> int
{
	return m_binningX;
}

void CCD::setBinningX(int binning)
{
	m_binningX = binning;
}

auto CCD::binningY() const -> int
{
	return m_binningY;
}

void CCD::setBinningY(int binning)
{
	m_binningY = binning;
}

auto CCD::getInnerOAGRadius(Telescope * telescope, Lens * lens) const -> double
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	double       radius =
			RADIAN_TO_DEGREES * 2 * qAtan(this->prismDistance() / (2.0 * telescope->focalLength() * lens_multipler));
	return radius;
}

auto CCD::getOuterOAGRadius(Telescope * telescope, Lens * lens) const -> double
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	double       radius =
			RADIAN_TO_DEGREES * 2 *
			qAtan((this->prismDistance() + this->prismHeight()) / (2.0 * telescope->focalLength() * lens_multipler));
	return radius;
}

auto CCD::getOAGActualFOVx(Telescope * telescope, Lens * lens) const -> double
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	double fovX = RADIAN_TO_DEGREES * 2 * qAtan(this->prismWidth() / (2.0 * telescope->focalLength() * lens_multipler));
	return fovX;
}

auto CCD::getActualFOVx(Telescope * telescope, Lens * lens) const -> double
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	double fovX = RADIAN_TO_DEGREES * 2 * qAtan(this->chipWidth() / (2.0 * telescope->focalLength() * lens_multipler));
	return fovX;
}

auto CCD::getActualFOVy(Telescope * telescope, Lens * lens) const -> double
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	double fovY = RADIAN_TO_DEGREES * 2 * qAtan(this->chipHeight() / (2.0 * telescope->focalLength() * lens_multipler));
	return fovY;
}

auto CCD::getFocuserFOV(Telescope * telescope, Lens * lens, double focuserSize) -> double
{
	// note: focuser size in inches
	const double lens_multipler = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	double fov = RADIAN_TO_DEGREES * 2 * qAtan((focuserSize * 25.4) / (2.0 * telescope->focalLength() * lens_multipler));
	return fov;
}

void CCD::writeToSettings(QSettings * settings, const int index) const
{
	QString prefix = "ccd/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "resolutionX", this->resolutionX());
	settings->setValue(prefix + "resolutionY", this->resolutionY());
	settings->setValue(prefix + "chip_width", this->chipWidth());
	settings->setValue(prefix + "chip_height", this->chipHeight());
	settings->setValue(prefix + "chip_rot_angle", this->rotationAngle());
	settings->setValue(prefix + "binningX", this->binningX());
	settings->setValue(prefix + "binningY", this->binningY());
	settings->setValue(prefix + "has_oag", this->hasOAG());
	settings->setValue(prefix + "prism_height", this->prismHeight());
	settings->setValue(prefix + "prism_width", this->prismWidth());
	settings->setValue(prefix + "prism_distance", this->prismDistance());
	settings->setValue(prefix + "prism_pos_angle", this->prismPosAngle());
}

/* ****************************************************************************************************************** */
// MARK: - Static Methods
/* ****************************************************************************************************************** */
auto CCD::propertyMap() -> QMap<int, QString>
{
	static const auto mapping =
			QMap<int, QString>{ { 0, QLatin1String("name") },          { 1, QLatin1String("chipHeight") },
					    { 2, QLatin1String("chipWidth") },     { 3, QLatin1String("resolutionX") },
					    { 4, QLatin1String("resolutionY") },   { 5, QLatin1String("rotationAngle") },
					    { 6, QLatin1String("binningX") },      { 7, QLatin1String("binningY") },
					    { 8, QLatin1String("hasOAG") },        { 9, QLatin1String("prismHeight") },
					    { 10, QLatin1String("prismWidth") },   { 11, QLatin1String("prismDistance") },
					    { 12, QLatin1String("prismPosAngle") } };
	return mapping;
}

/* ****************************************************************************************************************** */
// MARK: - Operators
/* ****************************************************************************************************************** */
auto operator<<(QDebug debug, const CCD & ccd) -> QDebug
{
	return debug.maybeSpace() << &ccd;
}

auto operator<<(QDebug debug, const CCD * ccd) -> QDebug
{
	QDebugStateSaver    saver(debug);

	const QMetaObject * metaObject = ccd->metaObject();
	debug.nospace() << "CCD(";
	int count = metaObject->propertyCount();
	for (int i = 0; i < count; ++i)
	{
		QMetaProperty metaProperty = metaObject->property(i);
		const char *  name         = metaProperty.name();
		debug.nospace() << name << ":" << ccd->property(name);
	}
	debug.nospace() << ")";
	return debug.maybeSpace();
}
