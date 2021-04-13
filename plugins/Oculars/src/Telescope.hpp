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

#ifndef TELESCOPE_HPP
#define TELESCOPE_HPP

#include <QObject>
#include <QString>
#include <QSettings>

//! @ingroup oculars
class Telescope : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(double diameter READ diameter WRITE setDiameter)
	Q_PROPERTY(double focalLength READ focalLength WRITE setFocalLength)
	Q_PROPERTY(bool hFlipped READ isHFlipped WRITE setHFlipped)
	Q_PROPERTY(bool vFlipped READ isVFlipped WRITE setVFlipped)
	Q_PROPERTY(bool equatorial READ isEquatorial WRITE setEquatorial)
public:
	Telescope();
	Q_INVOKABLE Telescope(const QObject& other);
	virtual ~Telescope();
	static Telescope* telescopeFromSettings(QSettings* theSettings, int telescopeIndex);
	void writeToSettings(QSettings * settings, const int index);
	static Telescope* telescopeModel();

	double diameter() const;
	void setDiameter(double theValue);
	double focalLength() const;
	void setFocalLength(double theValue);
	const QString name() const;
	void setName(QString theValue);
	bool isHFlipped() const;
	void setHFlipped(bool flipped);
	bool isVFlipped() const;
	void setVFlipped(bool flipped);
	bool isEquatorial() const;
	void setEquatorial(bool eq);
	QMap<int, QString> propertyMap();
private:
	QString m_name;		//!< proper name
	double m_diameter;	//!< millimetres
	double m_focalLength;	//!< millimetres
	bool m_hFlipped;	//!< horizontally flipped?
	bool m_vFlipped;	//!< vertically flipped?
	bool m_equatorial;	//!< equatorially mounted?
};

#endif /*TELESCOPE_HPP*/
