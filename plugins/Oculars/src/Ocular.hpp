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

#ifndef OCULAR_HPP
#define OCULAR_HPP

#include <QDebug>
#include <QObject>
#include <QString>
#include <QSettings>

class Telescope;
class Lens;

//! @ingroup oculars
class Ocular : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool binoculars READ isBinoculars WRITE setBinoculars)
	Q_PROPERTY(bool permanentCrosshair READ hasPermanentCrosshair WRITE setPermanentCrosshair)
	Q_PROPERTY(double apparentFOV READ apparentFOV WRITE setApparentFOV)
	Q_PROPERTY(double effectiveFocalLength READ effectiveFocalLength WRITE setEffectiveFocalLength)
	Q_PROPERTY(double fieldStop READ fieldStop WRITE setFieldStop)
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(QString reticlePath READ reticlePath WRITE setReticlePath)
public:
	Ocular();
	Q_INVOKABLE Ocular(const QObject& other);
	virtual ~Ocular();
	static Ocular * ocularFromSettings(const QSettings * theSettings, const int ocularIndex);
	void writeToSettings(QSettings * settings, const int index);
	static Ocular * ocularModel(void);

	bool isBinoculars(void) const;
	void setBinoculars(const bool flag);
	bool hasPermanentCrosshair(void) const;
	void setPermanentCrosshair(const bool flag);
	double apparentFOV(void) const;
	void setApparentFOV(const double fov);
	double effectiveFocalLength(void) const;
	void setEffectiveFocalLength(const double fl);
	double fieldStop(void) const;
	void setFieldStop(const double fs);
	QString name(void) const;
	void setName(const QString aName);
	QString reticlePath(void) const;
	void setReticlePath(const QString path);

	double actualFOV(const Telescope * telescope, const Lens *lens) const;
	double magnification(const Telescope * telescope, const Lens *lens) const;
	QMap<int, QString> propertyMap(void);

private:
	bool m_binoculars;
	bool m_permanentCrosshair;
	double m_apparentFOV;
	double m_effectiveFocalLength;
	double m_fieldStop;
	QString m_name;
	QString m_reticlePath;
};


#endif /* OCULAR_HPP */
