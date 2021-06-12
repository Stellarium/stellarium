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

#ifndef LENS_HPP
#define LENS_HPP

#include <QObject>
#include <QString>
#include <QMap>

class QSettings;

//! @ingroup oculars
class Lens : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ getName WRITE setName)
	Q_PROPERTY(double multipler READ getMultipler WRITE setMultipler)

public:
	Lens();
	Q_INVOKABLE Lens(const QObject& other);
	virtual ~Lens();
	static Lens* lensFromSettings(QSettings* theSettings, int lensIndex);
	void writeToSettings(QSettings * settings, const int index);
	static Lens* lensModel();

	double getMultipler() const;
	void setMultipler(double theValue);
	const QString getName() const;
	void setName(const QString& theValue);
	QMap<int, QString> propertyMap();

private:
	QString m_name;
	double m_multipler;
};

#endif
