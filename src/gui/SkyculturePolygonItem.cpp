#include "SkyculturePolygonItem.hpp"
#include <qstyleoption.h>


SkyculturePolygonItem::SkyculturePolygonItem(QString scId, int startTime, int endTime)
	: QGraphicsPolygonItem()
	, skycultureId(scId)
	, startTime(startTime)
	, endTime(endTime)
	, isHovered(false)
{
	setFlag(QGraphicsItem::ItemIsSelectable);
	//setAcceptHoverEvents(true);
	setToolTip(skycultureId);
	setAcceptHoverEvents(true);

	// default ruby shape for testing
	setPolygon(QPolygonF(QList<QPoint>{QPoint(600.0, 600.0), QPoint(620.0, 620.0), QPoint(620.0, 640.0), QPoint(600.0, 660.0), QPoint(580.0, 640.0), QPoint(580.0, 620.0), QPoint(600.0, 600.0)}));
	setPen(QPen(Qt::yellow));
	setBrush(QBrush(QColor(255, 0, 0, 100)));
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

void SkyculturePolygonItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QStyleOptionGraphicsItem opt = *option;

	if (isSelected()) {
		setPen(QPen(Qt::blue, 2));
		opt.state = QStyle::State_None; // prevent dashed bounding rect
	} else {
		setPen(QPen(Qt::black, 2));
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
