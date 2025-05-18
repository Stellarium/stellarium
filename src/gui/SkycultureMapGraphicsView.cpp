#include "SkycultureMapGraphicsView.hpp"
#include "SkyculturePolygonItem.hpp"
#include <qtimeline.h>


SkycultureMapGraphicsView::SkycultureMapGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
	, currentYear(0)
	, oldSkyCulture("")
{
	QGraphicsScene *scene = new QGraphicsScene(this);
	// scene->setItemIndexMethod(QGraphicsScene::NoIndex); // noIndex better when adding / removing many items
	setScene(scene);

	setRenderHint(QPainter::Antialiasing); // maybe unnecessary for this project
	setTransformationAnchor(AnchorUnderMouse);

	// when drawing the basemap picture it's important to zoom out far enough (otherwise the cultureListWidget Layout will be compressed)
	scale(qreal(0.3), qreal(0.3)); // default transformation (zoom)

	QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
	setMouseTracking(true);

	setDragMode(QGraphicsView::ScrollHandDrag);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	show();

	// add items (transfer to dedicated function later)


	// !!!  test basemap drawing  !!!


	// QFile file(":/graphicGui/test_coords.csv");
	// if(!file.exists()) {
	//    qInfo() << "file does not exist";
	// }
	// if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	// {
	//    qInfo() << "couldnt open file!";
	// }
	// else
	// {
	//    QTextStream inFileStream(&file);
	//    while (!inFileStream.atEnd()) {
	//       QString line = inFileStream.readLine();

	//       qInfo() << line;
	//       qInfo() << "0: " << line[0] << " 1: " << line[1] << " 2: " << line[2];
	//    }
	// }

	// QFile file(":/graphicGui/csv_welt_changedProjection_richtig.csv");
	// if(!file.exists()) {
	//    qInfo() << "file does not exist";
	// }
	// if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	// {
	//    qInfo() << "couldnt open file!";
	// }
	// else
	// {
	//    QTextStream inFileStream(&file);
	//    int counter = 0;
	//    while (!inFileStream.atEnd()) {
	//       QString line = inFileStream.readLine();
	//       counter++;
	//    }
	//    qInfo() << "number of lines: " << counter;
	// }


	// !!!  test basemap drawing  !!!


	QGraphicsPixmapItem *baseMap = scene->addPixmap(QPixmap(":/graphicGui/miscWorldMap.jpg"));
	baseMap->setTransformationMode(Qt::SmoothTransformation);

	scene->addRect(300.0, 600.0, 400.0, 200.0, QPen(Qt::black), QBrush(Qt::yellow));
	scene->addRect(0.0, 0.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(Qt::blue));
	scene->addRect(250.0, 250.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(QColor(255, 0, 0, 100)));

	SkyculturePolygonItem *poly_test = new SkyculturePolygonItem("Lokono", 550, 2500);
	poly_test->setPolygon(QPolygonF(QList<QPoint>{QPoint(800.0, 800.0), QPoint(840.0, 800.0), QPoint(880.0, 840.0), QPoint(880.0, 880.0),
												  QPoint(840.0, 920.0), QPoint(800.0, 920.0), QPoint(760.0, 880.0), QPoint(760.0, 840.0)}));

	SkyculturePolygonItem *poly_test_2 = new SkyculturePolygonItem("Aztekisch", 1000, 1200);

	// umrechnung lon in x:
	//lon + 180 / 360 = faktor (z.B. 0 = 0.5)  -->  width of map (boundingRect) * faktor
	//test_width = poly_test->boundingRect().width();

	//madagaskar(lat lon): unten -25.6246, 45.1646 | -24.9961, 47.1033 | -18.1623, 49.4327 | -17.6631, 49.5309 | -17.3732, 49.4139
	// QList<QPointF> irl_coords{QPointF(45.1646, -25.6246), QPointF(47.1033, -24.9961) , QPointF(49.4327, -18.1623) , QPointF(49.5309, -17.6631) , QPointF(49.4139, -17.3732)};
	// qInfo() << "pre addPolygon";
	// QGraphicsPolygonItem *crash_test = new QGraphicsPolygonItem();
	// crash_test->setPolygon(QPolygonF(convertIrlToView(irl_coords)));
	// crash_test->setBrush(QBrush(Qt::yellow));
	// crash_test->setPen(QPen(Qt::red, 0.25));
	// scene->addItem(crash_test);
	// qInfo() << "post addPolygon";

	// qreal mitte;
	// qreal heighte;
	// for(auto *items : scene->items()) {
	//    qInfo() << "typ: " << items->type();
	//    if(items->type() == QGraphicsPixmapItem::Type)
	//    {
	//       qInfo() << "hier!";
	//       mitte = items->boundingRect().width() / 2;
	//       heighte = items->boundingRect().height();
	//    }
	// }
	// scene->addLine(0, 0, 0, heighte, QPen(Qt::red, 20));
	// scene->addLine(mitte, 0, mitte, heighte, QPen(Qt::red, 20));

	scene->addItem(poly_test);
	scene->addItem(poly_test_2);
}

