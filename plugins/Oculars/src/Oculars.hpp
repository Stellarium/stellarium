/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#ifndef _OCULARS_HPP_
#define _OCULARS_HPP_

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "OcularDialog.hpp"
#include "Ocular.hpp"
#include "Telescope.hpp"

#include <QFont>

#define MIN_OCULARS_INI_VERSION 0.12

QT_BEGIN_NAMESPACE
class QSqlTableModel;
class QKeyEvent;
class QMouseEvent;
class QPixmap;
class QSqlQuery;
QT_END_NAMESPACE

class StelButton;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class Oculars : public StelModule
{
	Q_OBJECT

public:
	Oculars();
	virtual ~Oculars();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual bool configureGui(bool show=true);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
//	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	virtual void setStelStyle(const StelStyle& style);
	virtual void update(double deltaTime) {;}
	
	//! Returns the module-specific style sheet.
	//! The main StelStyle instance should be passed.
	const StelStyle getModuleStyleSheet(const StelStyle& style);

public slots:
	//! This method is called with we detect that our hot key is pressed.  It handles
	//! determining if we should do anything - based on a selected object - and painting
	//! labes to the screen.
	void enableOcular(bool b);

	void toggleCrosshair();

	void decrementOcularIndex();
	void decrementTelescopeIndex();
	void incrementOcularIndex();
	void incrementTelescopeIndex();

signals:
	void selectedOcularChanged();
	void selectedTelescopeChanged();

private slots:
	//! Signifies a change in ocular or telescope.  Sets new zoom level.
	void instrumentChanged();
	void determineMaxImageCircle();
	void loadOculars(); 
	void loadTelescopes();
	void setScaleImageCircle(bool state);

private:
	//! Renders crosshairs into the viewport.
	void drawCrosshairs();
	
	bool initializeDB();
	void initializeActions();

	//! This method is needed because the MovementMgr classes handleKeys() method consumes the event.
	//! Because we want the ocular view to track, we must intercept & process ourselves.  Only called
	//! while flagShowOculars == true.
	void interceptMovementKey(class QKeyEvent* event);
	
	void loadDatabaseObjects();
	
	//! Paint the mask into the viewport.
	void paintMask();
	
	//! This method is called by the zoom() method, when this plugin is toggled off; it resets to the default view.
	void unzoomOcular();
	
	//! This method is responsible for insuring a valid ini file for the plugin exists.  It first checks to see
	//! if one exists in the expected location.  If it does not, a default one is copied into place, and the process
	//! ends.  However, if one does exist, it opens it, and looks for the oculars_version key.  The value (or even
	//! presence) is used to determine if the ini file is usable.  If not, it is renamed, and a new one copied over.
	//! It does not ty to cope values over.
	void validateIniFile();

	//! Prepairs the values needed to zoom, and sets them into the syste,  Also recordd the state of
	//! the views beforehand, so that it can be reet afterwords.
	//! @param rezoom if true, this zoom operation is starting from an already zoomed state.  
	//!		False for the original state.
	void zoom(bool rezoom);
	
	//! This method is called by the zoom() method, when this plugin is toggled on; it resets the zoomed view.
	void zoomOcular();

	//! A list of all the oculars defined in the ini file.  Must have at least one, or module will not run.
	QList<Ocular *> oculars;
	QList<Telescope *> telescopes;
	int selectedOcularIndex;
	int selectedTelescopeIndex;
	
	QFont font;					//!< The font used for drawing labels.
	bool flagShowOculars;		//<! flag used to trak if we are in ocular mode.
	bool flagShowCrosshairs;	//<! flag used to track in crosshairs should be rendered in the ocular view.
	int usageMessageLabelID;	//!< the id of the label showing the usage message. -1 means it's not displayed.

	bool flagAzimuthalGrid;		//!< Flag to track if AzimuthalGrid was displayed at activation.
	bool flagEquatorGrid;		//!< Flag to track if EquatorGrid was displayed at activation.
	bool flagEquatorJ2000Grid;	//!< Flag to track if EquatorJ2000Grid was displayed at activation.
	bool flagEquatorLine;		//!< Flag to track if EquatorLine was displayed at activation.
	bool flagEclipticLine;		//!< Flag to track if EclipticLine was displayed at activation.
	bool flagMeridianLine;		//!< Flag to track if MeridianLine was displayed at activation.
	
	double maxImageCircle;		//<! The maximum image circle for all eyepieces.  Used to scale the mask.
	bool useMaxImageCircle;		//!< Read from the ini file, whether to scale the mask based on exit circle size.

	// for toolbar button
	QPixmap* pxmapGlow;
	QPixmap* pxmapOnIcon;
	QPixmap* pxmapOffIcon;
	StelButton* toolbarButton;
	
	OcularDialog *ocularDialog;
	bool visible;
	bool ready; //<! A flag that determines that this module is usable.  If false, we won't open.
	bool newIntrument; //<! true the first time draw is called for a new ocular or telescope, false otherwise.
	
	QSqlTableModel *ocularsTableModel;
	QSqlTableModel *telescopesTableModel;
	
	//Styles
	QByteArray * normalStyleSheet;
	QByteArray * nightStyleSheet;
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

