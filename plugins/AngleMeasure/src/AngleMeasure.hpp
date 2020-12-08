/*
 * Copyright (C) 2009 Matthew Gates
 * Copyright (C) 2014, 2020 Georg Zotti (extensions and properties)
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
#ifndef ANGLEMEASURE_HPP
#define ANGLEMEASURE_HPP

#include <QFont>
#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include "StelCore.hpp"

class QTimer;
class QPixmap;
class StelButton;
class AngleMeasureDialog;

/*! @defgroup angleMeasure Angle Measure plug-in
@{
The Angle Measure plugin is a small tool which is used to measure
the angular distance between two points on the sky.

*goes misty eyed* I recall measuring the size of the Cassini Division
when I was a student.
It was not the high academic glamor one might expect... It was cloudy...
It was rainy... The observatory lab had some old scopes set up at one end,
pointing at a photograph of Saturn at the other end of the lab. We measured.
We calculated. We wished we were in Hawaii.

@note Georg Zotti extended in 2014-09, enough to call it V4.0
@note Georg Zotti added properties in 2020-04, enough to call it V4.6

<b>Modes</b>

Equatorial Mode (original): mark start,end: distance/position angle in the
sky, line rotates with sky, spherical angles influenced by refraction
(numbers given on celestial sphere).

Horizontal Mode: mark start,end: distance/position angle in alt/azimuthal
coordinates, line stays fixed in alt-az system. Angle may be different near
to horizon because of refraction!

It is possible to link start and/or end to the sky. Distance/position angle
still always in alt/azimuthal coordinates.

@}
*/

//! @class AngleMeasure
//! @ingroup angleMeasure
//! Main class of the Angle Measure plug-in.
//! @author Matthew Gates
//! @author Alexander Wolf
//! @author Georg Zotti
class AngleMeasure : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled                          READ isEnabled                  WRITE enableAngleMeasure           NOTIFY flagAngleMeasureChanged)
	Q_PROPERTY(bool dmsFormat                        READ isDmsFormat                WRITE useDmsFormat                 NOTIFY dmsFormatChanged )
	Q_PROPERTY(bool flagShowEquatorial               READ isEquatorial               WRITE showEquatorial               NOTIFY flagShowEquatorialChanged )
	Q_PROPERTY(bool flagShowHorizontal               READ isHorizontal               WRITE showHorizontal               NOTIFY flagShowHorizontalChanged )
	Q_PROPERTY(bool flagShowHorizontalStartSkylinked READ isHorizontalStartSkylinked WRITE showHorizontalStartSkylinked NOTIFY flagShowHorizontalStartSkylinkedChanged )
	Q_PROPERTY(bool flagShowHorizontalEndSkylinked   READ isHorizontalEndSkylinked   WRITE showHorizontalEndSkylinked   NOTIFY flagShowHorizontalEndSkylinkedChanged )
	Q_PROPERTY(bool flagShowEquatorialPA             READ isEquatorialPADisplayed    WRITE showEquatorialPA             NOTIFY flagShowEquatorialPAChanged )
	Q_PROPERTY(bool flagShowHorizontalPA             READ isHorizontalPADisplayed    WRITE showHorizontalPA             NOTIFY flagShowHorizontalPAChanged )
	Q_PROPERTY(Vec3f equatorialTextColor             READ getEquatorialTextColor     WRITE setEquatorialTextColor       NOTIFY equatorialTextColorChanged )
	Q_PROPERTY(Vec3f equatorialLineColor             READ getEquatorialLineColor     WRITE setEquatorialLineColor       NOTIFY equatorialLineColorChanged )
	Q_PROPERTY(Vec3f horizontalTextColor             READ getHorizontalTextColor     WRITE setHorizontalTextColor       NOTIFY horizontalTextColorChanged )
	Q_PROPERTY(Vec3f horizontalLineColor             READ getHorizontalLineColor     WRITE setHorizontalLineColor       NOTIFY horizontalLineColorChanged )
public:
	AngleMeasure();
	virtual ~AngleMeasure();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	virtual bool configureGui(bool show=true);

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "AngleMeasure" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see saveSettings(), restoreSettings()
	void loadSettings();

