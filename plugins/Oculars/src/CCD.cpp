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

#include "CCD.hpp"
#include "Ocular.hpp"

#include <QDebug>
#include <QSettings>

CCD::CCD(QSqlRecord record)
{
	CCDID = record.value("id").toInt();
	name = record.value("name").toString();
	resolution_x = record.value("resolution_x").toInt();
	resolution_y = record.value("resolution_y").toInt();
	chip_width = record.value("chip_width").toFloat();
	chip_height = record.value("chip_height").toFloat();
	pixel_width = record.value("pixel_width").toFloat();
	pixel_height = record.value("pixel_height").toFloat();
}

CCD::~CCD()
{
}

float CCD::getActualFOVx(Ocular *ocular)
{
	float FOVx = (chip_width * 206.265) / ocular->getEffectiveFocalLength();
	return FOVx;
}

float CCD::getActualFOVy(Ocular *ocular)
{
	float FOVy = (chip_height * 206.265) / ocular->getEffectiveFocalLength();
	return FOVy;
}