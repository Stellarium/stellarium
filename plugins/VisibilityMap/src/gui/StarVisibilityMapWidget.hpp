/*
 * Visibility Map plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef STAR_VISIBILITY_MAP_WIDGET_HPP
#define STAR_VISIBILITY_MAP_WIDGET_HPP

#include <QPixmap>
#include <QPoint>
#include <QSize>

#include <QWidget>

class StelCore;

//! Displays a world map with visibility lines for a locked-in Stellarium object.
//!
//! Usage:
//!   - Connect StelObjectMgr::selectedObjectChanged to onSelectionChanged().
//!     This only enables/disables the Calculate button; it never redraws.
//!   - The dialog calls calculateVisibility() when the user presses the button.
//!     Only then are the lines computed from the current selection + JD and drawn.
//!   - The map keeps the last result until calculateVisibility() is called again,
//!     so the user can freely browse other objects in the main view without
//!     disturbing the visibility map.
//!
//! Five characteristic latitude circles are drawn (matching the symbology
//! of the reference visibility charts):
//!
//!  1. Limit of visibility  (solid line)       lat = ±(90° − |dec|)
//!  2. Extinction-free (5° above horizon)      lat = dec ± 85°
//!  3. Zenith passage   (plus symbols)         lat = dec
//!  4. Circumpolar limit – north (▲)           lat = 90° − dec
//!  5. Circumpolar limit – south (▼)           lat = −90° − dec
class StarVisibilityMapWidget : public QWidget
{
	Q_OBJECT

public:
	explicit StarVisibilityMapWidget(QWidget* parent = Q_NULLPTR);

	//! True when there is a currently selected object in Stellarium.
	bool hasSelection() const { return pendingObjectAvailable; }

	//! True when a calculation has been performed at least once.
	bool hasVisibilityResult() const { return hasCalculation; }

	// Accessors for the last calculated result — used by the dialog to
	// forward data to StarCalendarWidget.
	double  objectDecDeg()  const { return m_objectDecDeg; }
	double  objectRaDeg()   const { return m_objectRaDeg; }
	QString objectNameStr() const { return objectName; }
	int     calcYearVal()   const { return calcYear; }

signals:
	//! Emitted whenever the selected-object state changes, so the dialog
	//! can enable or disable the Calculate button accordingly.
	void selectionStateChanged(bool objectIsSelected);

	//! Emitted when the user clicks Calculate on a solar system object.
	//! The dialog uses this to disable the calendar view toggle.
	void solarSystemObjectSelected();

public slots:
	//! Called when StelObjectMgr::selectedObjectChanged fires.
	//! Only updates button-enable state; never redraws the map.
	void onSelectionChanged();

	//! Locks in the current selection + JD and redraws the visibility lines.
	//! Called by the dialog when the user clicks "Calculate visibility".
	void calculateVisibility();

	void setFlagShowGrid(bool show);
	void setFlagShowCities(bool show);
	void setFlagSetLocationOnClick(bool enable) { flagSetLocationOnClick = enable; }
	void setGoodVisibilityAltitude(int altDeg);  //!< Set the minimum altitude for the "good visibility" line (default 5°).
	void invalidateCache() { invalidateSceneCache(); update(); }
	void resetView();
	void zoomToCurrentLocation();

protected:
	void paintEvent(QPaintEvent* event) override;
	void changeEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	// ── coordinate helpers ──────────────────────────────────────────────────
	QPointF lonLatToPoint(double longitudeDeg, double latitudeDeg,
	                      const QRectF& mapRect) const;
	void screenToLonLat(const QPointF& point, const QRectF& mapRect,
	                    double& longitudeDeg, double& latitudeDeg) const;

	// ── drawing ─────────────────────────────────────────────────────────────
	void drawBaseMap(QPainter& painter, const QRectF& mapRect);
	void drawGeographicGrid(QPainter& painter, const QRectF& mapRect) const;
	void drawLocationLabels(QPainter& painter, const QRectF& mapRect) const;
	void drawVisibilityLines(QPainter& painter, const QRectF& mapRect) const;
	void drawLatitudeLine(QPainter& painter, const QRectF& mapRect,
	                      double latitudeDeg, const QPen& pen) const;
	void drawSymbolLine(QPainter& painter, const QRectF& mapRect,
	                    double latitudeDeg, int symbolType) const;
	void drawLegend(QPainter& painter, const QRectF& mapRect) const;
	void drawCurrentLocation(QPainter& painter, const QRectF& mapRect) const;
	void drawNoObjectMessage(QPainter& painter, const QRectF& mapRect) const;

	// ── cache ────────────────────────────────────────────────────────────────
	void rebuildSceneCache(const QRectF& mapRect);
	void invalidateSceneCache();
	void constrainView();

	// ── symbol type codes for drawSymbolLine ─────────────────────────────────
	enum SymbolType { SymbolPlus = 0, SymbolTriangleUp, SymbolTriangleDown };

	QPixmap earthMap;    //!< Bundled raster base map (:./DaylightMap/earth_map.png).
	StelCore*    core;

	bool   showGrid;
	bool   showCities;
	bool   flagSetLocationOnClick;
	int    goodVisibilityAltDeg;  //!< Minimum altitude for "good visibility" line, degrees (default 5)
	double centerLongitudeDeg;
	double centerLatitudeDeg;
	double longitudeSpanDeg;
	bool   dragging;
	QPoint lastMousePos;
	QPoint pressPos;

	QPixmap sceneCache;
	QSize   sceneCacheSize;
	bool    sceneCacheDirty;

	// ── Pending selection (updated by onSelectionChanged, never drawn) ────────
	bool    pendingObjectAvailable; //!< true when Stellarium has a selected object

	// ── Calculated / displayed result (set by calculateVisibility) ────────────
	bool    hasCalculation;         //!< true when lines have been calculated at least once
	bool    solarSystemSelected;    //!< true when last Calculate attempt was on a solar system object
	double  m_objectDecDeg;           //!< declination in degrees (J2000 + precession at calc time)
	double  m_objectRaDeg;            //!< RA in degrees (for caption only)
	QString objectName;             //!< localised name shown in the caption
	int     calcYear;
	int     calcMonth;
	int     calcDay;
};

#endif /* STAR_VISIBILITY_MAP_WIDGET_HPP */
