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

#include "MapWidget.hpp"
#include <cmath>
#include <QPainter>
#include <QMouseEvent>
#include "StelLocationMgr.hpp"
#include "StelApp.hpp"

namespace
{
constexpr int GUI_PIXMAPS_SCALE = 5;
constexpr int THRESHOLD_FOR_CLICK = 5;

template<typename Event>
QPointF position(const Event* event, const double devPixelRatio)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return event->position() * devPixelRatio;
#else
	return QPointF(event->pos()) * devPixelRatio;
#endif
}
}

MapWidget::MapWidget(QWidget* parent)
	: QWidget(parent)
	, map(":/graphicGui/miscWorldMap.jpg")
	, locationMarker(":/graphicGui/uieMapPointer.png")
	, markerVisible(true)
{
	updateScaledMapAndRect();
}

void MapWidget::setMarkerVisible(bool visible)
{
	markerVisible=visible;
}

void MapWidget::setMarkerPos(double longitude, double latitude)
{
	markerLat = latitude;
	markerLon = longitude;
	update();
}

void MapWidget::setLocationFilter(double longitude, double latitude, double newSearchRadius)
{
	searchCenterLon = longitude;
	searchCenterLat = latitude;
	searchRadius = newSearchRadius;
	searchAreaOutline = {};
	update();
}

auto MapWidget::mapPointToLonLat(const QPointF& mapPoint) const -> LonLat
{
	const auto lat = (mapPoint.y() - mapRect.center().y()) / mapRect.height() * -180;
	const auto lon = (mapPoint.x() - mapRect.center().x()) / mapRect.width()  * 360;
	return {std::remainder(lon, 360), std::clamp(lat, -90., 90.)};
}

auto MapWidget::lonLatToMapPoint(const double lon, const double lat) const -> MapPoint
{
	const auto x = mapRect.left() + (lon + 180) /  360. * mapRect.width();
	const auto y = mapRect.top()  + (lat -  90) / -180. * mapRect.height();
	return {x,y};
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) return;

	dragStart = position(event, devicePixelRatioF());
	currentDragShift = QPointF(0,0);
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (!(event->buttons() & Qt::LeftButton)) return;

	currentDragShift = position(event, devicePixelRatioF()) - dragStart;
	updateScaledMapAndRect();
	emitMapViewChanged();
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) return;

	const auto pos = position(event, devicePixelRatioF());
	currentDragShift = pos - dragStart;
	shift += currentDragShift;
	const bool moved = currentDragShift.manhattanLength() > THRESHOLD_FOR_CLICK * StelApp::getInstance().guiFontSizeRatio();
	currentDragShift = QPointF(0,0);
	if (moved)
	{
		updateScaledMapAndRect();
		emitMapViewChanged();
		return;
	}

	if(mapRect.top() > pos.y() || mapRect.bottom() < pos.y())
		return;

	const auto [lon, lat] = mapPointToLonLat(pos);

	StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
	QColor color=locationMgr->getColorForCoordinates(lon, lat);

	dragStart = QPointF(-1,-1);

	emit positionChanged(lon, lat, color);
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
	if (event->buttons() & Qt::LeftButton) return;

	const double delta = event->angleDelta().y();
	const auto pos = position(event, devicePixelRatioF());
	const auto offset = pos - QPointF(width(), height()) / 2.;

	double zoomFactor = std::pow(2., delta / 120.);
	const auto oldZoom = zoom;
	zoom = oldZoom * zoomFactor;
	if (zoom < 1)
	{
		zoom = 1;
		zoomFactor = zoom / oldZoom;
	}

	shift = (shift - offset) * zoomFactor + offset;

	updateScaledMapAndRect();
	emitMapViewChanged();
}

void MapWidget::setMap(const QPixmap &map)
{
	this->map = map;
	scaledMap = {};
	updateScaledMapAndRect();
}

void MapWidget::getMapView(double& centerLongitude,
                           double& centerLatitude,
                           double& currentZoom) const
{
	const double ratio = devicePixelRatioF();
	const QPointF center(width() * ratio / 2.0,
	                     height() * ratio / 2.0);
	const auto view = mapPointToLonLat(center);
	centerLongitude = view.longitude;
	centerLatitude = view.latitude;
	currentZoom = zoom;
}

