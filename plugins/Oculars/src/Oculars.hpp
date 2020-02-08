/*
 * Copyright (C) 2009 Timothy Reaves
 * Copyright (C) 2011 Bogdan Marinov
 * Copyright (C) 2017 Georg Zotti
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

#ifndef OCULARS_HPP
#define OCULARS_HPP

#include "CCD.hpp"
#include "Lens.hpp"
#include "Ocular.hpp"
#include "OcularDialog.hpp"
#include "StelModule.hpp"
#include "StelTexture.hpp"
#include "Telescope.hpp"
#include "VecMath.hpp"

#include <QFont>
#include <QSettings>

#define MIN_OCULARS_INI_VERSION 3.1f
#define DEFAULT_CCD_CROP_OVERLAY_SIZE 250

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QMenu;
class QMouseEvent;
class QPixmap;
class QSettings;
class QSignalMapper;
QT_END_NAMESPACE

class StelButton;
class StelAction;

/*! @defgroup oculars Oculars Plug-in
@{
This plugin serves several purposes:
 - the primary use is to see what the sky looks like through a particular
combination of eyepiece and telescope. I (TR) wanted to be able to get an idea of
what I should see when looking through a physical telescope, and understand
why one eyepiece may be better suited to a particular target. This can also
be very useful in deciding what telescope is best suited to a style of viewing.
And with the support for binoculars, you now have the ability to understand just
about any type of visual observing.
 - to show what a particular camera would be able to photograph of the sky. Also
to better plan what type of telescope (or camera lens) to pair with a particular
camera to capture what you want.
 - lastly, with the help of the Telrad sight, understand where object in the sky
are in relation to each other. This can be very useful if you have a non-GOTO
telescope, and to get an idea of how to star-hop from a known location to an
area of interest.

None of these activities can take the place of hands-on experience, but they are
a good way to supplement your visual astronomy interests.
@}
*/

//! Main class of the Oculars plug-in.
//! @class Oculars
//! @ingroup oculars
class Oculars : public StelModule
{
	Q_OBJECT

	Q_PROPERTY(bool enableOcular     READ getEnableOcular     WRITE enableOcular     NOTIFY enableOcularChanged)
	Q_PROPERTY(bool enableCrosshairs READ getEnableCrosshairs WRITE toggleCrosshairs NOTIFY enableCrosshairsChanged)
	Q_PROPERTY(bool enableCCD        READ getEnableCCD        WRITE toggleCCD        NOTIFY enableCCDChanged)
	Q_PROPERTY(bool enableTelrad     READ getEnableTelrad     WRITE toggleTelrad     NOTIFY enableTelradChanged)

	Q_PROPERTY(int selectedCCDIndex       READ getSelectedCCDIndex       WRITE selectCCDAtIndex       NOTIFY selectedCCDChanged)
	Q_PROPERTY(int selectedOcularIndex    READ getSelectedOcularIndex    WRITE selectOcularAtIndex    NOTIFY selectedOcularChanged)
	Q_PROPERTY(int selectedTelescopeIndex READ getSelectedTelescopeIndex WRITE selectTelescopeAtIndex NOTIFY selectedTelescopeChanged)
	Q_PROPERTY(int selectedLensIndex      READ getSelectedLensIndex      WRITE selectLensAtIndex      NOTIFY selectedLensChanged)
	Q_PROPERTY(double selectedCCDRotationAngle READ getSelectedCCDRotationAngle WRITE setSelectedCCDRotationAngle NOTIFY selectedCCDRotationAngleChanged)

