#ifndef PREVIEWPOLYGONITEM_HPP
#define PREVIEWPOLYGONITEM_HPP

#include <QGraphicsPolygonItem>

class PreviewPolygonItem : public QGraphicsPolygonItem
{

public:
	PreviewPolygonItem(bool isTemporary);
	PreviewPolygonItem(int startTime, int endTime);
	PreviewPolygonItem(int startTime, int endTime, bool isTemporary);
	PreviewPolygonItem(int startTime, int endTime, const QPolygonF &polygon);

	// public functions
	int getStartTime() const {return startTime;}
	int getEndTime() const {return endTime;}
	bool existsAtPointInTime(int year) const;

public slots:
	// slots

signals:
	// signals

protected:
	// protected functions

private:
	bool isTemporary = false;
	int startTime;
	int endTime;

	// private functions
	void initialize();
};

#endif // PREVIEWPOLYGONITEM_HPP
