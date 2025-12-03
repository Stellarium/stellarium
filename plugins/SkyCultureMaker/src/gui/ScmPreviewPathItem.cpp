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

#include "ScmPreviewPathItem.hpp"
#include <qpen.h>
#include <qstyleoption.h>

ScmPreviewPathItem::ScmPreviewPathItem()
	: QGraphicsPathItem()
{
	QPen pen;
	pen.setColor((QColor(255, 0, 0, 255)));
	pen.setStyle(Qt::DotLine);
	pen.setWidth(0);
	setPen(pen);

	setBrush(QBrush(QColor(255, 50, 50, 50)));
}

void ScmPreviewPathItem::setFirstPoint(QPointF point)
{
	// init path (path is required to have 3 elements by setMousePoint and setLastPoint)
	currentPath.moveTo(point);
	currentPath.lineTo(point + QPointF(1, 1));
	currentPath.lineTo(point);
	setPath(currentPath);
}

void ScmPreviewPathItem::setMousePoint(QPointF point)
{
	currentPath = path();
	currentPath.setElementPositionAt(1, point.x(), point.y());
	setPath(currentPath);
}

void ScmPreviewPathItem::setLastPoint(QPointF point)
{
	currentPath = path();
	currentPath.setElementPositionAt(2, point.x(), point.y());
	setPath(currentPath);
}

void ScmPreviewPathItem::reset()
{
	currentPath.clear();
	setPath(currentPath);
}