	Q_PROPERTY(bool flagGuiPanelEnabled          READ getFlagGuiPanelEnabled          WRITE enableGuiPanel NOTIFY flagGuiPanelEnabledChanged)
	Q_PROPERTY(bool flagInitFOVUsage             READ getFlagInitFovUsage             WRITE setFlagInitFovUsage NOTIFY flagInitFOVUsageChanged) 
	Q_PROPERTY(bool flagInitDirectionUsage       READ getFlagInitDirectionUsage       WRITE setFlagInitDirectionUsage NOTIFY flagInitDirectionUsageChanged) 
	Q_PROPERTY(bool flagShowResolutionCriteria   READ getFlagShowResolutionCriteria   WRITE setFlagShowResolutionCriteria  NOTIFY flagShowResolutionCriteriaChanged)
	Q_PROPERTY(bool flagRequireSelection   READ getFlagRequireSelection    WRITE setFlagRequireSelection    NOTIFY flagRequireSelectionChanged) 
	Q_PROPERTY(bool flagLimitMagnitude     READ getFlagLimitMagnitude      WRITE setFlagLimitMagnitude      NOTIFY flagLimitMagnitudeChanged) 
	Q_PROPERTY(bool flagHideGridsLines     READ getFlagHideGridsLines      WRITE setFlagHideGridsLines      NOTIFY flagHideGridsLinesChanged)
	Q_PROPERTY(bool flagScaleImageCircle   READ getFlagScaleImageCircle    WRITE setFlagScaleImageCircle    NOTIFY flagScaleImageCircleChanged)// flag scale image circle scaleImageCirclCheckBox
	Q_PROPERTY(bool flagSemiTransparency   READ getFlagUseSemiTransparency WRITE setFlagUseSemiTransparency NOTIFY flagUseSemiTransparencyChanged) 
	Q_PROPERTY(bool flagDMSDegrees         READ getFlagDMSDegrees          WRITE setFlagDMSDegrees          NOTIFY flagDMSDegreesChanged)
	Q_PROPERTY(bool flagAutosetMountForCCD READ getFlagAutosetMountForCCD  WRITE setFlagAutosetMountForCCD  NOTIFY flagAutosetMountForCCDChanged)
	Q_PROPERTY(bool flagScalingFOVForTelrad	READ getFlagScalingFOVForTelrad  WRITE setFlagScalingFOVForTelrad  NOTIFY flagScalingFOVForTelradChanged)
	Q_PROPERTY(bool flagScalingFOVForCCD	READ getFlagScalingFOVForCCD  WRITE setFlagScalingFOVForCCD  NOTIFY flagScalingFOVForCCDChanged)
	Q_PROPERTY(bool flagShowOcularsButton	READ getFlagShowOcularsButton  WRITE setFlagShowOcularsButton  NOTIFY flagShowOcularsButtonChanged)
	Q_PROPERTY(bool flagShowContour		READ getFlagShowContour   WRITE setFlagShowContour   NOTIFY flagShowContourChanged)
	Q_PROPERTY(bool flagShowCardinals		READ getFlagShowCardinals   WRITE setFlagShowCardinals   NOTIFY flagShowCardinalsChanged)
	Q_PROPERTY(bool flagAlignCrosshair		READ getFlagAlignCrosshair   WRITE setFlagAlignCrosshair   NOTIFY flagAlignCrosshairChanged)

	Q_PROPERTY(double arrowButtonScale     READ getArrowButtonScale        WRITE setArrowButtonScale        NOTIFY arrowButtonScaleChanged)
	Q_PROPERTY(int guiPanelFontSize        READ getGuiPanelFontSize        WRITE setGuiPanelFontSize        NOTIFY guiPanelFontSizeChanged)
	Q_PROPERTY(Vec3f textColor             READ getTextColor               WRITE setTextColor               NOTIFY textColorChanged)
	Q_PROPERTY(Vec3f lineColor             READ getLineColor               WRITE setLineColor               NOTIFY textColorChanged)

	Q_PROPERTY(bool flagShowCcdCropOverlay READ getFlagShowCcdCropOverlay  WRITE setFlagShowCcdCropOverlay  NOTIFY flagShowCcdCropOverlayChanged)
	Q_PROPERTY(int ccdCropOverlaySize      READ getCcdCropOverlaySize      WRITE setCcdCropOverlaySize      NOTIFY ccdCropOverlaySizeChanged)

	//BM: Temporary, until the GUI is finalized and some other method of getting
	//info from the main class is implemented.
	friend class OcularsGuiPanel;

public:
	Oculars();
	virtual ~Oculars() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;
	//! Returns the module-specific style sheet.	
	virtual void handleMouseClicks(class QMouseEvent* event) Q_DECL_OVERRIDE;
	virtual void update(double) Q_DECL_OVERRIDE {;}

