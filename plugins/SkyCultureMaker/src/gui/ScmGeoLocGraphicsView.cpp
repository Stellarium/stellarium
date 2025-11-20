/*
 * Sky Culture Maker plug-in for Stellarium
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

#include "ScmGeoLocGraphicsView.hpp"
#include <qevent.h>
#include <qjsonarray.h>
#include <qgraphicssvgitem.h>
#include <qscrollbar.h>
#include <QFileDialog>

#include <QJsonObject>
#include <QJsonDocument>
#include <QtGui>



ScmGeoLocGraphicsView::ScmGeoLocGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
	, viewScrolling(false)
	, firstShow(true)
	, currentYear(0)
	, mouseLastXY(0, 0)
{
	QGraphicsScene *scene = new QGraphicsScene(this);
	// scene->setItemIndexMethod(QGraphicsScene::NoIndex); // noIndex better when adding / removing many items
	setScene(scene);
	setInteractive(true);

	setRenderHint(QPainter::Antialiasing); // maybe unnecessary for this project
	setTransformationAnchor(AnchorUnderMouse);

	setCursor(Qt::CrossCursor);
	setMouseTracking(true);

	//setDragMode(QGraphicsView::NoDrag);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	show();

	// add items (transfer to dedicated function later) 1278.57,1899.83 1276.74,1900.03


	QGraphicsSvgItem *baseMap = new QGraphicsSvgItem(":/graphicGui/gen01_capPop500k_wAntarctica_015_op70.svg");
	scene->addItem(baseMap);
	scene->setSceneRect(- baseMap->boundingRect().width() * 0.75, - baseMap->boundingRect().height() * 0.5, baseMap->boundingRect().width() * 2.5, baseMap->boundingRect().height() * 2);
	qInfo() << "basemap width = " << baseMap->boundingRect().width() << " and height = " << baseMap->boundingRect().height();
	this->defaultRect = baseMap->boundingRect();

	scene->addItem(currentCapturePolygon);
	scene->addItem(previewCapturePath);


	qInfo() << "ende Map Constructor!";
}

void ScmGeoLocGraphicsView::wheelEvent(QWheelEvent *event)
{
	qreal zoomFactor = pow(2.0, event->angleDelta().y() / 240.0);
	qreal qgsZoomFactor = 1.0 + ( 1.01 - 1.0 ) / 120.0 * std::fabs( event->angleDelta().y() );
	qreal ctrZoomFactor = 0.0;
	if ( event->modifiers() & Qt::ControlModifier )
	{
		//holding ctrl while wheel zooming results in a finer zoom
		ctrZoomFactor = 1.0 + ( zoomFactor - 1.0 ) / 15.0;
		scaleView(ctrZoomFactor);
		qInfo() << "factor: " << zoomFactor << " vs. qzoomfactor: " << qgsZoomFactor << " vs. ctrlZoomfactor: " << ctrZoomFactor;
		return;
	}

	scaleView(zoomFactor); // faster scrolling = faster zoom
}

void ScmGeoLocGraphicsView::showEvent(QShowEvent *event)
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

void ScmGeoLocGraphicsView::scaleView(double factor)
{
	// calculate requested zoom before executing the zoom operation to limit the min / max zoom level
	const double scaling = transform().scale(factor, factor).mapRect(QRectF(0, 0, 1, 1)).width();
	qInfo() << "scale: " << scaling;

	if (scaling < 0.1 || scaling > 5000.0) // scaling < min or scaling > max zoom level
		return;

	scale(factor, factor);
}

void ScmGeoLocGraphicsView::updateTime(int year)
{
	currentYear = year;
	updateCultureVisibility();
}

void ScmGeoLocGraphicsView::selectPolygon(int id)
{
	PreviewPolygonItem *poly = polygonIdentifierMap.value(id);

	// if no fitting polygon was found --> return to prevent errors
	if(poly == nullptr)
	{
		return;
	}

	if(!poly->existsAtPointInTime(currentYear))
	{
		// signal connects to updateSkyCultureTimeValue in ScmSkyCultureDialog which invokes updateTime (in this class)
		emit(timeValueChanged(poly->getStartTime()));
	}

	const QRectF polyBbox = poly->boundingRect();
	fitInView(QRectF(polyBbox.x() - polyBbox.width() / 8, polyBbox.y() - polyBbox.height() / 8, polyBbox.width() * 1.25, polyBbox.height() * 1.25), Qt::KeepAspectRatio);
}

void ScmGeoLocGraphicsView::updateCultureVisibility()
{
	// iterate over all polygons --> if currentTime is between startTime and endTime show, else hide
	for(const auto &item : scene()->items()) {
		PreviewPolygonItem *previewPItem = qgraphicsitem_cast<PreviewPolygonItem *>(item);

		// if cast was unsuccessful (item is not an SkyculturePolygonItem) --> look at the next item
		if(!previewPItem)
			continue;

		// if the current year is between the start and end time of the polygon --> show (otherwise hide the item)
		if(previewPItem->existsAtPointInTime(currentYear))
		{
			previewPItem->setVisible(true);
		}
		else
		{
			previewPItem->setVisible(false);
		}
	}
}

void ScmGeoLocGraphicsView::reset()
{
	// remove all polygons from the scene
	for (auto it = polygonIdentifierMap.cbegin(); it != polygonIdentifierMap.cend(); ++it)
	{
		scene()->removeItem(it.value());
	}

	// clear the map used for identifying the polygon items
	polygonIdentifierMap.clear();

	// set firstShow to true, so that the default extent is shown
	firstShow = true;

	// clear the digitization previews
	currentCapturePolygon->setPolygon(QPolygonF());
	previewCapturePath->reset();
}

qreal ScmGeoLocGraphicsView::calculateScaleRatio(qreal width, qreal height)
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

void ScmGeoLocGraphicsView::keyPressEvent( QKeyEvent *e )
{
	// delete the last point in the capture poylgon (except the very first one)
	if (e->key() == Qt::Key_Backspace)
	{
		if (currentCapturePolygon->polygon().size() > 1)
		{
			QPolygonF newPoly = currentCapturePolygon->polygon();
			newPoly.removeLast();

			previewCapturePath->setLastPoint(newPoly.last());
			currentCapturePolygon->setPolygon(newPoly);
		}
	}
	// abort current capture ---> reset all points (including first one)
	else if (e->key() == Qt::Key_Escape)
	{
		currentCapturePolygon->setPolygon(QPolygonF());

		previewCapturePath->reset();
	}
}

void ScmGeoLocGraphicsView::mousePressEvent( QMouseEvent *e )
{
	qInfo() << "press Event: " << e->button();
	if (e->button() == Qt::LeftButton)
	{
		if(e->modifiers() & Qt::ShiftModifier)
		{
			viewScrolling = true;
			QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
		}
	}

	// if event is not accepted (mouse not over item) mouseReleaseEvent is not triggered
	e->setAccepted(true);
}

void ScmGeoLocGraphicsView::mouseReleaseEvent( QMouseEvent *e )
{
	setFocus();
	qInfo() << "release Event: " << e->button() << "\n";
	if (e->button() == Qt::LeftButton)
	{
		// open dialog (range of time) + save and reset the current capture polygon
		if(e->modifiers() & Qt::AltModifier)
		{
			if (currentCapturePolygon->polygon().size() < 3)
			{
				return;
			}

			// open dialog 'popup'
			emit showAddPolyDialog();
		}
		// do not set a point after a scrolling operation (maybe the user unintentionally released SHIFT)
		else if (!viewScrolling)
		{
			if (currentCapturePolygon->polygon().size() < 1)
			{
				previewCapturePath->setFirstPoint(mapToScene(e->pos()));
			}
			else
			{
				previewCapturePath->setLastPoint(mapToScene(e->pos()));
			}

			currentCapturePolygon->setPolygon(currentCapturePolygon->polygon() << mapToScene(e->pos()));
		}
	}

	if(viewScrolling)
	{
		viewScrolling = false;
		QGuiApplication::restoreOverrideCursor();
	}
}

void ScmGeoLocGraphicsView::mouseMoveEvent( QMouseEvent *e )
{
	if (!currentCapturePolygon->polygon().empty())
	{
		previewCapturePath->setMousePoint(mapToScene(e->pos()));
	}

	// reimplementation of default ScrollHandDrag in QGraphicsView
	if (viewScrolling) {
		QScrollBar *hBar = horizontalScrollBar();
		QScrollBar *vBar = verticalScrollBar();
		QPoint delta = e->pos() - mouseLastXY;
		hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
		vBar->setValue(vBar->value() - delta.y());
	}

	mouseLastXY = e->pos();

	QGraphicsView::mouseMoveEvent(e);

}

void ScmGeoLocGraphicsView::addCurrentPoly(int startTime, int endTime)
{
	// add the polygon to the scene so the user can see some progress while digitizing other polygons
	PreviewPolygonItem *poly = new PreviewPolygonItem(startTime, endTime, currentCapturePolygon->polygon());
	scene()->addItem(poly);

	// save poly
	if (polygonIdentifierMap.empty())
	{
		polygonIdentifierMap.insert(0, poly);
	}
	else
	{
		polygonIdentifierMap.insert(polygonIdentifierMap.lastKey() + 1, poly);
	}

	// convert the view coordinates to real world coordinates
	QPolygonF transformedPolygon = convertViewToWGS84(currentCapturePolygon->polygon());

	emit(addPolygonToCulture(scm::CulturePolygon(polygonIdentifierMap.lastKey(), startTime, endTime, transformedPolygon)));

	// reset capture poly and path
	currentCapturePolygon->setPolygon(QPolygonF());
	previewCapturePath->reset();

	// update the map
	updateCultureVisibility();
}

void ScmGeoLocGraphicsView::removePolygon(int id)
{
	// remove item from scene and delete the corresponding entry in polygonIdentifierMap
	scene()->removeItem(polygonIdentifierMap.value(id));
	polygonIdentifierMap.remove(id);
}

QPolygonF ScmGeoLocGraphicsView::convertViewToWGS84(const QPolygonF &viewCoordinatePolygon)
{
	QList<QPointF> result;

	qInfo() << "pre return --> liste input: "<< qSetRealNumberPrecision(20) << viewCoordinatePolygon;

	// convert view coordinates to native coordinate system of the current map
	for (const auto &point : viewCoordinatePolygon)
	{
		qreal xInMeter = ((point.x() / defaultRect.width()) * 40075014.1343236863613128) - 20037507.0671618431806564;
		qreal yInMeter = ((point.y() / defaultRect.height()) * -39034031.3094234876334668) + 18418386.3090785145759583;

		result.append(QPointF(xInMeter, yInMeter));
	}

	qInfo() << "pre return --> liste (view to meter): " << qSetRealNumberPrecision(20) << result;

	// convert map coordinates to WGS84 coordinates
	for (auto &point : result)
	{
		qreal xInLon= (point.x() * 180.0) / 20037508.3427892439067363739014;
		qreal yInLat = (qAtan(qExp(((point.y() * 180.0) / 20037508.3427892439067363739014) * (M_PI / 180.0))) * (360.0 / M_PI)) - 90.0;

		point.setX(xInLon);
		point.setY(yInLat);
	}

	qInfo() << "pre return --> liste (meter to latLon): " << qSetRealNumberPrecision(20) << result;
	// qreal xInLon= (point.x() * 180.0) / 20037508.3427892439067363739014;
	// qreal yInLat = (qAtan(qExp(((point.y() * 180.0) / 20037508.3427892439067363739014) * (M_PI / 180.0))) * (360.0 / M_PI)) - 90;

	return result;
}
