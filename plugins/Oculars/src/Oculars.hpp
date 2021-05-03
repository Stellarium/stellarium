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

#pragma once

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
#include <QtGlobal>

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
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
   Q_DISABLE_COPY_MOVE(Oculars)
#endif

   Q_PROPERTY(bool enableOcular READ getEnableOcular WRITE enableOcular NOTIFY enableOcularChanged)
   Q_PROPERTY(bool enableCrosshairs READ getEnableCrosshairs WRITE toggleCrosshairs NOTIFY enableCrosshairsChanged)
   Q_PROPERTY(bool enableCCD READ getEnableCCD WRITE toggleCCD NOTIFY enableCCDChanged)
   Q_PROPERTY(bool enableTelrad READ getEnableTelrad WRITE toggleTelrad NOTIFY enableTelradChanged)

   Q_PROPERTY(int selectedCCDIndex READ getSelectedCCDIndex WRITE selectCCDAtIndex NOTIFY selectedCCDChanged)
   Q_PROPERTY(
     int selectedOcularIndex READ getSelectedOcularIndex WRITE selectOcularAtIndex NOTIFY selectedOcularChanged)
   Q_PROPERTY(int selectedTelescopeIndex READ getSelectedTelescopeIndex WRITE selectTelescopeAtIndex NOTIFY
                selectedTelescopeChanged)
   Q_PROPERTY(int selectedLensIndex READ getSelectedLensIndex WRITE selectLensAtIndex NOTIFY selectedLensChanged)
   Q_PROPERTY(double selectedCCDRotationAngle READ getSelectedCCDRotationAngle WRITE setSelectedCCDRotationAngle NOTIFY
                selectedCCDRotationAngleChanged)
   Q_PROPERTY(double selectedCCDPrismPositionAngle READ getSelectedCCDPrismPositionAngle WRITE
                setSelectedCCDPrismPositionAngle NOTIFY selectedCCDPrismPositionAngleChanged)

   Q_PROPERTY(
     bool flagGuiPanelEnabled READ getFlagGuiPanelEnabled WRITE enableGuiPanel NOTIFY flagGuiPanelEnabledChanged)
   Q_PROPERTY(bool flagInitFOVUsage READ getFlagInitFovUsage WRITE setFlagInitFovUsage NOTIFY flagInitFOVUsageChanged)
   Q_PROPERTY(bool flagInitDirectionUsage READ getFlagInitDirectionUsage WRITE setFlagInitDirectionUsage NOTIFY
                flagInitDirectionUsageChanged)
   Q_PROPERTY(bool flagShowResolutionCriteria READ getFlagShowResolutionCriteria WRITE setFlagShowResolutionCriteria
                NOTIFY flagShowResolutionCriteriaChanged)
   Q_PROPERTY(bool flagRequireSelection READ getFlagRequireSelection WRITE setFlagRequireSelection NOTIFY
                flagRequireSelectionChanged)
   Q_PROPERTY(bool flagAutoLimitMagnitude READ getFlagAutoLimitMagnitude WRITE setFlagAutoLimitMagnitude NOTIFY
                flagAutoLimitMagnitudeChanged)
   Q_PROPERTY(
     bool flagHideGridsLines READ getFlagHideGridsLines WRITE setFlagHideGridsLines NOTIFY flagHideGridsLinesChanged)
   Q_PROPERTY(bool flagScaleImageCircle READ getFlagScaleImageCircle WRITE setFlagScaleImageCircle NOTIFY
                flagScaleImageCircleChanged)
   Q_PROPERTY(bool flagSemiTransparency READ getFlagUseSemiTransparency WRITE setFlagUseSemiTransparency NOTIFY
                flagUseSemiTransparencyChanged)
   Q_PROPERTY(int transparencyMask READ getTransparencyMask WRITE setTransparencyMask NOTIFY transparencyMaskChanged)
   Q_PROPERTY(bool flagDMSDegrees READ getFlagDMSDegrees WRITE setFlagDMSDegrees NOTIFY flagDMSDegreesChanged)
   Q_PROPERTY(bool flagAutosetMountForCCD READ getFlagAutosetMountForCCD WRITE setFlagAutosetMountForCCD NOTIFY
                flagAutosetMountForCCDChanged)
   Q_PROPERTY(bool flagScalingFOVForTelrad READ getFlagScalingFOVForTelrad WRITE setFlagScalingFOVForTelrad NOTIFY
                flagScalingFOVForTelradChanged)
   Q_PROPERTY(Vec4d telradFOV READ getTelradFOV WRITE setTelradFOV NOTIFY telradFOVChanged)
   Q_PROPERTY(bool flagScalingFOVForCCD READ getFlagScalingFOVForCCD WRITE setFlagScalingFOVForCCD NOTIFY
                flagScalingFOVForCCDChanged)
   Q_PROPERTY(bool flagShowOcularsButton READ getFlagShowOcularsButton WRITE setFlagShowOcularsButton NOTIFY
                flagShowOcularsButtonChanged)
   Q_PROPERTY(bool flagShowContour READ getFlagShowContour WRITE setFlagShowContour NOTIFY flagShowContourChanged)
   Q_PROPERTY(
     bool flagShowCardinals READ getFlagShowCardinals WRITE setFlagShowCardinals NOTIFY flagShowCardinalsChanged)
   Q_PROPERTY(
     bool flagAlignCrosshair READ getFlagAlignCrosshair WRITE setFlagAlignCrosshair NOTIFY flagAlignCrosshairChanged)

   Q_PROPERTY(int arrowButtonScale READ getArrowButtonScale WRITE setArrowButtonScale NOTIFY arrowButtonScaleChanged)
   Q_PROPERTY(int guiPanelFontSize READ getGuiPanelFontSize WRITE setGuiPanelFontSize NOTIFY guiPanelFontSizeChanged)
   Q_PROPERTY(Vec3f textColor READ getTextColor WRITE setTextColor NOTIFY textColorChanged)
   Q_PROPERTY(Vec3f lineColor READ getLineColor WRITE setLineColor NOTIFY textColorChanged)

   Q_PROPERTY(bool flagShowCcdCropOverlay READ getFlagShowCcdCropOverlay WRITE setFlagShowCcdCropOverlay NOTIFY
                flagShowCcdCropOverlayChanged)
   Q_PROPERTY(bool flagShowCcdCropOverlayPixelGrid READ getFlagShowCcdCropOverlayPixelGrid WRITE
                setFlagShowCcdCropOverlayPixelGrid NOTIFY flagShowCcdCropOverlayPixelGridChanged)
   Q_PROPERTY(
     int ccdCropOverlayHSize READ getCcdCropOverlayHSize WRITE setCcdCropOverlayHSize NOTIFY ccdCropOverlayHSizeChanged)
   Q_PROPERTY(
     int ccdCropOverlayVSize READ getCcdCropOverlayVSize WRITE setCcdCropOverlayVSize NOTIFY ccdCropOverlayVSizeChanged)

   Q_PROPERTY(bool flagShowFocuserOverlay READ getFlagShowFocuserOverlay WRITE setFlagShowFocuserOverlay NOTIFY
                flagShowFocuserOverlayChanged)
   Q_PROPERTY(bool flagUseSmallFocuserOverlay READ getFlagUseSmallFocuserOverlay WRITE setFlagUseSmallFocuserOverlay
                NOTIFY flagUseSmallFocuserOverlayChanged)
   Q_PROPERTY(bool flagUseMediumFocuserOverlay READ getFlagUseMediumFocuserOverlay WRITE setFlagUseMediumFocuserOverlay
                NOTIFY flagUseMediumFocuserOverlayChanged)
   Q_PROPERTY(bool flagUseLargeFocuserOverlay READ getFlagUseLargeFocuserOverlay WRITE setFlagUseLargeFocuserOverlay
                NOTIFY flagUseLargeFocuserOverlayChanged)
   Q_PROPERTY(Vec3f focuserColor READ getFocuserColor WRITE setFocuserColor NOTIFY focuserColorChanged)

   // BM: Temporary, until the GUI is finalized and some other method of getting
   // info from the main class is implemented.
   friend class OcularsGuiPanel;

