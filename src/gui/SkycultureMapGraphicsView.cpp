#include "SkycultureMapGraphicsView.hpp"
#include "SkyculturePolygonItem.hpp"


SkycultureMapGraphicsView::SkycultureMapGraphicsView(QWidget *parent)
   :QGraphicsView(parent)
	, oldSkyCulture("")
{
   QGraphicsScene *scene = new QGraphicsScene(this);
   // scene->setItemIndexMethod(QGraphicsScene::NoIndex); // noIndex better when adding / removing many items
   setScene(scene);
   //setRenderHint(QPainter::Antialiasing); // maybe unnecessary for this project
   //setTransformationAnchor(AnchorUnderMouse);
   scale(qreal(0.5), qreal(0.5)); // default transformation (zoom)
   QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
   setMouseTracking(true);
   setDragMode(QGraphicsView::ScrollHandDrag);
   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   show();

   // add items (transfer to dedicated function later)

   scene->addPixmap(QPixmap(":/graphicGui/miscWorldMap.jpg"));
   scene->addRect(300.0, 600.0, 400.0, 200.0, QPen(Qt::black), QBrush(Qt::yellow));
   scene->addRect(0.0, 0.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(Qt::blue));
   scene->addRect(250.0, 250.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(QColor(255, 0, 0, 100)));

   SkyculturePolygonItem *poly_test = new SkyculturePolygonItem("Lokono");
   poly_test->setPolygon(QPolygonF(QList<QPoint>{QPoint(800.0, 800.0), QPoint(840.0, 800.0), QPoint(880.0, 840.0), QPoint(880.0, 880.0),
                                                  QPoint(840.0, 920.0), QPoint(800.0, 920.0), QPoint(760.0, 880.0), QPoint(760.0, 840.0)}));

   SkyculturePolygonItem *poly_test_2 = new SkyculturePolygonItem("Aztec");

   scene->addItem(poly_test);
   scene->addItem(poly_test_2);
}

void SkycultureMapGraphicsView::wheelEvent(QWheelEvent *event)
{
   qInfo() << "cursor: " << viewport()->cursor();

   auto itemList = scene()->selectedItems();
   qInfo() << itemList.length();
   for(auto *i : scene()->selectedItems()) {
      SkyculturePolygonItem *pol = qgraphicsitem_cast<SkyculturePolygonItem *>(i);
      if(!pol)
         continue;
      qInfo() << "obj: " << pol << " und name: " << pol->getSkycultureId();
   }

   // foreach(QGraphicsItem *item, scene()->selectedItems()) {
   //    SkyculturePolygonItem *pol = qgraphicsitem_cast<SkyculturePolygonItem *>(item);
   //    if(!pol)
   //       continue;

   //    qInfo() << "obj: " << pol << " und name: " << pol->getSkycultureId();
   // }

   scaleView(pow(2.0, event->angleDelta().y() / 240.0)); // faster scrolling = faster zoom
}

void SkycultureMapGraphicsView::mouseMoveEvent(QMouseEvent *event)
{

	// if(event->buttons() == Qt::LeftButton)
	// {
	//    //viewport()->setCursor(Qt::ClosedHandCursor); // QGuiApplication::overrideCursor()
	//    QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
	// }
	// else
	// {
	//    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
	//    //viewport()->setCursor(Qt::ArrowCursor);
	// }

	QGraphicsView::mouseMoveEvent(event);
}

void SkycultureMapGraphicsView::mousePressEvent(QMouseEvent *event)
{
	// safe the currently selected skyculture before de-selecting
	if(scene()->selectedItems().length() > 0)
	{
		const QString previousCulture = qgraphicsitem_cast<SkyculturePolygonItem *>(scene()->selectedItems()[0])->getSkycultureId();
	}

	// de-select all graphic items present in the scene so the cursor can be set for dragging
	scene()->clearSelection();

	QGraphicsView::mousePressEvent(event);

	// if no item is selected (no polygon is clicked) set the right cursor for drag-mode
	if(scene()->selectedItems().length() == 0)
	{
		QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
	}
	else
	{
		// get the skyculture identifier (QString) of the current selected SkyculturePolygonItem
		const QString currentSkyCulture = qgraphicsitem_cast<SkyculturePolygonItem *>(scene()->selectedItems()[0])->getSkycultureId();

		// if the current culture is the same as before --> do not emit the signal so that the skyCulture isn't updated unnecessarily
		if(currentSkyCulture != oldSkyCulture)
		{
			// emit the current skyCulture, so viewDialog can handle the change
			emit cultureSelected(currentSkyCulture);
			oldSkyCulture = currentSkyCulture;
		}
	}
}

void SkycultureMapGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
	QGraphicsView::mouseReleaseEvent(event);

	QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
}

void SkycultureMapGraphicsView::scaleView(double factor)
{
	const double scaling = transform().scale(factor, factor).mapRect(QRectF(0, 0, 1, 1)).width();
	if (scaling < 0.2 || scaling > 40.0) // min / max zoom level
		return;

	scale(factor, factor);
}

void SkycultureMapGraphicsView::scaleView(double scaleFactor)
{
   const double factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
   if (factor < 0.2 || factor > 40.0) // min / max zoom level
      return;

   scale(scaleFactor, scaleFactor);
}
