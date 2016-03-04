/*
 * Copyright (C) 2009 Timothy Reaves
 * Copyright (C) 2011 Bogdan Marinov
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

#ifndef _OCULARS_HPP_
#define _OCULARS_HPP_

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

#define MIN_OCULARS_INI_VERSION 2

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
combination of eyepiece and telescope. I wanted to be able to get an idea of
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

None of these activities can take the plce of hands-on experience, but they are
a good way to supplement your visual astronomy interests.
@}
*/

//! Main class of the Oculars plug-in.
//! @class Oculars
//! @ingroup oculars
class Oculars : public StelModule
{
	Q_OBJECT
	//BM: Temporary, until the GUI is finalized and some other method of getting
	//info from the main class is implemented.
	friend class OcularsGuiPanel;

public:
	Oculars();
	virtual ~Oculars();
	static QSettings* appSettings();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual bool configureGui(bool show=true);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! Returns the module-specific style sheet.
	//! The main StelStyle instance should be passed.
	virtual const StelStyle getModuleStyleSheet(const StelStyle& style);
	//! This method is needed because the MovementMgr classes handleKeys() method consumes the event.
	//! Because we want the ocular view to track, we must intercept & process ourselves.  Only called
	//! while flagShowOculars or flagShowCCD == true.
	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual void update(double) {;}
	double ccdRotationAngle() const;

	QString getDimensionsString(double fovX, double fovY) const;
	QString getFOVString(double fov) const;

public slots:
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
	void incrementCCDIndex();
	void incrementOcularIndex();
	void incrementTelescopeIndex();
	void incrementLensIndex();
	void disableLens();
	void rotateCCD(QString amount); //!< amount must be a number.
	void selectCCDAtIndex(QString indexString); //!< indexString must be an integer, in the range of -1:ccds.count()
	void selectOcularAtIndex(QString indexString);  //!< indexString must be an integer, in the range of -1:oculars.count()
	void selectTelescopeAtIndex(QString indexString);  //!< indexString must be an integer, in the range of -1:telescopes.count()
	void selectLensAtIndex(QString indexString); //!< indexString must be an integer, in the range -1:lense.count<()
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

	void setFlagDecimalDegrees(const bool b);
	bool getFlagDecimalDegrees(void) const;

	void setFlagLimitMagnitude(const bool b);
	bool getFlagLimitMagnitude(void) const;

	void setFlagInitFovUsage(const bool b);
	bool getFlagInitFovUsage(void) const;

	void setFlagUseFlipForCCD(const bool b);
	bool getFlagUseFlipForCCD(void) const;

signals:
	void selectedCCDChanged();
	void selectedOcularChanged();
	void selectedTelescopeChanged();
	void selectedLensChanged();

private slots:
	//! Signifies a change in ocular or telescope.  Sets new zoom level.
	void instrumentChanged();
	void determineMaxEyepieceAngle();
	void setRequireSelection(bool state);
	void setScaleImageCircle(bool state);	
	void setScreenFOVForCCD();
	void retranslateGui();
	void setStelStyle(const QString& style);
	void updateOcularReticle(void);

private:
	//! Set up the Qt actions needed to activate the plugin.
	void initializeActivationActions();

	//! Returns TRUE if at least one bincular is defined.
	bool isBinocularDefined();

	//! Reneders the CCD bounding box on-screen.  A telescope must be selected, or this call does nothing.
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

	//! This method is responsible for insuring a valid ini file for the plugin exists.  It first checks to see
	//! if one exists in the expected location.  If it does not, a default one is copied into place, and the process
	//! ends.  However, if one does exist, it opens it, and looks for the oculars_version key.  The value (or even
	//! presence) is used to determine if the ini file is usable.  If not, it is renamed, and a new one copied over.
	//! It does not ty to cope values over.
	//! Once there is a valid ini file, it is loaded into the settings attribute.
	void validateAndLoadIniFile();

	//! Recordd the state of the GridLinesMgr views beforehand, so that it can be reset afterwords.
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

	//! Returns selected lens,or NULL if no lens is selected
	Lens* selectedLens();

	//! A list of all the oculars defined in the ini file.  Must have at least one, or module will not run.
	QList<CCD *> ccds;
	QList<Ocular *> oculars;
	QList<Telescope *> telescopes;
	QList<Lens *> lense;