public:
   Oculars();
   ~Oculars() override = default;

   ///////////////////////////////////////////////////////////////////////////
   // Methods defined in the StelModule class
   void init() override;
   void deinit() override;
   auto configureGui(bool show = true) -> bool override;
   void draw(StelCore * core) override;
   auto getCallOrder(StelModuleActionName actionName) const -> double override;
   //! Returns the module-specific style sheet.
   void handleMouseClicks(class QMouseEvent * event) override;
   void update(double deltaTime) override;

   auto getDimensionsString(double fovX, double fovY) const -> QString;

   //! return the plugin's own settings (these do not include the settings from the main application)
   //! This implementation return a singleton (class static) QSettings object.
   auto getSettings() -> QSettings * override;
   auto getEnableOcular() const -> bool;
   auto getSelectedCCDRotationAngle() const -> double;
   auto getSelectedCCDPrismPositionAngle() const -> double;
   auto getSelectedCCDIndex() const -> int;
   auto getSelectedOcularIndex() const -> int;
   auto getSelectedTelescopeIndex() const -> int;
   auto getSelectedLensIndex() const -> int;
   auto getEnableCCD() const -> bool;
   auto getEnableCrosshairs() const -> bool;
   auto getEnableTelrad() const -> bool;
   auto getFlagGuiPanelEnabled() const -> bool;
   auto getGuiPanelFontSize() const -> int;
   auto getTextColor() const -> Vec3f;
   auto getLineColor() const -> Vec3f;
   auto getFocuserColor() const -> Vec3f;
   auto getTelradFOV() const -> Vec4d;
   auto getFlagDMSDegrees() const -> bool;
   auto getFlagRequireSelection() const -> bool;
   auto getFlagAutoLimitMagnitude() const -> bool;
   auto getMagLimitStarsOcularsManual() const -> double;
   auto getFlagInitFovUsage() const -> bool;
   auto getFlagInitDirectionUsage() const -> bool;
   auto getFlagAutosetMountForCCD() const -> bool;
   auto getFlagScalingFOVForTelrad() const -> bool;
   auto getFlagScalingFOVForCCD() const -> bool;
   auto getFlagUseSemiTransparency() const -> bool;
   auto getTransparencyMask() const -> int;
   auto getFlagShowResolutionCriteria() const -> bool;
   auto getCcdCropOverlayHSize() const -> int;
   auto getCcdCropOverlayVSize() const -> int;
   auto getArrowButtonScale() const -> int;
   auto getFlagHideGridsLines() const -> bool;
   auto getFlagScaleImageCircle() const -> bool;
   auto getFlagShowOcularsButton() const -> bool;
   auto getFlagShowCcdCropOverlay() const -> bool;
   auto getFlagShowCcdCropOverlayPixelGrid() const -> bool;
   auto getFlagShowContour() const -> bool;
   auto getFlagShowCardinals() const -> bool;
   auto getFlagAlignCrosshair() const -> bool;
   auto getFlagShowFocuserOverlay() const -> bool;
   auto getFlagUseSmallFocuserOverlay() const -> bool;
   auto getFlagUseMediumFocuserOverlay() const -> bool;
   auto getFlagUseLargeFocuserOverlay() const -> bool;

