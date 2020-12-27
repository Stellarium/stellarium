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
#include <QSettings>

Lens::Lens()
	: m_multipler(1.)
{
}

Lens::Lens(const QObject& other)
	: m_name(other.property("name").toString())
	, m_multipler(other.property("multipler").toDouble())
{
}

Lens::~Lens()
{
}

static QMap<int, QString> mapping;
QMap<int, QString> Lens::propertyMap()
{
	if(mapping.isEmpty()) {
		mapping = QMap<int, QString>();
		mapping[0] = "name";
		mapping[1] = "multipler";
	}
	return mapping;
}

const QString Lens::getName() const
{
	return m_name;
}

void Lens::setName(const QString& theValue)
{
	m_name = theValue;
}

double Lens::getMultipler() const
{
	return m_multipler;
}

void Lens::setMultipler(double theValue)
{
	m_multipler = theValue;
}

void Lens::writeToSettings(QSettings * settings, const int index)
{
	QString prefix = "lens/" + QVariant(index).toString() + "/";
	settings->setValue(prefix + "name", this->getName());
	settings->setValue(prefix + "multipler", this->getMultipler());
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

Lens* Lens:: lensFromSettings(QSettings* theSettings, int lensIndex)
{
	Lens* lens = new Lens();
	QString prefix = "lens/" + QVariant(lensIndex).toString() + "/";

	lens->setName(theSettings->value(prefix + "name", "").toString());
	lens->setMultipler(theSettings->value(prefix + "multipler", "1").toDouble());

	return lens;
}

Lens* Lens::lensModel()
{
	Lens* model = new Lens();
	model->setName("My Lens");
	model->setMultipler(2.0);
	return model;
}
