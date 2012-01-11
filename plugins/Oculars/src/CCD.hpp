/*
 * Copyright (C) 2010 Timothy Reaves
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

#ifndef CCD_HPP_
#define CCD_HPP_

#include <QObject>
#include <QString>
#include <QSettings>

class Telescope;

class CCD : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(int resolutionX READ resolutionX WRITE setResolutionX)
	Q_PROPERTY(int resolutionY READ resolutionY WRITE setResolutionY)
	Q_PROPERTY(double chipWidth READ chipWidth WRITE setChipWidth)
	Q_PROPERTY(double chipHeight READ chipHeight WRITE setChipHeight)
	Q_PROPERTY(double pixelWidth READ pixelWidth WRITE setPixelWidth)
	Q_PROPERTY(double pixelHeight READ pixelHeight WRITE setPixelHeight)
public:
	CCD();
	Q_INVOKABLE CCD(const QObject& other);
	virtual ~CCD();
	static CCD* ccdFromSettings(QSettings* theSettings, int ccdIndex);
	static CCD* ccdModel();

	QString name() const;
	void setName(QString name);
	int getCCDID();
	int resolutionX() const;
	void setResolutionX(int resolution);
	int resolutionY() const;
	void setResolutionY(int resolution);
	double chipWidth() const;
	void setChipWidth(double width);
	double chipHeight() const;
	void setChipHeight(double height);
	double pixelWidth() const;
	void setPixelWidth(double width);
	double pixelHeight() const;
	void setPixelHeight(double height);

	/**
	  * The formula for this calculation comes from the Yerkes observatory.
	  * fov degrees = 2PI/360degrees * chipDimension mm / telescope FL mm
	  */
	double getActualFOVx(Telescope *telescope) const;
	double getActualFOVy(Telescope *telescope) const;
	QMap<int, QString> propertyMap();
private:
	QString m_name;
	//! total resolution width in pixels
	int m_resolutionX;
	//! total resolution height in pixels
	int m_resolutionY;
	//! chip width in millimeters
	double m_chipWidth;
	//! chip height in millimeters
	double m_chipHeight;
	//! height of 1 pixel in micron (micrometer)
	double m_pixelWidth;
	//! width of 1 pixel in micron (micrometer)
	double m_pixelHeight;
};


#endif /* CCD_HPP_ */
