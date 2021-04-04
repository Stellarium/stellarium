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

#ifndef QUASARS_HPP
#define QUASARS_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "Quasar.hpp"
#include <QFont>
#include <QVariantMap>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class StelPainter;

class QSettings;
class QTimer;
class QPixmap;
class StelButton;
class QuasarsDialog;

/*! @defgroup quasars Quasars Plug-in
@{
The %Quasars plugin provides visualization of some quasars brighter than 16
visual magnitude. A catalogue of quasars compiled from "Quasars and Active
Galactic Nuclei" (13th Ed.) (Veron+ 2010).

<b>Quasars Catalog</b>

The quasars catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "quasars.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [Quasars]).

@}
*/

//! @ingroup quasars
typedef QSharedPointer<Quasar> QuasarP;

//! @class Quasars
//! Main class of the %Quasars plugin.
//! @author Alexander Wolf
//! @ingroup quasars
class Quasars : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool quasarsVisible
		   READ getFlagShowQuasars
		   WRITE setFlagShowQuasars
		   NOTIFY flagQuasarsVisibilityChanged
		   )
	Q_PROPERTY(Vec3f quasarsColor
		   READ getMarkerColor
		   WRITE setMarkerColor
		   NOTIFY quasarsColorChanged)
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

	Quasars();
	virtual ~Quasars();

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
	//! @param v a vector representing the position in th sky around which to search for quasars.
	//! @param limitFov the field of view around the position v in which to search for quasars.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the quasars located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching Quasar object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensitive localized quasar name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching Quasar if exists or Q_NULLPTR.
	//! @param name The case in-sensitive english quasar name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Return the matching Quasar if exists or Q_NULLPTR.
	//! @param id The quasar id
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

	virtual QString getName() const { return "Quasars"; }

	virtual QString getStelObjectType() const { return Quasar::QUASAR_TYPE; }

	//! get a Quasar object by identifier
	QuasarP getByID(const QString& id) const;

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Quasars section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default quasars.json file from the resource embedded in the plugin lib/dll file.
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

	void setEnableAtStartup(bool b) { enableAtStartup=b; }
	bool getEnableAtStartup(void) { return enableAtStartup; }

	//! get the date and time the TLE elements were updated
	QDateTime getLastUpdate(void) {return lastUpdate;}

	//! get the update frequency in days
	int getUpdateFrequencyDays(void) {return updateFrequencyDays;}
	void setUpdateFrequencyDays(int days) {updateFrequencyDays = days;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}

	//! Get the list of all quasars.
	const QList<QuasarP>& getAllQuasars() const {return QSO;}

signals:
	//! @param state the new update state.
	void updateStateChanged(Quasars::UpdateState state);

	//! emitted after a JSON update has run.
	void jsonUpdateComplete(void);

	void flagQuasarsVisibilityChanged(bool b);
	void quasarsColorChanged(Vec3f);

public slots:
	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

	//! Enable/disable display of markers of quasars
	//! @param b boolean flag
	void setFlagShowQuasars(bool b);
	//! Get status to display of markers of quasars
	//! @return true if it's visible
	bool getFlagShowQuasars(void) { return flagShowQuasars; }

	//! Enable/disable usage of markers for quasars
	//! @param b boolean flag
	void setFlagUseQuasarMarkers(bool b);
	//! Get status usage of markers for quasars
	//! @return true if markers are used
	bool getFlagUseQuasarMarkers(void);

	//! Define whether the button toggling quasars should be visible
	void setFlagShowQuasarsButton(bool b);
	bool getFlagShowQuasarsButton(void) { return flagShowQuasarsButton; }

	//! Get count of quasars from catalog
	//! @return count of quasars
	int getCountQuasars(void) {return QsrCount;}

	//! Get status to display of distribution of quasars
	//! @return true if distribution of quasars is enabled
	bool getDisplayMode(void);
	//! Enable/disable display of distribution of quasars
	//! @param b (set true for display quasars as markers)
	void setDisplayMode(bool b);

	//! Get color for quasars markers
	//! @return color
	Vec3f getMarkerColor(void);
	//! Set color for quasars markers
	//! @param c color
	//! @code
	//! // example of usage in scripts
	//! Quasars.setMarkerColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setMarkerColor(const Vec3f& c);

	//! Connect this to StelApp font size.
	void setFontSize(int s){font.setPixelSize(s);}
private:
	// Font used for displaying our text
	QFont font;

	// if existing, delete Satellites section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	// Upgrade config.ini: rename old key settings to new
	void upgradeConfigIni(void);

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file and create list of quasars.
	void readJsonFile(void);

	//! Creates a backup of the catalog.json file called catalog.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version from the "version of the format" value in the catalog.json file
	//! @return version string, e.g. "1"
	int getJsonFileFormatVersion(void);

	//! Check format of the catalog of quasars
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void);

	//! parse JSON file and load quasars to map
	QVariantMap loadQSOMap(QString path=QString());

	//! set items for list of struct from data map
	void setQSOMap(const QVariantMap& map);

	QString catalogJsonPath;

	int QsrCount;

	StelTextureSP texPointer;
	QList<QuasarP> QSO;

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager * networkManager;
	QNetworkReply * downloadReply;
	QString updateUrl;	
	QTimer* updateTimer;
	QTimer* messageTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyDays;	
	bool enableAtStartup;

	void startDownload(QString url);
	void deleteDownloadProgressBar();

	QSettings* conf;

	// GUI
	QuasarsDialog* configDialog;
	bool flagShowQuasars;
	bool flagShowQuasarsButton;
	QPixmap* OnIcon;
	QPixmap* OffIcon;
	QPixmap* GlowIcon;
	StelButton* toolbarButton;
	class StelProgressController* progressBar;

private slots:
	//! check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);

	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");

	void reloadCatalog(void);
	//! Call when button "Save settings" in main GUI are pressed
	void 	saveSettings() { saveSettingsToConfig(); }
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class QuasarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* QUASARS_HPP */
