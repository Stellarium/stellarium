#ifndef SKYCULTUREMAPGRAPHICSVIEW_HPP
#define SKYCULTUREMAPGRAPHICSVIEW_HPP

#include <QGraphicsView>

//! @class SkyCultureSelectionMapView
//! Special GraphicsView that shows a world map and several polygons
class SkycultureMapGraphicsView : public QGraphicsView
{
   Q_OBJECT

public:
   SkycultureMapGraphicsView(QWidget *parent = nullptr);

   // public functions


public slots:
   // slots

signals:
   // signals

protected:
   void wheelEvent(QWheelEvent *event) override;
   void mouseMoveEvent(QMouseEvent *event) override;
   void mousePressEvent(QMouseEvent *event) override;
   void mouseReleaseEvent(QMouseEvent *event) override;
   void scaleView(double scaleFactor);

private:
   // private functions
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
