/*
 * Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SkyCultureMapGraphicsView.hpp"
#include "SkyCulturePolygonItem.hpp"
#include "StelLocaleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include <qjsonarray.h>
#include <qscrollbar.h>

#include <QGraphicsSvgItem>
#include <QJsonObject>
#include <QJsonDocument>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QFile>

SkyCultureMapGraphicsView::SkyCultureMapGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
	, viewScrolling(false)
	, mapMoved(false)
	, firstShow(true)
	, isRotated(false)
	, currentYear(0)
	, mouseLastXY(0, 0)
	, oldSkyCulture("")
{
	QGraphicsScene *scene = new QGraphicsScene(this);
	// scene->setItemIndexMethod(QGraphicsScene::NoIndex); // noIndex better when adding / removing many items
	setScene(scene);
	setInteractive(true);

	setRenderHint(QPainter::Antialiasing); // maybe unnecessary for this project
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

	setCursor(Qt::ArrowCursor);
	setMouseTracking(true);

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// set up QTimelines for smooth zoom on culture selection (through public 'selectCulture' slot)
	zoomToDefaultTimer.setDuration(2000);
	zoomOnTargetTimer.setDuration(3000);
	zoomToDefaultTimer.setUpdateInterval(20);
	zoomOnTargetTimer.setUpdateInterval(20);

	connect(&zoomToDefaultTimer, &QTimeLine::valueChanged, this, &SkyCultureMapGraphicsView::zoomToDefault);
	connect(&zoomToDefaultTimer, &QTimeLine::finished, &zoomOnTargetTimer, &QTimeLine::start);
	connect(&zoomOnTargetTimer, &QTimeLine::valueChanged, this, &SkyCultureMapGraphicsView::zoomOnTarget);

	// draw basemap and culture polygons
	drawMapContent();
}

void SkyCultureMapGraphicsView::drawMapContent()
{
	// delete all items
	scene()->clear();

	// evaluate projection
	QGraphicsSvgItem *baseMap = new QGraphicsSvgItem(":/graphicGui/skyCultureWorldMap.svgz");

	scene()->addItem(baseMap);
	scene()->setSceneRect(- baseMap->boundingRect().width() * 0.5, - baseMap->boundingRect().height() * 0.25, baseMap->boundingRect().width() * 2.0, baseMap->boundingRect().height() * 1.5);
	this->defaultRect = baseMap->boundingRect();

	loadCulturePolygons();
}

void SkyCultureMapGraphicsView::loadCulturePolygons()
{
	// loop over all skyCultures
	StelApp& app = StelApp::getInstance();
	QMap<QString, QString> cultureIdToTranslationMap = app.getSkyCultureMgr().getDirToI18Map();
	const QStringList cultureIds = cultureIdToTranslationMap.keys();

	for (const auto &currentCulture : cultureIds)
	{
		// find path of file
		const QString filePath = StelFileMgr::findFile("skyCultures/" + currentCulture + "/territory.json");
		if (filePath.isEmpty())
		{
			qCritical() << "Failed to * find * [ " << currentCulture << " ] territory file in sky culture directory";
			continue;
		}
		// try to open file
		QFile file(filePath);
		if (!file.open(QFile::ReadOnly))
		{
			qCritical() << "Failed to * open * [ " << currentCulture << " ] territory file in sky culture directory";
			continue;
		}
		// try to read file
		const auto jsonText = file.readAll();
		if (jsonText.isEmpty())
		{
			qCritical() << "Failed to read data from [ " << currentCulture << " ] territory file in sky culture directory";
			continue;
		}
		// try to parse file as JsonDocument
		QJsonParseError error;
		const auto jsonDoc = QJsonDocument::fromJson(jsonText, &error);
		if (error.error != QJsonParseError::NoError)
		{
			qCritical().nospace() << "Failed to parse  [ " << currentCulture << " ] territory file in sky culture directory: " << error.errorString();
			continue;
		}
		if (!jsonDoc.isObject())
		{
			qCritical() << "Failed to find the expected JSON structure in [ " << currentCulture << " ] territory file in sky culture directory";
			continue;
		}
		// try to access important information ---> create a new PolygonItem and add it to the scene
		const auto data = jsonDoc.object();
		if (data["polygons"].isArray())
		{
			const auto polygonArray = data["polygons"].toArray();
			for (const auto &currentPoly : polygonArray)
			{
				auto polygonObject = currentPoly.toObject();

				int startTime = polygonObject.value("startTime").toInt();
				int endTime;
				if (polygonObject.value("endTime").toString() == "∞")
				{
					endTime = QDateTime::currentDateTime().date().year();
				}
				else
				{
					endTime = polygonObject.value("endTime").toString().toInt();
				}
				QPolygonF geometry;

				const auto geometryArray = polygonObject.value("geometry").toArray();
				for (const auto &point : geometryArray)
				{
					auto pointArray = point.toArray();
					geometry << QPointF(pointArray[0].toDouble(), pointArray[1].toDouble());
				}

				SkyCulturePolygonItem *item = new SkyCulturePolygonItem(cultureIdToTranslationMap.value(currentCulture), startTime, endTime);
				item->setPolygon(convertLatLonToMeter(geometry));
				scene()->addItem(item);
			}
		}
	}
}

void SkyCultureMapGraphicsView::wheelEvent(QWheelEvent *e)
{
	qreal zoomFactor = pow(2.0, e->angleDelta().y() / 240.0);
	if (e->modifiers() & Qt::ControlModifier)
	{
		//holding ctrl while wheel zooming results in a finer zoom
		zoomFactor = 1.0 + ( zoomFactor - 1.0 ) / 15.0;
	}
	scaleView(zoomFactor);

	// ensure that the view is in bounds after scale operation
	QRectF currentViewRect = mapToScene(viewport()->rect()).boundingRect();
	if (!defaultRect.contains(currentViewRect.center()))
	{
		qreal x = currentViewRect.center().x();
		qreal y = currentViewRect.center().y();

		if (currentViewRect.center().x() < defaultRect.x())
		{
			x = defaultRect.x();
		}
		else if (currentViewRect.center().x() > defaultRect.x() + defaultRect.width())
		{
			x = defaultRect.x() + defaultRect.width();
		}
		if (currentViewRect.center().y() < defaultRect.y())
		{
			y = defaultRect.y();
		}
		else if (currentViewRect.center().y() > defaultRect.y() + defaultRect.height())
		{
			y = defaultRect.y() + defaultRect.height();
		}

		centerOn(x, y);
	}
}

void SkyCultureMapGraphicsView::mouseMoveEvent(QMouseEvent *e)
{
	// reimplementation of default ScrollHandDrag in QGraphicsView
	if (viewScrolling) {
		if (!mapMoved)
		{
			QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
		}
		QScrollBar *hBar = horizontalScrollBar();
		QScrollBar *vBar = verticalScrollBar();
		QPoint delta = e->pos() - mouseLastXY;
		int oldHBarValue= hBar->value();
		int oldVBarValue= vBar->value();
		int newHBarValue = oldHBarValue + (isRightToLeft() ? delta.x() : -delta.x());
		int newVBarValue = oldVBarValue - delta.y();
		int borderValue = 0; // scrollbar value of the respective border of the map

		// ensure the user is not able to pan too far away from the map
		QPointF mappedUpperLeftBorder = transform().map(QPointF(defaultRect.x(), defaultRect.y()));
		QPointF mappedlowerRightBorder = transform().map(QPointF(defaultRect.x() + defaultRect.width(), defaultRect.y() + defaultRect.height()));
		if (newHBarValue < oldHBarValue)
		{
			borderValue = mappedUpperLeftBorder.x() - (viewport()->width() / 2.0);
			if (newHBarValue < borderValue)
			{
				hBar->setValue(borderValue);
			}
			else
			{
				hBar->setValue(newHBarValue);
			}
		}
		else if (newHBarValue > oldHBarValue)
		{
			borderValue = mappedlowerRightBorder.x() - (viewport()->width() / 2.0);
			if (newHBarValue > borderValue)
			{
				hBar->setValue(borderValue);
			}
			else
			{
				hBar->setValue(newHBarValue);
			}
		}
		if (newVBarValue < oldVBarValue)
		{
			borderValue = mappedUpperLeftBorder.y() - (viewport()->height() / 2.0);
			if (newVBarValue < borderValue)
			{
				vBar->setValue(borderValue);
			}
			else
			{
				vBar->setValue(newVBarValue);
			}
		}
		else if (newVBarValue > oldVBarValue)
		{
			borderValue = mappedlowerRightBorder.y() - (viewport()->height() / 2.0);
			if (newVBarValue > borderValue)
			{
				vBar->setValue(borderValue);
			}
			else
			{
				vBar->setValue(newVBarValue);
			}
		}

		mapMoved = true;
	}
	mouseLastXY = e->pos();

	QGraphicsView::mouseMoveEvent(e);
}

void SkyCultureMapGraphicsView::mousePressEvent(QMouseEvent *e)
{
	if ( e->button() == Qt::LeftButton )
	{
		viewScrolling = true;
	}

	// if event is not accepted (mouse not over item) mouseReleaseEvent is not triggered
	e->setAccepted(true);
}

void SkyCultureMapGraphicsView::mouseReleaseEvent( QMouseEvent *e )
{
	setFocus();
	QGraphicsView::mouseReleaseEvent(e);

	if (!mapMoved)
	{
		QGraphicsItem *currentTopmostMouseGrabberItem = itemAt(e->pos());
		// the item is either SkyCulturePolygonItem or QGraphicsSvgItem (background) ---> try to cast it to SkyCulturePolygonItem
		SkyCulturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyCulturePolygonItem *>(currentTopmostMouseGrabberItem);
		if (scPolyItem)
		{
			const QString currentSkyCulture = scPolyItem->getSkyCultureId();
			// determine if a new culture is being selected
			if (oldSkyCulture != currentSkyCulture)
			{
				// if so, select all polygons of the respective culture, emit the cultureSelected Signal and set the oldSkyCulture to currentSkyCulture
				selectAllCulturePolygon(currentSkyCulture);
				emit cultureSelected(currentSkyCulture);
				oldSkyCulture = currentSkyCulture;
			}
		}
	}

	if(viewScrolling)
	{
		viewScrolling = false;
		if (mapMoved)
		{
			mapMoved = false;
			QGuiApplication::restoreOverrideCursor();
		}
	}
}

void SkyCultureMapGraphicsView::showEvent(QShowEvent *e)
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

	QGraphicsView::showEvent(e);
}

void SkyCultureMapGraphicsView::scaleView(double factor)
{
	// calculate requested zoom before executing the zoom operation to limit the min / max zoom level
	const double scaling = transform().scale(factor, factor).mapRect(QRectF(0, 0, 1, 1)).width();

	if (scaling < 0.23 || scaling > 500.0) // scaling < min or scaling > max zoom level
		return;

	scale(factor, factor);
}

QPolygonF SkyCultureMapGraphicsView::convertLatLonToMeter(const QPolygonF &latLonCoordinates)
{
	QPolygonF meterCoordinates;

	for(auto point : latLonCoordinates)
	{
		qreal xMeter = (point.x() * 20037508.3427892439067363739014) / 180.0;
		qreal yMeter = ((qLn(qTan(((90.0 + point.y())* M_PI) / 360.0)) / (M_PI / 180.0)) * 20037508.3427892439067363739014) / 180.0;
		meterCoordinates << QPointF(xMeter, yMeter);
	}

	return convertMeterToView(meterCoordinates);
}

QPolygonF SkyCultureMapGraphicsView::convertMeterToView(const QPolygonF &meterCoordinates)
{
	QPolygonF viewCoordinates;

	// cropped map (EPSG: 3857) extent:
	// x / lon: -20037507.0671618431806564 | 20037507.0671618431806564 ---> sum(abs) = 40075014.1343236863613128
	// y / lat: -19202484.5635684318840504 | 18418386.3090785145759583 ---> sum(abs) = 37620870.87264694646000862
	for(auto point : meterCoordinates)
	{
		// used map is cropped ---> project points (in meter) from full extent to smaller / cropped extent
		qreal xView = ((point.x() + 20037507.0671618431806564) / 40075014.1343236863613128) * defaultRect.width();
		qreal yView = ((point.y() - 18418386.3090785145759583) / -37620870.87264694646000862) * defaultRect.height();
		viewCoordinates << QPointF(xView, yView);
	}

	return viewCoordinates;
}

void SkyCultureMapGraphicsView::selectAllCulturePolygon(const QString &skyCultureId)
{
	const auto itemList = scene()->items();
	for (const auto &item : itemList)
	{
		// make sure the current item is a SkyCulturePolygonItem
		SkyCulturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyCulturePolygonItem *>(item);
		if (!scPolyItem)
		{
			continue;
		}

		// by default set selection to false
		scPolyItem->setSelectionState(false);

		if (skyCultureId == scPolyItem->getSkyCultureId())
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

void SkyCultureMapGraphicsView::selectCulture(const QString &skyCultureId, int startTime)
{
	QList<QGraphicsItem *> currentTimeItems = QList<QGraphicsItem *>();
	QList<QGraphicsItem *> startTimeItems = QList<QGraphicsItem *>();

	const auto itemList = scene()->items();
	for(const auto &item : itemList)
	{
		// cast QGraphicsItem to SkyCulturePolygonItem --> if the item is not a SkyCulturePolygonItem: skip it to prevent errors
		SkyCulturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyCulturePolygonItem *>(item);
		if(!scPolyItem)
		{
			continue;
		}

		if(skyCultureId == scPolyItem->getSkyCultureId())
		{
			if (scPolyItem->existsAtPointInTime(currentYear))
			{
				currentTimeItems.append(item);
			}
			else if (scPolyItem->existsAtPointInTime(startTime))
			{
				startTimeItems.append(item);
			}
		}
	}
	// if no polygon with the specified skyCultureId exists, this function simply clears the selection
	selectAllCulturePolygon(skyCultureId);

	if (!currentTimeItems.empty())
	{
		smoothFitInView(calculateBoundingBox(currentTimeItems));
	}
	else if (!startTimeItems.empty())
	{
		smoothFitInView(calculateBoundingBox(startTimeItems));
		emit timeValueChanged(startTime);
	}
	else
	{
		qInfo() << "couldn't find any polygon with name [" << skyCultureId << "]!";
		return;
	}
}

void SkyCultureMapGraphicsView::updateTime(int year)
{
	currentYear = year;
	updateCultureVisibility();
}

void SkyCultureMapGraphicsView::rotateMap(bool applyRotation)
{
	if (applyRotation)
	{
		StelCore *core = StelApp::getInstance().getCore();
		if (core->getCurrentLocation().getLatitude() < 0)
		{
			if (!isRotated)
			{
				rotate(180.0);
				isRotated = true;
			}
		}
		else
		{
			if (isRotated)
			{
				rotate(-180.0);
				isRotated = false;
			}
		}
	}
	else
	{
		if (isRotated)
		{
			rotate(-180.0);
			isRotated = false;
		}
	}
}

void SkyCultureMapGraphicsView::updateCultureVisibility()
{
	// iterate over all polygons --> if currentTime is between startTime and endTime: show polygon (else hide it)
	const auto itemList = scene()->items();
	for(const auto &item : itemList) {
		// cast generic QGraphicsItem to subclass SkyCulturePolygonItem
		SkyCulturePolygonItem *scPolyItem = qgraphicsitem_cast<SkyCulturePolygonItem *>(item);
		// if cast was unsuccessful (item is not an SkyCulturePolygonItem) --> look at the next item
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

void SkyCultureMapGraphicsView::smoothFitInView(QRectF targetRect)
{
	// zoomOnTargetTimer must be stopped if the user selects a different culture (starts a new zoom operation) while the old one is still in progress
	zoomOnTargetTimer.stop();

	// update global variables so that they can be accessed by the slots 'zoomToDefault' and 'zoomOnTarget'
	this->targetRect = targetRect;
	this->startingRect = mapToScene(viewport()->rect()).boundingRect();

	qreal maxDuration = 2000;
	qreal threshold = 1200;
	QEasingCurve factor(QEasingCurve::OutQuad);

	zoomToDefaultTimer.start();
	qreal deviation = qMax((qFabs(startingRect.center().x() - defaultRect.center().x()) + qFabs(startingRect.center().y() - defaultRect.center().y())) / 2,
						   qMin(qFabs(startingRect.width() - defaultRect.width()), qFabs(startingRect.height() - defaultRect.height())));

	// if value > threshold --> result > 1.0
	// valueForProgress returns 1.0 for all values greater than 1.0 (which equals maxDuration)
	zoomToDefaultTimer.setDuration(maxDuration * factor.valueForProgress(deviation * (1 / threshold)));
}

void SkyCultureMapGraphicsView::zoomToDefault(qreal zoomFactor)
{
	// transform the scene (scaling) to fit the current timestep
	qreal width = startingRect.width() + (defaultRect.width() - startingRect.width()) * zoomFactor;
	qreal height = startingRect.height() + (defaultRect.height() - startingRect.height()) * zoomFactor;

	qreal ratio = calculateScaleRatio(width, height);
	scale(ratio, ratio);

	// slowly move the center of the view to the new location
	QEasingCurve centerEasing(QEasingCurve::Linear);
	centerOn(startingRect.center() - (startingRect.center() - defaultRect.center()) * centerEasing.valueForProgress(zoomFactor));
}

void SkyCultureMapGraphicsView::zoomOnTarget(qreal zoomFactor)
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

qreal SkyCultureMapGraphicsView::calculateScaleRatio(qreal width, qreal height)
{
	// Rect of the current view with a margin of 2
	QRectF viewRect = viewport()->rect().adjusted(2, 2, - 2, - 2);
	if (viewRect.isEmpty())
	{
		return 0;
	}

	// Rect of the current transformation in scene coordinates
	// values of x / y of sceneRect are not important since only the width / height are used for the calculation
	QRectF sceneRect = transform().mapRect(QRectF(2, 2, width, height));
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

QRectF SkyCultureMapGraphicsView::calculateBoundingBox(const QList<QGraphicsItem *> graphicsItemList)
{
	QRectF boundingBox = QRectF();

	// iterate over list of items to build the Bbox
	for (auto &item : graphicsItemList)
	{
		boundingBox = boundingBox.united(item->boundingRect());
	}

	// slightly enlarge the Bbox so that the items don't take up the entire screen
	qreal width = boundingBox.width();
	qreal height = boundingBox.height();
	// threshold that controls how small the smaller dim of the Bbox is allowed to be (value in view coordinates, not real world)
	int minViewValue = 80;

	if(qMax(width, height) * 1.25 < minViewValue)
	{
		if(qMax(width, height) == width)
		{
			// width is the smaller dim --> set width to minViewValue and scale height accordingly
			width = minViewValue;
			height = (boundingBox.height() / boundingBox.width()) * minViewValue;
		}
		else
		{
			// height is the smaller dim --> set height to minViewValue and scale width accordingly
			height = minViewValue;
			width = (boundingBox.width() / boundingBox.height()) * minViewValue;
		}
		return QRectF(boundingBox.center().x() - width / 2, boundingBox.center().y() - height / 2, width, height);
	}

	return QRectF(boundingBox.x() - width / 8, boundingBox.y() - height / 8, width * 1.25, height * 1.25);
}

