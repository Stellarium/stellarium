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
	//setAcceptHoverEvents(true);
	setToolTip(skycultureId);
	setAcceptHoverEvents(true);

	// default ruby shape for testing
	setPolygon(QPolygonF(QList<QPoint>{QPoint(600.0, 600.0), QPoint(620.0, 620.0), QPoint(620.0, 640.0), QPoint(600.0, 660.0), QPoint(580.0, 640.0), QPoint(580.0, 620.0), QPoint(600.0, 600.0)}));

	setPen(QPen(QColor(200, 0, 0, 50)));
	setBrush(QBrush(QColor(255, 0, 0, 100)));
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
	qInfo() << "isHovered: " << isHovered;

	QGraphicsPolygonItem::hoverEnterEvent(event); // calls update()
}

void SkyculturePolygonItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	isHovered = false;
	qInfo() << "isHovered: " << isHovered;

	QGraphicsPolygonItem::hoverLeaveEvent(event); // calls update()
}

QVariant SkyculturePolygonItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	// prevent de-selection when hiding the item

	if (change == QGraphicsItem::ItemSelectedChange)
	{
		qInfo() << skycultureId << " SelectionChange triggered: " << value.toBool() << " and lastSelectedState: " << lastSelectedState;
		return lastSelectedState;
	}

	return QGraphicsPolygonItem::itemChange(change, value);
}

void SkyculturePolygonItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QStyleOptionGraphicsItem opt = *option;

	if (isSelected()) {
		setPen(QPen(QColor(0, 0, 200, 200), 0));
		setBrush(QBrush(QColor(0, 0, 255, 100)));
		opt.state = QStyle::State_None; // prevent dashed bounding rect
	} else {
		setPen(QPen(QColor(200, 0, 0, 200), 0));
	}

	if (isHovered)
	{
		setBrush(QBrush(QColor(255, 0, 0, 150)));
	}
	else
	{
		setBrush(QBrush(QColor(255, 0, 0, 100)));
	}

	QGraphicsPolygonItem::paint(painter, &opt, widget);
}
