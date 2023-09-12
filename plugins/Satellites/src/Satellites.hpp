/*
 * Copyright (C) 2009, 2012 Matthew Gates
 * Copyright (C) 2015 Nick Fedoseev (Iridium flares)
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

#ifndef SATELLITES_HPP
#define SATELLITES_HPP

#include "StelObjectModule.hpp"
#include "Satellite.hpp"
#include "StelFader.hpp"
#include "StelLocation.hpp"

#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QUrl>
#include <QVariantMap>

class StelButton;
class Planet;
class QNetworkAccessManager;
class QNetworkReply;
class QSettings;
class QTimer;

class SatellitesDialog;
class SatellitesListModel;

/*! @defgroup satellites Satellites Plug-in
@{
The %Satellites plugin displays the positions of artificial satellites in Earth
orbit based on a catalog of orbital data.

The Satellites class is the main class of the plug-in. It manages a collection
of Satellite objects and takes care of loading, saving and updating the
satellite catalog. It allows automatic updates from online sources and manages
a list of update file URLs.

To calculate satellite positions, the plugin uses an implementation of
the SGP4/SDP4 algorithms (J.L. Canales' gsat library).

<b>Satellite Properties</b>

<i>Name and identifiers</i>

Each satellite has a name. It's displayed as a label of the satellite hint and in the list of satellites. Names are not unique though, so they are used only
for presentation purposes.

In the <b>Satellite Catalog</b> satellites are uniquely identified by their NORAD number, which is encoded in TLEs.

<i>Grouping</i>

A satellite can belong to one or more groups such as "amateur", "geostationary" or "navigation". They have no other function but to help the user organize the satellite collection.

Group names are arbitrary strings defined in the <b>Satellite Catalog</b> for each satellite and are more similar to the concept of "tags" than a hierarchical grouping. A satellite may not belong to any group at all.

By convention, group names are in lowercase. The GUI translates some of the groups used in the default catalog.

<b>Satellite Catalog</b>

The satellite catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "satellites.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file.

@}
*/

//! Data structure containing unvalidated TLE set as read from a TLE list file.
//! @ingroup satellites
struct TleData
{
	//! NORAD catalog number, as extracted from the TLE set.
	QString id;
	//! Human readable name, as extracted from the TLE title line.
	QString name;
	QString first;
	QString second;
	int status;
	//! Flag indicating whether this satellite should be added.
	//! See Satellites::autoAddEnabled.
	bool addThis;
	//! Source of TLE (URL), can be used for assign satellites group
	QString sourceURL;
};

//! @ingroup satellites
typedef QList<TleData> TleDataList;
//! @ingroup satellites
typedef QHash<QString, TleData> TleDataHash ;

//! TLE update source, used only internally for now.
//! @ingroup satellites
struct TleSource
{
	//! URL from where the source list should be downloaded.
	QUrl url;
	//! The downloaded file, location set after finishing download.
	//! In the future may be a QTemporaryFile object.
	QFile* file;
	//! Flag indicating whether new satellites in this list should be added.
	//! See Satellites::autoAddEnabled.
	bool addNew;
};

//! @ingroup satellites
typedef QList<TleSource> TleSourceList;

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
struct IridiumFlaresPrediction
{
	QString datetime;
	QString satellite;
	float azimuth;		// radians
	float altitude;		// radians
	float magnitude;
};

typedef QList<IridiumFlaresPrediction> IridiumFlaresPredictionList;
#endif

