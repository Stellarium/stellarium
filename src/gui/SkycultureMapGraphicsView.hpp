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
	/**
	 * @brief Selects a culture on the map and sets the current time if necessary.
	 *
	 * @param skycultureId Name of the skyculture.
	 * @param startTime Start time of the skyculture.
	 */
	void selectCulture(const QString &skycultureId, int startTime);

	/**
	 * @brief Update the current time and visibility of all polygons.
	 *
	 * @param The year to which the current time is set.
	 */
	void updateTime(int year);

	/**
	 * @brief Rotate the map by 180° (if it has not already been rotated).
	 *
	 * @param Flag that indicated wether the rotation should be applied or removed.
	 */
	void rotateMap(bool applyRotation);

signals:
	void cultureSelected(const QString &skycultureId);
	void timeValueChanged(int year);

protected:
	void wheelEvent(QWheelEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void showEvent(QShowEvent *e) override;
	void scaleView(double scaleFactor);

private:
	/// Flag that indicates if the view is currently navigated by the user.
	bool viewScrolling;

	/// Flag that indicates if the map is moved by the user when holding a mouse button.
	bool mapMoved;

	/// Indicates wether the view was opened before.
	bool firstShow;

	/// Flag indicating wether the map has been rotated.
	bool isRotated;

	/// Current Year of the map (used to evaluate polygon visibility).
	int currentYear;

	/// Last position of the cursor.
	QPoint mouseLastXY;

	/// Name of the previously selected skyculture.
	QString oldSkyCulture;

	/// Timeline for smooth zoom to the full extent of the base map.
	QTimeLine zoomToDefaultTimer;

	/// Timeline for smooth zoom to the target area.
	QTimeLine zoomOnTargetTimer;

	/// Rectangle of the view when smooth zoom started.
	QRectF startingRect;

	/// Rectangle containing the full extent of the base map.
	QRectF defaultRect;

	/// Rectangle of the target area.
	QRectF targetRect;

	/**
	 * @brief Convert the points in the given list from EPSG 4326 to EPSG 3857.
	 *
	 * @param A list of points in lat / lon coordinates.
	 * @return A list of points in meter coordinates.
	 */
	QList<QPointF> convertLatLonToMeter(const QList<QPointF> &latLonCoordinates);

	/**
	 * @brief Convert the points in the given list from EPSG 3857 to view coordinates.
	 *
	 * @param A list of points in real-world coordinates.
	 * @return A list of points in view coordinates.
	 */
	QList<QPointF> convertMeterToView(const QList<QPointF> &meterCoordinates);

	/**
	 * @brief Calculate a bounding box that contains all given items.
	 *
	 * @param A list of items.
	 * @return The calculated bounding box.
	 */
	QRectF calculateBoundingBox(const QList<QGraphicsItem *> graphicsItemList);

	/**
	 * @brief Change the visibility of all polygons on the map.
	 *
	 */
	void updateCultureVisibility();

	/**
	 * @brief Start a smooth zoom on bounding box of the given rectangle.
	 *
	 * @param The target area as a rectangle.
	 */
	void smoothFitInView(QRectF targetRect);

	/**
	 * @brief Selects all polygon of the given culture.
	 *
	 * @param Name of the culture to select.
	 */
	void selectAllCulturePolygon(const QString &skycultureId);

	/**
	 * @brief Draws the basemap and all culture polygons.
	 *
	 */
	void drawMapContent();

	/**
	 * @brief Loads all available polygons from the respective territory files.
	 *
	 */
	void loadCulturePolygons();

	/**
	 * @brief Calculate the factor by which the view must be scaled to fit in the given width / height.
	 *
	 * @param width The width of the new view.
	 * @param height The height of the new view.
	 * @return Ratio by which the scene must be scaled to meet the given width / height.
	 */
	qreal calculateScaleRatio(qreal width, qreal height);

private slots:
	void zoomToDefault(qreal factor);
	void zoomOnTarget(qreal factor);
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
