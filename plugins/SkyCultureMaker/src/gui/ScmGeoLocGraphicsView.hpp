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

#ifndef SCMGEOLOCGRAPHICSVIEW_HPP
#define SCMGEOLOCGRAPHICSVIEW_HPP

#include "ScmPreviewPathItem.hpp"
#include "ScmPreviewPolygonItem.hpp"
#include "types/CulturePolygon.hpp"
#include <QGraphicsView>

//! @class ScmGeoLocGraphicsView
//! Special GraphicsView that allows the User to digitize polygons
class ScmGeoLocGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	ScmGeoLocGraphicsView(QWidget *parent = nullptr);

	void initializeGraphicsView();
	void addCurrentPoly(int startTime, int endTime);
	void removePolygon(int id);
	void reset();

public slots:
	void updateTime(int year);
	void selectPolygon(int id);

signals:
	void timeValueChanged(int year);
	void timeRangeChanged(int minYear, int maxYear);
	void showAddPolyDialog();
	void addPolygonToCulture(scm::CulturePolygon poly);

protected:
	void wheelEvent(QWheelEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	bool viewScrolling;
	bool firstShow;
	int currentYear;
	QPoint mouseLastXY;
	QRectF defaultRect;
	QMap<int, ScmPreviewPolygonItem *> polygonIdentifierMap;
	ScmPreviewPolygonItem *currentCapturePolygon = new ScmPreviewPolygonItem(true);
	ScmPreviewPathItem *previewCapturePath = new ScmPreviewPathItem();

	void updateCultureVisibility();
	void updatePreviewPath();
	void exportCulturePolygons();
	void scaleView(double scaleFactor);
	qreal calculateScaleRatio(qreal width, qreal height);
	QPolygonF convertViewToWGS84(const QPolygonF &viewCoordinatePolygon);
};

#endif // SCMGEOLOCGRAPHICSVIEW_HPP
