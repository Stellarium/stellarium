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

#include "Telescope.hpp"

#include <QDebug>
#include <QMetaProperty>
#include <QSettings>

Telescope::Telescope(QObject * parent)
	: QObject(parent)
{
}

void Telescope::initFromSettings(QSettings * theSettings, int telescopeIndex)
{
	QString     prefix    = "telescope/" + QVariant(telescopeIndex).toString() + "/";

	this->setName(theSettings->value(prefix + "name", "").toString());
	this->setFocalLength(theSettings->value(prefix + "focalLength", "0").toDouble());
	this->setDiameter(theSettings->value(prefix + "diameter", "0").toDouble());
	this->setHFlipped(theSettings->value(prefix + "hFlip").toBool());
	this->setVFlipped(theSettings->value(prefix + "vFlip").toBool());
	this->setEquatorial(theSettings->value(prefix + "equatorial").toBool());
}
/* ****************************************************************************************************************** */
// MARK: - Accessors & Mutators
/* ****************************************************************************************************************** */
auto Telescope::name() const -> QString
{
	return m_name;
}

void Telescope::setName(QString theValue)
{
	m_name = theValue;
}

auto Telescope::focalLength() const -> double
{
	return m_focalLength;
}

void Telescope::setFocalLength(double theValue)
{
	m_focalLength = theValue;
}

auto Telescope::diameter() const -> double
{
	return m_diameter;
}

void Telescope::setDiameter(double theValue)
{
	m_diameter = theValue;
}

auto Telescope::isHFlipped() const -> bool
{
	return m_flippedHorizontally;
}

void Telescope::setHFlipped(bool flipped)
{
	m_flippedHorizontally = flipped;
}

auto Telescope::isVFlipped() const -> bool
{
	return m_flippedVertically;
}

void Telescope::setVFlipped(bool flipped)
{
	m_flippedVertically = flipped;
}

auto Telescope::isEquatorial() const -> bool
{
	return m_equatorial;
}

void Telescope::setEquatorial(bool eq)
{
	m_equatorial = eq;
}

void Telescope::writeToSettings(QSettings * settings, const int index) const
{
	QString prefix = "telescope/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->name());
	settings->setValue(prefix + "focalLength", this->focalLength());
	settings->setValue(prefix + "diameter", this->diameter());
	settings->setValue(prefix + "hFlip", this->isHFlipped());
	settings->setValue(prefix + "vFlip", this->isVFlipped());
	settings->setValue(prefix + "equatorial", this->isEquatorial());
}

/* ****************************************************************************************************************** */
// MARK: - Static Methods
/* ****************************************************************************************************************** */
auto Telescope::propertyMap() -> QMap<int, QString>
{
	static const auto mapping =
			QMap<int, QString>{ { 0, QLatin1String("name") },        { 1, QLatin1String("diameter") },
					    { 2, QLatin1String("focalLength") }, { 3, QLatin1String("hFlipped") },
					    { 4, QLatin1String("vFlipped") },    { 5, QLatin1String("equatorial") } };
	return mapping;
}

/* ****************************************************************************************************************** */
// MARK: - Operators
/* ****************************************************************************************************************** */
auto operator<<(QDebug debug, const Telescope & telescope) -> QDebug
{
	return debug.maybeSpace() << &telescope;
}

auto operator<<(QDebug debug, const Telescope * telescope) -> QDebug
{
	QDebugStateSaver    saver(debug);

	const QMetaObject * metaObject = telescope->metaObject();
	debug.nospace() << "Telescope(";
	int count = metaObject->propertyCount();
	for (int i = 0; i < count; ++i) {
		QMetaProperty metaProperty = metaObject->property(i);
		const char *  name         = metaProperty.name();
		debug.nospace() << name << ":" << telescope->property(name);
	}
	debug.nospace() << ")";
	return debug.maybeSpace();
}