public slots:
   //! Update the ocular, telescope and sensor lists after the removal of a member.
   //! Necessary because of the way model/view management in the OcularDialog
   //! is implemented.
   void updateLists();
   void ccdRotationReset();
   void prismPositionAngleReset();
   void decrementCCDIndex();
   void decrementOcularIndex();
   void decrementTelescopeIndex();
   void decrementLensIndex();
   void displayPopupMenu();
   //! This method is called with we detect that our hot key is pressed.  It handles
   //! determining if we should do anything - based on a selected object.
   void enableOcular(bool b);
   void incrementCCDIndex();
   void incrementOcularIndex();
   void incrementTelescopeIndex();
   void incrementLensIndex();
   void disableLens();
   // Rotate reticle
   void rotateReticleClockwise();
   void rotateReticleCounterclockwise();

   void rotateCCD(int amount);                          //!< amount must be a number. This adds to the current rotation.
   void setSelectedCCDRotationAngle(double angle);      //!< set rotation angle for currently selected CCD
   void rotatePrism(int amount);                        //!< amount must be a number. This adds to the current rotation.
   void setSelectedCCDPrismPositionAngle(double angle); //!< set position angle for prism/OAG of currently selected CC
   void selectCCDAtIndex(int index);                    //!< index in the range of -1:ccds.count(), else call is ignored
   void selectOcularAtIndex(int index);    //!< index in the range of -1:oculars.count(), else call is ignored
   void selectTelescopeAtIndex(int index); //!< index in the range of -1:telescopes.count(), else call is ignored
   void selectLensAtIndex(int index);      //!< index in the range -1:lenses.count(), else call is ignored

   //! Toggles the sensor frame overlay.
   void toggleCCD(bool show);
   //! Toggles the sensor frame overlay (overloaded for blind switching).
   void toggleCCD();
   void toggleCrosshairs(bool show = true);
   //! Toggles the Telrad sight overlay.
   void toggleTelrad(bool show);
   //! Toggles the Telrad sight overlay (overloaded for blind switching).
   void toggleTelrad();

   void enableGuiPanel(bool enable = true);
   void setGuiPanelFontSize(int size);
   void setTextColor(Vec3f color);
   void setLineColor(Vec3f color);
   void setFocuserColor(Vec3f color);
   void setTelradFOV(Vec4d fov);
   void setFlagDMSDegrees(bool b);
   void setFlagRequireSelection(bool b);
   void setFlagAutoLimitMagnitude(bool b);
   void setMagLimitStarsOcularsManual(double mag);
   void setFlagInitFovUsage(bool b);
   void setFlagInitDirectionUsage(bool b);
   void setFlagScalingFOVForTelrad(bool b);
   void setFlagScalingFOVForCCD(bool b);
   void setFlagUseSemiTransparency(bool b);
   void setTransparencyMask(int v);
   void setFlagShowResolutionCriteria(bool b);
   void setCcdCropOverlayHSize(int size);
   void setCcdCropOverlayVSize(int size);
   void setArrowButtonScale(int val);
   void setFlagHideGridsLines(bool b);
   void setFlagScaleImageCircle(bool state);
   //! Define whether the button toggling eyepieces should be visible
   void setFlagShowOcularsButton(bool b);
   void setFontSize(int s);
   //! Connect this to StelApp font size.
   void setFontSizeFromApp(int s);
   void setFlagShowCcdCropOverlay(bool b);
   void setFlagShowCcdCropOverlayPixelGrid(bool b);
   void setFlagShowContour(bool b);
   void setFlagShowCardinals(bool b);
   void setFlagAlignCrosshair(bool b);
   void setFlagShowFocuserOverlay(bool b);
   void setFlagUseSmallFocuserOverlay(bool b);
   void setFlagUseMediumFocuserOverlay(bool b);
   void setFlagUseLargeFocuserOverlay(bool b);
   void setFlagAutosetMountForCCD(bool b);

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
   void selectedCCDPrismPositionAngleChanged(double value);

   void flagGuiPanelEnabledChanged(bool value);
   void guiPanelFontSizeChanged(int value);
   void textColorChanged(Vec3f color);
   void lineColorChanged(Vec3f color);
   void focuserColorChanged(Vec3f color);
   void flagHideGridsLinesChanged(bool value);
   void flagAutosetMountForCCDChanged(bool value);
   void flagScalingFOVForTelradChanged(bool value);
   void telradFOVChanged(Vec4d fov);
   void flagScalingFOVForCCDChanged(bool value);
   void flagUseSemiTransparencyChanged(bool value);
   void transparencyMaskChanged(int value);
   void flagShowResolutionCriteriaChanged(bool value);
   void arrowButtonScaleChanged(int value);
   void flagInitDirectionUsageChanged(bool value);
   void flagInitFOVUsageChanged(bool value);
   void flagRequireSelectionChanged(bool value);
   void flagAutoLimitMagnitudeChanged(bool value);
   void flagDMSDegreesChanged(bool value);
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
   void updateOcularReticle();
   void togglePixelGrid();
   void toggleCropOverlay();
   void toggleFocuserOverlay();
   void handleAutoLimitToggle(bool on);    //!< do a few activities in the background.
   void handleStarMagLimitToggle(bool on); //!< Handle switching the main program's star limitation flag

