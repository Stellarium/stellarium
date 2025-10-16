#include "PreviewPolygonItem.hpp"
#include <qgraphicsscene.h>
#include <qpen.h>
#include <qstyleoption.h>


PreviewPolygonItem::PreviewPolygonItem(bool isTemporary)
	: QGraphicsPolygonItem()
	, isTemporary(isTemporary)
{
	initialize();
}

PreviewPolygonItem::PreviewPolygonItem(int startTime, int endTime)
	: QGraphicsPolygonItem()
	, startTime(startTime)
	, endTime(endTime)
{
	initialize();
}

PreviewPolygonItem::PreviewPolygonItem(int startTime, int endTime, const QPolygonF &polygon)
	: QGraphicsPolygonItem()
	, startTime(startTime)
	, endTime(endTime)
{
	initialize();
	setPolygon(polygon);
}


void PreviewPolygonItem::initialize()
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

bool PreviewPolygonItem::existsAtPointInTime(int year) const
{
	if (isTemporary)
	{
		return true;
	}
	else
	{
		return startTime <= year and endTime >= year;
	}
}
