#ifndef UPDATESIGNALLINGITEM_HPP
#define UPDATESIGNALLINGITEM_HPP

#include <QGraphicsItem>

class MobileGui;

//! @class UpdateSignallingItem
//! This class uses its paint() to fire off a signal indicating that it's time to update
//! it does absolutely nothing else.
class UpdateSignallingItem : public QGraphicsItem
{
public:
	explicit UpdateSignallingItem(QGraphicsItem * parent, MobileGui *gui);

	void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
	virtual QRectF boundingRect() const { return QRectF(0, 0, 1, 1); }

private:
	MobileGui * gui;
};

#endif // UPDATESIGNALLINGITEM_HPP