void MapWidget::setMapView(double centerLongitude,
                           double centerLatitude,
                           double newZoom)
{
	if (newZoom < 1.0)
		newZoom = 1.0;

	const double ratio = devicePixelRatioF();
	const QSizeF virtualSize(height() * 2.0 * ratio * newZoom,
	                         height() * ratio * newZoom);
	const QPointF widgetCenter(width() * ratio / 2.0,
	                           height() * ratio / 2.0);
	const QPointF mapRelative((centerLongitude + 180.0) / 360.0 * virtualSize.width(),
	                          (centerLatitude - 90.0) / -180.0 * virtualSize.height());
	const QPointF baseTopLeft(std::round((width() * ratio - virtualSize.width()) / 2.0),
	                          std::round((height() * ratio - virtualSize.height()) / 2.0));

	zoom = newZoom;
	currentDragShift = QPointF(0, 0);
	shift = widgetCenter - mapRelative - baseTopLeft;
	updateScaledMapAndRect();
}

void MapWidget::emitMapViewChanged()
{
	double longitude = 0.0;
	double latitude = 0.0;
	double currentZoom = 1.0;
	getMapView(longitude, latitude, currentZoom);
	emit mapViewChanged(longitude, latitude, currentZoom);
}

void MapWidget::makeSearchAreaOutline(const int width, const int height)
{
	constexpr double nMax = 128;
	std::vector<QPointF> points;
	for (double n = 0; n <= nMax; ++n)
	{
		const auto bearing = n / nMax * 360;
		const auto point3d = StelLocation::pointAtDistanceDegrees(searchCenterLon, searchCenterLat, searchRadius, bearing);
		double lon, lat;
		StelUtils::rectToSphe(&lon, &lat, point3d);
		lon *= M_180_PI;
		lat *= M_180_PI;

		const auto [x, y] = lonLatToMapPoint(lon, lat);
		points.emplace_back(x, y);
	}

	// Special handling for poles, because they won't be
	// included in the path if connect the points naively.
	if (searchCenterLat + searchRadius > 90)
	{
		searchAreaOutline.moveTo(mapRect.topRight() + QPointF(1,-1));
		searchAreaOutline.lineTo(mapRect.topLeft() + QPointF(-1,-1));
		std::sort(points.begin(), points.end(),
		          [](auto& a, auto& b){ return a.x() < b.x(); });
		searchAreaOutline.lineTo(points.back().x() - mapRect.width(), points.back().y());
		for (const auto& p : points)
			searchAreaOutline.lineTo(p);
		searchAreaOutline.lineTo(points.front().x() + mapRect.width(), points.front().y());
		searchAreaOutline.closeSubpath();
	}
	else if (searchCenterLat - searchRadius < -90)
	{
		searchAreaOutline.moveTo(mapRect.bottomRight() + QPointF(1,1));
		searchAreaOutline.lineTo(mapRect.bottomLeft() + QPointF(-1,1));
		std::sort(points.begin(), points.end(),
		          [](auto& a, auto& b){ return a.x() < b.x(); });
		searchAreaOutline.lineTo(points.back().x() - mapRect.width(), points.back().y());
		for (const auto& p : points)
			searchAreaOutline.lineTo(p);
		searchAreaOutline.lineTo(points.front().x() + mapRect.width(), points.front().y());
		searchAreaOutline.closeSubpath();
	}
	else
	{
		// Handle possible jump over longitude ±180°
		bool needCopyOnTheLeft = false, needCopyOnTheRight = false;
		for (size_t n = 1; n < points.size(); ++n)
		{
			const auto& prevP = points[n-1];
			auto& p = points[n];
			if (std::abs(p.x() - width - prevP.x()) < std::abs(p.x() - prevP.x()))
			{
				p.setX(p.x() - width);
				needCopyOnTheRight = true;
			}
			else if (std::abs(p.x() + width - prevP.x()) < std::abs(p.x() - prevP.x()))
			{
				p.setX(p.x() + width);
				needCopyOnTheLeft = true;
			}
		}

		searchAreaOutline.moveTo(points[0]);
		for (const auto& p : points)
			searchAreaOutline.lineTo(p);
		searchAreaOutline.closeSubpath();
		if (needCopyOnTheLeft)
		{
			searchAreaOutline.moveTo(points[0].x() - width, points[0].y());
			for (const auto& p : points)
				searchAreaOutline.lineTo(p.x() - width, p.y());
			searchAreaOutline.closeSubpath();
		}
		if (needCopyOnTheRight)
		{
			searchAreaOutline.moveTo(points[0].x() + width, points[0].y());
			for (const auto& p : points)
				searchAreaOutline.lineTo(p.x() + width, p.y());
			searchAreaOutline.closeSubpath();
		}
	}
}

void MapWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	const double ratio = devicePixelRatioF();
	painter.scale(1/ratio, 1/ratio); // Work in units of device pixels

	const auto mainRect = mapRect.toRect();
	const int windowWidth = width();
	const int mapWidth = mainRect.width();
	const QPoint horizShift(mapWidth, 0);
	const int kMin = -1 - mainRect.x() / mapWidth + (mainRect.x() % mapWidth == 0);
	const int kMax = (windowWidth - mainRect.x()) / mapWidth;
	if (mapRect.width() > scaledMap.width())
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
	for (int k = kMin; k <= kMax; ++k)
		painter.drawPixmap(QRect(mainRect.topLeft() + k * horizShift, mainRect.size()), scaledMap);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	if (markerVisible)
	{
		// We want the marker to have odd width, so that the arrow tip is one pixel wide.
		auto locMarkerWidth = int(locationMarker.width() * (ratio / GUI_PIXMAPS_SCALE));
		if(locMarkerWidth % 2 == 0)
			++locMarkerWidth;
		const auto scaledMarker = locationMarker.scaledToWidth(locMarkerWidth, Qt::SmoothTransformation);

		const auto [markerCenterPosX, markerCenterPosY] = lonLatToMapPoint(markerLon, markerLat);

		for (int k = kMin; k <= kMax; ++k)
		{
			painter.drawPixmap(std::lround(markerCenterPosX + k * mapWidth) - scaledMarker.width()/2,
			                   std::lround(markerCenterPosY) - scaledMarker.height(),
			                   scaledMarker);
		}
	}

	if (searchRadius < 180)
	{
		if (searchAreaOutline.isEmpty())
			makeSearchAreaOutline(scaledMap.width(), scaledMap.height());

		for (int k = kMin; k <= kMax; ++k)
		{
			const QRect currMapRect(mainRect.topLeft() + k * horizShift, mainRect.size());
			painter.setClipRect(currMapRect);

			auto currPath = searchAreaOutline;
			currPath.translate(currMapRect.left() - mainRect.left(), 0);

			// Outline
			painter.setPen(QColor(0, 127, 255, 255));
			painter.setBrush(Qt::transparent);
			painter.setRenderHint(QPainter::Antialiasing);
			painter.drawPath(currPath);

			// Darken the areas that are filtered out
			auto path = currPath;
			path.addRect(currMapRect);
			painter.setPen(Qt::transparent);
			painter.setBrush(QColor(0,0,0,127));
			painter.drawPath(path);
		}
	}
}

void MapWidget::resizeEvent(QResizeEvent* event)
{
	updateScaledMapAndRect();
}

void MapWidget::updateScaledMapAndRect()
{
	const double ratio = devicePixelRatioF();
	const auto virtualSize = height() * QSize(2, 1) * (ratio * zoom);
	QSize realSize;
	if (virtualSize.width() > map.width() || virtualSize.height() > map.height())
		realSize = map.size();
	else
		realSize = virtualSize;

	if (scaledMap.size() != realSize)
	{
		// QPixmap::scaled() gives higher quality of resampling than QPainter::drawPixmap()
		scaledMap = map.scaled(realSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	while (shift.x() + currentDragShift.x() > virtualSize.width())
		shift.setX(shift.x() - virtualSize.width());
	while (shift.x() + currentDragShift.x() < -virtualSize.width())
		shift.setX(shift.x() + virtualSize.width());

	auto topLeft = QPointF(std::round(( width()*ratio-virtualSize.width ()) / 2.),
	                       std::round((height()*ratio-virtualSize.height()) / 2.));
	topLeft += shift + currentDragShift;

	const auto topLeftBeforeFix = topLeft;
	if (topLeft.y() > 0)
		topLeft.setY(0);
	else if (topLeft.y() + virtualSize.height() < height())
		topLeft.setY(height() - virtualSize.height());
	shift += topLeft - topLeftBeforeFix;

	mapRect = QRectF(topLeft, virtualSize);

	searchAreaOutline = {};
	update();
}
