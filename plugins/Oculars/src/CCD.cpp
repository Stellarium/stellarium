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
#include "Telescope.hpp"
#include "Lens.hpp"

#include <QDebug>
#include <QSettings>
#include <QtMath>

#include <cmath>

#define RADIAN_TO_DEGREES 57.2957795131

CCD::CCD()
	: m_resolutionX(4096)
	, m_resolutionY(4096)
	, m_chipWidth(36.8)
	, m_chipHeight(36.8)
	, m_chipRotAngle(0.)
	, m_binningX(1)
	, m_binningY(1)
	, m_has_oag(false)
	, m_oag_prismHeight(0.)
	, m_oag_prismWidth(0.)
	, m_oag_prismDistance(0.)
	, m_oag_prismPosAngle(0.)
{
}

CCD::CCD(const QObject& other)
	: m_name(other.property("name").toString())
	, m_resolutionX(other.property("resolutionX").toInt())
	, m_resolutionY(other.property("resolutionY").toInt())
	, m_chipWidth(other.property("chipWidth").toDouble())
	, m_chipHeight(other.property("chipHeight").toDouble())
	, m_chipRotAngle(other.property("chipRotAngle").toDouble())
	, m_binningX(other.property("binningX").toInt())
	, m_binningY(other.property("binningY").toInt())
	, m_has_oag(other.property("hasOAG").toBool())
	, m_oag_prismHeight(other.property("prismHeight").toDouble())
	, m_oag_prismWidth(other.property("prismWidth").toDouble())
	, m_oag_prismDistance(other.property("prismDistance").toDouble())
	, m_oag_prismPosAngle(other.property("prismPosAngle").toDouble())
{
}

CCD::~CCD()
{
}

const QMap<int, QString> CCD::mapping = {
	{0 , "name"},
	{1 , "chipHeight"},
	{2 , "chipWidth"},
	{3 , "resolutionX"},
	{4 , "resolutionY"},
	{5 , "chipRotAngle"},
	{6 , "binningX"},
	{7 , "binningY"},
	{8 , "hasOAG"},
	{9 , "prismHeight"},
	{10, "prismWidth"},
	{11, "prismDistance"},
	{12, "prismPosAngle"}};

QMap<int, QString> CCD::propertyMap()
{
	return mapping;
}

QString CCD::name() const
{
	return m_name;
}

void CCD::setName(QString name)
{
	m_name = name;
}

int CCD::resolutionX()  const 
{
	return m_resolutionX;
}

void CCD::setResolutionX(int resolution)
{
	m_resolutionX = resolution;
}

int CCD::resolutionY()  const 
{
	return m_resolutionY;
}

void CCD::setResolutionY(int resolution)
{
	m_resolutionY = resolution;
}

double CCD::chipWidth()  const
{
	return m_chipWidth;
}

void CCD::setChipWidth(double width)
{
	m_chipWidth = width;
}

double CCD::chipHeight()  const
{
	return m_chipHeight;
}

void CCD::setChipHeight(double height)
{
	m_chipHeight = height;
}

void CCD::setChipRotAngle(double angle)
{
	m_chipRotAngle = angle;
}

double CCD::chipRotAngle() const
{
	return m_chipRotAngle;
}

bool CCD::hasOAG() const
{
	return m_has_oag;
}

void CCD::setHasOAG(bool oag)
{
	m_has_oag = oag;
}

double CCD::prismHeight() const
{
	return m_oag_prismHeight;
}

void CCD::setPrismHeight(double height)
{
	m_oag_prismHeight = height;
}

double CCD::prismWidth() const
{
	return m_oag_prismWidth;
}

void CCD::setPrismWidth(double width)
{
	m_oag_prismWidth = width;
}

double CCD::prismDistance() const
{
	return m_oag_prismDistance;
}

void CCD::setPrismDistance(double distance)
{
	m_oag_prismDistance = distance;
}

void CCD::setPrismPosAngle(double angle)
{
	m_oag_prismPosAngle = angle;
}

double CCD::prismPosAngle() const
{
	return m_oag_prismPosAngle;
}

int CCD::binningX() const
{
	return m_binningX;
}

void CCD::setBinningX(int binning)
{
	m_binningX = binning;
}

int CCD::binningY() const
{
	return m_binningY;
}

void CCD::setBinningY(int binning)
{
	m_binningY = binning;
}

double CCD::getInnerOAGRadius(const Telescope *telescope, const Lens *lens) const
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double radius = RADIAN_TO_DEGREES * qAtan((prismDistance() - prismHeight() / 2) / (telescope->focalLength() * lens_multipler));
	return radius;
}

