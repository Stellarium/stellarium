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

#include "Lens.hpp"
#include "Telescope.hpp"
#include <math.h>
#include <QMetaProperty>

Ocular::Ocular(QObject * parent)
	: QObject(parent)
{
}
auto Ocular::propertyMap() -> QMap<int, QString>
{
	static const auto mapping = QMap<int, QString>{ { 0, QLatin1String("name") },
							{ 1, QLatin1String("apparentFOV") },
							{ 2, QLatin1String("effectiveFocalLength") },
							{ 3, QLatin1String("fieldStop") },
							{ 4, QLatin1String("binoculars") },
							{ 5, QLatin1String("permanentCrosshair") },
							{ 6, QLatin1String("reticlePath") } };
	return mapping;
}

/* ****************************************************************************************************************** */
// MARK: - Instance Methods
/* ****************************************************************************************************************** */
auto Ocular::actualFOV(const Telescope * telescope, const Lens * lens) const -> double
{
	double       actualFOV      = NAN;
	const double lensMultiplier = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	if (m_binoculars)
	{
		actualFOV = apparentFOV();
	} else if (fieldStop() > 0.0) {
		actualFOV = fieldStop() / (telescope->focalLength() * lensMultiplier) * DegreesPerRadian;
	} else {
		// actualFOV = apparent / mag
		actualFOV = apparentFOV() / (telescope->focalLength() * lensMultiplier / effectiveFocalLength());
	}
	return actualFOV;
}

void Ocular::initFromSettings(const QSettings * theSettings, const int ocularIndex)
{
	QString  prefix = "ocular/" + QVariant(ocularIndex).toString() + "/";

	this->setName(theSettings->value(prefix + "name", "").toString());
	this->setApparentFOV(theSettings->value(prefix + "afov", 0.0).toDouble());
	this->setEffectiveFocalLength(theSettings->value(prefix + "efl", 0.0).toDouble());
	this->setFieldStop(theSettings->value(prefix + "fieldStop", 0.0).toDouble());
	this->setBinoculars(theSettings->value(prefix + "binoculars", "false").toBool());
	this->setPermanentCrosshair(theSettings->value(prefix + "permanentCrosshair", "false").toBool());
	this->setReticlePath(theSettings->value(prefix + "reticlePath", "").toString());

	if (!(this->apparentFOV() > 0.0 && this->effectiveFocalLength() > 0.0))
	{
		qWarning() << "WARNING: Invalid data for ocular. Ocular values must be positive. \n"
                 << "\tafov: " << this->apparentFOV() << "\n"
                 << "\tefl: " << this->effectiveFocalLength() << "\n"
                 << "This ocular should be removed.";
	}
}

auto Ocular::magnification(const Telescope * telescope, const Lens * lens) const -> double
{
	double       magnification  =  NAN;
	const double lensMultiplier = (lens != Q_NULLPTR ? lens->multiplier() : 1.0);
	if (m_binoculars)
	{
		magnification = effectiveFocalLength();
	} else {
		magnification = telescope->focalLength() * lensMultiplier / effectiveFocalLength();
	}
	return magnification;
}

/* ****************************************************************************************************************** */
// MARK: - Accessors & Mutators
/* ****************************************************************************************************************** */
auto Ocular::name() const -> QString
{
	return m_name;
}

void Ocular::setName(QString aName)
{
	m_name = aName;
}

auto Ocular::apparentFOV() const -> double
{
	return m_apparentFOV;
}

void Ocular::setApparentFOV(const double fov)
{
	m_apparentFOV = fov;
}

auto Ocular::effectiveFocalLength() const -> double
{
	return m_effectiveFocalLength;
}

void Ocular::setEffectiveFocalLength(const double fl)
{
	m_effectiveFocalLength = fl;
}

auto Ocular::fieldStop() const -> double
{
	return m_fieldStop;
}

void Ocular::setFieldStop(const double fs)
{
	m_fieldStop = fs;
}

auto Ocular::isBinoculars() const -> bool
{
	return m_binoculars;
}

void Ocular::setBinoculars(const bool flag)
{
	m_binoculars = flag;
}

auto Ocular::hasPermanentCrosshair() const -> bool
{
	return m_permanentCrosshair;
}

void Ocular::setPermanentCrosshair(const bool flag)
{
	m_permanentCrosshair = flag;
}

auto Ocular::reticlePath() const -> QString
{
	return m_reticlePath;
}
void Ocular::setReticlePath(QString path)
{
	m_reticlePath = path;
}

/* ****************************************************************************************************************** */
// MARK: - Static Methods
/* ****************************************************************************************************************** */
void Ocular::writeToSettings(QSettings * settings, const int index) const
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

/* ****************************************************************************************************************** */
// MARK: - Operators
/* ****************************************************************************************************************** */
auto operator<<(QDebug debug, const Ocular & ocular) -> QDebug
{
	return debug.maybeSpace() << &ocular;
}

auto operator<<(QDebug debug, const Ocular * ocular) -> QDebug
{
	QDebugStateSaver    saver(debug);

	const QMetaObject * metaObject = ocular->metaObject();
	debug.nospace() << "Ocular(";
	int count = metaObject->propertyCount();
	for (int i = 0; i < count; ++i)
	{
		QMetaProperty metaProperty = metaObject->property(i);
		const char *  name         = metaProperty.name();
		debug.nospace() << name << ":" << ocular->property(name);
	}
	debug.nospace() << ")";
	return debug.maybeSpace();
}
