/*
 * Copyright (C) 2014 Georg Zotti
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
 
#ifndef ARCHAEOLINES_HPP
#define ARCHAEOLINES_HPP

#include <QFont>
#include <QColor>
#include <QKeyEvent>
#include "VecMath.hpp"
#include "StelModule.hpp"
#include "StelFader.hpp"
#include "StelCore.hpp"
#include "StelObjectMgr.hpp"

class QTimer;
class QPixmap;
class StelButton;
class ArchaeoLinesDialog;
class ArchaeoLine;

/*! @defgroup archaeoLines ArchaeoLines Plug-in
@{
A tool for archaeo-/ethnoastronomical alignment studies.

The ArchaeoLines plugin displays any combination of declination arcs most
relevant to archaeo- or ethnoastronomical studies. Of course, principles
used in this context are derived from natural observations, and many of
these declinations are still important in everyday astronomy.
 -# Declinations of equinoxes (i.e. equator) and the solstices
 -# Declinations of the crossquarter days (days right between solstices and equinoxes)
 -# Declinations of the Major Lunar Standstills
 -# Declinations of the Minor Lunar Standstills
 -# Declination of the Zenith passage
 -# Declination of the Nadir passage
 -# Declination of the currently selected object
 -# Current declination of the sun
 -# Current declination of the moon
 -# Current declination of a naked-eye planet
 -# Azimuth of currently selected object
 -# Hour Angle of currently selected object

Some religions, most notably Islam, adhere to a practice of observing a prayer direction towards a particular location.
Azimuth lines (vertical semicircles from zenith to nadir) for two locations can be shown. Default locations are Mecca (Kaaba) and Jerusalem.
The directions are computed based on spherical trigonometry on a spherical Earth.
In addition, up to 2 custom azimuth lines, and up to 2 custom declination lines can be drawn, also with customized labels.

The lunar lines include horizon parallax effects. There are two lines each
drawn, for maximum and minimum distance of the moon. Note that declination
of the moon at the major standstill can exceed the indicated limits if it
is high in the sky due to parallax effects.

It may be very instructive to let the time run quite fast and observe the
line of "current moon" swinging between its north and south limits each month.
These limits grow and shrink between the Major and Minor Standstills.

The sun likewise swings between the solstices. Over centuries, the solstice
declinations very slightly move as well.
@}
*/

//! @class ArchaeoLine
//! Class which manages a line (small circle) to display around the sky like the solstices line.
//! Modelled after @class SkyLine found in GridLinesMgr.cpp at V0.13.2, but with small-circle drawing.
//! @author Georg Zotti
//! @ingroup archaeoLines
class ArchaeoLine : public QObject
{
	Q_OBJECT
	Q_PROPERTY(Vec3f color READ getColor WRITE setColor NOTIFY colorChanged)
	Q_PROPERTY(bool flagLabel READ isLabelVisible WRITE setLabelVisible NOTIFY flagLabelChanged)
	Q_PROPERTY(double definingAngle READ getDefiningAngle WRITE setDefiningAngle NOTIFY definingAngleChanged)

	//Need to register Enum with Qt to be able to use it as Q_PROPERTY
	//or in signals/slots
	Q_ENUMS(Line)
public:
	enum Line { // we must start with the planet lines to allow proper handling in the combobox.
		CurrentPlanetNone, // actually a placeholder for counting/testing. By itself it makes no sense, i.e. deactivates the planet line
		CurrentPlanetMercury,
		CurrentPlanetVenus,
		CurrentPlanetMars,
		CurrentPlanetJupiter,
		CurrentPlanetSaturn,
		Equinox,
		Solstices,
		Crossquarters,
		MajorStandstill,
		MinorStandstill,
		ZenithPassage,
		NadirPassage,
		SelectedObject,
		CurrentSun,
		CurrentMoon,
		CustomDeclination1,
		CustomDeclination2,
		SelectedObjectHourAngle, // also still RA_of_date frame!
		SelectedObjectAzimuth, // This and the following types are in altaz frame!
		GeographicLocation1,
		GeographicLocation2,
		CustomAzimuth1,
		CustomAzimuth2
	};

