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

	Q_PROPERTY(bool tuiDateTime READ getTuiDateTime WRITE setTuiDateTime NOTIFY flagShowDateTimeChanged)
	Q_PROPERTY(bool tuiObjInfo  READ getTuiObjInfo  WRITE setTuiObjInfo  NOTIFY flagShowObjInfoChanged)
public:
	TextUserInterface();
	~TextUserInterface() override;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;
	void handleKeys(class QKeyEvent* event) override;

	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TextUserInterface
	//! Loads the module's configuration from the config file.
	void loadConfiguration(void);

public slots:
	//! Show/hide the TUI menu
	void setTuiMenuActive(bool tActive) { tuiActive = tActive;}
	//! Show/hide the TUI date time display
	void setTuiDateTime(bool tDateTime) { tuiDateTime = tDateTime; emit flagShowDateTimeChanged(tDateTime);}
	bool getTuiDateTime(){return tuiDateTime;}
	//! Show/hide the selected object's short object information 
	void setTuiObjInfo(bool tObjInfo) { tuiObjInfo = tObjInfo; emit flagShowObjInfoChanged(tObjInfo);}
	bool getTuiObjInfo(){return tuiObjInfo;}
	//! Set Gravity text for the TUI text
	void setTuiGravityUi(bool tGravityUi) { tuiGravityUi = tGravityUi; }
	//! Set light pollution level
	void setLightPollutionLevel(int level);

private slots:
	void setHomePlanet(const QString &planetName);
	void setAltitude(int altitude);
	void setLatitude(double latitude);
	void setLongitude(double longitude);
	void setStartupDateMode(const QString &mode);
	void setDateFormat(const QString &format);
	void setTimeFormat(const QString &format);
	void setSkyCulture(const QString &i18);
	void setAppLanguage(const QString &lang);
	void setSkyLanguage(const QString &lang);
	void saveDefaultSettings(void);
	void shutDown(void);

signals:
	void flagShowDateTimeChanged(bool);
	void flagShowObjInfoChanged(bool);

private:
	DummyDialog dummyDialog;
	QFont font;
	bool tuiActive;
	bool tuiDateTime; // property
	bool tuiObjInfo;  // property
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
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* TEXTUSERINTERFACE_HPP*/