void SkycultureMapGraphicsView::wheelEvent(QWheelEvent *event)
{
	// qreal akt_width;
	// for(auto *items : scene()->items()) {
	//    SkyculturePolygonItem *pol = qgraphicsitem_cast<SkyculturePolygonItem *>(items);
	//    if(!pol)
	//       continue;
	//    if(pol->getSkycultureId() == "Lokono")
	//    {
	//       akt_width = pol->boundingRect().width();
	//       qInfo() << "festgelegt: " << test_width << " und aktuelle: " << akt_width;
	//    }
	// }


	auto itemList = scene()->selectedItems();
	qInfo() << itemList.length();
	for(auto *i : scene()->selectedItems()) {
		SkyculturePolygonItem *pol = qgraphicsitem_cast<SkyculturePolygonItem *>(i);
		if(!pol)
			continue;
		qInfo() << "obj: " << pol << " und name: " << pol->getSkycultureId() << " und startZeit: " << pol->getStartTime() << " und endZeit: " << pol->getEndTime();
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

QList<QPointF> SkycultureMapGraphicsView::convertIrlToView(const QList<QPointF> &irl)
{
	QList<QPointF> view_coords;

	qreal map_width;
	qreal map_height;
	for(auto *item : scene()->items()) {
		if(item->type() == QGraphicsPixmapItem::Type)
		{
			map_width = item->boundingRect().width();
			map_height = item->boundingRect().height();
			qInfo() << "width: " << map_width << " height: " << map_height;
		}
	}

	for(auto point : irl)
	{
		qInfo() << "Punkt x: " << point.x() << " y: " << point.y();
		qreal x = ((point.x() + 180.0) / 360.0) * map_width;
		qreal y = ((point.y() - 90.0) / -180.0) * map_height;
		qInfo() << "berechnete x: " << x << " y: " << y;
		view_coords.append(QPointF(x, y));
	}
	qInfo() << "pre return --> liste: " << view_coords;

	return view_coords;
}

// void selectCulture(const QString &skycultureId)
// {
//    // determine the right culture polygon from its name
//    qInfo() << "tets";
//    // start zoom (async?) to polygon and set TimeSlider to correct time (startTime of selected polygon?)

//    // select item
// }

void SkycultureMapGraphicsView::updateTime(int year)
{
	currentYear = year;
	updateCultureVisibility();
}

void SkycultureMapGraphicsView::updateCultureVisibility()
{
	// iterate over all polygons --> if currentTime is between startTime and endTime show, else hide
	for(auto *item : scene()->items()) {
		// cast generic QGraphicsItem to subclass SkyculturePolygonItem
		SkyculturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyculturePolygonItem *>(item);

		// if cast was unsuccessful (item is not an SkyculturePolygonItem) --> go for the next item
		if(!scPolyItem)
			continue;

		// if the current year is between the start and end time of the polygon --> show (otherwise hide the item)
		if(currentYear >= scPolyItem->getStartTime() and currentYear <= scPolyItem->getEndTime())
		{
			scPolyItem->setVisible(true);
		}
		else
		{
			scPolyItem->setVisible(false);
		}
	}
}
