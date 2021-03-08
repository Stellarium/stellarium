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

#ifndef EXOPLANETS_HPP
#define EXOPLANETS_HPP

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
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
	Q_ENUMS(TemperatureScale)
	Q_PROPERTY(bool showExoplanets
		   READ getFlagShowExoplanets
		   WRITE setFlagShowExoplanets
		   NOTIFY flagExoplanetsVisibilityChanged
		   )
	Q_PROPERTY(Vec3f markerColor
		   READ getMarkerColor
		   WRITE setMarkerColor
		   NOTIFY markerColorChanged
		   )
	Q_PROPERTY(Vec3f habitableColor
		   READ getHabitableColor
		   WRITE setHabitableColor
		   NOTIFY habitableColorChanged
		   )
public:	
	//! @enum UpdateState
	//! Used for keeping for track of the download/update status
	enum UpdateState {
		Updating,				//!< Update in progress
		CompleteNoUpdates,	//!< Update completed, there we no updates
		CompleteUpdates,		//!< Update completed, there were updates
		DownloadError,			//!< Error during download phase
		OtherError				//!< Other error
	};
	//! @enum TemperatureScale
	//! Available temperature scales
	enum TemperatureScale
	{
		Kelvin		= 0,
		Celsius		= 1,
		Fahrenheit	= 2
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
	// Methods defined in StelObjectModule class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for exoplanets.
	//! @param limitFov the field of view around the position v in which to search for exoplanets.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the exoplanets located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching exoplanet system object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensitive localized exoplanet system name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching exoplanet system if exists or Q_NULLPTR.
	//! @param name The case in-sensitive english exoplanet system name
	virtual StelObjectP searchByName(const QString& name) const;

	//! Return the matching exoplanet system if exists or Q_NULLPTR.
	//! @param id The exoplanet system id
	virtual StelObjectP searchByID(const QString &id) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;

	virtual QStringList listAllObjects(bool inEnglish) const;

	virtual QString getName() const { return "Exoplanets"; }
	virtual QString getStelObjectType() const { return Exoplanet::EXOPLANET_TYPE; }

	//! get a exoplanet object by identifier
	ExoplanetP getByID(const QString& id) const;

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
	bool getUpdatesEnabled(void) const {return updatesEnabled;}
	//! set whether or not the plugin will try to update TLE data from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) {updatesEnabled=b;}

	void setEnableAtStartup(bool b) { enableAtStartup=b; }
	bool getEnableAtStartup(void) const { return enableAtStartup; }

	//! get the date and time the TLE elements were updated
	QDateTime getLastUpdate(void) const {return lastUpdate;}

	//! get the update frequency in hours
	int getUpdateFrequencyHours(void) const {return updateFrequencyHours;}
	void setUpdateFrequencyHours(int hours) {updateFrequencyHours = hours;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! Get the current updateState
	UpdateState getUpdateState(void) const {return updateState;}

	QList<double> getExoplanetsData(int mode) const
	{
		switch(mode)
		{
			case 1:
				return EPSemiAxisAll;
			case 2:
				return EPMassAll;
			case 3:
				return EPRadiusAll;
			case 4:
				return EPPeriodAll;
			case 5:
				return EPAngleDistanceAll;
			case 6:
				return EPEffectiveTempHostStarAll;
			case 7:
				return EPYearDiscoveryAll;
			case 8:
				return EPMetallicityHostStarAll;
			case 9:
				return EPVMagHostStarAll;
			case 10:
				return EPRAHostStarAll;
			case 11:
				return EPDecHostStarAll;
			case 12:
				return EPDistanceHostStarAll;
			case 13:
				return EPMassHostStarAll;
			case 14:
				return EPRadiusHostStarAll;
			default:
				return EPEccentricityAll;
		}
	}

	//! Get the list of all exoplanetary systems.
	const QList<ExoplanetP>& getAllExoplanetarySystems() const {return ep;}

signals:
	//! @param state the new update state.
	void updateStateChanged(Exoplanets::UpdateState state);

	//! emitted after a JSON update has run.
	void jsonUpdateComplete(void);

	void flagExoplanetsVisibilityChanged(bool b);
	void markerColorChanged(Vec3f);
	void habitableColorChanged(Vec3f);

public slots:
	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

	//! Enable/disable display of markers of exoplanetary systems
	//! @param b boolean flag
	void setFlagShowExoplanets(bool b);
	//! Get status to display of markers of exoplanetary systems
	//! @return true if it's visible
	bool getFlagShowExoplanets(void) const { return flagShowExoplanets; }

	//! Enable/disable display of designations of exoplanetary systems
	//! @param b boolean flag
	void setFlagShowExoplanetsDesignations(bool b);
	//! Get status to display of designations of exoplanetary systems
	//! @return true if it's visible
	bool getFlagShowExoplanetsDesignations(void) const;

	//! Define whether the button toggling exoplanets should be visible
	void setFlagShowExoplanetsButton(bool b);
	bool getFlagShowExoplanetsButton(void) const { return flagShowExoplanetsButton; }

	//! Get status to display of distribution of exoplanetary systems
	//! @return true if distribution of exoplanetary systems is enabled
	bool getDisplayMode(void) const;
	//! Enable/disable display of distribution of exoplanetary systems
	//! @param b
	void setDisplayMode(bool b);

	//! Get status to display of systems with exoplanets after their discovery
	//! @return true if markers of exoplanetary systems are visible after discovery of exoplanets
	bool getTimelineMode(void) const;
	//! Enable/disable display of systems with exoplanets after their discovery only
	//! @param b
	void setTimelineMode(bool b);

	//! Get status to display of exoplanetary systems with the potentially habitable exoplanets
	//! @return true if systems with only potentially habitable exoplanets are visible
	bool getHabitableMode(void) const;
	//! Enable/disable display of exoplanetary systems with the potentially habitable exoplanets only
	//! @param b
	void setHabitableMode(bool b);

	//! Get color for markers of exoplanetary systems
	//! @param h set false if you want get color of markers of potentially habitable exoplanets
	//! @return color
	Vec3f getMarkerColor() const;
	//! Set color for markers of exoplanetary systems
	//! @param c color
	//! @param h set true if you want set color for potentially habitable exoplanets
	//! @code
	//! // example of usage in scripts
	//! Exoplanets.setMarkerColor(Vec3f(1.0,0.0,0.0), true);
	//! @endcode
	void setMarkerColor(const Vec3f& c);

	//! Get color for markers of habitable exoplanetary systems
	//! @param h set false if you want get color of markers of potentially habitable exoplanets
	//! @return color
	Vec3f getHabitableColor() const;
	//! Set color for markers of exoplanetary systems
	//! @param c color
	//! @param h set true if you want set color for potentially habitable exoplanets
	//! @code
	//! // example of usage in scripts
	//! Exoplanets.setHabitableColor(Vec3f(1.0,0.0,0.0), true);
	//! @endcode
	void setHabitableColor(const Vec3f& c);

	//! Get count of planetary systems from catalog
	//! @return count of planetary systems
	int getCountPlanetarySystems(void) const
	{
		return PSCount;
	}

	//! Get count of exoplanets from catalog
	//! @return count of all exoplanets
	int getCountAllExoplanets(void) const
	{
		return EPCountAll;
	}

	//! Get count of potentially habitable exoplanets from catalog
	//! @return count of potentially habitable exoplanets
	int getCountHabitableExoplanets(void) const
	{
		return EPCountPH;
	}

	//! Set the temperature scale
	void setCurrentTemperatureScale(TemperatureScale tscale)
	{
		Exoplanet::temperatureScaleID = static_cast<int>(tscale);
	}
	//! Get the current temperature scale
	TemperatureScale getCurrentTemperatureScale() const
	{
		return static_cast<TemperatureScale>(Exoplanet::temperatureScaleID);
	}
	//! Get the key of current temperature scale
	QString getCurrentTemperatureScaleKey(void) const;
	//! Set the temperature scale from its key
	void setCurrentTemperatureScaleKey(QString key);

	//! Connect this to StelApp font size.
	void setFontSize(int s){font.setPixelSize(s);}
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
	bool backupJsonFile(bool deleteOriginal=false) const;

	//! Get the version of catalog format from the "version of the format" value in the exoplanets.json file
	//! @return version string, e.g. "1"
	int getJsonFileFormatVersion(void) const;

	//! Check format of the catalog of exoplanets
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void) const;

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

	int PSCount;	// Count of planetary systems
	int EPCountAll;	// Count of exoplents
	int EPCountPH;	// Count of exoplanets in habitable zone

	// Lists of various data about exoplanets for quick plot of graphs
	QList<double> EPEccentricityAll, EPSemiAxisAll, EPMassAll, EPRadiusAll, EPPeriodAll, EPAngleDistanceAll,
		      EPEffectiveTempHostStarAll, EPYearDiscoveryAll, EPMetallicityHostStarAll, EPVMagHostStarAll,
		      EPRAHostStarAll, EPDecHostStarAll, EPDistanceHostStarAll, EPMassHostStarAll, EPRadiusHostStarAll;

	StelTextureSP texPointer;
	QList<ExoplanetP> ep;

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager * networkManager;
	QNetworkReply * downloadReply;
	QString updateUrl;	
	QTimer* updateTimer;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyHours;	
	bool enableAtStartup;

	void startDownload(QString url);
	void deleteDownloadProgressBar();

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

	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");

	void reloadCatalog(void);
	//! Call when button "Save settings" in main GUI are pressed
	void 	saveSettings() { saveConfiguration(); }
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
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /* EXOPLANETS_HPP */
