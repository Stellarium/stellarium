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
}

MapWidget::MapWidget(QWidget* parent)
	: QWidget(parent)
	, map(":/graphicGui/miscWorldMap.jpg")
	, locationMarker(":/graphicGui/uieMapPointer.png")
	, markerVisible(true)
{
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

void MapWidget::mousePressEvent(QMouseEvent* event)
{
	const double ratio = devicePixelRatioF();
	const auto pos = QPointF(event->pos()) * ratio;

	if(!mapRect.contains(pos))
		return;

	const auto lat = (pos.y() - mapRect.center().y()) / mapRect.height() * -180;
	const auto lon = (pos.x() - mapRect.center().x()) / mapRect.width()  * 360;

	StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
	QColor color=locationMgr->getColorForCoordinates(lon, lat);

	emit positionChanged(lon, lat, color);
}

void MapWidget::setMap(const QPixmap &map)
{
	this->map = map;
	update();
}

void MapWidget::makeSearchAreaOutline(const int width, const int height)
{
	constexpr double nMax = 128;
	std::vector<QPointF> points;
	for (double n = 0; n < nMax; ++n)
	{
		const auto bearing = n / nMax * 360;
		const auto point3d = StelLocation::pointAtDistanceDegrees(searchCenterLon, searchCenterLat, searchRadius, bearing);
		double lon, lat;
		StelUtils::rectToSphe(&lon, &lat, point3d);
		lon *= M_180_PI;
		lat *= M_180_PI;

		const auto x = mapRect.left() + (lon + 180) /  360. * width;
		const auto y = mapRect.top()  + (lat -  90) / -180. * height;
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

	// QPixmap::scaled() gives higher quality of resampling than QPainter::drawPixmap()
	const auto scaledMap = map.scaled(size() * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	mapRect = QRectF(QPointF(std::round(( width()*ratio-scaledMap.width ()) / 2.),
							 std::round((height()*ratio-scaledMap.height()) / 2.)),
					 scaledMap.size());

	painter.setClipRect(mapRect);
	painter.drawPixmap(mapRect.topLeft(), scaledMap);

	if (markerVisible)
	{
		// We want the marker to have odd width, so that the arrow tip is one pixel wide.
		auto locMarkerWidth = int(locationMarker.width() * (ratio / GUI_PIXMAPS_SCALE));
		if(locMarkerWidth % 2 == 0)
			++locMarkerWidth;
		const auto scaledMarker = locationMarker.scaledToWidth(locMarkerWidth, Qt::SmoothTransformation);

		const auto markerCenterPosX = std::lround(mapRect.left() + (markerLon + 180) /  360. * scaledMap.width() );
		const auto markerCenterPosY = std::lround(mapRect.top()  + (markerLat -  90) / -180. * scaledMap.height());

		painter.drawPixmap(markerCenterPosX - scaledMarker.width()/2,
				   markerCenterPosY - scaledMarker.height(),
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
	searchAreaOutline = {};
}
