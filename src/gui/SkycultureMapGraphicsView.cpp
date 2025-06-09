#include "SkycultureMapGraphicsView.hpp"
#include "SkyculturePolygonItem.hpp"
#include <qjsonarray.h>
#include <qtimeline.h>
#include <qgraphicssvgitem.h>

#include <QJsonObject>
#include <QJsonDocument>

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


	// QGraphicsPixmapItem *baseMap = scene->addPixmap(QPixmap(":/graphicGui/miscWorldMap.jpg"));
	// baseMap->setTransformationMode(Qt::SmoothTransformation);

	QGraphicsSvgItem *baseMap = new QGraphicsSvgItem(":/graphicGui/baseMap_compressed.svgz");
	scene->addItem(baseMap);
	qInfo() << "basemap width = " << baseMap->boundingRect().width() << " and height = " << baseMap->boundingRect().height();

	//scene->addRect(300.0, 600.0, 400.0, 200.0, QPen(Qt::black), QBrush(Qt::yellow));
	//scene->addRect(0.0, 0.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(Qt::blue));
	//scene->addRect(250.0, 250.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(QColor(255, 0, 0, 100)));

	SkyculturePolygonItem *lokono_early = new SkyculturePolygonItem("Lokono", 550, 1560);
	lokono_early->setPolygon(QPolygonF(QList<QPoint>{QPoint(800.0, 800.0), QPoint(840.0, 800.0), QPoint(880.0, 840.0), QPoint(880.0, 880.0),
												  QPoint(840.0, 920.0), QPoint(800.0, 920.0), QPoint(760.0, 880.0), QPoint(760.0, 840.0)}));

	SkyculturePolygonItem *lokono_late = new SkyculturePolygonItem("Lokono", 1561, 2014);
	lokono_late->setPolygon(QPolygonF(QList<QPoint>{QPoint(900.0, 500.0), QPoint(940.0, 500.0), QPoint(980.0, 540.0), QPoint(980.0, 580.0),
													 QPoint(940.0, 620.0), QPoint(900.0, 620.0), QPoint(860.0, 580.0), QPoint(860.0, 540.0)}));

	SkyculturePolygonItem *aztec = new SkyculturePolygonItem("Aztekisch", 1000, 1200);

	SkyculturePolygonItem *tupi_test = new SkyculturePolygonItem("Tupi", -100, 100);
	auto tupiPolyList = QList<QPointF>();

	// umrechnung lon in x:
	//lon + 180 / 360 = faktor (z.B. 0 = 0.5)  -->  width of map (boundingRect) * faktor
	//test_width = lokono_early->boundingRect().width();

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
	//scene->addLine(0, 0, 0, baseMap->boundingRect().height(), QPen(Qt::red, 20));
	//scene->addLine(mitte, 0, mitte, heighte, QPen(Qt::red, 20));

	const QString filePath = StelFileMgr::findFile("skycultures/tupi/tupi_coords.geojson");
	if (filePath.isEmpty())
	{
		qCritical() << "Failed to * find *" << " [tupi polygon coordinate] " << "file in sky culture directory";
	}
	else
	{
		QFile file(filePath);
		if (!file.open(QFile::ReadOnly))
		{
			qCritical() << "Failed to * open *" << " [tupi polygon coordinate] " << "file in sky culture directory";
		}
		else
		{
			const auto jsonText = file.readAll();
			if (jsonText.isEmpty())
			{
				qCritical() << "Failed to read data from" << " [tupi polygon coordinate] " << "file in sky culture directory";
			}
			else
			{
				QJsonParseError error;
				const auto jsonDoc = QJsonDocument::fromJson(jsonText, &error);

				if (error.error != QJsonParseError::NoError)
				{
					qCritical().nospace() << "Failed to parse " << " [tupi polygon coordinate] " << " from sky culture directory " << ": " << error.errorString();
				}
				else
				{
					if (!jsonDoc.isObject())
					{
						qCritical() << "Failed to find the expected JSON structure in" << " [tupi polygon coordinate] " << " from sky culture directory";
					}
					else
					{
						const auto data = jsonDoc.object();
						qInfo() << "json object keys: " << data.keys();
						qInfo() << "data[features].toString(): " << data["features"].toString();
						qInfo() << "data[features][type].toString(): " << data["features"]["type"].toString();
						const auto feattures = data["features"];
						if (data["features"].isArray())
						{
							auto featureArray = data["features"].toArray(); // auto = QJasonArray --> header inlcude
							auto geoArray = featureArray[0].toObject().value("geometry").toArray();
							qInfo() << "geoArray size: " << geoArray.size();
							qInfo() << "geoArray[0]: " << geoArray[0];

							//tupi_test->setPolygon(QPolygonF(QList<QPoint>{QPoint(800.0, 800.0), QPoint(840.0, 800.0), QPoint(880.0, 840.0), QPoint(880.0, 880.0),
							//												 QPoint(840.0, 920.0), QPoint(800.0, 920.0), QPoint(760.0, 880.0), QPoint(760.0, 840.0)}));
							for(auto i : geoArray)
							{
								auto pointArray = i.toArray();
								qInfo() << "values: x = " << pointArray[0].toDouble() << " and y = " << pointArray[1].toDouble();
								tupiPolyList.append(QPointF(pointArray[0].toDouble(), pointArray[1].toDouble()));
							}
							//qInfo() << "featureArray size: " << featureArray;
							//qInfo() << "featureArray coordinates: " << featureArray[0].toObject().value("geometry").toArray().size();
						}
						//qInfo() << "features len: " << data["features"]["type"].toString().length();
						//qInfo() << "features[id].toString: " << data["features"].toString();
					}
				}
			}
		}
	}

	tupi_test->setPolygon(QPolygonF(convertLatLonToMeter(tupiPolyList, baseMap->boundingRect().width(), baseMap->boundingRect().height())));
	scene->addItem(tupi_test);

	scene->addItem(lokono_early);
	scene->addItem(lokono_late);
	scene->addItem(aztec);

	updateTime(0);
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
	qInfo() << "current mouse pos --> raw: " << event->pos() << " mapToScene: " << mapToScene(event->pos());

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
	// calculate requested zoom before executing the zoom operation to limit the min / max zoom level
	const double scaling = transform().scale(factor, factor).mapRect(QRectF(0, 0, 1, 1)).width();

	if (scaling < 0.2 || scaling > 40.0) // min / max zoom level
		return;

	scale(factor, factor);
}

