/*
 * Copyright (C) 2009 Matthew Gates
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
 
#ifndef TEXTUSERINTERFACE_HPP
#define TEXTUSERINTERFACE_HPP

#include "StelModule.hpp"
#include "DummyDialog.hpp"
#include "StelCore.hpp"

#include <QObject>
#include <QString>
#include <QFont>

class TuiNode;

//! The Text User Interface (TUI) plugin replaces the old (pre-0.10 series) text user interface.
//! It used to be activated with M until the 0.14 series, but was changed to Alt-T for 0.15 and later (to be consistent with the Ctrl-T hiding of the GUI).
class TextUserInterface : public StelModule
{
	Q_OBJECT
public:
	TextUserInterface();
	virtual ~TextUserInterface();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);

	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TextUserInterface
	//! Loads the module's configuration from the config file.
	void loadConfiguration(void);

public slots:
	//! Show/hide the TUI menu
	void setTuiMenuActive(bool tActive) { tuiActive = tActive;}
	//! Show/hide the TUI date time display
	void setTuiDateTime(bool tDateTime) { tuiDateTime = tDateTime; }
	//! Show/hide the selected object's short object information 
	void setTuiObjInfo(bool tObjInfo) { tuiObjInfo = tObjInfo; }
	//! Set Gravity text for the TUI text
	void setTuiGravityUi(bool tGravityUi) { tuiGravityUi = tGravityUi; }

private slots:
	void setHomePlanet(QString planetName);
	void setAltitude(int altitude);
	void setLatitude(double latitude);
	void setLongitude(double longitude);
	void setStartupDateMode(QString mode);
	void setDateFormat(QString format);
	void setTimeFormat(QString format);
	void setSkyCulture(QString i18);
	void setAppLanguage(QString lang);
	void setSkyLanguage(QString lang);
	void saveDefaultSettings(void);
	void shutDown(void);

private:
	DummyDialog dummyDialog;
	QFont font;
	bool tuiActive;
	bool tuiDateTime;
	bool tuiObjInfo;
	bool tuiGravityUi;
	TuiNode* currentNode;
	Vec3f color;

	double getLatitude(void);
	double getLongitude(void);
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TextUserInterfaceStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* TEXTUSERINTERFACE_HPP*/
