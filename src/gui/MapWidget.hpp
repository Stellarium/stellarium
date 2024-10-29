/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2022 Ruslan Kabatsayev
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


#ifndef MAPWIDGET_HPP
#define MAPWIDGET_HPP

#include <QRect>
#include <QWidget>
#include <QPixmap>

//! @class MapWidget
//! Special widget that shows a world map
class MapWidget : public QWidget
{
	Q_OBJECT

public:
	MapWidget(QWidget *parent = nullptr);
	//! Set the current marker position
	//! @param longitude longitude in degree in range [-180;180[
	//! @param latitude latitude in degree in range [-90;90]
	void setMarkerPos(double longitude, double latitude);
	//! allow hiding the location arrow (if sitting on an observer)
	void setMarkerVisible(bool visible);

	void setMap(const QPixmap &map);
	
signals:
	//! Signal emitted when we click on the map. It also delivers the color value at the clicked point.
	void positionChanged(double longitude, double latitude, const QColor &color);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	double markerLat=0, markerLon=0;
	QPixmap map;
	QPixmap locationMarker;
	QRectF mapRect; // in device pixels
	bool markerVisible;
};

#endif // MAPWIDGET_HPP
