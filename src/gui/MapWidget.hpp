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
#include <QPainterPath>

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
	//! Set the search circle to mark the locations available
	//! @param longitude longitude in degrees in the range [-180;180[
	//! @param latitude latitude in degrees in the range [-90;90]
	//! @param searchRadius search radius in degrees in the range ]0;180].
	//! If \p searchRadius is set to 180, filtering is considered disabled.
	void setLocationFilter(double longitude, double latitude, double searchRadius);

	void setMap(const QPixmap &map);

	void getMapView(double& centerLongitude, double& centerLatitude,
	                double& zoom) const;
	void setMapView(double centerLongitude, double centerLatitude, double zoom);
	
signals:
	//! Signal emitted when we click on the map. It also delivers the color value at the clicked point.
	void positionChanged(double longitude, double latitude, const QColor &color);
	void mapViewChanged(double centerLongitude, double centerLatitude, double zoom);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

	struct LonLat
	{
		double longitude;
		double latitude;
	};
	LonLat mapPointToLonLat(const QPointF& mapPoint) const;
	struct MapPoint
	{
		double x, y;
	};
	MapPoint lonLatToMapPoint(double lon, double lat) const;

private:
	void makeSearchAreaOutline(int width, int height);
	void updateScaledMapAndRect();
	void emitMapViewChanged();

private:
	double markerLat=0, markerLon=0;
	double searchCenterLon=0, searchCenterLat=0, searchRadius=180;
	QPainterPath searchAreaOutline;
	QPixmap map, scaledMap;
	QPixmap locationMarker;
	QRectF mapRect; // in device pixels
	QPointF shift;
	QPointF dragStart;
	QPointF currentDragShift;
	double zoom = 1;
	bool markerVisible;
};

#endif // MAPWIDGET_HPP
