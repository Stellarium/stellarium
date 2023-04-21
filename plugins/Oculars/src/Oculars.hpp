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
#include "Finder.hpp"
#include "OcularDialog.hpp"
#include "SolarSystem.hpp"
#include "StelModule.hpp"
#include "StelTextureTypes.hpp"
#include "Telescope.hpp"
#include "VecMath.hpp"

#include <QFont>
#include <QSettings>

#define MIN_OCULARS_INI_VERSION 3.2f
#define DEFAULT_CCD_CROP_OVERLAY_SIZE 250

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QMenu;
class QMouseEvent;
class QPixmap;
class QSettings;
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

	Q_ENUMS(PluginMode)
public:
	enum PluginMode
	{
		OcNone,     //!< disabled
		OcTelrad,   //!< showing Telrad mode: naked eye view, just a Telrad overlay
		OcFinder,   //!< showing Finderscope mode: slightly magnified by a compact instrument
		OcOcular,   //!< central circle, magnified, different skyDrawer settings
		OcSensor    //!< central box, magnified, different skyDrawer settings
	};

	// We use one master mode property for mutually exclusive operation
	Q_PROPERTY(PluginMode pluginMode READ getPluginMode       WRITE setPluginMode    NOTIFY pluginModeChanged)
	// While the switching StelActions could work with Lambda functions, the GUI panel in the screen corner still needs Boolean properties to highlight the buttons.
	// TODO 2021: Try to make them or the slots private.
	Q_PROPERTY(bool flagModeOcular        READ getFlagModeOcular         WRITE setFlagModeOcular      NOTIFY flagModeOcularChanged)
	Q_PROPERTY(bool flagModeSensor        READ getFlagModeSensor         WRITE setFlagModeSensor      NOTIFY flagModeSensorChanged)
	Q_PROPERTY(bool flagModeTelrad        READ getFlagModeTelrad         WRITE setFlagModeTelrad      NOTIFY flagModeTelradChanged)
	Q_PROPERTY(bool flagModeFinder        READ getFlagModeFinder         WRITE setFlagModeFinder      NOTIFY flagModeFinderChanged)
	// This is another property for an Action. Switching it has no side effects, though, therefore it is not in the PluginMode enum.
	Q_PROPERTY(bool enableCrosshairs      READ getEnableCrosshairs       WRITE toggleCrosshairs NOTIFY enableCrosshairsChanged)

	Q_PROPERTY(int selectedCCDIndex       READ getSelectedCCDIndex       WRITE selectCCDAtIndex       NOTIFY selectedCCDChanged)
	Q_PROPERTY(int selectedOcularIndex    READ getSelectedOcularIndex    WRITE selectOcularAtIndex    NOTIFY selectedOcularChanged)
	Q_PROPERTY(int selectedTelescopeIndex READ getSelectedTelescopeIndex WRITE selectTelescopeAtIndex NOTIFY selectedTelescopeChanged)
	Q_PROPERTY(int selectedLensIndex      READ getSelectedLensIndex      WRITE selectLensAtIndex      NOTIFY selectedLensChanged)
	Q_PROPERTY(double selectedCCDRotationAngle      READ getSelectedCCDRotationAngle      WRITE setSelectedCCDRotationAngle      NOTIFY selectedCCDRotationAngleChanged)
	Q_PROPERTY(double selectedCCDPrismPositionAngle READ getSelectedCCDPrismPositionAngle WRITE setSelectedCCDPrismPositionAngle NOTIFY selectedCCDPrismPositionAngleChanged)

	Q_PROPERTY(bool flagGuiPanelEnabled        READ getFlagGuiPanelEnabled        WRITE enableGuiPanel                NOTIFY flagGuiPanelEnabledChanged)
	Q_PROPERTY(bool flagInitFOVUsage           READ getFlagInitFovUsage           WRITE setFlagInitFovUsage           NOTIFY flagInitFOVUsageChanged)
	Q_PROPERTY(bool flagInitDirectionUsage     READ getFlagInitDirectionUsage     WRITE setFlagInitDirectionUsage     NOTIFY flagInitDirectionUsageChanged)
	Q_PROPERTY(bool flagShowResolutionCriteria READ getFlagShowResolutionCriteria WRITE setFlagShowResolutionCriteria NOTIFY flagShowResolutionCriteriaChanged)
	Q_PROPERTY(bool flagRequireSelection       READ getFlagRequireSelection       WRITE setFlagRequireSelection       NOTIFY flagRequireSelectionChanged)
	Q_PROPERTY(bool flagAutoLimitMagnitude     READ getFlagAutoLimitMagnitude     WRITE setFlagAutoLimitMagnitude     NOTIFY flagAutoLimitMagnitudeChanged)
	Q_PROPERTY(bool flagHideGridsLines         READ getFlagHideGridsLines         WRITE setFlagHideGridsLines         NOTIFY flagHideGridsLinesChanged)
	Q_PROPERTY(bool flagScaleImageCircle       READ getFlagScaleImageCircle       WRITE setFlagScaleImageCircle       NOTIFY flagScaleImageCircleChanged)// flag scale image circle scaleImageCirclCheckBox
	Q_PROPERTY(bool flagSemiTransparency       READ getFlagUseSemiTransparency    WRITE setFlagUseSemiTransparency    NOTIFY flagUseSemiTransparencyChanged)
	Q_PROPERTY(int transparencyMask            READ getTransparencyMask           WRITE setTransparencyMask           NOTIFY transparencyMaskChanged)
	Q_PROPERTY(bool flagDMSDegrees             READ getFlagDMSDegrees             WRITE setFlagDMSDegrees             NOTIFY flagDMSDegreesChanged)
	Q_PROPERTY(bool flagHorizontalCoordinates  READ getFlagHorizontalCoordinates  WRITE setFlagHorizontalCoordinates  NOTIFY flagHorizontalCoordinatesChanged)
	Q_PROPERTY(bool flagAutosetMountForCCD     READ getFlagAutosetMountForCCD     WRITE setFlagAutosetMountForCCD     NOTIFY flagAutosetMountForCCDChanged)
	Q_PROPERTY(bool flagScalingFOVForTelrad	   READ getFlagScalingFOVForTelrad    WRITE setFlagScalingFOVForTelrad    NOTIFY flagScalingFOVForTelradChanged) // TODO: Rename to flagTelradAutozoom etc. to be clearer.
	Q_PROPERTY(Vec4f telradFOV                 READ getTelradFOV                  WRITE setTelradFOV                  NOTIFY telradFOVChanged)
	Q_PROPERTY(bool flagScalingFOVForCCD	   READ getFlagScalingFOVForCCD       WRITE setFlagScalingFOVForCCD       NOTIFY flagScalingFOVForCCDChanged)
	Q_PROPERTY(bool flagMaxExposureTimeForCCD  READ getFlagMaxExposureTimeForCCD  WRITE setFlagMaxExposureTimeForCCD  NOTIFY flagMaxExposureTimeForCCDChanged)
	Q_PROPERTY(bool flagShowOcularsButton	   READ getFlagShowOcularsButton      WRITE setFlagShowOcularsButton      NOTIFY flagShowOcularsButtonChanged)
	Q_PROPERTY(bool flagShowContour		   READ getFlagShowContour            WRITE setFlagShowContour            NOTIFY flagShowContourChanged)
	Q_PROPERTY(bool flagShowCardinals	   READ getFlagShowCardinals          WRITE setFlagShowCardinals          NOTIFY flagShowCardinalsChanged)
	Q_PROPERTY(bool flagAlignCrosshair	   READ getFlagAlignCrosshair         WRITE setFlagAlignCrosshair         NOTIFY flagAlignCrosshairChanged)

	Q_PROPERTY(int arrowButtonScale READ getArrowButtonScale WRITE setArrowButtonScale NOTIFY arrowButtonScaleChanged)
	Q_PROPERTY(int guiPanelFontSize READ getGuiPanelFontSize WRITE setGuiPanelFontSize NOTIFY guiPanelFontSizeChanged)
	Q_PROPERTY(Vec3f textColor      READ getTextColor        WRITE setTextColor        NOTIFY textColorChanged)
	Q_PROPERTY(Vec3f lineColor      READ getLineColor        WRITE setLineColor        NOTIFY textColorChanged)

	Q_PROPERTY(bool flagShowCcdCropOverlay READ getFlagShowCcdCropOverlay  WRITE setFlagShowCcdCropOverlay NOTIFY flagShowCcdCropOverlayChanged)
	Q_PROPERTY(bool flagShowCcdCropOverlayPixelGrid READ getFlagShowCcdCropOverlayPixelGrid WRITE setFlagShowCcdCropOverlayPixelGrid NOTIFY flagShowCcdCropOverlayPixelGridChanged)
	//Q_PROPERTY(int ccdCropOverlaySize      READ getCcdCropOverlaySize      WRITE setCcdCropOverlaySize   NOTIFY ccdCropOverlaySizeChanged)
	Q_PROPERTY(int ccdCropOverlayHSize     READ getCcdCropOverlayHSize      WRITE setCcdCropOverlayHSize   NOTIFY ccdCropOverlayHSizeChanged)
	Q_PROPERTY(int ccdCropOverlayVSize     READ getCcdCropOverlayVSize      WRITE setCcdCropOverlayVSize   NOTIFY ccdCropOverlayVSizeChanged)

	Q_PROPERTY(bool flagShowFocuserOverlay      READ getFlagShowFocuserOverlay	WRITE setFlagShowFocuserOverlay      NOTIFY flagShowFocuserOverlayChanged)
	Q_PROPERTY(bool flagUseSmallFocuserOverlay  READ getFlagUseSmallFocuserOverlay	WRITE setFlagUseSmallFocuserOverlay  NOTIFY flagUseSmallFocuserOverlayChanged)
	Q_PROPERTY(bool flagUseMediumFocuserOverlay READ getFlagUseMediumFocuserOverlay	WRITE setFlagUseMediumFocuserOverlay NOTIFY flagUseMediumFocuserOverlayChanged)
	Q_PROPERTY(bool flagUseLargeFocuserOverlay  READ getFlagUseLargeFocuserOverlay	WRITE setFlagUseLargeFocuserOverlay  NOTIFY flagUseLargeFocuserOverlayChanged)
	Q_PROPERTY(Vec3f focuserColor               READ getFocuserColor                WRITE setFocuserColor                NOTIFY focuserColorChanged)

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
	void prismPositionAngleReset();
	void decrementCCDIndex();
	void decrementOcularIndex();
	void decrementFinderIndex();
	void decrementTelescopeIndex();
	void decrementLensIndex();
	void displayPopupMenu();
	//! Just return current mode of operation
	PluginMode getPluginMode() const { return pluginMode; }
	//! Master switch. Must handle mode transitions cleanly: Disable mode specific settings/restore main app settings, change mode, enable possible new mode specific settings
	void setPluginMode(PluginMode mode);

	void incrementCCDIndex();
	void incrementOcularIndex();
	void incrementFinderIndex();
	void incrementTelescopeIndex();
	void incrementLensIndex();
	void disableLens();
	// Rotate reticle
	void  rotateReticleClockwise();
	void  rotateReticleCounterclockwise();

	void rotateCCD(int amount);     //!< amount must be a number. This adds to the current rotation.
	double getSelectedCCDRotationAngle(void) const; //!< get rotation angle from currently selected CCD
	void setSelectedCCDRotationAngle(double angle); //!< set rotation angle for currently selected CCD

	void rotatePrism(int amount);     //!< amount must be a number. This adds to the current rotation.
	double getSelectedCCDPrismPositionAngle(void) const; //!< get position angle from prism/OAG of currently selected CCD
	void setSelectedCCDPrismPositionAngle(double angle); //!< set position angle for prism/OAG of currently selected CCD
	
	void selectCCDAtIndex(int index);           //!< index in the range of -1:ccds.count(), else call is ignored
	int getSelectedCCDIndex() const {return selectedCCDIndex; }

	void selectOcularAtIndex(int index);            //!< index in the range of -1:oculars.count(), else call is ignored
	int getSelectedOcularIndex() const {return selectedOcularIndex; }

	void selectTelescopeAtIndex(int index);            //!< index in the range of -1:telescopes.count(), else call is ignored
	int getSelectedTelescopeIndex() const {return selectedTelescopeIndex; }

	void selectLensAtIndex(int index);           //!< index in the range -1:lenses.count(), else call is ignored
	int getSelectedLensIndex() const {return selectedLensIndex; }

	void selectFinderAtIndex(int index);           //!< index in the range 0:finders.count(), else call is ignored
	int getSelectedFinderIndex() const {return selectedFinderIndex; }

	//! Toggles the sensor frame overlay //(overloaded for blind switching).
	void toggleCCD();
	void toggleCrosshairs(bool show = true);
	bool getEnableCrosshairs() const { return flagShowCrosshairs; }

	//! Toggles the Telrad sight overlay //(overloaded for blind switching).
	void toggleTelrad();
	void toggleFinder();

	void enableGuiPanel(bool enable = true);
	bool getFlagGuiPanelEnabled(void) const {return flagGuiPanelEnabled;}
	void setGuiPanelFontSize(int size);
	int getGuiPanelFontSize()const {return guiPanelFontSize;}

	void setTextColor(const Vec3f &color) {textColor=color; emit textColorChanged(color);}
	Vec3f getTextColor() const {return textColor;}

	void setLineColor(const Vec3f &color) {lineColor=color; emit lineColorChanged(color);}
	Vec3f getLineColor() const {return lineColor;}

	void setFocuserColor(const Vec3f &color) { focuserColor=color; emit focuserColorChanged(color);}
	Vec3f getFocuserColor() const {return focuserColor;}

	void setTelradFOV(const Vec4f &fov);
	Vec4f getTelradFOV() const;

	void setFlagDMSDegrees(const bool b);
	bool getFlagDMSDegrees(void) const;

	void setFlagHorizontalCoordinates(const bool b);
	bool getFlagHorizontalCoordinates(void) const;

	void setFlagRequireSelection(const bool b);
	bool getFlagRequireSelection(void) const;
	
	void setFlagAutoLimitMagnitude(const bool b);
	bool getFlagAutoLimitMagnitude(void) const;

	void setMagLimitStarsOcularsManual(double mag);
	double getMagLimitStarsOcularsManual() const;

	void setMagLimitPlanetsOcularsManual(double mag);
	double getMagLimitPlanetsOcularsManual() const;

	void setMagLimitDSOsOcularsManual(double mag);
	double getMagLimitDSOsOcularsManual() const;

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

	void setFlagMaxExposureTimeForCCD(const bool b);
	bool getFlagMaxExposureTimeForCCD(void) const;

	void setFlagUseSemiTransparency(const bool b);
	bool getFlagUseSemiTransparency(void) const;

	void setTransparencyMask(const int v);
	int getTransparencyMask(void) const;

	void setFlagShowResolutionCriteria(const bool b);
	bool getFlagShowResolutionCriteria(void) const;

	void setCcdCropOverlayHSize(int size);
	int getCcdCropOverlayHSize()const {return ccdCropOverlayHSize;}

	void setCcdCropOverlayVSize(int size);
	int getCcdCropOverlayVSize()const {return ccdCropOverlayVSize;}

	void setArrowButtonScale(const int val);
	int getArrowButtonScale() const;

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

	void setFlagShowCcdCropOverlayPixelGrid(const bool b);
	bool getFlagShowCcdCropOverlayPixelGrid(void) const;

	void setFlagShowContour(const bool b);
	bool getFlagShowContour(void) const;

	void setFlagShowCardinals(const bool b);
	bool getFlagShowCardinals(void) const;

	void setFlagAlignCrosshair(const bool b);
	bool getFlagAlignCrosshair(void) const;

	void setFlagShowFocuserOverlay(const bool b);
	bool getFlagShowFocuserOverlay(void) const;

	void setFlagUseSmallFocuserOverlay(const bool b);
	bool getFlagUseSmallFocuserOverlay(void) const;

	void setFlagUseMediumFocuserOverlay(const bool b);
	bool getFlagUseMediumFocuserOverlay(void) const;

	void setFlagUseLargeFocuserOverlay(const bool b);
	bool getFlagUseLargeFocuserOverlay(void) const;