	QString getDimensionsString(double fovX, double fovY) const;
	QString getFOVString(double fov) const;

public slots:
	//! return the plugin's own settings (these do not include the settings from the main application)
	//! This implementation return a singleton (class static) QSettings object.
	virtual QSettings* getSettings() Q_DECL_OVERRIDE;
	//! Update the ocular, telescope and sensor lists after the removal of a member.
	//! Necessary because of the way model/view management in the OcularDialog
	//! is implemented.
	void updateLists();
	void ccdRotationReset();
	void decrementCCDIndex();
	void decrementOcularIndex();
	void decrementTelescopeIndex();
	void decrementLensIndex();
	void displayPopupMenu();
	//! This method is called with we detect that our hot key is pressed.  It handles
	//! determining if we should do anything - based on a selected object.
	void enableOcular(bool b);
	bool getEnableOcular() const { return flagShowOculars; }
	void incrementCCDIndex();
	void incrementOcularIndex();
	void incrementTelescopeIndex();
	void incrementLensIndex();
	void disableLens();
	// Rotate reticle
	void  rotateReticleClockwise();
	void  rotateReticleCounterclockwise();

	void rotateCCD(int amount);     //!< amount must be a number. This adds to the current rotation.
	double getSelectedCCDRotationAngle(void) const; //!< get rotation angle from currently selected CCD
	void setSelectedCCDRotationAngle(double angle); //!< set rotation angle for currently selected CCD
	
	void selectCCDAtIndex(int index);           //!< index in the range of -1:ccds.count(), else call is ignored
	int getSelectedCCDIndex() const {return selectedCCDIndex; }

	void selectOcularAtIndex(int index);            //!< index in the range of -1:oculars.count(), else call is ignored
	int getSelectedOcularIndex() const {return selectedOcularIndex; }

	void selectTelescopeAtIndex(int index);            //!< index in the range of -1:telescopes.count(), else call is ignored
	int getSelectedTelescopeIndex() const {return selectedTelescopeIndex; }

	void selectLensAtIndex(int index);           //!< index in the range -1:lenses.count(), else call is ignored
	int getSelectedLensIndex() const {return selectedLensIndex; }

	//! Toggles the sensor frame overlay.
	void toggleCCD(bool show);
	//! Toggles the sensor frame overlay (overloaded for blind switching).
	void toggleCCD();
	bool getEnableCCD() const { return flagShowCCD; }
	void toggleCrosshairs(bool show = true);
	bool getEnableCrosshairs() const { return flagShowCrosshairs; }
	//! Toggles the Telrad sight overlay.
	void toggleTelrad(bool show);
	bool getEnableTelrad() const { return flagShowTelrad; }
	//! Toggles the Telrad sight overlay (overloaded for blind switching).
	void toggleTelrad();
	
	void enableGuiPanel(bool enable = true);
	bool getFlagGuiPanelEnabled(void) const {return flagGuiPanelEnabled;}
	void setGuiPanelFontSize(int size);
	int getGuiPanelFontSize()const {return guiPanelFontSize;}

	void setTextColor(Vec3f color) {textColor=color; emit textColorChanged(color);}
	Vec3f getTextColor() const {return textColor;}
	void setLineColor(Vec3f color) {lineColor=color; emit lineColorChanged(color);}
	Vec3f getLineColor() const {return lineColor;}

	void setFlagDMSDegrees(const bool b);
	bool getFlagDMSDegrees(void) const;

	void setFlagRequireSelection(const bool b);
	bool getFlagRequireSelection(void) const;
	
	void setFlagLimitMagnitude(const bool b);
	bool getFlagLimitMagnitude(void) const;

	void setFlagInitFovUsage(const bool b);
	bool getFlagInitFovUsage(void) const;

	void setFlagInitDirectionUsage(const bool b);
	bool getFlagInitDirectionUsage(void) const;

	void setFlagAutosetMountForCCD(const bool b);
	bool getFlagAutosetMountForCCD(void) const;

	void setFlagScalingFOVForTelrad(const bool b);
	bool getFlagScalingFOVForTelrad(void) const;

	void setFlagScalingFOVForCCD(const bool b);
	bool getFlagScalingFOVForCCD(void) const;

	void setFlagUseSemiTransparency(const bool b);
	bool getFlagUseSemiTransparency(void) const;

	void setFlagShowResolutionCriteria(const bool b);
	bool getFlagShowResolutionCriteria(void) const;

