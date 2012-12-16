/*
 * Copyright (C) 2011 Alexander Wolf
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

#ifndef _SUPERNOVAE_HPP_
#define _SUPERNOVAE_HPP_

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelFader.hpp"
#include "Supernova.hpp"
#include <QFont>
#include <QVariantMap>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>
#include <QHash>

class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;
class QSettings;
class QTimer;
class SupernovaeDialog;

typedef QSharedPointer<Supernova> SupernovaP;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class Supernovae : public StelObjectModule
{
	Q_OBJECT
public:	
	//! @enum UpdateState
	//! Used for keeping for track of the download/update status
	enum UpdateState {
		Updating,		//!< Update in progress
		CompleteNoUpdates,	//!< Update completed, there we no updates
		CompleteUpdates,	//!< Update completed, there were updates
		DownloadError,		//!< Error during download phase
		OtherError		//!< Other error
	};

	Supernovae();
	virtual ~Supernovae();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	virtual void drawPointer(StelCore* core, class StelRenderer* renderer, StelProjectorP projector);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for satellites.
	//! @param core the StelCore to use for computations.
	//! @return an list containing the satellites located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching satellite object's pointer if exists or NULL.
	//! @param nameI18n The case in-sensistive satellite name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching satellite if exists or NULL.
	//! @param name The case in-sensistive standard program name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Historical Supernovae"; }

	//! get a supernova object by identifier
	SupernovaP getByID(const QString& id);

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Pulsars section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default supernovae.json file from the resource embedded in the plugin lib/dll file.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);

	//! get whether or not the plugin will try to update catalog data from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled(void) {return updatesEnabled;}
	//! set whether or not the plugin will try to update catalog data from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) {updatesEnabled=b;}

	//! get the date and time the supernovae were updated
	QDateTime getLastUpdate(void) {return lastUpdate;}

	//! get the update frequency in days
	int getUpdateFrequencyDays(void) {return updateFrequencyDays;}
	void setUpdateFrequencyDays(int days) {updateFrequencyDays = days;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}

	//! get list of supernovae
	QString getSupernovaeList();

signals:
	//! @param state the new update state.
	void updateStateChanged(Supernovae::UpdateState state);

	//! emitted after a JSON update has run.
	void jsonUpdateComplete(void);

public slots:
	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");
	void messageTimeout(void);

private:
	// Font used for displaying our text
	QFont font;

	// if existing, delete Satellites section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file and create list of supernovae.
	void readJsonFile(void);

	//! Creates a backup of the supernovae.json file called supernovae.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version from the "version" value in the supernovas.json file
	//! @return version string, e.g. "1"
	int getJsonFileVersion(void);

	//! parse JSON file and load supernovaes to map
	QVariantMap loadSNeMap(QString path=QString());

	//! set items for list of struct from data map
	void setSNeMap(const QVariantMap& map);

	QString sneJsonPath;

	class StelTextureNew* texPointer;
	QList<SupernovaP> snstar;
	QHash<QString, double> snlist;

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager* downloadMgr;
	QString updateUrl;
	QString updateFile;
	QProgressBar* progressBar;
	QTimer* updateTimer;
	QTimer* messageTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyDays;

	QSettings* conf;

	// GUI
	SupernovaeDialog* configDialog;	

private slots:
	//! check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);
	void updateDownloadComplete(QNetworkReply* reply);

};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SupernovaeStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_SUPERNOVAE_HPP_*/