double CCD::getOuterOAGRadius(const Telescope *telescope, const Lens *lens) const
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double radius = RADIAN_TO_DEGREES * qAtan((prismDistance() + prismHeight() / 2) / (telescope->focalLength() * lens_multipler));
	return radius;
}

double CCD::getOAGActualFOVx(const Telescope *telescope, const Lens *lens) const
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double fovX = RADIAN_TO_DEGREES * 2 * qAtan(this->prismWidth() /(2.0 * telescope->focalLength() * lens_multipler));
	return fovX;
}

double CCD::getActualFOVx(const Telescope *telescope, const Lens *lens) const
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double fovX = RADIAN_TO_DEGREES * 2 * qAtan(this->chipWidth() /(2.0 * telescope->focalLength() * lens_multipler));
	return fovX;
}

double CCD::getActualFOVy(const Telescope *telescope, const Lens *lens) const
{
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double fovY = RADIAN_TO_DEGREES * 2 * qAtan(this->chipHeight() /(2.0 * telescope->focalLength() * lens_multipler));
	return fovY;
}

double CCD::getCentralAngularResolutionX(const Telescope *telescope, const Lens *lens) const
{
	const auto lensMultipler = lens ? lens->getMultipler() : 1.0;
	const auto pixelSize = chipWidth() * binningX() / resolutionX();
	const auto focalLen = telescope->focalLength() * lensMultipler;
	return RADIAN_TO_DEGREES * 2 * std::atan(pixelSize / (2 * focalLen));
}

double CCD::getCentralAngularResolutionY(const Telescope *telescope, const Lens *lens) const
{
	const auto lensMultipler = lens ? lens->getMultipler() : 1.0;
	const auto pixelSize = chipHeight() * binningY() / resolutionY();
	const auto focalLength = telescope->focalLength() * lensMultipler;
	return RADIAN_TO_DEGREES * 2 * std::atan(pixelSize / (2 * focalLength));
}

double CCD::getFocuserFOV(const Telescope *telescope, const Lens *lens, double focuserSize) const
{
	// note: focuser size in inches
	const double lens_multipler = (lens != Q_NULLPTR ? lens->getMultipler() : 1.0);
	double fov = RADIAN_TO_DEGREES * 2 * qAtan((focuserSize*25.4) /(2.0 * telescope->focalLength() * lens_multipler));
	return fov;
}

void CCD::writeToSettings(QSettings * settings, const int index)
{
	QString prefix = "ccd/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "resolutionX", this->resolutionX());
	settings->setValue(prefix + "resolutionY", this->resolutionY());
	settings->setValue(prefix + "chip_width", this->chipWidth());
	settings->setValue(prefix + "chip_height", this->chipHeight());
	settings->setValue(prefix + "chip_rot_angle", this->chipRotAngle());
	settings->setValue(prefix + "binningX", this->binningX());
	settings->setValue(prefix + "binningY", this->binningY());
	settings->setValue(prefix + "has_oag", this->hasOAG());
	settings->setValue(prefix + "prism_height", this->prismHeight());
	settings->setValue(prefix + "prism_width", this->prismWidth());
	settings->setValue(prefix + "prism_distance", this->prismDistance());
	settings->setValue(prefix + "prism_pos_angle", this->prismPosAngle());
}

CCD* CCD::ccdFromSettings(QSettings* settings, int ccdIndex)
{
	CCD* ccd = new CCD();
	QString prefix = "ccd/" + QVariant(ccdIndex).toString() + "/";
	ccd->setName(settings->value(prefix + "name", "").toString());
	ccd->setResolutionX(settings->value(prefix + "resolutionX", 4096).toInt());
	ccd->setResolutionY(settings->value(prefix + "resolutionY", 4096).toInt());
	ccd->setChipWidth(settings->value(prefix + "chip_width", 36.8).toDouble());
	ccd->setChipHeight(settings->value(prefix + "chip_height", 36.8).toDouble());
	ccd->setChipRotAngle(settings->value(prefix + "chip_rot_angle", 0.0).toDouble());
	ccd->setBinningX(settings->value(prefix + "binningX", 1).toInt());
	ccd->setBinningY(settings->value(prefix + "binningY", 1).toInt());
	ccd->setHasOAG(settings->value(prefix + "has_oag", "false").toBool());
	ccd->setPrismHeight(settings->value(prefix + "prism_height", 0.0).toDouble());
	ccd->setPrismWidth(settings->value(prefix + "prism_width", 0.0).toDouble());
	ccd->setPrismDistance(settings->value(prefix + "prism_distance", 0.0).toDouble());
	ccd->setPrismPosAngle(settings->value(prefix + "prism_pos_angle", 0.0).toDouble());
	return ccd;
}

CCD* CCD::ccdModel()
{
	CCD* model = new CCD();
	model->setName("My CCD");
	return model;
}
