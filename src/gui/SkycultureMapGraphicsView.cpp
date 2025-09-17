#include "SkycultureMapGraphicsView.hpp"
#include "SkyculturePolygonItem.hpp"
#include <qjsonarray.h>
#include <qgraphicssvgitem.h>

#include <QJsonObject>
#include <QJsonDocument>

SkycultureMapGraphicsView::SkycultureMapGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
	, minYear(-2000)
	, maxYear(2000)
	, firstShow(true)
	, currentYear(0)
	, oldSkyCulture("")
{
	QGraphicsScene *scene = new QGraphicsScene(this);
	// scene->setItemIndexMethod(QGraphicsScene::NoIndex); // noIndex better when adding / removing many items
	setScene(scene);

	setRenderHint(QPainter::Antialiasing); // maybe unnecessary for this project
	setTransformationAnchor(AnchorUnderMouse);

	// when drawing the basemap picture it's important to zoom out far enough (otherwise the cultureListWidget Layout will be compressed)
	//scale(qreal(0.3), qreal(0.3)); // default transformation (zoom)

	QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
	setMouseTracking(true);

	setDragMode(QGraphicsView::ScrollHandDrag);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	show();

	// set up QTimelines for smooth zoom on culture selection through public 'selectCulture' slot
	zoomToDefaultTimer.setDuration(2000);
	zoomOnTargetTimer.setDuration(3000);

	zoomToDefaultTimer.setUpdateInterval(20);
	zoomOnTargetTimer.setUpdateInterval(20);

	connect(&zoomToDefaultTimer, &QTimeLine::valueChanged, this, &SkycultureMapGraphicsView::zoomToDefault);
	connect(&zoomToDefaultTimer, &QTimeLine::finished, &zoomOnTargetTimer, &QTimeLine::start);
	connect(&zoomOnTargetTimer, &QTimeLine::valueChanged, this, &SkycultureMapGraphicsView::zoomOnTarget);

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
	scene->setSceneRect(- baseMap->boundingRect().width() * 0.75, - baseMap->boundingRect().height() * 0.5, baseMap->boundingRect().width() * 2.5, baseMap->boundingRect().height() * 2);
	qInfo() << "basemap width = " << baseMap->boundingRect().width() << " and height = " << baseMap->boundingRect().height();
	this->defaultRect = baseMap->boundingRect();

	//scene->addRect(- 500.0, - 500.0, 200.0, 200.0, QPen(Qt::black), QBrush(Qt::yellow));
	//scene->addRect(0.0, 0.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(Qt::blue));
	//scene->addRect(250.0, 250.0, 500.0, 500.0, QPen(Qt::yellow), QBrush(QColor(255, 0, 0, 100)));

	SkyculturePolygonItem *lokono_early = new SkyculturePolygonItem("Lokono", 550, 1560);
	lokono_early->setPolygon(QPolygonF(QList<QPoint>{QPoint(800.0, 800.0), QPoint(840.0, 800.0), QPoint(880.0, 840.0), QPoint(880.0, 880.0),
												  QPoint(840.0, 920.0), QPoint(800.0, 920.0), QPoint(760.0, 880.0), QPoint(760.0, 840.0)}));

	SkyculturePolygonItem *lokono_late = new SkyculturePolygonItem("Lokono", 1561, 2014);
	lokono_late->setPolygon(QPolygonF(QList<QPoint>{QPoint(900.0, 500.0), QPoint(940.0, 500.0), QPoint(980.0, 540.0), QPoint(980.0, 580.0),
													 QPoint(940.0, 620.0), QPoint(900.0, 620.0), QPoint(860.0, 580.0), QPoint(860.0, 540.0)}));

	SkyculturePolygonItem *aztec = new SkyculturePolygonItem("Aztekisch", 1000, 1200);

	SkyculturePolygonItem *tupi_test = new SkyculturePolygonItem("Tupi-Guarani", -100, 100);
	auto tupiPolyList = QList<QPointF>();

	// load culture Polygon from (geo)JSON

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
								//qInfo() << "values: x = " << pointArray[0].toDouble() << " and y = " << pointArray[1].toDouble();
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

	// workaround needed to preserve the correct component sizes (without the 'scale' operation the culturesListWidget is being squished and the culture names are not readable)
	//scale(0.2, 0.2); // empirically determined value
	//smoothFitInView(baseMap->boundingRect()); // reusing the smooth zoom feature from culture selection, in theory only the first part (zoom to default) is needed
	targetRect = baseMap->boundingRect();
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
	qInfo() << "current transformation" << transform();

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
void SkycultureMapGraphicsView::showEvent(QShowEvent *event)
{
	// fit the base map to the current view when the widget is first shown
	// (This cannot be done beforehand because the calculation is based on the current size of the viewPort.
	// The viewPort is smaller than it should be until the first show because the widget is located on the stackedWidget in ViewDialog.
	// The boolean firstShow is used so the map doesn't reset itself every time the user changes pages or closes the ViewDialog.)
	if (firstShow)
	{
		qreal ratio = calculateScaleRatio(defaultRect.width(), defaultRect.height());
		scale(ratio, ratio);
		centerOn(defaultRect.center());
		firstShow = false;
	}

	QGraphicsView::showEvent(event);
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

		// qInfo() << "Punkt x: " << point.x() << " y: " << point.y();
		qreal xMeter = (point.x() * 20037508.3427892439067363739014) / 180.0;;
		qreal yMeter = ((qLn(qTan(((90.0 + point.y())* M_PI) / 360.0)) / (M_PI / 180.0)) * 20037508.3427892439067363739014) / 180.0;
		// qInfo() << "berechnete x: " << xMeter << " y: " << yMeter;

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
		// x: - 20037507,0671618431806564 | 20040258.7695968560874462 --> 40,077,765.8367586992681026
		// x: - 20037507,0671618431806564 | 20037507,0671618431806564 --> 40,075,014.1343236863613128
		//		20037508,3427892439067363739014 * 2 = 40,075,016.6855784878134727478028
		// y: -  7538976.9896 | 18418386.3091 (corresponds to - 90.0 to  83.6236 in WGS84 bounds) --> 25,957,363.2987

		// lat extent:
		// without Antarctica: -7538976.9895804952830076 | 18418386.3090785145759583 --> 25957363.298659009859
		// with Antarctica:    -20615645,00034497305751  | 18418386,3090785145759583 --> 39034031.30942348763347



		//qInfo() << "Punkt x: " << point.x() << " y: " << point.y();
		qreal xView = ((point.x() + 20037507.0671618431806564) / 40075014.1343236863613128) * mapWidth;
		qreal yView = ((point.y() - 18418386.3090785145759583) / -39034031.3094234876334668) * mapHeight;
		//qInfo() << "berechnete x: " << xView << " y: " << yView;
		view_coords.append(QPointF(xView, yView));
	}
	qInfo() << "pre return --> liste: " << view_coords;

	return view_coords;
}

