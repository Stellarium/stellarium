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
