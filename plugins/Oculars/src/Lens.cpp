/*
 * Copyright (C) 2009 Timothy Reaves
 * Copyright (C) 2013 Pawel Stolowski
 * Copyright (C) 2013 Alexander Wolf
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

#include "Lens.hpp"
#include <QMetaObject>
#include <QMetaProperty>
#include <QSettings>

Lens::Lens(QObject * parent)
	: QObject(parent)
{
}

/* ****************************************************************************************************************** */
// MARK: - Instance Methods
/* ****************************************************************************************************************** */
void Lens::initFromSettings(QSettings * theSettings, int lensIndex)
{
	QString prefix = "lens/" + QVariant(lensIndex).toString() + "/";

	this->setName(theSettings->value(prefix + propertyMap().value(0), "").toString());
	this->setMultiplier(theSettings->value(prefix + propertyMap().value(1), "1").toDouble());
}

auto Lens::name() const -> QString
{
	return m_name;
}

void Lens::setName(const QString & theValue)
{
	m_name = theValue;
}

auto Lens::multiplier() const -> double
{
	return m_multiplier;
}

void Lens::setMultiplier(double theValue)
{
	m_multiplier = theValue;
}

void Lens::writeToSettings(QSettings * settings, const int index) const
{
	QString prefix = "lens/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + propertyMap().value(0), this->name());
	settings->setValue(prefix + propertyMap().value(1), this->multiplier());
}

/* ****************************************************************************************************************** */
// MARK: - Static Methods
/* ****************************************************************************************************************** */
auto Lens::propertyMap() -> QMap<int, QString>
{
	static const auto mapping = QMap<int, QString>{ { 0, QLatin1String("name") }, { 1, QLatin1String("multipler") } };
	return mapping;
}

/* ****************************************************************************************************************** */
// MARK: - Operators
/* ****************************************************************************************************************** */
auto operator<<(QDebug debug, const Lens & lens) -> QDebug
{
	return debug.maybeSpace() << &lens;
}

auto operator<<(QDebug debug, const Lens * lens) -> QDebug
{
	QDebugStateSaver    saver(debug);

	const QMetaObject * metaObject = lens->metaObject();
	debug.nospace() << "Lens(";
	int count = metaObject->propertyCount();
	for (int i = 0; i < count; ++i)
	{
		QMetaProperty metaProperty = metaObject->property(i);
		const char *  name         = metaProperty.name();
		debug.nospace() << name << ":" << lens->property(name);
	}
	debug.nospace() << ")";
	return debug.maybeSpace();
}
