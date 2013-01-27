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

#ifndef BARLOW_HPP_
#define BARLOW_HPP_

#include <QObject>
#include <QString>
#include <QMap>

class QSettings;

class Barlow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(double multipler READ multipler WRITE setMultipler)

public:
    Barlow();
    Q_INVOKABLE Barlow(const QObject& other);
    virtual ~Barlow();
    static Barlow* barlowFromSettings(QSettings* theSettings, int barlowIndex);
    static Barlow* barlowModel();

    double multipler() const;
    void setMultipler(double theValue);
    const QString name() const;
    void setName(const QString& theValue);
    QMap<int, QString> propertyMap();

private:
    QString m_name;
    double m_multipler;
};

#endif