	void setCcdCropOverlaySize(int size);
	int getCcdCropOverlaySize()const {return ccdCropOverlaySize;}

	void setArrowButtonScale(const double val);
	double getArrowButtonScale() const;

	void setFlagHideGridsLines(const bool b);
	bool getFlagHideGridsLines(void) const;
	
	void setFlagScaleImageCircle(bool state);
	bool getFlagScaleImageCircle(void) const { return flagScaleImageCircle;}

	//! Define whether the button toggling eyepieces should be visible
	void setFlagShowOcularsButton(bool b);
	bool getFlagShowOcularsButton(void) { return flagShowOcularsButton; }

	void setFontSize(int s){font.setPixelSize(s);}
	//! Connect this to StelApp font size.
	void setFontSizeFromApp(int s){font.setPixelSize(s+1);}

	void setFlagShowCcdCropOverlay(const bool b);
	bool getFlagShowCcdCropOverlay(void) const;

	void setFlagShowContour(const bool b);
	bool getFlagShowContour(void) const;

	void setFlagShowCardinals(const bool b);
	bool getFlagShowCardinals(void) const;

	void setFlagAlignCrosshair(const bool b);
	bool getFlagAlignCrosshair(void) const;

signals:
	void enableOcularChanged(bool value);
	void enableCrosshairsChanged(bool value);
	void enableCCDChanged(bool value);
	void enableTelradChanged(bool value);
	void selectedCCDChanged(int value);
	void selectedOcularChanged(int value);
	void selectedTelescopeChanged(int value);
	void selectedLensChanged(int value);
	void selectedCCDRotationAngleChanged(double value);

	void flagGuiPanelEnabledChanged(bool value);
	void guiPanelFontSizeChanged(int value);
	void textColorChanged(Vec3f color);
	void lineColorChanged(Vec3f color);
	void flagHideGridsLinesChanged(bool value);
	void flagAutosetMountForCCDChanged(bool value);
	void flagScalingFOVForTelradChanged(bool value);
	void flagScalingFOVForCCDChanged(bool value);
	void flagUseSemiTransparencyChanged(bool value);
	void flagShowResolutionCriteriaChanged(bool value);
	void arrowButtonScaleChanged(double value);
	void flagInitDirectionUsageChanged(bool value);
	void flagInitFOVUsageChanged(bool value);
	void flagRequireSelectionChanged(bool value);
	void flagLimitMagnitudeChanged(bool value);
	void flagDMSDegreesChanged(bool value);
	void flagScaleImageCircleChanged(bool value);
	void flagShowOcularsButtonChanged(bool value);
	void flagShowCcdCropOverlayChanged(bool value);
	void ccdCropOverlaySizeChanged(int value);
	void flagShowContourChanged(bool value);
	void flagShowCardinalsChanged(bool value);
	void flagAlignCrosshairChanged(bool value);

private slots:
	//! Signifies a change in ocular or telescope.  Sets new zoom level.
	void instrumentChanged();
	void determineMaxEyepieceAngle();
	void setScreenFOVForCCD();
	void retranslateGui();
	void updateOcularReticle(void);

private:
	//! Set up the Qt actions needed to activate the plugin.
	void initializeActivationActions();

	//! Returns TRUE if at least one bincular is defined.
	bool isBinocularDefined();

	//! Renders the CCD bounding box on-screen.  A telescope must be selected, or this call does nothing.
	void paintCCDBounds();
	//! Renders crosshairs into the viewport.
	void paintCrosshairs();
	//! Paint the mask into the viewport.
	void paintOcularMask(const StelCore * core);
	//! Renders the three Telrad circles, but only if not in ocular mode.
	void paintTelrad();

	//! Paints the text about the current object selections to the upper right hand of the screen.
	//! Should only be called from a 'ready' state; currently from the draw() method.
	void paintText(const StelCore * core);

	//! This method is called by the zoom() method, when this plugin is toggled off; it resets to the default view.
	void unzoomOcular();

	//! This method is responsible for ensuring a valid ini file for the plugin exists.  It first checks to see
	//! if one exists in the expected location.  If it does not, a default one is copied into place, and the process
	//! ends.  However, if one does exist, it opens it, and looks for the oculars_version key.  The value (or even
	//! presence) is used to determine if the ini file is usable.  If not, it is renamed, and a new one copied over.
	//! It does not try to copy values over.
	//! Once there is a valid ini file, it is loaded into the settings attribute.
	void validateAndLoadIniFile();