//! @class Satellites
//! Main class of the %Satellites plugin.
//! @author Matthew Gates
//! @author Bogdan Marinov
//! @ingroup satellites
class Satellites : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagHintsVisible		READ getFlagHintsVisible		WRITE setFlagHintsVisible		NOTIFY flagHintsVisibleChanged)
	Q_PROPERTY(bool flagLabelsVisible		READ getFlagLabelsVisible		WRITE setFlagLabelsVisible		NOTIFY flagLabelsVisibleChanged)
	Q_PROPERTY(int  labelFontSize			READ getLabelFontSize			WRITE setLabelFontSize			NOTIFY labelFontSizeChanged)
	Q_PROPERTY(bool autoAddEnabled			READ isAutoAddEnabled			WRITE setAutoAddEnabled			NOTIFY autoAddEnabledChanged)
	Q_PROPERTY(bool autoRemoveEnabled		READ isAutoRemoveEnabled		WRITE setAutoRemoveEnabled		NOTIFY autoRemoveEnabledChanged)
	Q_PROPERTY(bool flagIconicMode			READ getFlagIconicMode			WRITE setFlagIconicMode			NOTIFY flagIconicModeChanged)
	Q_PROPERTY(bool flagHideInvisible		READ getFlagHideInvisible		WRITE setFlagHideInvisible		NOTIFY flagHideInvisibleChanged)
	Q_PROPERTY(bool flagColoredInvisible		READ getFlagColoredInvisible		WRITE setFlagColoredInvisible		NOTIFY flagColoredInvisibleChanged)
	Q_PROPERTY(bool flagOrbitLines			READ getFlagOrbitLines			WRITE setFlagOrbitLines			NOTIFY flagOrbitLinesChanged)
	Q_PROPERTY(bool updatesEnabled			READ getUpdatesEnabled			WRITE setUpdatesEnabled			NOTIFY updatesEnabledChanged)
	Q_PROPERTY(int  updateFrequencyHours		READ getUpdateFrequencyHours		WRITE setUpdateFrequencyHours		NOTIFY updateFrequencyHoursChanged)
	Q_PROPERTY(int  orbitLineSegments		READ getOrbitLineSegments		WRITE setOrbitLineSegments		NOTIFY orbitLineSegmentsChanged)
	Q_PROPERTY(int  orbitLineFadeSegments		READ getOrbitLineFadeSegments		WRITE setOrbitLineFadeSegments		NOTIFY orbitLineFadeSegmentsChanged)
	Q_PROPERTY(int  orbitLineSegmentDuration	READ getOrbitLineSegmentDuration	WRITE setOrbitLineSegmentDuration	NOTIFY orbitLineSegmentDurationChanged)
	Q_PROPERTY(int  orbitLineThickness		READ getOrbitLineThickness		WRITE setOrbitLineThickness		NOTIFY orbitLineThicknessChanged)
	Q_PROPERTY(int  tleEpochAgeDays			READ getTleEpochAgeDays			WRITE setTleEpochAgeDays		NOTIFY tleEpochAgeDaysChanged)
	Q_PROPERTY(Vec3f invisibleSatelliteColor	READ getInvisibleSatelliteColor		WRITE setInvisibleSatelliteColor	NOTIFY invisibleSatelliteColorChanged)
	Q_PROPERTY(Vec3f transitSatelliteColor		READ getTransitSatelliteColor		WRITE setTransitSatelliteColor		NOTIFY transitSatelliteColorChanged)
	Q_PROPERTY(bool flagUmbraVisible		READ getFlagUmbraVisible		WRITE setFlagUmbraVisible		NOTIFY flagUmbraVisibleChanged)
	Q_PROPERTY(bool flagUmbraAtFixedDistance	READ getFlagUmbraAtFixedDistance	WRITE setFlagUmbraAtFixedDistance	NOTIFY flagUmbraAtFixedDistanceChanged)
	Q_PROPERTY(double umbraDistance			READ getUmbraDistance			WRITE setUmbraDistance			NOTIFY umbraDistanceChanged)
	Q_PROPERTY(Vec3f umbraColor			READ getUmbraColor			WRITE setUmbraColor			NOTIFY umbraColorChanged)
	Q_PROPERTY(bool flagPenumbraVisible		READ getFlagPenumbraVisible		WRITE setFlagPenumbraVisible		NOTIFY flagPenumbraVisibleChanged)
	Q_PROPERTY(Vec3f penumbraColor			READ getPenumbraColor			WRITE setPenumbraColor			NOTIFY penumbraColorChanged)
	// custom filter
	Q_PROPERTY(bool flagCFKnownStdMagnitude		READ getFlagCFKnownStdMagnitude		WRITE setFlagCFKnownStdMagnitude	NOTIFY flagCFKnownStdMagnitudeChanged)
	Q_PROPERTY(bool flagCFApogee			READ getFlagCFApogee			WRITE setFlagCFApogee			NOTIFY flagCFApogeeChanged)
	Q_PROPERTY(double minCFApogee			READ getMinCFApogee			WRITE setMinCFApogee			NOTIFY minCFApogeeChanged)
	Q_PROPERTY(double maxCFApogee			READ getMaxCFApogee			WRITE setMaxCFApogee			NOTIFY maxCFApogeeChanged)
	Q_PROPERTY(bool flagCFPerigee			READ getFlagCFPerigee			WRITE setFlagCFPerigee			NOTIFY flagCFPerigeeChanged)
	Q_PROPERTY(double minCFPerigee			READ getMinCFPerigee			WRITE setMinCFPerigee			NOTIFY minCFPerigeeChanged)
	Q_PROPERTY(double maxCFPerigee			READ getMaxCFPerigee			WRITE setMaxCFPerigee			NOTIFY maxCFPerigeeChanged)
	Q_PROPERTY(bool flagCFEccentricity		READ getFlagCFEccentricity		WRITE setFlagCFEccentricity		NOTIFY flagCFEccentricityChanged)
	Q_PROPERTY(double minCFEccentricity		READ getMinCFEccentricity		WRITE setMinCFEccentricity		NOTIFY minCFEccentricityChanged)
	Q_PROPERTY(double maxCFEccentricity		READ getMaxCFEccentricity		WRITE setMaxCFEccentricity		NOTIFY maxCFEccentricityChanged)
	Q_PROPERTY(bool flagCFPeriod			READ getFlagCFPeriod			WRITE setFlagCFPeriod			NOTIFY flagCFPeriodChanged)
	Q_PROPERTY(double minCFPeriod			READ getMinCFPeriod			WRITE setMinCFPeriod			NOTIFY minCFPeriodChanged)
	Q_PROPERTY(double maxCFPeriod			READ getMaxCFPeriod			WRITE setMaxCFPeriod			NOTIFY maxCFPeriodChanged)
	Q_PROPERTY(bool flagCFInclination		READ getFlagCFInclination		WRITE setFlagCFInclination		NOTIFY flagCFInclinationChanged)
	Q_PROPERTY(double minCFInclination		READ getMinCFInclination		WRITE setMinCFInclination		NOTIFY minCFInclinationChanged)
	Q_PROPERTY(double maxCFInclination		READ getMaxCFInclination		WRITE setMaxCFInclination		NOTIFY maxCFInclinationChanged)
	Q_PROPERTY(bool flagCFRCS			READ getFlagCFRCS			WRITE setFlagCFRCS			NOTIFY flagCFRCSChanged)
	Q_PROPERTY(double minCFRCS			READ getMinCFRCS			WRITE setMinCFRCS			NOTIFY minCFRCSChanged)
	Q_PROPERTY(double maxCFRCS			READ getMaxCFRCS			WRITE setMaxCFRCS			NOTIFY maxCFRCSChanged)
	// visual filter
	Q_PROPERTY(bool flagVFAltitude			READ getFlagVFAltitude			WRITE setFlagVFAltitude			NOTIFY flagVFAltitudeChanged)
	Q_PROPERTY(double minVFAltitude			READ getMinVFAltitude			WRITE setMinVFAltitude			NOTIFY minVFAltitudeChanged)
	Q_PROPERTY(double maxVFAltitude			READ getMaxVFAltitude			WRITE setMaxVFAltitude			NOTIFY maxVFAltitudeChanged)
	Q_PROPERTY(bool flagVFMagnitude			READ getFlagVFMagnitude			WRITE setFlagVFMagnitude		NOTIFY flagVFMagnitudeChanged)
	Q_PROPERTY(double minVFMagnitude		READ getMinVFMagnitude			WRITE setMinVFMagnitude			NOTIFY minVFMagnitudeChanged)
	Q_PROPERTY(double maxVFMagnitude		READ getMaxVFMagnitude			WRITE setMaxVFMagnitude			NOTIFY maxVFMagnitudeChanged)


