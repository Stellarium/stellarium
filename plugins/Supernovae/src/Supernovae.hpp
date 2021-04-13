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

#ifndef SUPERNOVAE_HPP
#define SUPERNOVAE_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "Supernova.hpp"
#include <QFont>
#include <QVariantMap>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QSettings;
class QTimer;
class SupernovaeDialog;

class StelPainter;

/*! @defgroup historicalSupernovae Historical Supernovae Plug-in
@{
The %Historical Supernovae plugin displays the positions some historical
supernovae brighter than 10 visual magnitude.

<b>Supernovae Catalog</b>

The supernovae catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "supernovae.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file.

@}
*/

//! @ingroup historicalSupernovae
typedef QSharedPointer<Supernova> SupernovaP;

//! @class Supernovae
//! Main class of the %Historical Supernovae plugin.
//! @author Alexander Wolf
//! @ingroup historicalSupernovae
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
	virtual void draw(StelCore* core);
	virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for supernovae.
	//! @param limitFov the field of view around the position v in which to search for supernovae.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the supernovae located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching supernova object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensitive localized supernova name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching supernova if exists or Q_NULLPTR.
	//! @param name The case in-sensitive english supernova name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Return the matching supernova if exists, or an "empty" pointer.
	//! @param id The supernova id
	virtual StelObjectP searchByID(const QString &id) const
	{
		return qSharedPointerCast<StelObject>(getByID(id));
	}

	virtual QStringList listAllObjects(bool inEnglish) const;

	virtual QString getName() const { return "Historical Supernovae"; }
	virtual QString getStelObjectType() const { return Supernova::SUPERNOVA_TYPE; }

	//! get a supernova object by identifier
	SupernovaP getByID(const QString& id) const;

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Supernovae section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default supernovae.json file from the resource embedded in the plugin lib/dll file.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);

	//! Get whether or not the plugin will try to update catalog data from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled(void) {return updatesEnabled;}
	//! Set whether or not the plugin will try to update catalog data from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) {updatesEnabled=b;}

	//! Get the date and time the supernovae were updated
	QDateTime getLastUpdate(void) {return lastUpdate;}

	//! Get the update frequency in days
	int getUpdateFrequencyDays(void) {return updateFrequencyDays;}
	void setUpdateFrequencyDays(int days) {updateFrequencyDays = days;}

	//! Get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}

	//! Get list of supernovae
	QString getSupernovaeList() const;

	//! Get lower limit of  brightness for displayed supernovae
	float getLowerLimitBrightness() const;

	//! Get count of supernovae from catalog
	int getCountSupernovae(void) const {return SNCount;}

	//! Get the list of all bright historical supernovae.
	const QList<SupernovaP>& getAllBrightSupernovae() const {return snstar;}

signals:
	//! @param state the new update state.
	void updateStateChanged(Supernovae::UpdateState state);

	//! Emitted after a JSON update has run.
	void jsonUpdateComplete(void);

public slots:
	// TODO: Add functions for scripting support

	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");

	void reloadCatalog(void);

	//! Connect this to StelApp font size.
	void setFontSize(int s){font.setPixelSize(s);}
private:
	// Font used for displaying our text
	QFont font;

	// if existing, delete Satellites section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	//! Replace the JSON file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! Read the JSON file and create list of supernovae.
	void readJsonFile(void);

	//! Creates a backup of the supernovae.json file called supernovae.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version from the "version" value in the supernovas.json file
	//! @return version string, e.g. "1"
	int getJsonFileVersion(void) const;

	//! Check format of the catalog of supernovae
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void) const;

	//! Parse JSON file and load supernovaes to map
	QVariantMap loadSNeMap(QString path=QString());

	//! Set items for list of struct from data map
	void setSNeMap(const QVariantMap& map);

	QString sneJsonPath;

	int SNCount;

	StelTextureSP texPointer;
	QList<SupernovaP> snstar;
	QHash<QString, double> snlist;

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager * networkManager;
	QNetworkReply * downloadReply;
	QString updateUrl;	
	class StelProgressController* progressBar;
	QTimer* updateTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyDays;

	void startDownload(QString url);
	void deleteDownloadProgressBar();

	QSettings* conf;

	// GUI
	SupernovaeDialog* configDialog;	

private slots:
	//! Check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);
	//! Call when button "Save settings" in main GUI are pressed
	void 	saveSettings() { saveSettingsToConfig(); }

	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);
};

#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SupernovaeStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* SUPERNOVAE_HPP */
