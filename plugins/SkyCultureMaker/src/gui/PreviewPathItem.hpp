#ifndef PREVIEWPATHITEM_HPP
#define PREVIEWPATHITEM_HPP

#include <QGraphicsPathItem>

class PreviewPathItem : public QGraphicsPathItem
{

public:
	PreviewPathItem();

	// public functions
	void setFirstPoint(QPointF point);
	void setMousePoint(QPointF point);
	void setLastPoint(QPointF point);

	void reset();

public slots:
	// slots

signals:
	// signals

protected:
	// protected functions

private:
	QPainterPath currentPath;

};

#endif // PREVIEWPATHITEM_HPP