	int selectedCCDIndex; //!< index of the current CCD, in the range of -1:ccds.count().  -1 means no CCD is selected.
	int selectedOcularIndex; //!< index of the current ocular, in the range of -1:oculars.count().  -1 means no ocular is selected.
	int selectedTelescopeIndex; //!< index of the current telescope, in the range of -1:telescopes.count(). -1 means none is selected.
	int selectedLensIndex; //!<  index of the current lens, in the range of -1:lense.count(). -1 means no lens is selected

	QFont font;			//!< The font used for drawing labels.
	bool flagShowCCD;		//!< flag used to track f we are in CCD mode.
	bool flagShowOculars;		//!< flag used to track if we are in ocular mode.
	bool flagShowCrosshairs;	//!< flag used to track in crosshairs should be rendered in the ocular view.
	bool flagShowTelrad;		//!< If true, display the Telrad overlay.
	int usageMessageLabelID;	//!< the id of the label showing the usage message. -1 means it's not displayed.

	bool flagAzimuthalGrid;		//!< Flag to track if AzimuthalGrid was displayed at activation.
	bool flagGalacticGrid;		//!< Flag to track if GalacticGrid was displayed at activation.
	bool flagEquatorGrid;		//!< Flag to track if EquatorGrid was displayed at activation.
	bool flagEquatorJ2000Grid;	//!< Flag to track if EquatorJ2000Grid was displayed at activation.
	bool flagEquatorLine;		//!< Flag to track if EquatorLine was displayed at activation.
	bool flagEclipticLine;		//!< Flag to track if EclipticLine was displayed at activation.
	bool flagEclipticJ2000Grid;	//!< Flag to track if EclipticJ2000Grid was displayed at activation.
	bool flagMeridianLine;		//!< Flag to track if MeridianLine was displayed at activation.
	bool flagLongitudeLine;		//!< Flag to track if LongitudeLine was displayed at activation.
	bool flagHorizonLine;		//!< Flag to track if HorizonLine was displayed at activation.
	bool flagGalacticEquatorLine;	//!< Flag to track if GalacticEquatorLine was displayed at activation.
	bool flagAdaptation;		//!< Flag to track if adaptationCheckbox was enabled at activation.

	bool flagLimitStars;		//!< Flag to track limit magnitude for stars
	float magLimitStars;		//!< Value of limited magnitude for stars
	bool flagLimitDSOs;		//!< Flag to track limit magnitude for DSOs
	float magLimitDSOs;		//!< Value of limited magnitude for DSOs
	bool flagLimitPlanets;		//!< Flag to track limit magnitude for planets, asteroids, comets etc.
	float magLimitPlanets;		//!< Value of limited magnitude for planets, asteroids, comets etc.

	bool flagMoonScale;		//!< Flag to track of usage zooming of the Moon

	double maxEyepieceAngle;	//!< The maximum aFOV of any eyepiece.
	bool requireSelection;		//!< Read from the ini file, whether an object is required to be selected to zoom in.
	bool flagLimitMagnitude;	//!< Read from the ini file, whether a magnitude is required to be limited.
	bool useMaxEyepieceAngle;	//!< Read from the ini file, whether to scale the mask based aFOV.
	//! Display the GUI control panel
	bool guiPanelEnabled;
	bool flagDecimalDegrees;
	bool flipVert;
	bool flipHorz;

	QSignalMapper * ccdRotationSignalMapper;  //!< Used to rotate the CCD. */
	QSignalMapper * ccdsSignalMapper; //!< Used to determine which CCD was selected from the popup navigator. */
	QSignalMapper * ocularsSignalMapper; //!< Used to determine which ocular was selected from the popup navigator. */
	QSignalMapper * telescopesSignalMapper; //!< Used to determine which telescope was selected from the popup navigator. */
	QSignalMapper * lenseSignalMapper; //!< Used to determine which lens was selected from the popup navigator */

	// for toolbar button
	QPixmap * pxmapGlow;
	QPixmap * pxmapOnIcon;
	QPixmap * pxmapOffIcon;
	StelButton * toolbarButton;

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

	//Styles
	QByteArray normalStyleSheet;
	QByteArray nightStyleSheet;

	//Reticle
	StelTextureSP reticleTexture;
	double actualFOV;		//!< Holds the FOV of the ocular/tescope/lens cobination; what the screen is zoomed to.
	double initialFOV;		//!< Holds the initial FOV
	bool flagInitFOVUsage;		//!< Flag used to track if we use default initial FOV (value at the startup of planetarium).
	bool flagUseFlipForCCD;		//!< Flag used to track if we use flips for CCD
	double reticleRotation;
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
};

#endif /*_OCULARS_HPP_*/
