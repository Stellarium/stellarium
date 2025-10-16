#ifndef POLYGONINFOTREEITEM_HPP
#define POLYGONINFOTREEITEM_HPP

#include <QTreeWidgetItem>

class PolygonInfoTreeItem : public QTreeWidgetItem
{

public:
	PolygonInfoTreeItem(int identifier, int startTime, int endTime, int vertexCount);

	// public functions
	int getId() const;

public slots:
	// slots

signals:
	// signals

protected:
	// protected functions

private:
	int id;
};

#endif // POLYGONINFOTREEITEM_HPP