void SkycultureMapGraphicsView::selectAllCulturePolygon(const QString &skycultureId)
{
	for (const auto &item : scene()->items())
	{
		SkyculturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyculturePolygonItem *>(item);
		if (!scPolyItem)
		{
			continue;
		}

		scPolyItem->setSelectionState(false);

		if (skycultureId == scPolyItem->getSkycultureId())
		{
			// items can't be selected when hidden
			if (!scPolyItem->isVisible())
			{
				scPolyItem->setVisible(true);
				scPolyItem->setSelectionState(true);
				scPolyItem->setVisible(false);
			}
			else
			{
				scPolyItem->setSelectionState(true);
			}
		}
	}
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
		emit(timeValueChanged(skyCulturePolygon->getStartTime()));
	}

	// select the new culture
	//scene()->clearSelection();
	//skyCulturePolygon->setSelected(true);
	selectAllCulturePolygon(skycultureId);

	// start zoom (async?) to polygon and set TimeSlider to correct time (startTime of selected polygon?)
	//fitInView(skyCulturePolygon->boundingRect(), Qt::KeepAspectRatio); --> funktioniert aber sehr passgenau
	// x = x - 1/4 width, y = y - 1/4 height, width = width * 1.5, height = height * 1.5 --> entweder dynamisch oder min (z.B. 100 oder so)
	const QRectF polyBbox = skyCulturePolygon->boundingRect();
	int minViewValue = 25;

	qreal width = polyBbox.width();
	qreal height = polyBbox.height();


	if(qMax(polyBbox.width(), polyBbox.height()) / 8 < minViewValue)
	{
		qreal factor = 0; // ratio between bigger and smaller dim --> used to scale larger dim accordingly

		if(qMax(polyBbox.width(), polyBbox.height()) == polyBbox.width())
		{
			// width is the smaller dim --> set width to minViewValue and scale height accordingly
			width = minViewValue * 8;
			factor = polyBbox.height() / polyBbox.width();
			height = (factor * minViewValue) * 8;
		}
		else
		{
			// height is the smaller dim --> set height to minViewValue and scale width accordingly
			height = minViewValue * 8;
			factor = polyBbox.width() / polyBbox.height();
			width = (factor * minViewValue) * 8;
		}

		smoothFitInView(QRectF(polyBbox.center().x() - width / 2, polyBbox.center().y() - height / 2, width, height));
		return;
	}

	smoothFitInView(QRectF(polyBbox.x() - width / 8, polyBbox.y() - height / 8, width * 1.25, height * 1.25));
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

