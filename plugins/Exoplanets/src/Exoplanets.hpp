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

#ifndef _EXOPLANETS_HPP_
#define _EXOPLANETS_HPP_

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "Exoplanet.hpp"
#include <QFont>
#include <QVariantMap>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>

class QNetworkAccessManager;
class QNetworkReply;
class QSettings;
class QTimer;
class ExoplanetsDialog;
class StelPainter;
class StelButton;

/*! @defgroup exoplanets Exoplanets Plug-in
@{
The %Exoplanets plugin plots the position of stars with exoplanets. Exoplanets
data is derived from [The Extrasolar Planets Encyclopaedia](http://exoplanet.eu/).
List of potential habitable exoplanets and data about them were taken from
[The Habitable Exoplanets Catalog](http://phl.upr.edu/projects/habitable-exoplanets-catalog)
by [Planetary Habitability Laboratory](http://phl.upr.edu/home).

<b>Exoplanets Catalog</b>

The exoplanets catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "exoplanets.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [Exoplanets]).

@}
*/

//! @ingroup exoplanets
typedef QSharedPointer<Exoplanet> ExoplanetP;

//! @class Exoplanets
//! Main class of the %Exoplanets plugin.
//! @author Alexander Wolf
//! @ingroup exoplanets
class Exoplanets : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool showExoplanets READ getFlagShowExoplanets WRITE setFlagShowExoplanets)
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
	
	Exoplanets();
	virtual ~Exoplanets();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for exoplanets.
	//! @param core the StelCore to use for computations.
	//! @return an list containing the exoplanets located inside the limitFov circle around position v.
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
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object English name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;

	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const { Q_UNUSED(objType) Q_UNUSED(inEnglish) return QStringList(); }

	virtual QString getName() const { return "Exoplanets"; }

	//! get a exoplanet object by identifier
	ExoplanetP getByID(const QString& id);

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Exoplanets section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default exoplanets.json file from the resource embedded in the plugin lib/dll file.
	void restoreDefaults(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void loadConfiguration(void);

	//! Save the settings to the main configuration file.
	void saveConfiguration(void);

	//! get whether or not the plugin will try to update TLE data from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled(void) {return updatesEnabled;}
	//! set whether or not the plugin will try to update TLE data from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) {updatesEnabled=b;}

	bool getDisplayMode(void);
	void setDisplayMode(bool b);

	bool getTimelineMode(void);
	void setTimelineMode(bool b);

	bool getHabitableMode(void);
	void setHabitableMode(bool b);

	QString getMarkerColor(bool habitable);
	void setMarkerColor(QString c, bool h);

	void setEnableAtStartup(bool b) { enableAtStartup=b; }
	bool getEnableAtStartup(void) { return enableAtStartup; }

	//! get the date and time the TLE elements were updated
	QDateTime getLastUpdate(void) {return lastUpdate;}

	//! get the update frequency in hours
	int getUpdateFrequencyHours(void) {return updateFrequencyHours;}
	void setUpdateFrequencyHours(int hours) {updateFrequencyHours = hours;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}

	//! Get count of planetary systems from catalog
	int getCountPlanetarySystems(void) const
	{
		return PSCount;
	}

	//! Get count of exoplanets from catalog
	int getCountAllExoplanets(void) const
	{
		return EPCountAll;
	}

	//! Get count of potentially habitable exoplanets from catalog
	int getCountHabitableExoplanets(void) const
	{
		return EPCountPH;
	}

	QList<double> getExoplanetsData(int mode)
	{
		switch(mode)
		{
			case 1:
				return EPSemiAxisAll;
				break;
			case 2:
				return EPMassAll;
				break;
			case 3:
				return EPRadiusAll;
				break;
			case 4:
				return EPPeriodAll;
				break;
			case 5:
				return EPAngleDistanceAll;
				break;
			default:
				return EPEccentricityAll;
		}
	}

signals:
	//! @param state the new update state.
	void updateStateChanged(Exoplanets::UpdateState state);

	//! emitted after a JSON update has run.
	void jsonUpdateComplete(void);

public slots:
	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

	void setFlagShowExoplanets(bool b) { flagShowExoplanets=b; }
	bool getFlagShowExoplanets(void) { return flagShowExoplanets; }

	//! Define whether the button toggling exoplanets should be visible
	void setFlagShowExoplanetsButton(bool b);
	bool getFlagShowExoplanetsButton(void) { return flagShowExoplanetsButton; }

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");
	void messageTimeout(void);

private:
	// Font used for displaying our text
	QFont font;

	// if existing, delete Satellites section in main config.ini, then create with default values
	void resetConfiguration(void);

	// Upgrade config.ini: rename old key settings to new
	void upgradeConfigIni(void);

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file and create list of exoplanets.
	void readJsonFile(void);

	//! Creates a backup of the exoplanets.json file called exoplanets.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version of catalog format from the "version of the format" value in the exoplanets.json file
	//! @return version string, e.g. "1"
	int getJsonFileFormatVersion(void);

	//! Check format of the catalog of exoplanets
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void);

	//! parse JSON file and load exoplanets to map
	QVariantMap loadEPMap(QString path=QString());

	//! set items for list of struct from data map
	void setEPMap(const QVariantMap& map);

	//! A fake method for strings marked for translation.
	//! Use it instead of translations.h for N_() strings, except perhaps for
	//! keyboard action descriptions. (It's better for them to be in a single
	//! place.)
	static void translations();

	QString jsonCatalogPath;

	int PSCount;
	int EPCountAll;
	int EPCountPH;

	QList<double> EPEccentricityAll, EPSemiAxisAll, EPMassAll, EPRadiusAll, EPPeriodAll, EPAngleDistanceAll;

	StelTextureSP texPointer;
	QList<ExoplanetP> ep;

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager* downloadMgr;
	QString updateUrl;	
	QTimer* updateTimer;
	QTimer* messageTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyHours;	
	bool enableAtStartup;

	QSettings* conf;

	// GUI
	ExoplanetsDialog* exoplanetsConfigDialog;
	bool flagShowExoplanets;
	bool flagShowExoplanetsButton;
	StelButton* toolbarButton;
	class StelProgressController* progressBar;

private slots:
	//! check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);
	void updateDownloadComplete(QNetworkReply* reply);

};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class ExoplanetsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_EXOPLANETS_HPP_*/