signals:
	void flagAngleMeasureChanged(bool b);
	void dmsFormatChanged(bool b);
	void flagShowEquatorialChanged(bool b);
	void flagShowHorizontalChanged(bool b);
	void flagShowEquatorialPAChanged(bool b);
	void flagShowHorizontalPAChanged(bool b);
	void flagShowHorizontalStartSkylinkedChanged(bool b);
	void flagShowHorizontalEndSkylinkedChanged(bool b);
	void equatorialTextColorChanged(Vec3f c);
	void equatorialLineColorChanged(Vec3f c);
	void horizontalTextColorChanged(Vec3f c);
	void horizontalLineColorChanged(Vec3f c);

public slots:
	bool isEnabled() const    { return flagShowAngleMeasure; }
	bool isDmsFormat() const  { return flagUseDmsFormat; }
	bool isEquatorial() const { return flagShowEquatorial; }
	bool isHorizontal() const { return flagShowHorizontal; }
	bool isEquatorialPADisplayed() const { return flagShowEquatorialPA; }
	bool isHorizontalPADisplayed() const { return flagShowHorizontalPA; }
	bool isHorizontalStartSkylinked() const { return flagShowHorizontalStartSkylinked; }
	bool isHorizontalEndSkylinked()   const { return flagShowHorizontalEndSkylinked; }
	Vec3f getEquatorialTextColor() const { return equatorialTextColor; }
	Vec3f getEquatorialLineColor() const { return equatorialLineColor; }
	Vec3f getHorizontalTextColor() const { return horizontalTextColor; }
	Vec3f getHorizontalLineColor() const { return horizontalLineColor; }

	void enableAngleMeasure(bool b);
	void useDmsFormat(bool b);      //!< Use d/m/s instead of degree/minute/second symbols.
	void showEquatorialPA(bool b);
	void showHorizontalPA(bool b);
	void showEquatorial(bool b);
	void showHorizontal(bool b);
	void showHorizontalStartSkylinked(bool b);
	void showHorizontalEndSkylinked(bool b);
	void setEquatorialTextColor(Vec3f color);
	void setEquatorialLineColor(Vec3f color);
	void setHorizontalTextColor(Vec3f color);
	void setHorizontalLineColor(Vec3f color);

private slots:
	void updateMessageText();
	void clearMessage();

private:
	QFont font;
	bool flagShowAngleMeasure;
	bool withDecimalDegree;
	LinearFader lineVisible;
	LinearFader messageFader;
	QTimer* messageTimer;
	QString messageEnabled;
	QString messageLeftButton;
	QString messageRightButton;
	QString messagePA;
	bool dragging;
	Vec3d startPoint;
	Vec3d endPoint;
	Vec3d perp1StartPoint;
	Vec3d perp1EndPoint;
	Vec3d perp2StartPoint;
	Vec3d perp2EndPoint;
	double angleEquatorial;
	bool flagUseDmsFormat;
	bool flagShowEquatorial;
	bool flagShowHorizontal;
	bool flagShowEquatorialPA;
	bool flagShowHorizontalPA;
	bool flagShowHorizontalStartSkylinked;
	bool flagShowHorizontalEndSkylinked;
	Vec3f equatorialTextColor;
	Vec3f equatorialLineColor;
	Vec3f horizontalTextColor;
	Vec3f horizontalLineColor;
	Vec3d startPointHor;
	Vec3d endPointHor;
	Vec3d perp1StartPointHor;
	Vec3d perp1EndPointHor;
	Vec3d perp2StartPointHor;
	Vec3d perp2EndPointHor;
	double angleHorizontal;

	StelButton* toolbarButton;

	void calculateEnds();
	void calculateEndsOneLine(const Vec3d &start, const Vec3d &end, Vec3d &perp1Start, Vec3d &perp1End, Vec3d &perp2Start, Vec3d &perp2End, double &angleEquatorial);
	//! return a nicely formatted angle string.
	//! @arg angle in radians
	QString formatAngleString(double angle) const;
	QString calculatePositionAngle(const Vec3d p1, const Vec3d p2) const;
	void drawOne(StelCore *core, const StelCore::FrameType frameType, const StelCore::RefractionMode refractionMode, const Vec3f txtColor, const Vec3f lineColor);

	QSettings* conf;

	// GUI
	AngleMeasureDialog* configDialog;
};

#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class AngleMeasureStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*ANGLEMEASURE_HPP*/