private slots:
	// Switching or retrieving these 3 properties should not be done normally. We need those only for the Actions linked to the GUI buttons.
	//! Indicates the ocular view overlay. Activating this also triggers setPluginMode()
	void setFlagModeOcular(bool show);
	//! Indicates the ocular view overlay.
	bool getFlagModeOcular() { return flagModeOcular; }

	//! Indicates the sensor frame overlay. Activating this also triggers setPluginMode()
	void setFlagModeSensor(bool show);
	//! Indicates the sensor frame overlay.
	bool getFlagModeSensor() { return flagModeSensor; }

	//! Indicates the Telrad sight overlay. Activating this also triggers setPluginMode()
	void setFlagModeTelrad(bool show);
	//! Indicates the Telrad sight overlay.
	bool getFlagModeTelrad() const { return flagModeTelrad; }

	//! Indicates the Finder overlay. Activating this also triggers setPluginMode()
	void setFlagModeFinder(bool show);
	//! Indicates the Finder overlay.
	bool getFlagModeFinder() const { return flagModeFinder; }

signals:
	void pluginModeChanged(PluginMode mode);
	void flagModeOcularChanged(bool value);
	void flagModeSensorChanged(bool value);
	void flagModeTelradChanged(bool value);
	void flagModeFinderChanged(bool value);
	void enableCrosshairsChanged(bool value);
	//void enableCCDChanged(bool value);
	//void enableTelradChanged(bool value);
	void selectedCCDChanged(int value);
	void selectedOcularChanged(int value);
	void selectedTelescopeChanged(int value);
	void selectedLensChanged(int value);
	void selectedFinderChanged(int value);
	void selectedCCDRotationAngleChanged(double value);
	void selectedCCDPrismPositionAngleChanged(double value);

	void flagGuiPanelEnabledChanged(bool value);
	void guiPanelFontSizeChanged(int value);
	void textColorChanged(const Vec3f &color);
	void lineColorChanged(const Vec3f &color);
	void focuserColorChanged(const Vec3f &color);
	void flagHideGridsLinesChanged(bool value);
	void flagAutosetMountForCCDChanged(bool value);
	void flagScalingFOVForTelradChanged(bool value);
	void telradFOVChanged(Vec4f fov);
	void flagScalingFOVForCCDChanged(bool value);
	void flagMaxExposureTimeForCCDChanged(bool value);
	void flagUseSemiTransparencyChanged(bool value);
	void transparencyMaskChanged(int value);
	void flagShowResolutionCriteriaChanged(bool value);
	void arrowButtonScaleChanged(int value);
	void flagInitDirectionUsageChanged(bool value);
	void flagInitFOVUsageChanged(bool value);
	void flagRequireSelectionChanged(bool value);
	void flagAutoLimitMagnitudeChanged(bool value);
	void flagDMSDegreesChanged(bool value);
	void flagHorizontalCoordinatesChanged(bool value);
	void flagScaleImageCircleChanged(bool value);
	void flagShowOcularsButtonChanged(bool value);
	void flagShowCcdCropOverlayChanged(bool value);	
	void ccdCropOverlayHSizeChanged(int value);
	void ccdCropOverlayVSizeChanged(int value);
	void flagShowCcdCropOverlayPixelGridChanged(bool value);
	void flagShowContourChanged(bool value);
	void flagShowCardinalsChanged(bool value);
	void flagAlignCrosshairChanged(bool value);
	void flagShowFocuserOverlayChanged(bool value);
	void flagUseSmallFocuserOverlayChanged(bool value);
	void flagUseMediumFocuserOverlayChanged(bool value);
	void flagUseLargeFocuserOverlayChanged(bool value);