QList<QPointF> SkycultureMapGraphicsView::convertLatLonToMeter(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight)
{
	QList<QPointF> meter_coords;

	qInfo() << "=== convert latlon to meter ===";

	qInfo() << "width: " << mapWidth << " height: " << mapHeight;

	for(auto point : irl)
	{
		// default extent:
		// x (lon) --> -180.0 | 180.0 --> 360.0
		// y (lat) --> - 90.0 |  90.0 --> 180.0

		// cropped extent:
		// x (lon) --> -180.0 | 180.0    --> 360.0
		// y (lat) --> - 90.0 |  83.6236 --> 173.6236

		// y  --> - 7538976.9896 | 18418386.3091
		// y  --> - 31.98162445  | 78.13393 --> 110.11555445 (wenn max lat = 85.06)
		// y  --> - 33.8425396  | 82.68031177 --> 116.52285 (wenn max lat = 90.0)

		//start = 83.6236 --> 0 --> ???

		qInfo() << "Punkt x: " << point.x() << " y: " << point.y();
		qreal xMeter = (point.x() * 20037508.34) / 180.0;;
		qreal yMeter = ((qLn(qTan(((90.0 + point.y())* M_PI) / 360.0)) / (M_PI / 180.0)) * 20037508.34) / 180.0;
		qInfo() << "berechnete x: " << xMeter << " y: " << yMeter;

		meter_coords.append(QPointF(xMeter, yMeter));
	}

	return convertMeterToView(meter_coords, mapWidth, mapHeight);
}

