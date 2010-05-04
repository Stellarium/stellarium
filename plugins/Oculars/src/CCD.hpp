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
#include <QSqlRecord>

class Ocular;

class CCD : public QObject
{
	Q_OBJECT
public:
	CCD(QSqlRecord record);
	virtual ~CCD();
	const QString getName() {return name;};
	int getCCDID() {return CCDID;};
	int getResolutionX() {return resolution_x;};
	int getResolutionY() {return resolution_y;};
	float getChipWidth() {return chip_width;};
	float getChipHeight() {return chip_height;};
	float getPixelWidth() {return pixel_width;};
	float getPixelHeight() {return pixel_height;};

	float getActualFOVx(Ocular *ocular);
	float getActualFOVy(Ocular *ocular);
private:
	int CCDID;
	QString name;
	//! total resolution width in pixels
	int resolution_x;
	//! total resolution height in pixels
	int resolution_y;
	//! chip width in millimeters
	float chip_width;
	//! chip height in millimeters
	float chip_height;
	//! height of 1 pixel in micron (micrometer)
	float pixel_width;
	//! width of 1 pixel in micron (micrometer)
	float pixel_height;
};

#endif /* CCD_HPP_ */