private slots:
	//! Signifies a change in ocular or telescope.  Sets new zoom level.
	void instrumentChanged();
	void determineMaxEyepieceAngle();
	void setScreenFOVForCCD();
	void retranslateGui();
	void updateOcularReticle(void);
	void togglePixelGrid();
	void toggleCropOverlay();
	void toggleFocuserOverlay();
	void handleAutoLimitToggle(bool on);	//!< do a few activities in the background.
	void handleStarMagLimitToggle(bool on); //!< Handle switching the main program's star limitation flag	
	void updateLatestSelectedSSO();

private:
	//! Compute the limiting magnitude for a telescope with aperture given in mm
	static double computeLimitMagnitude(double aperture);

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

	//! This method is responsible for ensuring a valid ini file for the plugin exists.  It first checks to see
	//! if one exists in the expected location.  If it does not, a default one is copied into place, and the process
	//! ends.  However, if one does exist, it opens it, and looks for the oculars_version key.  The value (or even
	//! presence) is used to determine if the ini file is usable.  If not, it is renamed, and a new one copied over.
	//! It does not try to copy values over.
	//! Once there is a valid ini file, it is loaded into the settings attribute.
	void validateAndLoadIniFile();

	//! Switch state of line displays for the Main application. Should be called when entering or leaving oculars view or when toggling the respective switch.
	void handleLines(bool visibleMain);
	//! Inhibit scaling up planets that may be active in the Main application.  Should be called when entering or leaving oculars, finder and sensors mode or when toggling the respective switch.
	//! Set scaleMain to true to set the scale as given in the main program, false to store status and scale to 1.
	void handlePlanetScaling(bool scaleMain);

	//! adjust star, planet and nebula limiting magnitudes as required for Oculars and Finder views
	//! set mode to the requested PluginMode.
	//! @note: Currently it is recommended to first switch to OcNone, then OcOcular or OcFinder, and not switch between the two modes directly.
	void handleMagnitudeLimits(PluginMode mode);

	//! toggles the actual ocular view.
	//! Record the state of the GridLinesMgr and other settings beforehand, so that they can be reset afterwards.
	//! @param zoomedIn if true, this zoom operation is starting from an already zoomed state.
	//!		False for the original state.
	//void zoom(bool zoomedIn);

	//! This method is called by the zoom() method, when this plugin is toggled on; it resets the zoomed view.
	void zoomOcular();

	void hideUsageMessageIfDisplayed();

	//! Creates the sub-menu listing lense in the pop-up menu
	QMenu* addLensSubmenu(QMenu* parent);

	//! Creates the sub-menu listing telescopes in the pop-up menu.
	QMenu* addTelescopeSubmenu(QMenu* parent);

	//! Returns selected lens,or Q_NULLPTR if no lens is selected
	Lens* selectedLens();

	//! Store state of line displays from the Main application. The Oculars view may disable them.
	void storeLineStateMain();
	//! Restore state of line displays for the Main application. May be called when leaving oculars view or when toggling the respective switch.
	void restoreLineStateMain();


	//! A list of all the oculars defined in the ini file.  Must have at least one, or module will not run.
	QList<CCD *> ccds;
	QList<Ocular *> oculars;
	QList<Telescope *> telescopes;
	QList<Lens *> lenses;
	QList<Finder *> finders;

	int selectedCCDIndex;           //!< index of the current CCD, in the range of -1:ccds.count().  -1 means no CCD is selected.
	int selectedOcularIndex;        //!< index of the current ocular, in the range of -1:oculars.count().  -1 means no ocular is selected.
	int selectedTelescopeIndex;     //!< index of the current telescope, in the range of -1:telescopes.count(). -1 means none is selected.
	int selectedLensIndex;          //!< index of the current lens, in the range of -1:lenses.count(). -1 means no lens is selected.
	int selectedFinderIndex;        //!< index of the current finder, in the range of -1:finders.count(). -1 means no finder is selected.
	double selectedCCDRotationAngle;//!< allows rotating via property/remotecontrol API
	double selectedCCDPrismPositionAngle;//!< allows rotating via property/remotecontrol API
	int arrowButtonScale;           //!< allows scaling of the GUI "previous/next" Ocular/CCD/Telescope etc. buttons

	QFont font;			//!< The font used for drawing labels.
	PluginMode pluginMode;          //!< Current operational mode
	// The next 4 are mutually exclusive "slave mode" flags to keep the buttons in the GUI showing active/inactive highlight state.
	bool flagModeOcular;		//!< flag used to track if we are in ocular mode.
	bool flagModeSensor;		//!< flag used to track if we are in CCD mode.
	bool flagModeTelrad;		//!< flag used to track if we are in Telrad mode.
	bool flagModeFinder;		//!< flag used to track if we are in Finder mode.

	bool flagShowCrosshairs;	//!< flag used to track in crosshairs should be rendered in the ocular view.
	int usageMessageLabelID;	//!< the id of the label showing the usage message. -1 means it's not displayed.

	bool flagCardinalPointsMain;	//!< Flag to track if CardinalPoints was displayed at activation.
	bool flagAdaptationMain;	//!< Flag to track if adaptationCheckbox was enabled at activation.

	// allow tracking of settings for manually configured magnitude limits for ocular and finder modes. See handleMagnitudeLimits().
	bool flagLimitStarsMain;        //!< Flag to track limitation of stellar magnitude in the main program
	double magLimitStarsMain;       //!< Value of limited stellar magnitude in the main program
	bool flagLimitPlanetsMain;      //!< Flag to track limit magnitude for planets, asteroids, comets etc.
	double magLimitPlanetsMain;     //!< Value of limited magnitude for planets, asteroids, comets etc.
	bool flagLimitDSOsMain;		//!< Flag to track limit magnitude for DSOs
	double magLimitDSOsMain;	//!< Value of limited magnitude for DSOs

	bool flagLimitStarsOculars;	//!< Track whether a stellar magnitude limit should be activated when Oculars view is selected.
					//!< This flag is not a StelProperty, but linked to SkyDrawer.flagStarMagnitudeLimit while oculars view is active.
					//!< This is required to set the manual limitation flag in SkyDrawer.
	double magLimitStarsOculars;    //!< Value of limited magnitude for stars in oculars mode, when not auto-defined with flagAutoLimitMagnitude.
					//!< This value is not a StelProperty, but linked to SkyDrawer.customStarMagLimit while oculars view is active and automatic setting of magnitude is not active.
					//!< If user modifies the magnitude while flagAutoLimitMagnitude is true, the value will not be stored permanently. [FIXME: Recheck this sentence.]
	bool flagLimitPlanetsOculars;	//!< Track whether a planet magnitude limit should be activated when Oculars view is selected.
					//!< This flag is not a StelProperty, but linked to SkyDrawer.flagPlanetMagnitudeLimit while oculars view is active.
					//!< This is required to set the manual limitation flag in SkyDrawer.
	double magLimitPlanetsOculars;  //!< Value of limited magnitude for planets in oculars mode, when not auto-defined with flagAutoLimitMagnitude.
					//!< This value is not a StelProperty, but linked to SkyDrawer.customPlanetMagLimit while oculars view is active and automatic setting of magnitude is not active.
					//!< If user modifies the magnitude while flagAutoLimitMagnitude is true, the value will not be stored permanently. [FIXME: Recheck this sentence.]
	bool flagLimitDSOsOculars;	//!< Track whether a nebula magnitude limit should be activated when Oculars view is selected.
					//!< This flag is not a StelProperty, but linked to SkyDrawer.flagNebulaMagnitudeLimit while oculars view is active.
					//!< This is required to set the manual limitation flag in SkyDrawer.
	double magLimitDSOsOculars;     //!< Value of limited magnitude for nebula in oculars mode, when not auto-defined with flagAutoLimitMagnitude.
					//!< This value is not a StelProperty, but linked to SkyDrawer.customNebulaMagLimit while oculars view is active and automatic setting of magnitude is not active.
					//!< If user modifies the magnitude while flagAutoLimitMagnitude is true, the value will not be stored permanently. [FIXME: Recheck this sentence.]

	bool flagLimitStarsFinder; 	//!< Track whether a stellar magnitude limit should be activated when Finder view is selected.
					//!< This flag is not a StelProperty, but linked to SkyDrawer.flagStarMagnitudeLimit while finder view is active.
					//!< This is required to set the manual limitation flag in SkyDrawer.
	double magLimitStarsFinder;     //!< Value of limited magnitude for stars in finder mode, when not auto-defined with flagAutoLimitMagnitude.
					//!< This value is not a StelProperty, but linked to SkyDrawer.customStarMagLimit while finder view is active and automatic setting of magnitude is not active.
					//!< If user modifies the magnitude while flagAutoLimitMagnitude is true, the value will not be stored permanently. [FIXME: Recheck this sentence.]
	bool flagLimitPlanetsFinder;	//!< Track whether a planet magnitude limit should be activated when Finder view is selected.
					//!< This flag is not a StelProperty, but linked to SkyDrawer.flagPlanetMagnitudeLimit while finderview is active.
					//!< This is required to set the manual limitation flag in SkyDrawer.
	double magLimitPlanetsFinder;   //!< Value of limited magnitude for planets in finder mode, when not auto-defined with flagAutoLimitMagnitude.
					//!< This value is not a StelProperty, but linked to SkyDrawer.customPlanetMagLimit while finder view is active and automatic setting of magnitude is not active.
					//!< If user modifies the magnitude while flagAutoLimitMagnitude is true, the value will not be stored permanently. [FIXME: Recheck this sentence.]
	bool flagLimitDSOsFinder;	//!< Track whether a nebula magnitude limit should be activated when finder view is selected.
					//!< This flag is not a StelProperty, but linked to SkyDrawer.flagNebulaMagnitudeLimit while finder view is active.
					//!< This is required to set the manual limitation flag in SkyDrawer.
	double magLimitDSOsFinder;      //!< Value of limited magnitude for nebula in finder mode, when not auto-defined with flagAutoLimitMagnitude.
					//!< This value is not a StelProperty, but linked to SkyDrawer.customNebulaMagLimit while finder view is active and automatic setting of magnitude is not active.
					//!< If user modifies the magnitude while flagAutoLimitMagnitude is true, the value will not be stored permanently. [FIXME: Recheck this sentence.]

	bool flagAutoLimitMagnitude;    //!< Decide whether stellar and other magnitudes should be auto-limited based on telescope/ocular combination.
					//!< If false, the manual limitation value magLimit(Stars|Planets|Nebula)(Oculars|Finder) takes over, and limitation is decided by flagLimit(Stars|Planets|Nebula)(Oculars|Finder).
					//!< If true, flagLimit(Stars|Planets|Nebula)(Oculars|Finder) is set true when activating Oculars view, and will remain true.
					//!

	// allow tracking and restoring star scales between various modes
	double relativeStarScaleMain;   //!< Value to store the usual relative star scale when activating ocular or CCD view
	double absoluteStarScaleMain;   //!< Value to store the usual absolute star scale when activating ocular or CCD view
	double relativeStarScaleOculars;	//!< Value to store the relative star scale when switching off ocular view
	double absoluteStarScaleOculars;	//!< Value to store the absolute star scale when switching off ocular view
	double relativeStarScaleCCD;    //!< Value to store the relative star scale when switching off CCD view
	double absoluteStarScaleCCD;    //!< Value to store the absolute star scale when switching off CCD view
	double relativeStarScaleFinder; //!< Value to store the relative star scale when switching off finder view
	double absoluteStarScaleFinder; //!< Value to store the absolute star scale when switching off finder view

	// variables for handlePlanetScaling()
	bool flagMoonScaleMain;	        //!< Flag to track of usage of artificial scaling of the Moon
	bool flagMinorBodiesScaleMain;  //!< Flag to track of usage of artificial scaling of minor bodies
	bool flagPlanetScaleMain;       //!< Flag to track of usage of artificial scaling of planets
	bool flagSunScaleMain;          //!< Flag to track of usage of artificial scaling of the Sun
	bool flagDSOPropHintMain;	//!< Flag to track of usage proportional hints for DSO

	double milkyWaySaturationMain;

	double maxEyepieceAngle;        //!< The maximum aFOV of any eyepiece.
	bool flagRequireSelection;      //!< Decide whether an object is required to be selected to zoom in.
	bool flagScaleImageCircle;      //!< Decide whether to scale the mask based aFOV.

	bool flagGuiPanelEnabled;        //!< Display the GUI control panel
	bool flagDMSDegrees;             //!< Use decimal degrees in CCD frame display
	bool flagHorizontalCoordinates;  //!< Use horizontal coordinates instead equatorial coordinates for cross of center CCD
	bool flagSemiTransparency;       //!< Draw the area outside the ocular circle not black but let some stars through.
	int transparencyMask;		 //!< Value of transparency for semi-transparent mask

	// variables for handleLines()
	bool flagHideGridsLines;         //!< Switch off all grids and lines of GridMgr while in Ocular view
	bool flagGridLinesDisplayedMain; //!< keep track of gridline display while possibly suppressing their display.
	bool flagConstellationLinesMain; //!< keep track of constellation display while possibly suppressing their display.
	bool flagConstellationBoundariesMain; //!< keep track of constellation display while possibly suppressing their display.
	bool flagAsterismLinesMain;      //!< keep track of asterism display while possibly suppressing their display.
	bool flagRayHelpersLinesMain;      //!< keep track of ray helpers display while possibly suppressing their display.

	bool flipVertMain;               //!< keep track of screen flip in main program
	bool flipHorzMain;               //!< keep track of screen flip in main program

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
	StelAction * actionShowFinder;
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
	Vec3f focuserColor;

	Planet* selectedSSO;

	//Reticle
	StelTextureSP reticleTexture;
	StelTextureSP protractorTexture;
	StelTextureSP protractorFlipVTexture;
	StelTextureSP protractorFlipHTexture;
	StelTextureSP protractorFlipHVTexture;
	double actualFOV;		//!< Holds the FOV of the ocular/tescope/lens combination; what the screen is zoomed to.
	double initialFOV;		//!< Holds the initial FOV, degrees
	bool flagInitFOVUsage;		//!< Flag used to track if we use default initial FOV (value at the startup of planetarium).
	bool flagInitDirectionUsage;	//!< Flag used to track if we use default initial direction (value at the startup of planetarium).
	bool flagAutosetMountForCCD;	//!< Flag used to track if we use automatic switch to type of mount for CCD frame
	bool flagScalingFOVForTelrad;	//!< Flag used to track if we use automatic scaling FOV for Telrad
	bool flagScalingFOVForCCD;	//!< Flag used to track if we use automatic scaling FOV for CCD
	bool flagMaxExposureTimeForCCD;	//!< Flag used to track if we show max exposure time for CCD
	bool flagShowResolutionCriteria;	//!< Show various criteria for angular resolution based on telescope/ocular
	bool equatorialMountEnabledMain;	//!< Keep track of mount used in main program.
	double reticleRotation;
	bool flagShowCcdCropOverlay;		//!< Flag used to track if the ccd crop overlay should be shown.
	bool flagShowCcdCropOverlayPixelGrid;	//!< Flag used to track if the ccd full grid overlay should be shown.
	int ccdCropOverlayHSize;		//!< Holds the ccd crop overlay size
	int ccdCropOverlayVSize;		//!< Holds the ccd crop overlay size
	bool flagShowContour;
	bool flagShowCardinals;
	bool flagAlignCrosshair;
	Vec4f telradFOV;			//!< 4-vector of circles. The fourth element is non-standard.
	bool flagShowFocuserOverlay;		//!< Flag used to track if the focuser crop overlay should be shown.
	bool flagUseSmallFocuserOverlay;	//!< Flag used to track if a small-sized focuser (1.25") overlay should be shown.
	bool flagUseMediumFocuserOverlay;	//!< Flag used to track if a medium-sized focuser (2.0") overlay should be shown.
	bool flagUseLargeFocuserOverlay;	//!< Flag used to track if a large-sized focuser (3.3") overlay should be shown.
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
	virtual StelModule* getStelModule() const Q_DECL_OVERRIDE;
	virtual StelPluginInfo getPluginInfo() const Q_DECL_OVERRIDE;
	virtual QObjectList getExtensionList() const Q_DECL_OVERRIDE { return QObjectList(); }
};

#endif /* OCULARS_HPP */
