/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScmPreviewPolygonItem.hpp"
#include <qgraphicsscene.h>
#include <qpen.h>
#include <qstyleoption.h>


ScmPreviewPolygonItem::ScmPreviewPolygonItem(bool isTemporary)
	: QGraphicsPolygonItem()
	, isTemporary(isTemporary)
{
	initialize();
}

ScmPreviewPolygonItem::ScmPreviewPolygonItem(int startTime, int endTime)
	: QGraphicsPolygonItem()
	, isTemporary(false)
	, startTime(startTime)
	, endTime(endTime)
{
	initialize();
}

ScmPreviewPolygonItem::ScmPreviewPolygonItem(int startTime, int endTime, const QPolygonF &polygon)
	: QGraphicsPolygonItem()
	, isTemporary(false)
	, startTime(startTime)
	, endTime(endTime)
{
	initialize();
	setPolygon(polygon);
}


void ScmPreviewPolygonItem::initialize()
{
	QPen pen;
	QBrush brush;

	if (isTemporary)
	{
		pen.setColor((QColor(255, 0, 0, 255)));
		pen.setWidth(0);

		brush.setColor((QColor(255, 50, 50, 50)));
		brush.setStyle(Qt::SolidPattern);
	}
	else
	{
		pen.setColor((QColor(255, 206, 27, 255)));
		pen.setStyle(Qt::DotLine);
		pen.setWidth(0);

		brush.setColor((QColor(255, 206, 27, 100))); // coral: 255, 133, 89 | mustard: 255, 206, 27
		brush.setStyle(Qt::SolidPattern);
	}

	setPen(pen);
	setBrush(brush);
	setPolygon(QPolygonF());
}

bool ScmPreviewPolygonItem::existsAtPointInTime(int year) const
{
	if (isTemporary)
	{
		return true;
	}
	else
	{
		return (startTime <= year) && (endTime >= year);
	}
}
