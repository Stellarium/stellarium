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

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "OcularDialog.hpp"
#include "CCD.hpp"
#include "Ocular.hpp"
#include "Telescope.hpp"
#include "Barlow.hpp"

#include <QFont>
#include <QSettings>

#define MIN_OCULARS_INI_VERSION 0.12

QT_BEGIN_NAMESPACE
class QAction;
class QKeyEvent;
class QMenu;
class QMouseEvent;
class QPixmap;
class QSettings;
class QSignalMapper;
QT_END_NAMESPACE

class StelButton;

//! Main class of the Oculars plug-in.
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
	virtual void draw(StelCore* core, class StelRenderer* renderer);
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
	void decrementBarlowIndex();
	void displayPopupMenu();
	//! This method is called with we detect that our hot key is pressed.  It handles
	//! determining if we should do anything - based on a selected object.
	void enableOcular(bool b);
	void incrementCCDIndex();
	void incrementOcularIndex();
	void incrementTelescopeIndex();
	void incrementBarlowIndex();
	void disableBarlow();
	void rotateCCD(QString amount); //!< amount must be a number.
	void selectCCDAtIndex(QString indexString); //!< indexString must be an integer, in the range of -1:ccds.count()
	void selectOcularAtIndex(QString indexString);  //!< indexString must be an integer, in the range of -1:oculars.count()
	void selectTelescopeAtIndex(QString indexString);  //!< indexString must be an integer, in the range of -1:telescopes.count()
	void selectBarlowAtIndex(QString indexString); //!< indexString must be an integer, in the range -1:barlows.count<()
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

signals:
	void selectedCCDChanged();
	void selectedOcularChanged();
	void selectedTelescopeChanged();
	void selectedBarlowChanged();

private slots:
	//! Signifies a change in ocular or telescope.  Sets new zoom level.
	void instrumentChanged();
	void determineMaxEyepieceAngle();
	void setRequireSelection(bool state);
	void setScaleImageCircle(bool state);
	void setScreenFOVForCCD();
	void retranslateGui();
	void setStelStyle(const QString& style);

private:
	//! Set up the Qt actions needed to activate the plugin.
	void initializeActivationActions();

	//! Returns TRUE if at least one bincular is defined.
	bool isBinocularDefined();

	//! Reneders the CCD bounding box on-screen.  A telescope must be selected, or this call does nothing.
	void paintCCDBounds(class StelRenderer* renderer);
	//! Renders crosshairs into the viewport.
	void paintCrosshairs(class StelRenderer* renderer);
	//! Paint the mask into the viewport.
	void paintOcularMask(class StelRenderer* renderer);
	//! Renders the three Telrad circles, but only if not in ocular mode.
	void paintTelrad(class StelRenderer* renderer);


	//! Paints the text about the current object selections to the upper right hand of the screen.
	//! Should only be called from a 'ready' state; currently from the draw() method.
	void paintText(const StelCore* core, StelRenderer* renderer);

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

	//! Creates the sub-menu listing barlow lenses in the pop-up menu
	QMenu* addBarlowSubmenu(QMenu* parent);

	//! Creates the sub-menu listing telescopes in the pop-up menu.
	QMenu* addTelescopeSubmenu(QMenu* parent);

	//! Returns selected barlow,or NULL if no barlow is selected
	Barlow* selectedBarlow();

	//! A list of all the oculars defined in the ini file.  Must have at least one, or module will not run.
	QList<CCD *> ccds;
	QList<Ocular *> oculars;
	QList<Telescope *> telescopes;
	QList<Barlow *> barlows;

	int selectedCCDIndex; //!< index of the current CCD, in the range of -1:ccds.count().  -1 means no CCD is selected.
	int selectedOcularIndex; //!< index of the current ocular, in the range of -1:oculars.count().  -1 means no ocular is selected.
	int selectedTelescopeIndex; //!< index of the current telescope, in the range of -1:telescopes.count(). -1 means none is selected.
	int selectedBarlowIndex; //!<  index of the current barlow lends, in the range of -1:barlows.count(). -1 means no barlow is selected

	QFont font;					//!< The font used for drawing labels.
	bool flagShowCCD;				//!< flag used to track f we are in CCD mode.
	bool flagShowOculars;		//!< flag used to track if we are in ocular mode.
	bool flagShowCrosshairs;	//!< flag used to track in crosshairs should be rendered in the ocular view.
	bool flagShowTelrad;			//!< If true, display the Telrad overlay.
	int usageMessageLabelID;	//!< the id of the label showing the usage message. -1 means it's not displayed.

	bool flagAzimuthalGrid;		//!< Flag to track if AzimuthalGrid was displayed at activation.
	bool flagGalacticGrid;		//!< Flag to track if GalacticGrid was displayed at activation.
	bool flagEquatorGrid;		//!< Flag to track if EquatorGrid was displayed at activation.
	bool flagEquatorJ2000Grid;	//!< Flag to track if EquatorJ2000Grid was displayed at activation.
	bool flagEquatorLine;		//!< Flag to track if EquatorLine was displayed at activation.
	bool flagEclipticLine;		//!< Flag to track if EclipticLine was displayed at activation.
	bool flagEclipticJ2000Grid;	//!< Flag to track if EclipticJ2000Grid was displayed at activation.
	bool flagMeridianLine;		//!< Flag to track if MeridianLine was displayed at activation.
	bool flagHorizonLine;		//!< Flag to track if HorizonLine was displayed at activation.
	bool flagGalacticPlaneLine;	//!< Flag to track if GalacticPlaneLine was displayed at activation.

	double ccdRotationAngle;	//!< The angle to rotate the CCD bounding box. */
	double maxEyepieceAngle;	//!< The maximum aFOV of any eyepiece.
	bool requireSelection;		//!< Read from the ini file, whether an object is required to be selected to zoom in.
	bool useMaxEyepieceAngle;	//!< Read from the ini file, whether to scale the mask based aFOV.
	//! Display the GUI control panel
	bool guiPanelEnabled;
	bool flagDecimalDegrees;

	QSignalMapper* ccdRotationSignalMapper;  //!< Used to rotate the CCD. */
	QSignalMapper* ccdsSignalMapper; //!< Used to determine which CCD was selected from the popup navigator. */
	QSignalMapper* ocularsSignalMapper; //!< Used to determine which ocular was selected from the popup navigator. */
	QSignalMapper* telescopesSignalMapper; //!< Used to determine which telescope was selected from the popup navigator. */
	QSignalMapper* barlowSignalMapper; //!< Used to determine which barlow was selected from the popup navigator */

	// for toolbar button
	QPixmap* pxmapGlow;
	QPixmap* pxmapOnIcon;
	QPixmap* pxmapOffIcon;
	StelButton* toolbarButton;

	OcularDialog *ocularDialog;
	bool ready; //!< A flag that determines that this module is usable.  If false, we won't open.

	QAction* actionShowOcular;
	QAction* actionShowCrosshairs;
	QAction* actionShowSensor;
	QAction* actionShowTelrad;
	QAction* actionConfiguration;
	QAction* actionMenu;

	class OcularsGuiPanel* guiPanel;

	//Styles
	QByteArray normalStyleSheet;
	QByteArray nightStyleSheet;
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class OcularsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_OCULARS_HPP_*/
