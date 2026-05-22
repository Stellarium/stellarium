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

QPointF MapWidget::mapPointToLonLat(const QPointF& mapPoint) const
{
	const auto lat = (mapPoint.y() - mapRect.center().y()) / mapRect.height() * -180;
	const auto lon = (mapPoint.x() - mapRect.center().x()) / mapRect.width()  * 360;
	return {lon, lat};
}

QPointF MapWidget::lonLatToMapPoint(const double lon, const double lat) const
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
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) return;

	const auto pos = position(event, devicePixelRatioF());
	currentDragShift = pos - dragStart;
	shift += currentDragShift;
	const bool moved = currentDragShift.x() != 0 || currentDragShift.y() != 0;
	currentDragShift = QPointF(0,0);
	if (moved)
	{
		updateScaledMapAndRect();
		return;
	}

	if(!mapRect.contains(pos))
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
}

void MapWidget::setMap(const QPixmap &map)
{
	this->map = map;
	updateScaledMapAndRect();
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
		searchAreaOutline.moveTo(mapRect.topRight());
		searchAreaOutline.lineTo(mapRect.topLeft());
		std::sort(points.begin(), points.end(),
		          [](auto& a, auto& b){ return a.x() < b.x(); });
		searchAreaOutline.lineTo(mapRect.left(), points.front().y());
		for (const auto& p : points)
			searchAreaOutline.lineTo(p);
		searchAreaOutline.lineTo(mapRect.right(), points.back().y());
		searchAreaOutline.closeSubpath();
	}
	else if (searchCenterLat - searchRadius < -90)
	{
		searchAreaOutline.moveTo(mapRect.bottomRight());
		searchAreaOutline.lineTo(mapRect.bottomLeft());
		std::sort(points.begin(), points.end(),
		          [](auto& a, auto& b){ return a.x() < b.x(); });
		searchAreaOutline.lineTo(mapRect.left(), points.front().y());
		for (const auto& p : points)
			searchAreaOutline.lineTo(p);
		searchAreaOutline.lineTo(mapRect.right(), points.back().y());
		searchAreaOutline.closeSubpath();
	}
	else
	{
		// Handle possible split over longitude ±180°
		std::vector<QPointF> pointsWest, pointsEast;
		bool lastPointWasWest;
		if (points[0].x() < mapRect.left() + width / 2.)
		{
			pointsWest.push_back(points[0]);
			lastPointWasWest = true;
		}
		else
		{
			pointsEast.push_back(points[0]);
			lastPointWasWest = false;
		}

		for (size_t n = 1; n < points.size(); ++n)
		{
			const auto& p = points[n];
			if (lastPointWasWest)
			{
				const auto& prevP = pointsWest.back();
				if (std::abs(p.x() - width - prevP.x()) < std::abs(p.x() - prevP.x()))
				{
					// Switching over to the east, need to make
					// the western path cross the left side.
					pointsWest.emplace_back(p.x() - width, p.y());
					// Also we need to make the eastern part
					// to continue from the right side.
					pointsEast.emplace_back(prevP.x() + width, prevP.y());
					lastPointWasWest = false;
					pointsEast.emplace_back(p);
				}
				else
				{
					pointsWest.emplace_back(p);
				}
			}
			else
			{
				const auto& prevP = pointsEast.back();
				if (std::abs(p.x() + width - prevP.x()) < std::abs(p.x() - prevP.x()))
				{
					// Switching over to the west, need to make
					// the eastern path cross the right side.
					pointsEast.emplace_back(p.x() + width, p.y());
					// Also we need to make the western part
					// to continue from the left side.
					pointsWest.emplace_back(prevP.x() - width, prevP.y());
					lastPointWasWest = true;
					pointsWest.emplace_back(p);
				}
				else
				{
					pointsEast.emplace_back(p);
				}
			}
		}

		if (!pointsWest.empty())
		{
			searchAreaOutline.moveTo(pointsWest[0]);
			for (const auto& p : pointsWest)
				searchAreaOutline.lineTo(p);
			searchAreaOutline.closeSubpath();
		}
		if (!pointsEast.empty())
		{
			searchAreaOutline.moveTo(pointsEast[0]);
			for (const auto& p : pointsEast)
				searchAreaOutline.lineTo(p);
			searchAreaOutline.closeSubpath();
		}
	}
}

void MapWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	const double ratio = devicePixelRatioF();
	painter.scale(1/ratio, 1/ratio); // Work in units of device pixels

	painter.setClipRect(mapRect);
	painter.drawPixmap(mapRect.toRect(), scaledMap);

	if (markerVisible)
	{
		// We want the marker to have odd width, so that the arrow tip is one pixel wide.
		auto locMarkerWidth = int(locationMarker.width() * (ratio / GUI_PIXMAPS_SCALE));
		if(locMarkerWidth % 2 == 0)
			++locMarkerWidth;
		const auto scaledMarker = locationMarker.scaledToWidth(locMarkerWidth, Qt::SmoothTransformation);

		const auto [markerCenterPosX, markerCenterPosY] = lonLatToMapPoint(markerLon, markerLat);

		painter.drawPixmap(std::lround(markerCenterPosX) - scaledMarker.width()/2,
		                   std::lround(markerCenterPosY) - scaledMarker.height(),
		                   scaledMarker);
	}

	if (searchRadius < 180)
	{
		if (searchAreaOutline.isEmpty())
			makeSearchAreaOutline(scaledMap.width(), scaledMap.height());
		// Outline
		painter.setPen(QColor(0, 127, 255, 255));
		painter.setBrush(Qt::transparent);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.drawPath(searchAreaOutline);

		// Darken the areas that are filtered out
		auto path = searchAreaOutline;
		path.addRect(mapRect);
		painter.setPen(Qt::transparent);
		painter.setBrush(QColor(0,0,0,127));
		painter.drawPath(path);
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
