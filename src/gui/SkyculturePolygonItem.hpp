#ifndef SKYCULTUREPOLYGONITEM_HPP
#define SKYCULTUREPOLYGONITEM_HPP

#include <QGraphicsPolygonItem>
#include <QGraphicsView>

class SkyculturePolygonItem : public QGraphicsPolygonItem
{

public:
   SkyculturePolygonItem(QString skycultureId);

   // public functions
   QString getSkycultureId() const {return skycultureId;}
   void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

public slots:
   // slots

signals:
   // signals

protected:
   // protected functions
   void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
   void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
   QString skycultureId;
   bool isHovered;
};

#endif // SKYCULTUREPOLYGONITEM_HPP