	void toggleLines(bool visible);

	//! toggles the actual ocular view.
	//! Record the state of the GridLinesMgr and other settings beforehand, so that they can be reset afterwards.
	//! @param zoomedIn if true, this zoom operation is starting from an already zoomed state.
	//!		False for the original state.
	void zoom(bool zoomedIn);

	//! This method is called by the zoom() method, when this plugin is toggled on; it resets the zoomed view.
	void zoomOcular();

	void hideUsageMessageIfDisplayed();

	//! Creates the sub-menu listing lense in the pop-up menu
	QMenu* addLensSubmenu(QMenu* parent);

	//! Creates the sub-menu listing telescopes in the pop-up menu.
	QMenu* addTelescopeSubmenu(QMenu* parent);

	//! Returns selected lens,or Q_NULLPTR if no lens is selected
	Lens* selectedLens();

	//! A list of all the oculars defined in the ini file.  Must have at least one, or module will not run.
	QList<CCD *> ccds;
	QList<Ocular *> oculars;
	QList<Telescope *> telescopes;
	QList<Lens *> lenses;

	int selectedCCDIndex;           //!< index of the current CCD, in the range of -1:ccds.count().  -1 means no CCD is selected.
	int selectedOcularIndex;        //!< index of the current ocular, in the range of -1:oculars.count().  -1 means no ocular is selected.
	int selectedTelescopeIndex;     //!< index of the current telescope, in the range of -1:telescopes.count(). -1 means none is selected.
	int selectedLensIndex;          //!< index of the current lens, in the range of -1:lense.count(). -1 means no lens is selected
	double selectedCCDRotationAngle;//!< allows rotating via property/remotecontrol API
	double arrowButtonScale;        //!< allows scaling of the GUI "previous/next" Ocular/CCD/Telescope etc. buttons

	QFont font;			//!< The font used for drawing labels.
	bool flagShowCCD;		//!< flag used to track if we are in CCD mode.
	bool flagShowOculars;		//!< flag used to track if we are in ocular mode.
	bool flagShowCrosshairs;	//!< flag used to track in crosshairs should be rendered in the ocular view.
	bool flagShowTelrad;		//!< If true, display the Telrad overlay.
	int usageMessageLabelID;	//!< the id of the label showing the usage message. -1 means it's not displayed.

	bool flagCardinalPointsMain;	//!< Flag to track if CardinalPoints was displayed at activation.
	bool flagAdaptationMain;	//!< Flag to track if adaptationCheckbox was enabled at activation.

	bool flagLimitStarsMain;        //!< Flag to track limit magnitude for stars
	double magLimitStarsMain;       //!< Value of limited magnitude for stars
	bool flagLimitDSOsMain;	        //!< Flag to track limit magnitude for DSOs
	double magLimitDSOsMain;	        //!< Value of limited magnitude for DSOs
	bool flagLimitPlanetsMain;      //!< Flag to track limit magnitude for planets, asteroids, comets etc.
	double magLimitPlanetsMain;     //!< Value of limited magnitude for planets, asteroids, comets etc.
	double relativeStarScaleMain;   //!< Value to store the usual relative star scale when activating ocular or CCD view
	double absoluteStarScaleMain;   //!< Value to store the usual absolute star scale when activating ocular or CCD view
	double relativeStarScaleOculars;	//!< Value to store the relative star scale when switching off ocular view
	double absoluteStarScaleOculars;	//!< Value to store the absolute star scale when switching off ocular view
	double relativeStarScaleCCD;    //!< Value to store the relative star scale when switching off CCD view
	double absoluteStarScaleCCD;    //!< Value to store the absolute star scale when switching off CCD view
	bool flagMoonScaleMain;	        //!< Flag to track of usage zooming of the Moon
	bool flagMinorBodiesScaleMain;  //!< Flag to track of usage zooming of minor bodies
	double milkyWaySaturation;

