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

#ifndef SKYCULTUREMAPGRAPHICSVIEW_HPP
#define SKYCULTUREMAPGRAPHICSVIEW_HPP

#include <qtimeline.h>
#include <QGraphicsView>

//! @class SkyCultureMapGraphicsView
//! Special GraphicsView that shows a world map and several (culture) polygons
class SkycultureMapGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	SkycultureMapGraphicsView(QWidget *parent = nullptr);

public slots:
	void selectCulture(const QString &skycultureId, int startTime);
	void updateTime(int year);
	void rotateMap(bool isRotated);

signals:
	void cultureSelected(const QString &skycultureId);
	void timeValueChanged(int year);
	void timeRangeChanged(int minYear, int maxYear);

protected:
	void wheelEvent(QWheelEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void showEvent(QShowEvent *e) override;
	void scaleView(double scaleFactor);

private:
	bool viewScrolling;
	bool mapMoved;
	bool firstShow;
	bool isRotated;
	int currentYear;
	QPoint mouseLastXY;
	QString oldSkyCulture;
	QTimeLine zoomToDefaultTimer;
	QTimeLine zoomOnTargetTimer;
	QRectF startingRect;
	QRectF defaultRect;
	QRectF targetRect;

	QList<QPointF> convertLatLonToMeter(const QList<QPointF> &latLonCoordinates);
	QList<QPointF> convertMeterToView(const QList<QPointF> &meterCoordinates);
	QRectF calculateBoundingBox(const QList<QGraphicsItem *> graphicsItemList);
	void updateCultureVisibility();
	void smoothFitInView(QRectF targetRect);
	void selectAllCulturePolygon(const QString &skycultureId);
	void drawMapContent();
	void loadCulturePolygons();
	qreal calculateScaleRatio(qreal width, qreal height);

private slots:
	void zoomToDefault(qreal factor);
	void zoomOnTarget(qreal factor);
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