QList<QPointF> SkycultureMapGraphicsView::convertMeterToView(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight)
{
	QList<QPointF> view_coords;

	qInfo() << "=== convert meter to view ===";

	qInfo() << "width: " << mapWidth << " height: " << mapHeight;

	for(auto point : irl)
	{
		// EPSG 3857 WGS 84 extent:
		// x: - 20037508.34 | 20037508.34 (corresponds to -180.0 to 180.0 in WGS84 bounds) --> 40,075,016.68
		// y: - 20048966.10 | 20048966.10 (corresponds to -85.05112878 to 85.05112878 in WGS84 bounds) --> 40,097,932.20

		// cropped EPSG 3857 WGS 84 extent:
		// x: - 20037507.0672 | 20037507.0672 (corresponds to -180.0 to 180.0 in WGS84 bounds) --> 40,075,014.1344
		// y: -  7538976.9896 | 18418386.3091 (corresponds to - 90.0 to  83.6236 in WGS84 bounds) --> 25,957,363.2987



		qInfo() << "Punkt x: " << point.x() << " y: " << point.y();
		qreal xView = ((point.x() + 20037507.0672) / 40075014.1344) * mapWidth;
		qreal yView = ((point.y() - 18418386.3091) / -25957363.2987) * mapHeight;
		qInfo() << "berechnete x: " << xView << " y: " << yView;
		view_coords.append(QPointF(xView, yView));
	}
	qInfo() << "pre return --> liste: " << view_coords;

	return view_coords;
}

void SkycultureMapGraphicsView::selectCulture(const QString &skycultureId)
{
	qInfo() << "beginning of selectCulture!";
	// variable to the best fitting polygon (either one that exists at the current year or the one with the earliest startTime)
	SkyculturePolygonItem *skyCulturePolygon = nullptr;

	// determine the right culture polygon from its name and time
	for(auto *item : scene()->items())
	{
		// cast QGraphicsItem to SkyculturePolygonItem --> if the item is not a SkyculturePolygonItem: skip it to prevent errors
		SkyculturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyculturePolygonItem *>(item);
		if(!scPolyItem)
		{
			continue;
		}
		if(skycultureId == scPolyItem->getSkycultureId())
		{
			// if there is an polygon in the current time --> safe it and continue
			if(scPolyItem->existsAtPointInTime(currentYear))
			{
				skyCulturePolygon = scPolyItem;

				break;
			}
			// otherwise safe the polygon with the earliest startTime
			else
			{
				if(skyCulturePolygon != nullptr)
				{
					if(skyCulturePolygon->getStartTime() < scPolyItem->getStartTime())
					{
						continue;
					}
				}
				skyCulturePolygon = scPolyItem;
			}
		}
	}

	// if no fitting polygon was found --> return to prevent errors
	if(skyCulturePolygon == nullptr)
	{
		qInfo() << "couldn't find any polygon with name [" << skycultureId << "]!";
		return;
	}

	// if needed change the current year and update the polygon visibility
	if(!skyCulturePolygon->existsAtPointInTime(currentYear))
	{
		// signal connects to updateSkyCultureTime in ViewDialog which invokes updateTime (in this class)
		emit(timeChanged(skyCulturePolygon->getStartTime()));
	}

	// select the new culture
	scene()->clearSelection();
	skyCulturePolygon->setSelected(true);

	// start zoom (async?) to polygon and set TimeSlider to correct time (startTime of selected polygon?)
	//fitInView(skyCulturePolygon->boundingRect(), Qt::KeepAspectRatio); --> funktioniert aber sehr passgenau
	// x = x - 1/4 width, y = y - 1/4 height, width = width * 1.5, height = height * 1.5 --> entweder dynamisch oder min (z.B. 100 oder so)
	const QRectF polyBbox = skyCulturePolygon->boundingRect();

	// qInfo() << "pre eval width: " << polyBbox.width() << " height: " << polyBbox.height();
	// int width = (polyBbox.width() < 130) ? 130 : polyBbox.width();
	// int height = (polyBbox.height() < 130) ? 130 : polyBbox.height();
	// qInfo() << "post eval width: " << width << " height: " << height;

	fitInView(polyBbox.x() - polyBbox.width() / 4, polyBbox.y() - polyBbox.height() / 4, polyBbox.width() * 1.5, polyBbox.height() * 1.5, Qt::KeepAspectRatio);
}

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

		// if cast was unsuccessful (item is not an SkyculturePolygonItem) --> look at the next item
		if(!scPolyItem)
			continue;

		// if the current year is between the start and end time of the polygon --> show (otherwise hide the item)
		if(scPolyItem->existsAtPointInTime(currentYear))
		{
			scPolyItem->setVisible(true);
		}
		else
		{
			scPolyItem->setVisible(false);
		}
	}
}
