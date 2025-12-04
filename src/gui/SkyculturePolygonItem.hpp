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

#ifndef SKYCULTUREPOLYGONITEM_HPP
#define SKYCULTUREPOLYGONITEM_HPP

#include <QGraphicsPolygonItem>

//! @class SkyculturePolygonItem
//! Simple QGraphicsPolygonItem defined by a polygon, startTime and EndTime.
class SkyculturePolygonItem : public QGraphicsPolygonItem
{

public:
	SkyculturePolygonItem(QString skycultureId, int startTime, int endTime);

	const QString& getSkycultureId() const { return skycultureId; }

	int getStartTime() const {return startTime;}
	int getEndTime() const {return endTime;}
	void setDefaultBrushColor(const QColor color) {defaultBrushColor = color;}
	void setSelectedBrushColor(const QColor color) {selectedBrushColor = color;}
	void setDefaultPenColor(const QColor color) {defaultPenColor = color;}
	void setSelectedPenColor(const QColor color) {selectedPenColor = color;}

	void setSelectionState(bool newSelectionState);
	bool existsAtPointInTime(int year) const;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

public slots:

signals:

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
	QString skycultureId;
	int startTime;
	int endTime;
	bool lastSelectedState;
	bool isHovered;
	QColor defaultBrushColor;
	QColor selectedBrushColor;
	QColor defaultPenColor;
	QColor selectedPenColor;

};

#endif // SKYCULTUREPOLYGONITEM_HPP