private:
   //! Compute the limiting magnitude for a telescope
   static auto        computeLimitMagnitude(Ocular * ocular, Telescope * telescope) -> double;

   //! Set up the Qt actions needed to activate the plugin.
   void               initializeActivationActions();

   //! Returns TRUE if at least one bincular is defined.
   auto               isBinocularDefined() -> bool;

   //! Renders the CCD bounding box on-screen.  A telescope must be selected, or this call does nothing.
   void               paintCCDBounds();
   //! Renders crosshairs into the viewport.
   void               paintCrosshairs();
   //! Paint the mask into the viewport.
   void               paintOcularMask(const StelCore * core);
   //! Renders the three Telrad circles, but only if not in ocular mode.
   void               paintTelrad();

   //! Paints the text about the current object selections to the upper right hand of the screen.
   //! Should only be called from a 'ready' state; currently from the draw() method.
   void               paintText(const StelCore * core);

   //! This method is called by the zoom() method, when this plugin is toggled off; it resets to the default view.
   void               unzoomOcular();

   //! This method is responsible for ensuring a valid ini file for the plugin exists.  It first checks to see
   //! if one exists in the expected location.  If it does not, a default one is copied into place, and the process
   //! ends.  However, if one does exist, it opens it, and looks for the oculars_version key.  The value (or even
   //! presence) is used to determine if the ini file is usable.  If not, it is renamed, and a new one copied over.
   //! It does not try to copy values over.
   //! Once there is a valid ini file, it is loaded into the settings attribute.
   void               validateAndLoadIniFile();

   void               toggleLines(bool visible) const;

   //! toggles the actual ocular view.
   //! Record the state of the GridLinesMgr and other settings beforehand, so that they can be reset afterwards.
   //! @param zoomedIn if true, this zoom operation is starting from an already zoomed state.
   //!		False for the original state.
   void               zoom(bool zoomedIn);

   //! This method is called by the zoom() method, when this plugin is toggled on; it resets the zoomed view.
   void               zoomOcular();

   void               hideUsageMessageIfDisplayed();

   //! Creates the sub-menu listing lense in the pop-up menu
   auto               addLensSubmenu(QMenu * parent) -> QMenu *;

   //! Creates the sub-menu listing telescopes in the pop-up menu.
   auto               addTelescopeSubmenu(QMenu * parent) -> QMenu *;

   //! Returns selected lens,or Q_NULLPTR if no lens is selected
   auto               selectedLens() -> Lens *;

   //! A list of all the oculars defined in the ini file.  Must have at least one, or module will not run.
   QList<CCD *>       ccds;
   QList<Ocular *>    oculars;
   QList<Telescope *> telescopes;
   QList<Lens *>      lenses;

   int selectedCCDIndex; //!< index of the current CCD, in the range of -1:ccds.count().  -1 means no CCD is selected.
   int selectedOcularIndex; //!< index of the current ocular, in the range of -1:oculars.count().  -1 means no ocular is
                            //!< selected.
   int selectedTelescopeIndex; //!< index of the current telescope, in the range of -1:telescopes.count(). -1 means none
                               //!< is selected.
   int selectedLensIndex; //!< index of the current lens, in the range of -1:lense.count(). -1 means no lens is selected
   double selectedCCDRotationAngle;      //!< allows rotating via property/remotecontrol API
   double selectedCCDPrismPositionAngle; //!< allows rotating via property/remotecontrol API
   int    arrowButtonScale; //!< allows scaling of the GUI "previous/next" Ocular/CCD/Telescope etc. buttons

   QFont  font;                //!< The font used for drawing labels.
   bool   flagShowCCD;         //!< flag used to track if we are in CCD mode.
   bool   flagShowOculars;     //!< flag used to track if we are in ocular mode.
   bool   flagShowCrosshairs;  //!< flag used to track in crosshairs should be rendered in the ocular view.
   bool   flagShowTelrad;      //!< If true, display the Telrad overlay.
   int    usageMessageLabelID; //!< the id of the label showing the usage message. -1 means it's not displayed.

   bool   flagCardinalPointsMain; //!< Flag to track if CardinalPoints was displayed at activation.
   bool   flagAdaptationMain;     //!< Flag to track if adaptationCheckbox was enabled at activation.

   bool   flagLimitStarsMain;    //!< Flag to track limitation of stellar magnitude in the main program
   double magLimitStarsMain;     //!< Value of limited stellar magnitude in the main program
   bool   flagLimitStarsOculars; //!< Track whether a stellar magnitude limit should be activated when Oculars view is
                                 //!< selected. This flag is not a StelProperty, but linked to
                                 //!< SkyDrawer.flagStarMagnitudeLimit while oculars view is active. This is required to
                                 //!< set the manual limitation flag in SkyDrawer.
   double magLimitStarsOculars;  //!< Value of limited magnitude for stars in oculars mode, when not auto-defined with
                                 //!< flagAutoLimitMagnitude. This value is not a StelProperty, but linked to
                                 //!< SkyDrawer.customStarMagLimit while oculars view is active and automatic setting of
   //!< magnitude is not active. If user modifies the magnitude while flagAutoLimitMagnitude is
   //!< true, the value will not be stored permanently. [FIXME: Recheck this sentence.]
   bool flagAutoLimitMagnitude; //!< Decide whether stellar magnitudes should be auto-limited based on telescope/ocular
                                //!< combination. If false, the manual limitation value magLimitStarsOculars takes over,
                                //!< and limitation is decided by flagLimitStarsOculars. If true, flagLimitStarsOculars
                                //!< is set true when activating Oculars view, and will remain true.
   bool flagLimitDSOsMain;      //!< Flag to track limit magnitude for DSOs
   double magLimitDSOsMain;     //!< Value of limited magnitude for DSOs
   bool   flagLimitPlanetsMain; //!< Flag to track limit magnitude for planets, asteroids, comets etc.
   double magLimitPlanetsMain;  //!< Value of limited magnitude for planets, asteroids, comets etc.
   double relativeStarScaleMain;    //!< Value to store the usual relative star scale when activating ocular or CCD view
   double absoluteStarScaleMain;    //!< Value to store the usual absolute star scale when activating ocular or CCD view
   double relativeStarScaleOculars; //!< Value to store the relative star scale when switching off ocular view
   double absoluteStarScaleOculars; //!< Value to store the absolute star scale when switching off ocular view
   double relativeStarScaleCCD;     //!< Value to store the relative star scale when switching off CCD view
   double absoluteStarScaleCCD;     //!< Value to store the absolute star scale when switching off CCD view
   bool   flagMoonScaleMain;        //!< Flag to track of usage zooming of the Moon
   bool   flagMinorBodiesScaleMain; //!< Flag to track of usage zooming of minor bodies
   double milkyWaySaturation;

   double maxEyepieceAngle;     //!< The maximum aFOV of any eyepiece.
   bool   flagRequireSelection; //!< Decide whether an object is required to be selected to zoom in.
   bool   flagScaleImageCircle; //!< Decide whether to scale the mask based aFOV.

   bool   flagGuiPanelEnabled;        //!< Display the GUI control panel
   bool   flagDMSDegrees;             //!< Use decimal degrees in CCD frame display
   bool   flagSemiTransparency;       //!< Draw the area outside the ocular circle not black but let some stars through.
   int    transparencyMask;           //!< Value of transparency for semi-transparent mask
   bool   flagHideGridsLines;         //!< Switch off all grids and lines of GridMgr while in Ocular view
   bool   flagGridLinesDisplayedMain; //!< keep track of gridline display while possibly suppressing their display.
   bool   flagConstellationLinesMain; //!< keep track of constellation display while possibly suppressing their display.
   bool
     flagConstellationBoundariesMain; //!< keep track of constellation display while possibly suppressing their display.
   bool      flagAsterismLinesMain;   //!< keep track of asterism display while possibly suppressing their display.
   bool      flagRayHelpersLinesMain; //!< keep track of ray helpers display while possibly suppressing their display.
   bool      flipVertMain;            //!< keep track of screen flip in main program
   bool      flipHorzMain;            //!< keep track of screen flip in main program

   // for toolbar button
   QPixmap * pxmapGlow;
   QPixmap * pxmapOnIcon;
   QPixmap * pxmapOffIcon;
   StelButton *            toolbarButton;
   bool                    flagShowOcularsButton;

   OcularDialog *          ocularDialog;
   bool                    ready; //!< A flag that determines that this module is usable.  If false, we won't open.

   StelAction *            actionShowOcular;
   StelAction *            actionShowCrosshairs;
   StelAction *            actionShowSensor;
   StelAction *            actionShowTelrad;
   StelAction *            actionConfiguration;
   StelAction *            actionMenu;
   StelAction *            actionTelescopeIncrement;
   StelAction *            actionTelescopeDecrement;
   StelAction *            actionOcularIncrement;
   StelAction *            actionOcularDecrement;

   class OcularsGuiPanel * guiPanel;
   int                     guiPanelFontSize;
   Vec3f                   textColor;
   Vec3f                   lineColor;
   Vec3f                   focuserColor;

   // Reticle
   StelTextureSP           reticleTexture;
   StelTextureSP           cardinalsNormalTexture;
   StelTextureSP           cardinalsMirroredTexture;
   double actualFOV;        //!< Holds the FOV of the ocular/tescope/lens combination; what the screen is zoomed to.
   double initialFOV;       //!< Holds the initial FOV, degrees
   bool   flagInitFOVUsage; //!< Flag used to track if we use default initial FOV (value at the startup of planetarium).
   bool   flagInitDirectionUsage;  //!< Flag used to track if we use default initial direction (value at the startup of
                                   //!< planetarium).
   bool   flagAutosetMountForCCD;  //!< Flag used to track if we use automatic switch to type of mount for CCD frame
   bool   flagScalingFOVForTelrad; //!< Flag used to track if we use automatic scaling FOV for Telrad
   bool   flagScalingFOVForCCD;    //!< Flag used to track if we use automatic scaling FOV for CCD
   bool   flagShowResolutionCriteria; //!< Show various criteria for angular resolution based on telescope/ocular
   bool   equatorialMountEnabledMain; //!< Keep track of mount used in main program.
   double reticleRotation;
   bool   flagShowCcdCropOverlay;          //!< Flag used to track if the ccd crop overlay should be shown.
   bool   flagShowCcdCropOverlayPixelGrid; //!< Flag used to track if the ccd full grid overlay should be shown.
   int    ccdCropOverlayHSize;             //!< Holds the ccd crop overlay size
   int    ccdCropOverlayVSize;             //!< Holds the ccd crop overlay size
   bool   flagShowContour;
   bool   flagShowCardinals;
   bool   flagAlignCrosshair;
   Vec4d  telradFOV;                   //!< 4-vector of circles. The fourth element is non-standard.
   bool   flagShowFocuserOverlay;      //!< Flag used to track if the focuser crop overlay should be shown.
   bool   flagUseSmallFocuserOverlay;  //!< Flag used to track if a small-sized focuser (1.25") overlay should be shown.
   bool   flagUseMediumFocuserOverlay; //!< Flag used to track if a medium-sized focuser (2.0") overlay should be shown.
   bool   flagUseLargeFocuserOverlay;  //!< Flag used to track if a large-sized focuser (3.3") overlay should be shown.
};

#include "StelPluginInterface.hpp"
#include <QObject>

//! This class is used by Qt to manage a plug-in interface
class OcularsStelPluginInterface
   : public QObject
   , public StelPluginInterface
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
   Q_INTERFACES(StelPluginInterface)
public:
   auto getStelModule() const -> StelModule * override;
   auto getPluginInfo() const -> StelPluginInfo override;
   auto getExtensionList() const -> QObjectList override { return QObjectList(); }
};
