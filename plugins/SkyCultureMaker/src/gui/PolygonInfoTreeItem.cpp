#include "PolygonInfoTreeItem.hpp"


PolygonInfoTreeItem::PolygonInfoTreeItem(int identifier, int startTime, int endTime, int vertexCount)
	: QTreeWidgetItem()
	, id(identifier)
{
	setText(0, QString::number(startTime));
	setText(1, QString::number(endTime));
	setText(2, QString::number(vertexCount));

	setText(3, "press to delete");
	setIcon(3, QIcon(":/graphicGui/uieCloseButton-hover.png"));

	setTextAlignment(0, Qt::AlignCenter);
	setTextAlignment(1, Qt::AlignCenter);
	setTextAlignment(2, Qt::AlignCenter);
	setTextAlignment(3, Qt::AlignCenter);
}

int PolygonInfoTreeItem::getId() const
{
	return id;
}
