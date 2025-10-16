#include "PreviewPathItem.hpp"
#include <qpen.h>
#include <qstyleoption.h>

PreviewPathItem::PreviewPathItem()
	: QGraphicsPathItem()
{
	// dotted Pen

	QPen pen;
	pen.setColor((QColor(255, 0, 0, 255)));
	pen.setStyle(Qt::DotLine);
	pen.setWidth(0);
	setPen(pen);

	setBrush(QBrush(QColor(255, 50, 50, 50)));
}

void PreviewPathItem::setFirstPoint(QPointF point)
{
	// init path (path is required to have 3 elements by setMousePoint and setLastPoint)
	qInfo() << "post clear currentPath: " << currentPath;
	currentPath.moveTo(point);
	currentPath.lineTo(point + QPointF(1, 1));
	currentPath.lineTo(point);
	qInfo() << "post init currentPath: " << currentPath;
	setPath(currentPath);
}

void PreviewPathItem::setMousePoint(QPointF point)
{
	currentPath = path();
	currentPath.setElementPositionAt(1, point.x(), point.y());
	setPath(currentPath);
}

void PreviewPathItem::setLastPoint(QPointF point)
{
	currentPath = path();
	currentPath.setElementPositionAt(2, point.x(), point.y());
	setPath(currentPath);
}

void PreviewPathItem::reset()
{
	currentPath.clear();
	setPath(currentPath);
}