void SkycultureMapGraphicsView::smoothFitInView(QRectF targetRect)
{
	// zoomOnTargetTimer needs to be stopped when the user selects another culture (starts a new zoom operation) while the old one hasnt finished
	zoomOnTargetTimer.stop();

	// update global variables so that they can be accessed by the slots 'zoomToDefault' and 'zoomOnTarget'
	this->targetRect = targetRect;
	this->startingRect = mapToScene(viewport()->rect()).boundingRect();

	qInfo() << "startingRect w: " << startingRect.width() << " h: " << startingRect.height() << " defaultRect w: " << defaultRect.width() << " h: " << defaultRect.height() << " diff w: " << qFabs((startingRect.width() - defaultRect.width())) << " h: " << qFabs(startingRect.height() - defaultRect.height());

	qInfo() << "pre: " << zoomToDefaultTimer.currentTime();

	qreal maxDuration = 2000;
	qreal threshold = 1200;
	QEasingCurve factor(QEasingCurve::OutQuad);
	qInfo() << "normal: " << 0.8 << " " << 2000 * 0.8 << " outQuad: " << factor.valueForProgress(1.2) << " " << 2000 * factor.valueForProgress(1.2);
	//qreal factor = maxDuration / pow(threshold, 2.0);

	// 500 * 4 = 2000 --> 250 * 4 = 1000 ---> 100 * 4 = 400 --> 2000 / 500 = 4 (Faktor)
	// für 2000 = a * 500² --> 2000 = 250000a --> a = 2000 / 250000 --> a = 0.008

	zoomToDefaultTimer.start();

	qInfo() << "post: " << zoomToDefaultTimer.currentTime();

	qreal deviation = qMax((qFabs(startingRect.center().x() - defaultRect.center().x()) + qFabs(startingRect.center().y() - defaultRect.center().y())) / 2, qMin(qFabs(startingRect.width() - defaultRect.width()), qFabs(startingRect.height() - defaultRect.height())) );
	//qreal deviation = (qFabs(startingRect.center().x() - defaultRect.center().x()) + qFabs(startingRect.center().y() - defaultRect.center().y())) / 2;


	// if value > threshold --> result > 1.0 --> valueForProgress return 1.0 for all values > 1.0 --> maxDuration
	zoomToDefaultTimer.setDuration(2000 * factor.valueForProgress(deviation * (1 / threshold)));

	qInfo() << "value: " << deviation << " eased: " << factor.valueForProgress(deviation * (1 / threshold)) <<  "current duration: " << zoomToDefaultTimer.duration();
}

void SkycultureMapGraphicsView::zoomToDefault(qreal zoomFactor)
{
	// transform the scene (scaling) to fit the current timestep
	qreal width = startingRect.width() + (defaultRect.width() - startingRect.width()) * zoomFactor;
	qreal height = startingRect.height() + (defaultRect.height() - startingRect.height()) * zoomFactor;

	qreal ratio = calculateScaleRatio(width, height);

	scale(ratio, ratio);

	// slowly move the center of the view to the new location
	QEasingCurve centerEasing(QEasingCurve::Linear);
	//centerOn(startingRect.center() - (startingRect.center() - defaultRect.center()) * zoomFactor);
	centerOn(startingRect.center() - (startingRect.center() - defaultRect.center()) * centerEasing.valueForProgress(zoomFactor));
}

void SkycultureMapGraphicsView::zoomOnTarget(qreal zoomFactor)
{
	// transform the scene (scaling) to fit the current timestep
	qreal width = defaultRect.width() - (defaultRect.width() - targetRect.width()) * zoomFactor;
	qreal height = defaultRect.height() - (defaultRect.height() - targetRect.height()) * zoomFactor;

	qreal ratio = calculateScaleRatio(width, height);

	scale(ratio, ratio);

	// slowly move the center of the view to the new location
	QEasingCurve centerEasing(QEasingCurve::InCubic);
	centerOn(defaultRect.center() - (defaultRect.center() - targetRect.center()) * centerEasing.valueForProgress(zoomFactor));
}

qreal SkycultureMapGraphicsView::calculateScaleRatio(qreal width, qreal height)
{
	// Rect of the current view with a margin of 2
	QRectF viewRect = viewport()->rect().adjusted(2, 2, - 2, - 2);
	if (viewRect.isEmpty())
	{
		return 0;
	}

	// Rect of the current transformation in scene coordinates
	QRectF sceneRect = transform().mapRect(QRectF(2, 2, width, height)); // values of x / y of sceneRect are not important since only the width / height are used for the calculation
	if (sceneRect.isEmpty())
	{
		return 0;
	}

	// calculate the x / y ratio for scaling
	qreal xratio = viewRect.width() / sceneRect.width();
	qreal yratio = viewRect.height() / sceneRect.height();

	// keep original aspect ratio
	return qMin(xratio, yratio);
}

void SkycultureMapGraphicsView::initializeTime()
{
	minYear = -2000;
	maxYear = QDateTime::currentDateTime().date().year();
	currentYear = maxYear;

	emit(timeRangeChanged(minYear, maxYear));
	emit(timeValueChanged(currentYear));
}
