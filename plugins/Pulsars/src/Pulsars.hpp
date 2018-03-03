/*
 * Copyright (C) 2012 Alexander Wolf
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

#ifndef _PULSARS_HPP_
#define _PULSARS_HPP_

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "Pulsar.hpp"
#include <QFont>
#include <QVariantMap>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>

class QNetworkAccessManager;
class QNetworkReply;
class QSettings;
class QTimer;
class QPixmap;
class StelButton;
class PulsarsDialog;

class StelPainter;

/*! @defgroup pulsars Pulsars Plug-in
@{
The %Pulsars plugin plots the position of various pulsars, with object information
about each one. Pulsar data is derived from Catalog of Pulsars
([Taylor+ 1995](http://cdsads.u-strasbg.fr/cgi-bin/nph-bib_query?1993ApJS...88..529T&db_key=AST&nosetcookie=1))
for 0.1.x series and derived from The ATNF Pulsar Catalogue (Manchester, R. N.,
Hobbs, G. B., Teoh, A. & Hobbs, M., Astron. J., 129, 1993-2006 (2005)
([astro-ph/0412641](http://arxiv.org/abs/astro-ph/0412641))) for series 0.2.x.

<b>Pulsars Catalog</b>

The pulsars catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "pulsars.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [Pulsars]).

@}
*/

//! @ingroup pulsars
typedef QSharedPointer<Pulsar> PulsarP;

//! @class Pulsars
//! Main class of the %Pulsars plugin.
//! @author Alexander Wolf
//! @ingroup pulsars
class Pulsars : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool pulsarsVisible
		   READ getFlagShowPulsars
		   WRITE setFlagShowPulsars
		   NOTIFY flagPulsarsVisibilityChanged
		   )
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

	Pulsars();
	virtual ~Pulsars();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for satellites.
	//! @param core the StelCore to use for computations.
	//! @return an list containing the satellites located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching satellite object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensistive satellite name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching satellite if exists or Q_NULLPTR.
	//! @param name The case in-sensistive standard program name
	virtual StelObjectP searchByName(const QString& name) const;

	virtual StelObjectP searchByID(const QString &id) const
	{
		return qSharedPointerCast<StelObject>(getByID(id));
	}

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false, bool inEnglish=false) const;
	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Pulsars"; }
	virtual QString getStelObjectType() const { return Pulsar::PULSAR_TYPE; }

	//! get a Pulsar object by identifier
	PulsarP getByID(const QString& id) const;

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Pulsars section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default pulsars.json file from the resource embedded in the plugin lib/dll file.
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

	//! get the date and time the pulsars were updated
	QDateTime getLastUpdate(void) {return lastUpdate;}

	//! get the update frequency in days
	int getUpdateFrequencyDays(void) {return updateFrequencyDays;}
	void setUpdateFrequencyDays(int days) {updateFrequencyDays = days;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}

signals:
	//! @param state the new update state.
	void updateStateChanged(Pulsars::UpdateState state);

	//! emitted after a JSON update has run.
	void jsonUpdateComplete(void);

	void flagPulsarsVisibilityChanged(bool b);

public slots:
	//! Define whether the button toggling pulsars should be visible
	void setFlagShowPulsarsButton(bool b);
	bool getFlagShowPulsarsButton(void) { return flagShowPulsarsButton; }

	//! Enable/disable display of markers of pulsars
	//! @param b boolean flag
	void setFlagShowPulsars(bool b);
	//! Get status to display of markers of pulsars
	//! @return true if it's visible
	bool getFlagShowPulsars(void) { return flagShowPulsars; }

	//! Get status to display of distribution of pulsars
	//! @return true if distribution of pulsars is enabled
	bool getDisplayMode(void);
	//! Enable/disable display of distribution of pulsars
	//! @param b
	void setDisplayMode(bool b);

	//! Get status for usage of separate color for pulsars with glitches
	//! @return true if separate color is used for pulsars with glitches
	bool getGlitchFlag(void);
	//! Enable/disable the use of a separate color for pulsars with glitches
	//! @param boolean flag
	void setGlitchFlag(bool b);

	//! Get color for pulsars markers
	//! @param mtype set false if you want get color of pulsars with glitches
	//! @return color
	Vec3f getMarkerColor(bool mtype = true);
	//! Set color for pulsars markers
	//! @param c color
	//! @param mtype set false if you want set color for pulsars with glitches
	//! @code
	//! // example of usage in scripts
	//! Pulsars.setMarkerColor(Vec3f(1.0,0.0,0.0), true);
	//! @endcode
	void setMarkerColor(const Vec3f& c, bool mtype = true);

	//! Get count of pulsars from catalog
	//! @return count of pulsars
	int getCountPulsars(void) {return PsrCount;}

	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

private:
	// Font used for displaying our text
	QFont font;

	// if existing, delete Satellites section in main config.ini, then create with default values
	void restoreDefaultConfigIni(void);

	// Upgrade config.ini: rename old key settings to new
	void upgradeConfigIni(void);

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file and create list of Pulsars.
	void readJsonFile(void);

	//! Creates a backup of the pulsars.json file called pulsars.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version from the "version of the format" value in the pulsars.json file
	//! @return version string, e.g. "2"
	int getJsonFileFormatVersion(void);

	//! Check format of the catalog of pulsars
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void);

	//! parse JSON file and load pulsars to map
	QVariantMap loadPSRMap(QString path=QString());

	//! set items for list of struct from data map
	void setPSRMap(const QVariantMap& map);

	QString jsonCatalogPath;

	StelTextureSP texPointer;
	QList<PulsarP> psr;

	int PsrCount;

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager* downloadMgr;
	QString updateUrl;	
	QTimer* updateTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyDays;	
	bool enableAtStartup;

	QSettings* conf;

	// GUI
	PulsarsDialog* configDialog;
	bool flagShowPulsars;
	bool flagShowPulsarsButton;
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
	void updateDownloadComplete(QNetworkReply* reply);

	void reloadCatalog(void);

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class PulsarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*_PULSARS_HPP_*/
