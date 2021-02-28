/*
 * Copyright (C) 2021 Georg Zotti
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

#ifndef FINDER_HPP
#define FINDER_HPP

#include <QDebug>
#include <QObject>
#include <QString>
#include <QSettings>

//! @ingroup oculars
//! The Finder is a compact optical device with fixed magnification. Real-world finderscopes and binoculars are usually described with magnification, aperture and true field of view.
//! Sometimes, esp. for terrestrial application, we find field of view specified as W [metres] per distance D [km]. This can be converted into trueFOV=2*atan(W / 2000D)
class Finder : public QObject
{
	Q_OBJECT
	//Q_PROPERTY(bool binoculars READ isBinoculars WRITE setBinoculars) // This could be added as purely informagtive flag without any consequence for display now.
	Q_PROPERTY(bool permanentCrosshair READ hasPermanentCrosshair WRITE setPermanentCrosshair)
	Q_PROPERTY(double trueFOV          READ trueFOV               WRITE setTrueFOV)
	Q_PROPERTY(double magnification    READ magnification         WRITE setMagnification)
	Q_PROPERTY(double aperture         READ aperture              WRITE setAperture)
	Q_PROPERTY(QString name            READ name                  WRITE setName)
	Q_PROPERTY(QString reticlePath     READ reticlePath           WRITE setReticlePath) // Some military binoculars or targetting scopes have grids.
	Q_PROPERTY(bool hFlipped           READ isHFlipped            WRITE setHFlipped)
	Q_PROPERTY(bool vFlipped           READ isVFlipped            WRITE setVFlipped)
	Q_PROPERTY(bool equatorial         READ isEquatorial          WRITE setEquatorial)

public:
	Finder();
	Q_INVOKABLE Finder(const QObject& other);
	virtual ~Finder();
	static Finder * finderFromSettings(const QSettings * theSettings, const int finderIndex);
	void writeToSettings(QSettings * settings, const int index);
	static Finder * finderModel(void);
	double apparentFOV() const {return m_magnification*m_trueFOV;}
	QMap<int, QString> propertyMap(void);

public slots:
	//bool isBinoculars(void) const;
	//void setBinoculars(const bool flag);
	bool hasPermanentCrosshair(void) const;
	void setPermanentCrosshair(const bool flag);
	double trueFOV(void) const;
	void setTrueFOV(const double fov);
	double magnification(void) const;
	void setMagnification(const double mag);
	double aperture(void) const;
	void setAperture(const double ap);
	bool isHFlipped() const;              //!< horizontally flipped?
	void setHFlipped(bool flipped);       //!< horizontally flipped?
	bool isVFlipped() const;              //!< vertically flipped?
	void setVFlipped(bool flipped);       //!< vertically flipped?
	bool isEquatorial() const;            //!< equatorially mounted?
	void setEquatorial(bool eq);          //!< equatorially mounted?
	QString name(void) const;
	void setName(const QString aName);
	QString reticlePath(void) const;
	void setReticlePath(const QString path);

private:
	//bool m_binoculars;
	bool    m_permanentCrosshair;
	double  m_trueFOV;
	double  m_magnification;
	double  m_aperture;        //!< aperture, mm
	bool    m_hFlipped;	   //!< horizontally flipped?
	bool    m_vFlipped;	   //!< vertically flipped?
	bool    m_equatorial;	   //!< equatorially mounted?
	QString m_name;
	QString m_reticlePath;
};


#endif /* OCULAR_HPP */