public:
	//! @enum UpdateState
	//! Used for keeping track of the download/update status
	enum UpdateState
	{
		Updating,		//!< Update in progress
		CompleteNoUpdates,	//!< Update completed, there we no updates
		CompleteUpdates,	//!< Update completed, there were updates
		DownloadError,		//!< Error during download phase
		OtherError		//!< Other error
	};

	//! Flags used to filter the satellites list according to their status.
	enum Status
	{
		Visible,
		NotVisible,
		Both,
		NewlyAdded,
		OrbitError
	};

	Satellites();
	~Satellites() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	void deinit() override;
	void update(double deltaTime) override;
	void draw(StelCore* core) override;
	void drawPointer(StelCore* core, StelPainter& painter);
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for satellites.
	//! @param limitFov the field of view around the position v in which to search for satellites.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the satellites located inside the limitFov circle around position v.
	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;

	//! Return the matching satellite object's pointer if exists or nullptr.
	//! @param nameI18n The case in-sensitive satellite name
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;

	//! Return the matching satellite if exists or nullptr.
	//! @param name The case in-sensitive standard program name
	StelObjectP searchByName(const QString& name) const override;

	//! Return the matching satellite if exists or nullptr.
	//! @param id The satellite id (NORAD)
	StelObjectP searchByID(const QString &id) const override;
	
	//! Return the satellite with the given catalog number.
	//! Used as a helper function by searchByName() and
	//! searchByNameI18n().
	//! @param noradNumber search string in the format "NORAD XXXX".
	//! @returns a null pointer if no such satellite is found.
	StelObjectP searchByNoradNumber(const QString& noradNumber) const;

	//! Return the satellite with the given International Designator.
	//! Used as a helper function by searchByName() and
	//! searchByNameI18n().
	//! @param intlDesignator search string in the format "XXXX-XXXX".
	//! @returns a null pointer if no such satellite is found.
	StelObjectP searchByInternationalDesignator(const QString& intlDesignator) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const override;

	QStringList listAllObjects(bool inEnglish) const override;

	QString getName() const override { return "Satellites"; }
	QString getStelObjectType() const override { return Satellite::SATELLITE_TYPE; }

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin. 
	bool configureGui(bool show=true) override;

	//! Set up the plugin with default values.  This means clearing out the Satellites section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also 
	//! creates the default satellites.json file from the resource embedded in the plugin lib/dll file.
	void restoreDefaults(void);
	//! Delete TLE sources in main config.ini, then create with default values.
	void restoreDefaultTleSources();

	//! Read (or re-read) the plugin's settings from the configuration file.
	//! This will be called from init() and also when restoring defaults
	//! (i.e. from the configuration dialog / restore defaults button).
	void loadSettings();

	//! Save the plugin's settings to the main configuration file.
	void saveSettingsToConfig();

	//! Get the groups used in the currently loaded satellite collection.
	//! See @ref groups for details. Use getGroupIdList() if you need a list.
	QSet<QString> getGroups() const;
	//! Get a sorted list of group names.
	//! See @ref groups for details. Use getGroups() if you don't need a list.
	QStringList getGroupIdList() const;
	//! Add this group to the global list.
	void addGroup(const QString& groupId);

	//! get satellite objects filtered by group.  If an empty string is used for the
	//! group name, return all satallites
	QHash<QString,QString> getSatellites(const QString& group=QString(), Status vis=Both) const;
	//! Get a model representing the list of satellites.
	SatellitesListModel* getSatellitesListModel();

	//! Get a satellite object by its identifier (i.e. NORAD number).
	SatelliteP getById(const QString& id) const;
	
	//! Returns a list of all satellite IDs.
	QStringList listAllIds() const;
	
	//! Add to the current collection the satellites described by the data list.
	//! The changes are not saved to file.
	//! Calls add(TleData).
	void add(const TleDataList& newSatellites);
	
	//! Remove the selected satellites.
	//! The changes are not saved to file.
	void remove(const QStringList& idList);

	//! get the date and time the TLE elements were updated
	QDateTime getLastUpdate(void) const {return lastUpdate;}

	//! get the update frequency in hours
	int getUpdateFrequencyHours(void) const {return updateFrequencyHours;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! get the update frequency in hours
	//void setUpdateFrequencyHours(int hours);

	//! Get the current updateState
	UpdateState getUpdateState(void) const {return updateState;}

	//! Get a list of URLs which are sources of TLE data.
	//! @returns a list of URL strings, some with prefixes - see #updateUrls
	//! for details.
	QStringList getTleSources(void) const {return updateUrls;}

	//! Set the list of URLs which are sources of TLE data.
	//! In addition to replacing the current list of sources, it also
	//! saves them to the configuration file. Allows marking sources for
	//! auto-addition by adding a prefix to the URL string.
	//! @see updateUrls
	//! @param tleSources a list of valid URLs (http://, ftp://, file://),
	//! allowed prefixes are "0,", "1," or no prefix.
	void setTleSources(QStringList tleSources);
	
	//! Saves the current list of update URLs to the configuration file.
	void saveTleSources(const QStringList& urls);
	
	//! Reads update file(s) in celestrak's .txt format, and updates
	//! the TLE elements for existing satellites from them.
	//! Indirectly emits signals updateStateChanged() and tleUpdateComplete(),
	//! as it calls updateSatellites().
	//! See updateFromOnlineSources() for the other kind of update operation.
	//! @param paths a list of paths to update files
	//! @param deleteFiles if set, the update files are deleted after
	//!        they are used, else they are left alone
	void updateFromFiles(QStringList paths, bool deleteFiles=false);
	
	//! Updates the loaded satellite collection from the provided data.
	//! Worker function called by updateFromFiles() and saveDownloadedUpdate().
	//! (Respectively, user-initiated update from file(s) and user- or auto-
	//! initiated update from online source(s).)
	//! Emits updateStateChanged() and tleUpdateComplete().
	//! @note Instead of splitting this method off updateFromFiles() and passing
	//! the auto-add flag through data structures, another possibility was to
	//! modify updateFromFiles to use the same prefix trick (adding "1,"
	//! to file paths). I decided against it because I thought it would be more
	//! complex. :) --BM
	//! @param[in,out] newTleSets a hash with satellite IDs as keys; it's
	//! modified by the method!
	void updateSatellites(TleDataHash& newTleSets);
	
	//! Reads a TLE list from a file to the supplied hash.
	//! If an entry with the same ID exists in the given hash, its contents
	//! are overwritten with the new values.
	//! \param openFile a reference to an \b open file.
	//! @param[in,out] tleList a hash with satellite IDs as keys.
	//! @param[in] addFlagValue value to be set to TleData::addThis for all.
	//! @param[in] tleURL a URL of TLE's source (e.g. Celestrak URL)
	//! @todo If this can accept a QIODevice, it will be able to read directly
	//! QNetworkReply-s... --BM
	static void parseTleFile(QFile& openFile, TleDataHash& tleList, bool addFlagValue = false, const QString& tleURL = "");

	//! Insert a three line TLE into the hash array.
	//! @param[in] line The second line from the TLE
	static QString getSatIdFromLine2(const QString& line);

	//! Reads qs.mag and rcs files and its parsing for getting id,  standard magnitude and RCS values
	//! for satellites.
	//! @note We are having permissions for use this file from Mike McCants.	
	void loadExtraData();

	QList<CommLink> getCommunicationData(const QString& id);
	QString getLastSelectedSatelliteID() { return lastSelectedSatellite; }
	
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	//! Get depth of prediction for Iridium flares
	int getIridiumFlaresPredictionDepth(void) const { return iridiumFlaresPredictionDepth; }

	IridiumFlaresPredictionList getIridiumFlaresPrediction();
#endif

signals:
	void flagHintsVisibleChanged(bool b);
	void flagLabelsVisibleChanged(bool b);
	void labelFontSizeChanged(int s);
	void flagOrbitLinesChanged(bool b);
	void flagIconicModeChanged(bool b);
	void flagHideInvisibleChanged(bool b);
	void flagColoredInvisibleChanged(bool b);
	void updatesEnabledChanged(bool b);
	void updateFrequencyHoursChanged(int i);
	void autoAddEnabledChanged(bool b);
	void autoRemoveEnabledChanged(bool b);
	void orbitLineSegmentsChanged(int i);
	void orbitLineFadeSegmentsChanged(int i);
	void orbitLineThicknessChanged(int i);
	void orbitLineSegmentDurationChanged(int i);
	void tleEpochAgeDaysChanged(int i);
	void invisibleSatelliteColorChanged(Vec3f);
	void transitSatelliteColorChanged(Vec3f);
	void flagUmbraVisibleChanged(bool b);
	void flagUmbraAtFixedDistanceChanged(bool b);
	void umbraColorChanged(Vec3f);
	void umbraDistanceChanged(double d);
	void flagPenumbraVisibleChanged(bool b);
	void penumbraColorChanged(Vec3f);
	void flagCFKnownStdMagnitudeChanged(bool b);
	void flagCFApogeeChanged(bool b);
	void minCFApogeeChanged(double v);
	void maxCFApogeeChanged(double v);
	void flagCFPerigeeChanged(bool b);
	void minCFPerigeeChanged(double v);
	void maxCFPerigeeChanged(double v);	
	void flagCFEccentricityChanged(bool b);
	void minCFEccentricityChanged(double v);
	void maxCFEccentricityChanged(double v);
	void flagCFPeriodChanged(bool b);
	void minCFPeriodChanged(double v);
	void maxCFPeriodChanged(double v);
	void flagCFInclinationChanged(bool b);
	void minCFInclinationChanged(double v);
	void maxCFInclinationChanged(double v);
	void flagCFRCSChanged(bool b);
	void minCFRCSChanged(double v);
	void maxCFRCSChanged(double v);
	void flagVFAltitudeChanged(bool b);
	void minVFAltitudeChanged(double v);
	void maxVFAltitudeChanged(double v);
	void flagVFMagnitudeChanged(bool b);
	void minVFMagnitudeChanged(double v);
	void maxVFMagnitudeChanged(double v);

	//! Emitted when some of the plugin settings have been changed.
	//! Used to communicate with the configuration window.
	void settingsChanged();

	//! Emitted when custom filters of the plugin have been changed.
	//! Used to communicate with the configuration window.
	void customFilterChanged();
	
	//! emitted when the update status changes, e.g. when 
	//! an update starts, completes and so on.  Note that
	//! on completion of an update, tleUpdateComplete is also
	//! emitted with the number of updates done.
	//! @param state the new update state.
	void updateStateChanged(Satellites::UpdateState state);

	//! Emitted after an update has run.
	//! @param updated the number of updated satellites;
	//! @param total the total number of satellites in the catalog;
	//! @param added the number of newly added satellites;
	//! @param missing the number of satellites that were not found in the
	//! update source(s) (and were removed, if autoRemoveEnabled is set).
	void tleUpdateComplete(int updated, int total, int added, int missing);

	void satGroupVisibleChanged();

	void satSelectionChanged(QString satID);

public slots:
	//! get whether or not the plugin will try to update TLE data from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled(void) {return updatesEnabled;}
	//! Set whether the plugin will try to download updates from the Internet.
	//! Emits settingsChanged() if the value changes.
	//! @param b if true, updates will be enabled, else they will be disabled.
	void setUpdatesEnabled(bool enabled);
	
	bool isAutoAddEnabled() { return autoAddEnabled; }
	//! Emits settingsChanged() if the value changes.
	void setAutoAddEnabled(bool enabled);
	
	bool isAutoRemoveEnabled() { return autoRemoveEnabled; }
	//! Emits settingsChanged() if the value changes.
	void setAutoRemoveEnabled(bool enabled);
	
	//! Set whether satellite position hints (icons or star-like dot) should be displayed.
	//! Note that hint visibility also applies to satellite labels.
	//! Emits settingsChanged() if the value changes.
	void setFlagHintsVisible(bool b);
	bool getFlagHintsVisible() {return hintFader;}

	//! Set whether text labels should be displayed next to satellite hints.
	//! Emits settingsChanged() if the value changes.
	//! @todo Decide how to sync with "actionShow_Satellite_Labels".
	void setFlagLabelsVisible(bool b);
	bool getFlagLabelsVisible();

	//! Emits settingsChanged() if the value changes.
	void setFlagIconicMode(bool b);
	bool getFlagIconicMode();

	bool getFlagHideInvisible();
	void setFlagHideInvisible(bool b);

	bool getFlagColoredInvisible();
	void setFlagColoredInvisible(bool b);

	//! Get color for invisible satellites
	//! @return color
	Vec3f getInvisibleSatelliteColor();
	//! Set color for invisible satellites
	void setInvisibleSatelliteColor(const Vec3f& c);

	//! Get color for satellites in transit through the Sun or the Moon (color of markers)
	//! @return color
	Vec3f getTransitSatelliteColor();
	//! Set color for satellites in transit through the Sun or the Moon (color of markers)
	void setTransitSatelliteColor(const Vec3f& c);
	
	//! get the label font size.
	//! @return the pixel size of the font
	int getLabelFontSize() {return labelFont.pixelSize();}
	//! set the label font size.
	//! @param size the pixel size of the font
	//! Emits settingsChanged() if the value changes.
	void setLabelFontSize(int size);
	
	//! Set the Internet update frequency.
	//! Emits settingsChanged() if the value changes.
	void setUpdateFrequencyHours(int hours);
	
	//! Start an Internet update.
	//! This method starts the process of an Internet update: it tries to
	//! download TLE lists from online resources and then use them to
	//! update the orbital data (and names, etc.) of the included satellites.
	//! This only initialized the download. The rest of the work is done by
	//! saveDownloadedUpdate() and updateSatellites().
	//! Update sources are described in updateUrls (see for accessors details).
	//! If autoAddEnabled is true when this function is called, new satellites
	//! in the chosen update sources will be added during the update. 
	//! If autoRemoveEnabled is true when this function is called, any existing
	//! satellite that can't be found in the downloaded update lists will be
	//! removed.
	//! See updateFromFiles() for the other type of update operation.
	void updateFromOnlineSources();

	//! Choose whether or not to draw orbit lines.  Each satellite has its own setting
	//! as well, but this can be used to turn on/off all those satellites which elect to
	//! have orbit lines all in one go.
	//! @param b - true to turn on orbit lines, false to turn off
	void setFlagOrbitLines(bool b);
	//! Get the current status of the orbit line rendering flag.
	bool getFlagOrbitLines();

	//! return number of segments for orbit lines
	int getOrbitLineSegments() {return Satellite::orbitLineSegments;}
	//! set number of segments for orbit lines
	void setOrbitLineSegments(int s);

	//! return number of fading segments at end of orbit
	int getOrbitLineFadeSegments() {return Satellite::orbitLineFadeSegments;}
	//! set number of fading segments at end of orbit
	void setOrbitLineFadeSegments(int s);

	//! return the thickness of orbit
	int getOrbitLineThickness() {return Satellite::orbitLineThickness;}
	//! set the thickness of orbit
	void setOrbitLineThickness(int s);

	//! return duration of a single segments
	int getOrbitLineSegmentDuration() {return Satellite::orbitLineSegmentDuration;}
	//! set duration of a single segments
	void setOrbitLineSegmentDuration(int s);

	//! return the valid age of TLE's epoch
	int getTleEpochAgeDays() { return Satellite::tleEpochAge; }
	//! set the valid age of TLE's epoch
	void setTleEpochAgeDays(int age);

	void recalculateOrbitLines(void);

	//! Set whether ring of Earth's umbra should be displayed.
	//! Emits settingsChanged() if the value changes.
	void setFlagUmbraVisible(bool b);
	bool getFlagUmbraVisible() { return flagUmbraVisible; }

	//! Set whether ring of Earth's umbra should be displayed at fixed distance.
	//! Emits settingsChanged() if the value changes.
	//! V23.1: For now this had to be disabled/set to false.
	void setFlagUmbraAtFixedDistance(bool b);
	bool getFlagUmbraAtFixedDistance() { return flagUmbraAtFixedDistance; }

	//! Get color for ring of Earth's umbra
	//! @return color
	Vec3f getUmbraColor() { return umbraColor; }
	//! Set color for ring of Earth's umbra
	void setUmbraColor(const Vec3f& c);

	//! Get the fixed distance for center of visualized Earth's umbra
	//! @return distance, km
	double getUmbraDistance() { return fixedUmbraDistance; }
	//! Set the fixed distance for center of visualized Earth's umbra
	void setUmbraDistance(double d);

	//! Set whether ring of Earth's penumbra should be displayed.
	//! Emits settingsChanged() if the value changes.
	void setFlagPenumbraVisible(bool b);
	bool getFlagPenumbraVisible() { return flagPenumbraVisible; }

	//! Get color for ring of Earth's penumbra
	//! @return color
	Vec3f getPenumbraColor() { return penumbraColor; }
	//! Set color for ring of Earth's penumbra
	void setPenumbraColor(const Vec3f& c);

	//! Display a message on the screen for a few seconds.
	//! This is used for plugin-specific warnings and such.
	void displayMessage(const QString& message, const QString hexColor="#999999");

	//! Save the current satellite catalog to disk.
	void saveCatalog(QString path=QString());

	//! Set whether custom filter 'known standard magnitude' enabled.
	//! Emits customFilterChanged()
	void setFlagCFKnownStdMagnitude(bool b);
	bool getFlagCFKnownStdMagnitude() { return Satellite::flagCFKnownStdMagnitude; }

	//! Set whether custom filter 'apogee' enabled.
	//! Emits customFilterChanged()
	void setFlagCFApogee(bool b);
	bool getFlagCFApogee() { return Satellite::flagCFApogee; }

	//! Set whether custom filter 'apogee' maximum value (in kilometers).
	//! Emits customFilterChanged()
	void setMaxCFApogee(double v);
	double getMaxCFApogee() { return Satellite::maxCFApogee; }

	//! Set whether custom filter 'apogee' minimum value (in kilometers).
	//! Emits customFilterChanged()
	void setMinCFApogee(double v);
	double getMinCFApogee() { return Satellite::minCFApogee; }

	//! Set whether custom filter 'perigee' enabled.
	//! Emits customFilterChanged()
	void setFlagCFPerigee(bool b);
	bool getFlagCFPerigee() { return Satellite::flagCFPerigee; }

	//! Set whether custom filter 'perigee' maximum value (in kilometers).
	//! Emits customFilterChanged()
	void setMaxCFPerigee(double v);
	double getMaxCFPerigee() { return Satellite::maxCFPerigee; }

	//! Set whether custom filter 'perigee' minimum value (in kilometers).
	//! Emits customFilterChanged()
	void setMinCFPerigee(double v);
	double getMinCFPerigee() { return Satellite::minCFPerigee; }

	//! Set whether visual filter 'altitude' enabled.
	void setFlagVFAltitude(bool b);
	bool getFlagVFAltitude() { return Satellite::flagVFAltitude; }

	//! Set visual filter 'altitude' maximum value (in kilometers).
	void setMaxVFAltitude(double v);
	double getMaxVFAltitude() { return Satellite::maxVFAltitude; }

	//! Set visual filter 'altitude' minimum value (in kilometers).
	void setMinVFAltitude(double v);
	double getMinVFAltitude() { return Satellite::minVFAltitude; }

	//! Set whether visual filter 'magnitude' enabled.
	void setFlagVFMagnitude(bool b);
	bool getFlagVFMagnitude() { return Satellite::flagVFMagnitude; }

	//! Set visual filter 'magnitude' maximum value.
	void setMaxVFMagnitude(double v);
	double getMaxVFMagnitude() { return Satellite::maxVFMagnitude; }

	//! Set visual filter 'magnitude' minimum value.
	void setMinVFMagnitude(double v);
	double getMinVFMagnitude() { return Satellite::minVFMagnitude; }

	//! Set whether custom filter 'eccentricity' enabled.
	//! Emits customFilterChanged()
	void setFlagCFEccentricity(bool b);
	bool getFlagCFEccentricity() { return Satellite::flagCFEccentricity; }

	//! Set whether custom filter 'eccentricity' maximum value.
	//! Emits customFilterChanged()
	void setMaxCFEccentricity(double v);
	double getMaxCFEccentricity() { return Satellite::maxCFEccentricity; }

	//! Set whether custom filter 'eccentricity' minimum value.
	//! Emits customFilterChanged()
	void setMinCFEccentricity(double v);
	double getMinCFEccentricity() { return Satellite::minCFEccentricity; }

	//! Set whether custom filter 'period' enabled.
	//! Emits customFilterChanged()
	void setFlagCFPeriod(bool b);
	bool getFlagCFPeriod() { return Satellite::flagCFPeriod; }

	//! Set whether custom filter 'period' maximum value (in minutes).
	//! Emits customFilterChanged()
	void setMaxCFPeriod(double v);
	double getMaxCFPeriod() { return Satellite::maxCFPeriod; }

	//! Set whether custom filter 'period' minimum value (in minutes).
	//! Emits customFilterChanged()
	void setMinCFPeriod(double v);
	double getMinCFPeriod() { return Satellite::minCFPeriod; }

	//! Set whether custom filter 'inclination' enabled.
	//! Emits customFilterChanged()
	void setFlagCFInclination(bool b);
	bool getFlagCFInclination() { return Satellite::flagCFInclination; }

	//! Set whether custom filter 'inclination' maximum value (in degrees).
	//! Emits customFilterChanged()
	void setMaxCFInclination(double v);
	double getMaxCFInclination() { return Satellite::maxCFInclination; }

	//! Set whether custom filter 'inclination' minimum value (in degrees).
	//! Emits customFilterChanged()
	void setMinCFInclination(double v);
	double getMinCFInclination() { return Satellite::minCFInclination; }

	//! Set whether custom filter 'rcs' enabled.
	//! Emits customFilterChanged()
	void setFlagCFRCS(bool b);
	bool getFlagCFRCS() { return Satellite::flagCFRCS; }

	//! Set whether custom filter 'rcs' maximum value (in square meters).
	//! Emits customFilterChanged()
	void setMaxCFRCS(double v);
	double getMaxCFRCS() { return Satellite::maxCFRCS; }

	//! Set whether custom filter 'rcs' minimum value (in square meters).
	//! Emits customFilterChanged()
	void setMinCFRCS(double v);
	double getMinCFRCS() { return Satellite::minCFRCS; }

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	//! Set depth of prediction for Iridium flares
	//! @param depth in days
	void setIridiumFlaresPredictionDepth(int depth) { iridiumFlaresPredictionDepth=depth; }
#endif

private slots:
	//! Update satellites visibility on wide range of dates changes - by month or year
	void updateSatellitesVisibility();
	//! Call when button "Save settings" in main GUI are pressed
	void saveSettings() { saveSettingsToConfig(); }	
	void translateData();
	void updateEarthShadowEnlargementFlag(bool state) { earthShadowEnlargementDanjon=state; }

private:
	//! Drawing the circles of Earth's umbra and penumbra
	void drawCircles(StelCore* core, StelPainter& painter);
	//! Add to the current collection the satellite described by the data.
	//! @warning Use only in other methods! Does not update satelliteListModel!
	//! @todo This probably could be done easier if Satellite had a constructor
	//! accepting TleData... --BM
	//! @returns true if the addition was successful.
	bool add(const TleData& tleData);
	//! Guess the groups of satellites
	QStringList guessGroups(const TleData& tleData);
	//! Get the standard magnitude and RCS data for satellite
	//! @return standard magnitude, RCS
	QPair<double, double> getStdMagRCS(const TleData& tleData);
	QString getSatelliteDescription(int satID);
	QList<CommLink> getCommunicationData(const TleData& tleData);
	
	//! Delete Satellites section in main config.ini, then create with default values.
	void restoreDefaultSettings();
	//! Replace the catalog file with the default one.
	void restoreDefaultCatalog();
	//! Load the satellites from the catalog file.
	//! Removes existing satellites first if there are any.
	//! this will be done once at init, and also if the defaults are reset.
	void loadCatalog();
	//! Creates a backup of the satellites.json file called satellites.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupCatalog(bool deleteOriginal=false);
	//! Read the version number from the "creator" value in the catalog file.
	//! @return version string, e.g. "0.6.1"
	const QString readCatalogVersion();

	//! Checks valid range dates of life of satellites
	bool isValidRangeDates(const StelCore* core) const;

	//! Save a structure representing a satellite catalog to a JSON file.
	//! If no path is specified, catalogPath is used.
	//! @see createDataMap()
	bool saveDataMap(const QVariantMap& map, QString path=QString());
	//! Load a structure representing a satellite catalog from a JSON file.
	//! If no path is specified, catalogPath is used.
	QVariantMap loadDataMap(QString path=QString());
	//! Parse a satellite catalog structure into internal satellite data.
	void setDataMap(const QVariantMap& map);
	//! Make a satellite catalog structure from current satellite data.
	//! @return a representation of a JSON file.
	QVariantMap createDataMap();
	
	//! Sets lastUpdate to the current date/time and saves it to the settings.
	void markLastUpdate();

	//! Check format of the catalog of satellites
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat();

	void setSatGroupVisible(const QString& groupId, bool visible);
	
	void bindingGroups();
	//! A fake method for strings marked for translation.
	//! Use it instead of translations.h for N_() strings, except perhaps for
	//! keyboard action descriptions. (It's better for them to be in a single
	//! place.)
	static void translations();

	void createSuperGroupsList();

	//! Path to the satellite catalog file.
	QString catalogPath;
	//! Plug-in data directory.
	//! Initialized by init(). Contains the catalog file (satellites.json),
	//! temporary TLE lists downloaded during an online update, or whatever
	//! other modifiable files the plug-in needs.
	QDir dataDir;
	
	QList<SatelliteP> satellites;
	SatellitesListModel* satelliteListModel;

	QHash<int, double> qsMagList, rcsList;
	QMap<int, QVariantMap> satComms;
	QMap<QString, QVariantMap> groupComms;
	
	//! Union of the groups used by all loaded satellites - see @ref groups.
	//! For simplicity, it can only grow until the plug-in is unloaded -
	//! a group is not removed even if there are no more satellites tagged with
	//! it.
	QSet<QString> groups;
	
	LinearFader hintFader;
	StelTextureSP texPointer, texCross;
	
	//! @name Bottom toolbar button
	//@{
	StelButton* toolbarButton;	
	//@}	
	QSharedPointer<Planet> earth, sun;
	Vec3f defaultHintColor;
	QFont labelFont;
	
	//! @name Updater module
	//@{
	UpdateState updateState;
	QNetworkAccessManager* downloadMgr;
	//! List of TLE source lists for automatic updates.
	//! Use getTleSources() to get the value, setTleSources() to set it (and
	//! save it to configuration).
	//! URLs prefixed with "1," indicate that satellites from this source will
	//! be auto-added if autoAddEnabled is true. URLs prefixed with "0," or
	//! without a prefix are used only to update existing satellites. This
	//! system was introduced to avoid using a custom type as a parameter in
	//! setTleSources(), which in turn allows it to be used in scripts.
	QStringList updateUrls;
	//! Temporary stores update URLs and files during an online update.
	//! In use only between updateFromOnlineSources() and the final call to
	//! saveDownloadedUpdate(). @b DO @b NOT use elsewhere!
	//! As a side effect it prevents problems if the user calls
	//! setTleSources() while an update is in progress.
	TleSourceList updateSources;
	class StelProgressController* progressBar;
	int numberDownloadsComplete;
	QTimer* updateTimer;
	//! Flag enabling automatic Internet updates.
	bool updatesEnabled;
	//! Flag enabling the automatic addition of new satellites on update.
	//! This will apply only for the selected update sources.
	bool autoAddEnabled;
	//! Flag enabling the automatic removal of missing satellites on update.
	bool autoRemoveEnabled;
	QDateTime lastUpdate;
	int updateFrequencyHours;
	//@}

	//! @name Umbra/penumbra module
	//@{
	//! Flag enabling visualization the Earth's umbra.
	bool flagUmbraVisible;
	//! Flag enabling visualization the Earth's umbra at fixed distance
	//! V23.1: This had to be disabled until computation will be implemented correctly.
	bool flagUmbraAtFixedDistance; // MUST REMAIN false FOR NOW!
	Vec3f umbraColor;
	//! The distance for center of visualized Earth's umbra in kilometers
	double fixedUmbraDistance;
	//! Flag enabling visualization the Earth's penumbra.
	bool flagPenumbraVisible;
	Vec3f penumbraColor;
	//! Used to track whether earth shadow enlargement shall be computed after Danjon (1951)
	bool earthShadowEnlargementDanjon;
	//@}

	//! @name Screen message infrastructure
	//@{
	QList<int> messageIDs;
	//@}

	QString lastSelectedSatellite;

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
	int iridiumFlaresPredictionDepth;
#endif
	// GUI
	SatellitesDialog* configDialog;

	QMultiMap<QString, QString> satSuperGroupsMap;

	static QString SatellitesCatalogVersion;

private slots:
	//! check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);
	//! Save the downloaded file and finish the update if it's the last one.
	//! Calls updateSatellites() and indirectly emits updateStateChanged()
	//! and updateFinished().
	//! Ends the update process started with updateFromOnlineSources().
	//! @todo I've kept the previous behaviour, which was to save the update to
	//! temporary files and then read them. If we give up on the idea to
	//! re-use them later when adding manually satellites, parseTleFile()
	//! can be modified to read directly form QNetworkReply-s. --BM
	void saveDownloadedUpdate(QNetworkReply* reply);
	void updateObserverLocation(const StelLocation &loc);
	void changeSelectedSatellite(QString id) { lastSelectedSatellite = id; }
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SatellitesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* SATELLITES_HPP */

