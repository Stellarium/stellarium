/*
 * Copyright (C) 2009 Timothy Reaves
 * Copytight (C) 2013 Pawel Stolowski
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

#include "Barlow.hpp"
#include <QSettings>

Barlow::Barlow()
{
}

Barlow::Barlow(const QObject& other)
{
    this->m_multipler = other.property("multipler").toDouble();
    this->m_name = other.property("name").toString();
}

Barlow::~Barlow()
{
}

static QMap<int, QString> mapping;
QMap<int, QString> Barlow::propertyMap()
{
    if(mapping.isEmpty()) {
        mapping = QMap<int, QString>();
        mapping[0] = "name";
        mapping[1] = "multipler";
     }
    return mapping;
}

const QString Barlow::name() const
{
    return m_name;
}

void Barlow::setName(const QString& theValue)
{
    m_name = theValue;
}

double Barlow::multipler() const
{
    return m_multipler;
}

void Barlow::setMultipler(double theValue)
{
    m_multipler = theValue;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

Barlow* Barlow:: barlowFromSettings(QSettings* theSettings, int barlowIndex)
{
    Barlow* barlow = new Barlow();
    QString prefix = "barlow/" + QVariant(barlowIndex).toString() + "/";

    barlow->setName(theSettings->value(prefix + "name", "").toString());
    barlow->setMultipler(theSettings->value(prefix + "multipler", "1").toDouble());

    return barlow;
}

Barlow* Barlow::barlowModel()
{
    Barlow* model = new Barlow();
    model->setName("My Barlow");
    model->setMultipler(2.0f);
    return model;
}