	ArchaeoLine(ArchaeoLine::Line lineType, double definingAngle);
	virtual ~ArchaeoLine(){}
	void draw(StelCore* core, float intensity=1.0f) const;
	const Vec3f& getColor() const {return color;}
	bool isDisplayed(void) const {return fader;}
signals:
	void colorChanged(Vec3f c);
	void flagLabelChanged(bool on);
	void definingAngleChanged(double angle);
public slots:
	void setColor(const Vec3f& c);
	void update(const double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	void setFadeDuration(const float duration) {fader.setDuration(static_cast<int>(duration*1000.f));}
	void setDisplayed(const bool displayed){fader = displayed;}
	void setFontSize(const int newSize){font.setPixelSize(newSize);}
	//! To be connected to StelApp font size. newSize will be enlarged by 1.
	void setFontSizeFromApp(const int newSize){font.setPixelSize(newSize+1);}
	//! reset declination/azimuth angle (degrees) of this arc. Any azimuth angles MUST be given counted from north.
	void setDefiningAngle(const double angle);
	double getDefiningAngle(void) const {return definingAngle;} // returns declination for most, or azimuth.
	//! Re-translates the label.
	void updateLabel();
	void setLabelVisible(const bool b);
	bool isLabelVisible() const {return flagLabel;}
	void setLineType(const ArchaeoLine::Line line) {lineType=line; updateLabel();} // Meaningful only for CurrentPlanet... types
	//! change label. Used only for selected-object line - the other labels should not be changed!
	void setLabel(const QString newLabel){label=newLabel;}
	QString getLabel() const {return label;}

private:
	ArchaeoLine::Line lineType;
	double definingAngle; // degrees. This is declination for non-azimuth lines, azimuth for geographic locations and custom azimuths.
	Vec3f color;
	StelCore::FrameType frameType;
	bool flagLabel; //! show the label. (some should be permanently silent)
	QString label;
	LinearFader fader;
	QFont font;
};

//! Main class of the ArchaeoLines plug-in.
//! Provides an on-screen visualisation of several small circles relevant mainly to archaeoastronomy.
//! In addition, a few azimuth lines can be shown.
//! GZ 2014-12, updated 2016-06, 2018-10.
class ArchaeoLines : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled                            READ isEnabled                          WRITE enableArchaeoLines              NOTIFY archaeoLinesEnabledChanged)
	Q_PROPERTY(bool flagShowEquinox                    READ isEquinoxDisplayed                 WRITE showEquinox                     NOTIFY showEquinoxChanged)
	Q_PROPERTY(bool flagShowSolstices                  READ isSolsticesDisplayed               WRITE showSolstices                   NOTIFY showSolsticesChanged)
	Q_PROPERTY(bool flagShowCrossquarters              READ isCrossquartersDisplayed           WRITE showCrossquarters               NOTIFY showCrossquartersChanged)
	Q_PROPERTY(bool flagShowMajorStandstills           READ isMajorStandstillsDisplayed        WRITE showMajorStandstills            NOTIFY showMajorStandstillsChanged)
	Q_PROPERTY(bool flagShowMinorStandstills           READ isMinorStandstillsDisplayed        WRITE showMinorStandstills            NOTIFY showMinorStandstillsChanged)
	Q_PROPERTY(bool flagShowZenithPassage              READ isZenithPassageDisplayed           WRITE showZenithPassage               NOTIFY showZenithPassageChanged)
	Q_PROPERTY(bool flagShowNadirPassage               READ isNadirPassageDisplayed            WRITE showNadirPassage                NOTIFY showNadirPassageChanged)
	Q_PROPERTY(bool flagShowSelectedObject             READ isSelectedObjectDisplayed          WRITE showSelectedObject              NOTIFY showSelectedObjectChanged)
	Q_PROPERTY(bool flagShowSelectedObjectAzimuth      READ isSelectedObjectAzimuthDisplayed   WRITE showSelectedObjectAzimuth       NOTIFY showSelectedObjectAzimuthChanged)
	Q_PROPERTY(bool flagShowSelectedObjectHourAngle    READ isSelectedObjectHourAngleDisplayed WRITE showSelectedObjectHourAngle     NOTIFY showSelectedObjectHourAngleChanged)
	Q_PROPERTY(bool flagShowCurrentSun                 READ isCurrentSunDisplayed              WRITE showCurrentSun                  NOTIFY showCurrentSunChanged)
	Q_PROPERTY(bool flagShowCurrentMoon                READ isCurrentMoonDisplayed             WRITE showCurrentMoon                 NOTIFY showCurrentMoonChanged)
	Q_PROPERTY(ArchaeoLine::Line enumShowCurrentPlanet READ whichCurrentPlanetDisplayed        WRITE showCurrentPlanet               NOTIFY currentPlanetChanged)
	Q_PROPERTY(bool flagShowGeographicLocation1        READ isGeographicLocation1Displayed     WRITE showGeographicLocation1         NOTIFY showGeographicLocation1Changed)
	Q_PROPERTY(bool flagShowGeographicLocation2        READ isGeographicLocation2Displayed     WRITE showGeographicLocation2         NOTIFY showGeographicLocation2Changed)
	Q_PROPERTY(double geographicLocation1Longitude     READ getGeographicLocation1Longitude    WRITE setGeographicLocation1Longitude NOTIFY geographicLocation1Changed)
	Q_PROPERTY(double geographicLocation1Latitude      READ getGeographicLocation1Latitude     WRITE setGeographicLocation1Latitude  NOTIFY geographicLocation1Changed)
	Q_PROPERTY(double geographicLocation2Longitude     READ getGeographicLocation2Longitude    WRITE setGeographicLocation2Longitude NOTIFY geographicLocation2Changed)
	Q_PROPERTY(double geographicLocation2Latitude      READ getGeographicLocation2Latitude     WRITE setGeographicLocation2Latitude  NOTIFY geographicLocation2Changed)
	Q_PROPERTY(bool flagShowCustomAzimuth1             READ isCustomAzimuth1Displayed          WRITE showCustomAzimuth1              NOTIFY showCustomAzimuth1Changed)
	Q_PROPERTY(bool flagShowCustomAzimuth2             READ isCustomAzimuth2Displayed          WRITE showCustomAzimuth2              NOTIFY showCustomAzimuth2Changed)
	// Note: following 2 are only "forwarding properties", no proper variables!
	Q_PROPERTY(double customAzimuth1 READ getCustomAzimuth1 WRITE setCustomAzimuth1 NOTIFY customAzimuth1Changed)
	Q_PROPERTY(double customAzimuth2 READ getCustomAzimuth2 WRITE setCustomAzimuth2 NOTIFY customAzimuth2Changed)
	Q_PROPERTY(bool flagShowCustomDeclination1 READ   isCustomDeclination1Displayed WRITE  showCustomDeclination1 NOTIFY showCustomDeclination1Changed)
	Q_PROPERTY(bool flagShowCustomDeclination2 READ   isCustomDeclination2Displayed WRITE  showCustomDeclination2 NOTIFY showCustomDeclination2Changed)
	// Note: following 2 are only "forwarding properties", no proper variables!
	Q_PROPERTY(double customDeclination1 READ getCustomDeclination1 WRITE setCustomDeclination1 NOTIFY customDeclination1Changed)
	Q_PROPERTY(double customDeclination2 READ getCustomDeclination2 WRITE setCustomDeclination2 NOTIFY customDeclination2Changed)

	// More "forwarding properties" for geo locations and custom azimuths/declination labels.
	Q_PROPERTY(QString geographicLocation1Label READ getGeographicLocation1Label WRITE setGeographicLocation1Label NOTIFY geographicLocation1LabelChanged)
	Q_PROPERTY(QString geographicLocation2Label READ getGeographicLocation2Label WRITE setGeographicLocation2Label NOTIFY geographicLocation2LabelChanged)
	Q_PROPERTY(QString customAzimuth1Label      READ getCustomAzimuth1Label      WRITE setCustomAzimuth1Label      NOTIFY customAzimuth1LabelChanged)
	Q_PROPERTY(QString customAzimuth2Label      READ getCustomAzimuth2Label      WRITE setCustomAzimuth2Label      NOTIFY customAzimuth2LabelChanged)
	Q_PROPERTY(QString customDeclination1Label  READ getCustomDeclination1Label  WRITE setCustomDeclination1Label  NOTIFY customDeclination1LabelChanged)
	Q_PROPERTY(QString customDeclination2Label  READ getCustomDeclination2Label  WRITE setCustomDeclination2Label  NOTIFY customDeclination2LabelChanged)

	// Properties to simplify color setting.
	Q_PROPERTY(Vec3f equinoxColor 		      READ getEquinoxColor                 WRITE setEquinoxColor                 NOTIFY equinoxColorChanged                 )
	Q_PROPERTY(Vec3f solsticesColor               READ getSolsticesColor               WRITE setSolsticesColor               NOTIFY solsticesColorChanged               )
	Q_PROPERTY(Vec3f crossquartersColor           READ getCrossquartersColor           WRITE setCrossquartersColor           NOTIFY crossquartersColorChanged           )
	Q_PROPERTY(Vec3f majorStandstillColor         READ getMajorStandstillColor         WRITE setMajorStandstillColor         NOTIFY majorStandstillColorChanged         )
	Q_PROPERTY(Vec3f minorStandstillColor         READ getMinorStandstillColor         WRITE setMinorStandstillColor         NOTIFY minorStandstillColorChanged         )
	Q_PROPERTY(Vec3f zenithPassageColor           READ getZenithPassageColor           WRITE setZenithPassageColor           NOTIFY zenithPassageColorChanged           )
	Q_PROPERTY(Vec3f nadirPassageColor            READ getNadirPassageColor            WRITE setNadirPassageColor            NOTIFY nadirPassageColorChanged            )
	Q_PROPERTY(Vec3f selectedObjectColor          READ getSelectedObjectColor          WRITE setSelectedObjectColor          NOTIFY selectedObjectColorChanged          )
	Q_PROPERTY(Vec3f selectedObjectAzimuthColor   READ getSelectedObjectAzimuthColor   WRITE setSelectedObjectAzimuthColor   NOTIFY selectedObjectAzimuthColorChanged   )
	Q_PROPERTY(Vec3f selectedObjectHourAngleColor READ getSelectedObjectHourAngleColor WRITE setSelectedObjectHourAngleColor NOTIFY selectedObjectHourAngleColorChanged )
	Q_PROPERTY(Vec3f currentSunColor              READ getCurrentSunColor              WRITE setCurrentSunColor              NOTIFY currentSunColorChanged              )
	Q_PROPERTY(Vec3f currentMoonColor             READ getCurrentMoonColor             WRITE setCurrentMoonColor             NOTIFY currentMoonColorChanged             )
	Q_PROPERTY(Vec3f currentPlanetColor           READ getCurrentPlanetColor           WRITE setCurrentPlanetColor           NOTIFY currentPlanetColorChanged           )
	Q_PROPERTY(Vec3f geographicLocation1Color     READ getGeographicLocation1Color     WRITE setGeographicLocation1Color     NOTIFY geographicLocation1ColorChanged     )
	Q_PROPERTY(Vec3f geographicLocation2Color     READ getGeographicLocation2Color     WRITE setGeographicLocation2Color     NOTIFY geographicLocation2ColorChanged     )
	Q_PROPERTY(Vec3f customAzimuth1Color          READ getCustomAzimuth1Color          WRITE setCustomAzimuth1Color          NOTIFY customAzimuth1ColorChanged          )
	Q_PROPERTY(Vec3f customAzimuth2Color          READ getCustomAzimuth2Color          WRITE setCustomAzimuth2Color          NOTIFY customAzimuth2ColorChanged          )
	Q_PROPERTY(Vec3f customDeclination1Color      READ getCustomDeclination1Color      WRITE setCustomDeclination1Color      NOTIFY customDeclination1ColorChanged      )
	Q_PROPERTY(Vec3f customDeclination2Color      READ getCustomDeclination2Color      WRITE setCustomDeclination2Color      NOTIFY customDeclination2ColorChanged      )

	Q_PROPERTY(int lineWidth    READ getLineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
public:
	ArchaeoLines();
	virtual ~ArchaeoLines() Q_DECL_OVERRIDE;
	

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;
	virtual void handleKeys(class QKeyEvent* event) Q_DECL_OVERRIDE {event->setAccepted(false);}
	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;
	//////////////////////////////////////////////////////////////////////////

	//! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings().
	void restoreDefaultSettings();

	//! Load the plug-in's settings from the configuration file.
	//! Settings are kept in the "ArchaeoLines" section in Stellarium's
	//! configuration file. If no such section exists, it will load default
	//! values.
	//! @see restoreDefaultSettings()
	void loadSettings();

signals:
	void archaeoLinesEnabledChanged(bool on);
	void showEquinoxChanged(bool on);
	void showSolsticesChanged(bool on);
	void showCrossquartersChanged(bool on);
	void showMajorStandstillsChanged(bool on);
	void showMinorStandstillsChanged(bool on);
	void showZenithPassageChanged(bool on);
	void showNadirPassageChanged(bool on);
	void showSelectedObjectChanged(bool on);
	void showSelectedObjectAzimuthChanged(bool on);
	void showSelectedObjectHourAngleChanged(bool on);
	void showCurrentSunChanged(bool on);
	void showCurrentMoonChanged(bool on);
	void showGeographicLocation1Changed(bool on);
	void showGeographicLocation2Changed(bool on);
	void geographicLocation1Changed();
	void geographicLocation2Changed();
	void showCustomAzimuth1Changed(bool on);
	void showCustomAzimuth2Changed(bool on);
	void customAzimuth1Changed(double az);
	void customAzimuth2Changed(double az);
	void showCustomDeclination1Changed(bool on);
	void showCustomDeclination2Changed(bool on);
	void customDeclination1Changed(double dec);
	void customDeclination2Changed(double dec);
	void currentPlanetChanged(ArchaeoLine::Line l); // meaningful only CurrentPlanetNone...CurrentPlanetSaturn.
	void geographicLocation1LabelChanged(QString label);
	void geographicLocation2LabelChanged(QString label);
	void customAzimuth1LabelChanged(QString label);
	void customAzimuth2LabelChanged(QString label);
	void customDeclination1LabelChanged(QString label);
	void customDeclination2LabelChanged(QString label);
	void equinoxColorChanged(Vec3f color);
	void solsticesColorChanged(Vec3f color);
	void crossquartersColorChanged(Vec3f color);
	void majorStandstillColorChanged(Vec3f color);
	void minorStandstillColorChanged(Vec3f color);
	void zenithPassageColorChanged(Vec3f color);
	void nadirPassageColorChanged(Vec3f color);
	void selectedObjectColorChanged(Vec3f color);
	void selectedObjectAzimuthColorChanged(Vec3f color);
	void selectedObjectHourAngleColorChanged(Vec3f color);
	void currentSunColorChanged(Vec3f color);
	void currentMoonColorChanged(Vec3f color);
	void currentPlanetColorChanged(Vec3f color);
	void geographicLocation1ColorChanged(Vec3f color);
	void geographicLocation2ColorChanged(Vec3f color);
	void customAzimuth1ColorChanged(Vec3f color);
	void customAzimuth2ColorChanged(Vec3f color);
	void customDeclination1ColorChanged(Vec3f color);
	void customDeclination2ColorChanged(Vec3f color);
	void lineWidthChanged(int width);

public slots:
	void enableArchaeoLines(bool b);

	bool isEnabled() const {return flagShowArchaeoLines;}
	bool isEquinoxDisplayed() const {return flagShowEquinox;}
	bool isSolsticesDisplayed() const {return flagShowSolstices;}
	bool isCrossquartersDisplayed() const {return flagShowCrossquarters;}
	bool isMajorStandstillsDisplayed() const {return flagShowMajorStandstills;}
	bool isMinorStandstillsDisplayed() const {return flagShowMinorStandstills;}
	bool isZenithPassageDisplayed() const {return flagShowZenithPassage;}
	bool isNadirPassageDisplayed() const {return flagShowNadirPassage;}
	bool isSelectedObjectDisplayed() const {return flagShowSelectedObject;}
	bool isSelectedObjectAzimuthDisplayed() const {return flagShowSelectedObjectAzimuth;}
	bool isSelectedObjectHourAngleDisplayed() const {return flagShowSelectedObjectHourAngle;}
	bool isCurrentSunDisplayed() const {return flagShowCurrentSun;}
	bool isCurrentMoonDisplayed() const {return flagShowCurrentMoon;}
	ArchaeoLine::Line whichCurrentPlanetDisplayed() const {return enumShowCurrentPlanet;}
	bool isGeographicLocation1Displayed() const {return flagShowGeographicLocation1;}
	bool isGeographicLocation2Displayed() const {return flagShowGeographicLocation2;}
	bool isCustomAzimuth1Displayed() const {return flagShowCustomAzimuth1;}
	bool isCustomAzimuth2Displayed() const {return flagShowCustomAzimuth2;}
	bool isCustomDeclination1Displayed() const {return flagShowCustomDeclination1;}
	bool isCustomDeclination2Displayed() const {return flagShowCustomDeclination2;}

	void showEquinox(bool b);
	void showSolstices(bool b);
	void showCrossquarters(bool b);
	void showMajorStandstills(bool b);
	void showMinorStandstills(bool b);
	void showZenithPassage(bool b);
	void showNadirPassage(bool b);
	void showSelectedObject(bool b);
	void showSelectedObjectAzimuth(bool b);
	void showSelectedObjectHourAngle(bool b);
	void showCurrentSun(bool b);
	void showCurrentMoon(bool b);
	void showCurrentPlanet(ArchaeoLine::Line l); // Allowed values for l: CurrentPlanetNone...CurrentPlanetSaturn.
	void showCurrentPlanetNamed(QString planet); // Allowed values for planet: "none", "Mercury", "Venus", "Mars", "Jupiter", "Saturn".
	void showGeographicLocation1(bool b);
	void showGeographicLocation2(bool b);
	void setGeographicLocation1Longitude(double lng);
	void setGeographicLocation1Latitude(double lat);
	void setGeographicLocation2Longitude(double lng);
	void setGeographicLocation2Latitude(double lat);
	void setGeographicLocation1Label(QString label);
	void setGeographicLocation2Label(QString label);
	QString getGeographicLocation1Label() const {return geographicLocation1Line->getLabel();}
	QString getGeographicLocation2Label() const {return geographicLocation2Line->getLabel();}
	double getGeographicLocation1Longitude() const {return geographicLocation1Longitude; }
	double getGeographicLocation1Latitude()  const {return geographicLocation1Latitude; }
	double getGeographicLocation2Longitude() const {return geographicLocation2Longitude; }
	double getGeographicLocation2Latitude()  const {return geographicLocation2Latitude; }
	void showCustomAzimuth1(bool b);
	void showCustomAzimuth2(bool b);
	void setCustomAzimuth1(double az);
	double getCustomAzimuth1() const { return customAzimuth1Line->getDefiningAngle(); }
	void setCustomAzimuth2(double az);
	double getCustomAzimuth2() const { return customAzimuth2Line->getDefiningAngle(); }
	void setCustomAzimuth1Label(QString label);
	void setCustomAzimuth2Label(QString label);
	QString getCustomAzimuth1Label() const {return customAzimuth1Line->getLabel();}
	QString getCustomAzimuth2Label() const {return customAzimuth2Line->getLabel();}
	void showCustomDeclination1(bool b);
	void showCustomDeclination2(bool b);
	void setCustomDeclination1(double dec);
	double getCustomDeclination1() const { return customDeclination1Line->getDefiningAngle(); }
	void setCustomDeclination2(double dec);
	double getCustomDeclination2() const { return customDeclination2Line->getDefiningAngle(); }
	void setCustomDeclination1Label(QString label);
	void setCustomDeclination2Label(QString label);
	QString getCustomDeclination1Label() const {return customDeclination1Line->getLabel();}
	QString getCustomDeclination2Label() const {return customDeclination2Line->getLabel();}

	// called by the dialog GUI, converts GUI's QColor (0..255) to Stellarium's Vec3f float color.
	//void setLineColor(ArchaeoLine::Line whichLine, QColor color);
	// called by the dialog UI, converts Stellarium's Vec3f float color to QColor (0..255).
	//QColor getLineColor(ArchaeoLine::Line whichLine) const;
	//! query a line for its current defining angle. Returns declination or azimuth, respectively.
	double getLineAngle(ArchaeoLine::Line whichLine) const;
	QString getLineLabel(ArchaeoLine::Line whichLine) const;

	// Trivial property getters/setters
	Vec3f getEquinoxColor()                 const {return equinoxColor;}
	Vec3f getSolsticesColor()               const {return solsticesColor;}
	Vec3f getCrossquartersColor()           const {return crossquartersColor;}
	Vec3f getMajorStandstillColor()         const {return majorStandstillColor;}
	Vec3f getMinorStandstillColor()         const {return minorStandstillColor;}
	Vec3f getZenithPassageColor()           const {return zenithPassageColor;}
	Vec3f getNadirPassageColor()            const {return nadirPassageColor;}
	Vec3f getSelectedObjectColor()          const {return selectedObjectColor;}
	Vec3f getSelectedObjectAzimuthColor()   const {return selectedObjectAzimuthColor;}
	Vec3f getSelectedObjectHourAngleColor() const {return selectedObjectHourAngleColor;}
	Vec3f getCurrentSunColor()              const {return currentSunColor;}
	Vec3f getCurrentMoonColor()             const {return currentMoonColor;}
	Vec3f getCurrentPlanetColor()           const {return currentPlanetColor;}
	Vec3f getGeographicLocation1Color()     const {return geographicLocation1Color;}
	Vec3f getGeographicLocation2Color()     const {return geographicLocation2Color;}
	Vec3f getCustomAzimuth1Color()          const {return customAzimuth1Color;}
	Vec3f getCustomAzimuth2Color()          const {return customAzimuth2Color;}
	Vec3f getCustomDeclination1Color()      const {return customDeclination1Color;}
	Vec3f getCustomDeclination2Color()      const {return customDeclination2Color;}
	void setEquinoxColor(Vec3f color);
	void setSolsticesColor(Vec3f color);
	void setCrossquartersColor(Vec3f color);
	void setMajorStandstillColor(Vec3f color);
	void setMinorStandstillColor(Vec3f color);
	void setZenithPassageColor(Vec3f color);
	void setNadirPassageColor(Vec3f color);
	void setSelectedObjectColor(Vec3f color);
	void setSelectedObjectAzimuthColor(Vec3f color);
	void setSelectedObjectHourAngleColor(Vec3f color);
	void setCurrentSunColor(Vec3f color);
	void setCurrentMoonColor(Vec3f color);
	void setCurrentPlanetColor(Vec3f color);
	void setGeographicLocation1Color(Vec3f color);
	void setGeographicLocation2Color(Vec3f color);
	void setCustomAzimuth1Color(Vec3f color);
	void setCustomAzimuth2Color(Vec3f color);
	void setCustomDeclination1Color(Vec3f color);
	void setCustomDeclination2Color(Vec3f color);

	int getLineWidth() const {return lineWidth;}
	void setLineWidth(int width);

private slots:
	//! a slot connected to core which cares for location changes, updating the geographicLocation lines.
	void updateObserverLocation(const StelLocation &loc);

private:
	QFont font;
	bool flagShowArchaeoLines;
	LinearFader lineFader;
	int lineWidth;

	Vec3f equinoxColor;
	Vec3f solsticesColor;
	Vec3f crossquartersColor;
	Vec3f majorStandstillColor;
	Vec3f minorStandstillColor;
	Vec3f zenithPassageColor;
	Vec3f nadirPassageColor;
	Vec3f selectedObjectColor;
	Vec3f selectedObjectAzimuthColor;
	Vec3f selectedObjectHourAngleColor;
	Vec3f currentSunColor;
	Vec3f currentMoonColor;
	Vec3f currentPlanetColor;
	Vec3f geographicLocation1Color;
	Vec3f geographicLocation2Color;
	Vec3f customAzimuth1Color;
	Vec3f customAzimuth2Color;
	Vec3f customDeclination1Color;
	Vec3f customDeclination2Color;

	bool flagShowEquinox;
	bool flagShowSolstices;
	bool flagShowCrossquarters;
	bool flagShowMajorStandstills;
	bool flagShowMinorStandstills;
	bool flagShowZenithPassage;
	bool flagShowNadirPassage;
	bool flagShowSelectedObject;
	bool flagShowSelectedObjectAzimuth;
	bool flagShowSelectedObjectHourAngle;
	bool flagShowCurrentSun;
	bool flagShowCurrentMoon;
	ArchaeoLine::Line enumShowCurrentPlanet;
	bool flagShowGeographicLocation1;
	double geographicLocation1Longitude;
	double geographicLocation1Latitude;
	bool flagShowGeographicLocation2;
	double geographicLocation2Longitude;
	double geographicLocation2Latitude;
	bool flagShowCustomAzimuth1;
	bool flagShowCustomAzimuth2;
	bool flagShowCustomDeclination1;
	bool flagShowCustomDeclination2;
	double lastJDE; // cache last-time-computed to every 10 days or so?

	ArchaeoLine * equinoxLine;
	ArchaeoLine * northernSolsticeLine;
	ArchaeoLine * southernSolsticeLine;
	ArchaeoLine * northernCrossquarterLine;
	ArchaeoLine * southernCrossquarterLine;
	ArchaeoLine * northernMajorStandstillLine0;
	ArchaeoLine * northernMajorStandstillLine1;
	ArchaeoLine * northernMinorStandstillLine2;
	ArchaeoLine * northernMinorStandstillLine3;
	ArchaeoLine * southernMinorStandstillLine4;
	ArchaeoLine * southernMinorStandstillLine5;
	ArchaeoLine * southernMajorStandstillLine6;
	ArchaeoLine * southernMajorStandstillLine7;
	ArchaeoLine * zenithPassageLine;
	ArchaeoLine * nadirPassageLine;
	ArchaeoLine * selectedObjectLine;
	ArchaeoLine * selectedObjectAzimuthLine;
	ArchaeoLine * selectedObjectHourAngleLine;
	ArchaeoLine * currentSunLine;
	ArchaeoLine * currentMoonLine;
	ArchaeoLine * currentPlanetLine;
	ArchaeoLine * geographicLocation1Line;
	ArchaeoLine * geographicLocation2Line;
	ArchaeoLine * customAzimuth1Line;
	ArchaeoLine * customAzimuth2Line;
	ArchaeoLine * customDeclination1Line;
	ArchaeoLine * customDeclination2Line;

	StelButton* toolbarButton;
  
	QSettings* conf;

	// GUI
	ArchaeoLinesDialog* configDialog;
	StelCore* core; // used quite often, better keep a reference...
	StelObjectMgr* objMgr;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class ArchaeoLinesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule()    const Q_DECL_OVERRIDE;
	virtual StelPluginInfo getPluginInfo() const Q_DECL_OVERRIDE;
	virtual QObjectList getExtensionList() const Q_DECL_OVERRIDE { return QObjectList(); }
};

#endif /*ARCHAEOLINES_HPP*/

