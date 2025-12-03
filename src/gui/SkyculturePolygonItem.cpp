/*
 * Stellarium
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

#include "SkyculturePolygonItem.hpp"
#include <qgraphicsscene.h>
#include <qpen.h>
#include <qstyleoption.h>

SkyculturePolygonItem::SkyculturePolygonItem(QString scId, int startTime, int endTime)
	: QGraphicsPolygonItem()
	, skycultureId(scId)
	, startTime(startTime)
	, endTime(endTime)
	, isHovered(false)
	, lastSelectedState(false)
{
	setFlag(QGraphicsItem::ItemIsSelectable);
	setToolTip(skycultureId);
	setAcceptHoverEvents(true);

	defaultBrushColor = QColor(255, 0, 0, 100);
	selectedBrushColor = QColor(255, 206, 27, 100);

	defaultPenColor = QColor(200, 0, 0, 50);
	selectedPenColor = QColor(255, 206, 27, 50);
}

void SkyculturePolygonItem::setSelectionState(bool newSelectionState)
{
	lastSelectedState = newSelectionState;
	setSelected(newSelectionState);
}

bool SkyculturePolygonItem::existsAtPointInTime(int year) const
{
	return startTime <= year and endTime >= year;
}

void SkyculturePolygonItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	isHovered = true;
	QGraphicsPolygonItem::hoverEnterEvent(event);
}

void SkyculturePolygonItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	isHovered = false;
	QGraphicsPolygonItem::hoverLeaveEvent(event);
}

QVariant SkyculturePolygonItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	// prevent de-selection when hiding the item

	if (change == QGraphicsItem::ItemSelectedChange)
	{
		return lastSelectedState;
	}

	return QGraphicsPolygonItem::itemChange(change, value);
}

void SkyculturePolygonItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QStyleOptionGraphicsItem opt = *option;

	// set brush / pen to default colors and overwrite it later if necessary
	setBrush(QBrush(defaultBrushColor));
	setPen(QPen(defaultPenColor, 0));

	if (isSelected()) {
		setBrush(QBrush(selectedBrushColor));
		setPen(QPen(selectedPenColor, 0));
		opt.state = QStyle::State_None; // prevent dashed bounding rect
	}

	if (isHovered)
	{
		QColor hoverBrushColor = brush().color();
		QColor hoverPenColor = pen().color();
		hoverBrushColor.setAlpha(hoverBrushColor.alpha() + 50);
		hoverPenColor.setAlpha(hoverPenColor.alpha() + 50);

		setBrush(QBrush(hoverBrushColor));
		setPen(QPen(hoverPenColor, 0));
	}

	QGraphicsPolygonItem::paint(painter, &opt, widget);
}
