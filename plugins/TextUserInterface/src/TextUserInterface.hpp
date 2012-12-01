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
 
#ifndef TEXTUSERINTERFACE_HPP_
#define _TEXTUSERINTERFACE_HPP_ 1

#include "StelModule.hpp"
#include "DummyDialog.hpp"

#include <QObject>
#include <QString>
#include <QFont>

class TuiNode;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
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
	virtual void draw(StelCore* core, class StelRenderer* renderer);
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
	void saveDefaultSettings(void);
	void shutDown(void);
	void setBortleScale(int bortle);

private:
	DummyDialog dummyDialog;
	QFont font;
	bool tuiActive;
	bool tuiDateTime;
	bool tuiObjInfo;
	bool tuiGravityUi;
	TuiNode* currentNode;

	double getLatitude(void);
	double getLongitude(void);
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TextUserInterfaceStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_TEXTUSERINTERFACE_HPP_*/
