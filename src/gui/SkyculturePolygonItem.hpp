#ifndef SKYCULTUREPOLYGONITEM_HPP
#define SKYCULTUREPOLYGONITEM_HPP

#include <QGraphicsPolygonItem>

class SkyculturePolygonItem : public QGraphicsPolygonItem
{

public:
	SkyculturePolygonItem(QString skycultureId, int startTime, int endTime);

	// public functions
	void setSelectionState(bool newSelectionState);
	const QString& getSkycultureId() const {return skycultureId;}
	int getStartTime() const {return startTime;}
	int getEndTime() const {return endTime;}
	bool existsAtPointInTime(int year) const;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

public slots:
	// slots

signals:
	// signals

protected:
	// protected functions
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
	// important culture data
	QString skycultureId;
	int startTime;
	int endTime;

	// utility for change of appearance at runtime
	bool lastSelectedState;
	bool isHovered;

};

#endif // SKYCULTUREPOLYGONITEM_HPP
