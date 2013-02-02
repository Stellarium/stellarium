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

#ifndef OCULAR_HPP_
#define OCULAR_HPP_

#include <QDebug>
#include <QObject>
#include <QString>
#include <QSettings>

class Telescope;
class Barlow;

class Ocular : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(double appearentFOV READ appearentFOV WRITE setAppearentFOV)
	Q_PROPERTY(double effectiveFocalLength READ effectiveFocalLength WRITE setEffectiveFocalLength)
	Q_PROPERTY(double fieldStop READ fieldStop WRITE setFieldStop)
	Q_PROPERTY(bool binoculars READ isBinoculars WRITE setBinoculars)
public:
	Ocular();
	Q_INVOKABLE Ocular(const QObject& other);
	virtual ~Ocular();
	static Ocular* ocularFromSettings(QSettings* theSettings, int ocularIndex);
	static Ocular* ocularModel();

	const QString name() const;
	void setName(QString aName);
	double appearentFOV() const;
	void setAppearentFOV(double fov);
	double effectiveFocalLength() const;
	void setEffectiveFocalLength(double fl);
	double fieldStop() const;
	void setFieldStop(double fs);
	bool isBinoculars() const;
	void setBinoculars(bool flag);

    double actualFOV(Telescope *telescope, Barlow *barlow) const;
        double magnification(Telescope *telescope, Barlow *barlow) const;
	QMap<int, QString> propertyMap();

private:
	QString m_name;
	double m_appearentFOV;
	double m_effectiveFocalLength;
	double m_fieldStop;
	bool m_binoculars;
};


#endif /* OCULAR_HPP_ */
