#include "UpdateSignallingItem.hpp"
#include "MobileGui.hpp"

UpdateSignallingItem::UpdateSignallingItem(QGraphicsItem * parent, MobileGui *gui) :
	QGraphicsItem(parent), gui(gui)
{
}

void UpdateSignallingItem::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	Q_UNUSED(painter)
	Q_UNUSED(option)
	Q_UNUSED(widget)
	gui->updateGui();
}
