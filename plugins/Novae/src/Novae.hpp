/*
 * Copyright (C) 2013 Alexander Wolf
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

#ifndef NOVAE_HPP
#define NOVAE_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelFader.hpp"
#include "Nova.hpp"
#include "StelTextureTypes.hpp"
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
class NovaeDialog;
class StelPainter;

/*! @defgroup brightNovae Bright Novae Plug-in
@{
The %Bright Novae plugin displays the positions some bright
novae in the Milky Way galaxy.

<b>Bright Novae Catalog</b>

The novae catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "novae.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [Novae]).

@}
*/

//! @ingroup brightNovae
typedef QSharedPointer<Nova> NovaP;

//! @class Novae
//! Main class of the %Bright Novae plugin.
//! @author Alexander Wolf
//! @ingroup brightNovae
class Novae : public StelObjectModule
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

	Novae();
	virtual ~Novae();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for novae.
	//! @param limitFov the field of view around the position v in which to search for novae.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the novae located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching Nova object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensitive localized Nova name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching Nova if exists or Q_NULLPTR.
	//! @param name The case in-sensitive english Nova name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Return the matching Nova if exists or Q_NULLPTR.
	//! @param id The Nova id
	virtual StelObjectP searchByID(const QString &id) const
	{
		return qSharedPointerCast<StelObject>(getByID(id));
	}

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;
	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Bright Novae"; }
	virtual QString getStelObjectType() const { return Nova::NOVA_TYPE; }

	//! get a nova object by identifier
	NovaP getByID(const QString& id) const;

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Novae section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default novae.json file from the resource embedded in the plugin lib/dll file.
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

	//! Get the date and time the novae were updated
	QDateTime getLastUpdate(void) const {return lastUpdate;}

	//! Get the update frequency in days
	int getUpdateFrequencyDays(void) const {return updateFrequencyDays;}
	void setUpdateFrequencyDays(int days) {updateFrequencyDays = days;}

	//! Get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) const {return updateState;}

	//! Get list of novae
	QString getNovaeList();

	//! Get lower limit of  brightness for displayed novae
	float getLowerLimitBrightness();

	//! Get count of novae from catalog
	int getCountNovae(void) const {return NovaCnt;}

	//! Get the list of all bright novae.
	const QList<NovaP>& getAllBrightNovae() const {return nova;}

signals:
	//! @param state the new update state.
	void updateStateChanged(Novae::UpdateState state);

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

	// if existing, delete Novae section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	//! Replace the JSON file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! Read the JSON file and create list of novae.
	void readJsonFile(void);

	//! Creates a backup of the novae.json file called novae.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version from the "version" value in the novae.json file
	//! @return version string, e.g. "1"
	int getJsonFileVersion(void) const;

	//! Check format of the catalog of novae
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void) const;

	//! Parse JSON file and load novae to map
	QVariantMap loadNovaeMap(QString path=QString());

	//! Set items for list of struct from data map
	void setNovaeMap(const QVariantMap& map);

	QString novaeJsonPath;

	int NovaCnt;

	StelTextureSP texPointer;
	QList<NovaP> nova;
	QHash<QString, double> novalist;

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
	NovaeDialog* configDialog;	

private slots:
	//! Check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);

	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);

	//! Call when button "Save settings" in main GUI are pressed
	void saveSettings() { saveSettingsToConfig(); }
};

#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class NovaeStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*NOVAE_HPP*/
