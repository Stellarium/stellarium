/*
  * Copyright (C) 2010 Bernhard Reutner-Fischer
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
  * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  */

#ifndef CCD_HPP_
#define CCD_HPP_

#include <QObject>
#include <QString>
#include <QSettings>

class Ocular;

class CCD : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(int resolutionX READ resolutionX WRITE setResolutionX)
	Q_PROPERTY(int resolutionY READ resolutionY WRITE setResolutionY)
	Q_PROPERTY(float chipWidth READ chipWidth WRITE setChipWidth)
	Q_PROPERTY(float chipHeight READ chipHeight WRITE setChipHeight)
	Q_PROPERTY(float pixelWidth READ pixelWidth WRITE setPixelWidth)
	Q_PROPERTY(float pixelHeight READ pixelHeight WRITE setPixelHeight)
public:
	CCD();
	Q_INVOKABLE CCD(const QObject& other);
	virtual ~CCD();
	static CCD* ccdFromSettings(QSettings* theSettings, QString theGroupName);
	static CCD* ccdModel();

	QString name() const;
	void setName(QString name);
	int getCCDID();
	int resolutionX()  const;
	void setResolutionX(int resolution);
	int resolutionY()  const;
	void setResolutionY(int resolution);
	float chipWidth()  const;
	void setChipWidth(float width);
	float chipHeight()  const;
	void setChipHeight(float height);
	float pixelWidth()  const;
	void setPixelWidth(float width);
	float pixelHeight()  const;
	void setPixelHeight(float height);

	float getActualFOVx(Ocular *ocular) const;
	float getActualFOVy(Ocular *ocular) const;
	QMap<int, QString> propertyMap();
private:
	int ccdID;
	QString m_name;
	//! total resolution width in pixels
	int m_resolutionX;
	//! total resolution height in pixels
	int m_resolutionY;
	//! chip width in millimeters
	float m_chipWidth;
	//! chip height in millimeters
	float m_chipHeight;
	//! height of 1 pixel in micron (micrometer)
	float m_pixelWidth;
	//! width of 1 pixel in micron (micrometer)
	float m_pixelHeight;
};


#endif /* CCD_HPP_ */
