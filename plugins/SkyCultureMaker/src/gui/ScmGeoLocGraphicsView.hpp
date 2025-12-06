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

	void addCurrentPoly(int startTime, int endTime);
	void removePolygon(int id);
	void reset();

public slots:
	/**
	 * @brief Update the current time and visibility of all polygons.
	 *
	 * @param The year to which the current time is set.
	 */
	void updateTime(int year);

	/**
	 * @brief Selects the polygon with the given identifier. If no such identifier is present, nothing happens.
	 *
	 * @param The identifier of the polygon to be selected.
	 */
	void selectPolygon(int id);

signals:
	void timeValueChanged(int year);
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
	/// Flag that indicates if the view is currently navigated by the user.
	bool viewScrolling;

	bool firstShow;
	int currentYear;
	QPoint mouseLastXY;

	/// Bounding rect of the map.
	QRectF defaultRect;

	/// Map that pairs an identifier with each item on the map. (used for removal and selection)
	QMap<int, ScmPreviewPolygonItem *> polygonIdentifierMap;

	/// PolygonItem that visualizes the current digitization progress.
	ScmPreviewPolygonItem *currentCapturePolygon = new ScmPreviewPolygonItem(true);

	/// PathItem that displays a preview of the current polygon.
	ScmPreviewPathItem *previewCapturePath = new ScmPreviewPathItem();

	/**
	 * @brief Change the visibility of all polygons on the map.
	 *
	 */
	void updateCultureVisibility();

	/**
	 * @brief Scale the view with respect to the min / max zoom level.
	 *
	 * @param scaleFactor The factor by which the view should be scaled.
	 */
	void scaleView(double scaleFactor);

	/**
	 * @brief Calculate the factor by which the view must be scaled to fit in the given width / height.
	 *
	 * @param width The width of the new view.
	 * @param height The height of the new view.
	 * @return Ratio by which the scene must be scaled to meet the given width / height.
	 */
	qreal calculateScaleRatio(qreal width, qreal height);

	/**
	 * @brief Convert the points of the given polygon from view coordinates to real-world coordinates.
	 *
	 * @param A float point polygon in view coordinates.
	 * @return A float point polygon in real-world coordinates.
	 */
	QPolygonF convertViewToWGS84(const QPolygonF &viewCoordinatePolygon);
};

#endif // SCMGEOLOCGRAPHICSVIEW_HPP