	double maxEyepieceAngle;        //!< The maximum aFOV of any eyepiece.
	bool flagRequireSelection;      //!< Read from the ini file, whether an object is required to be selected to zoom in.
	bool flagLimitMagnitude;        //!< Read from the ini file, whether a magnitude is required to be limited.
	//bool useMaxEyepieceAngle;     //!< Read from the ini file, whether to scale the mask based aFOV. GZ: RENAMED TO BE MORE CONSISTENT WITH INI NAME
	bool flagScaleImageCircle;      //!< Read from the ini file, whether to scale the mask based aFOV.

	bool flagGuiPanelEnabled;        //!< Display the GUI control panel
	bool flagDMSDegrees;             //!< Use decimal degrees in CCD frame display
	bool flagSemiTransparency;       //!< Draw the area outside the ocular circle not black but let some stars through.
	bool flagHideGridsLines;         //!< Switch off all grids and lines of GridMgr while in Ocular view
	bool flagGridLinesDisplayedMain; //!< keep track of gridline display while possibly suppressing their display.
	bool flagConstellationLinesMain; //!< keep track of constellation display while possibly suppressing their display.
	bool flagConstellationBoundariesMain; //!< keep track of constellation display while possibly suppressing their display.
	bool flagAsterismLinesMain;      //!< keep track of asterism display while possibly suppressing their display.
	bool flagRayHelpersLinesMain;      //!< keep track of ray helpers display while possibly suppressing their display.
	bool flipVertMain;               //!< keep track of screen flip in main program
	bool flipHorzMain;               //!< keep track of screen flip in main program

	QSignalMapper * ccdRotationSignalMapper;  //!< Used to rotate the CCD.
	QSignalMapper * ccdsSignalMapper;         //!< Used to determine which CCD was selected from the popup navigator.
	QSignalMapper * ocularsSignalMapper;      //!< Used to determine which ocular was selected from the popup navigator.
	QSignalMapper * telescopesSignalMapper;   //!< Used to determine which telescope was selected from the popup navigator.
	QSignalMapper * lensesSignalMapper;       //!< Used to determine which lens was selected from the popup navigator

	// for toolbar button
	QPixmap * pxmapGlow;
	QPixmap * pxmapOnIcon;
	QPixmap * pxmapOffIcon;
	StelButton * toolbarButton;
	bool flagShowOcularsButton;

	OcularDialog *ocularDialog;
	bool ready; //!< A flag that determines that this module is usable.  If false, we won't open.

	StelAction * actionShowOcular;
	StelAction * actionShowCrosshairs;
	StelAction * actionShowSensor;
	StelAction * actionShowTelrad;
	StelAction * actionConfiguration;
	StelAction * actionMenu;
	StelAction * actionTelescopeIncrement;
	StelAction * actionTelescopeDecrement;
	StelAction * actionOcularIncrement;
	StelAction * actionOcularDecrement;

	class OcularsGuiPanel * guiPanel;
	int guiPanelFontSize;
	Vec3f textColor;
	Vec3f lineColor;

	//Reticle
	StelTextureSP reticleTexture;
	StelTextureSP cardinalsNormalTexture;
	StelTextureSP cardinalsMirroredTexture;
	double actualFOV;		//!< Holds the FOV of the ocular/tescope/lens cobination; what the screen is zoomed to.
	double initialFOV;		//!< Holds the initial FOV
	bool flagInitFOVUsage;		//!< Flag used to track if we use default initial FOV (value at the startup of planetarium).
	bool flagInitDirectionUsage;	//!< Flag used to track if we use default initial direction (value at the startup of planetarium).
	bool flagAutosetMountForCCD;	//!< Flag used to track if we use automatic switch to type of mount for CCD frame
	bool flagScalingFOVForTelrad;	//!< Flag used to track if we use automatic scaling FOV for Telrad
	bool flagScalingFOVForCCD;	//!< Flag used to track if we use automatic scaling FOV for CCD
	bool flagShowResolutionCriteria;
	bool equatorialMountEnabledMain;  //!< Keep track of mount used in main program.
	double reticleRotation;
	bool flagShowCcdCropOverlay;  // !< Flag used to track if the ccd crop overlay should be shown.
	int ccdCropOverlaySize;  //!< Holds the ccd crop overlay size
	bool flagShowContour;
	bool flagShowCardinals;
	bool flagAlignCrosshair;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class OcularsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* OCULARS_HPP */
