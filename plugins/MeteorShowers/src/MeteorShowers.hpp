/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013 Marcos Cardinot
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

#ifndef METEORSHOWERS_HPP_
#define METEORSHOWERS_HPP_

#include <QColor>

#include "MeteorObj.hpp"
#include "MeteorShower.hpp"
#include "StelObjectModule.hpp"

class MeteorShowerDialog;
class QNetworkAccessManager;
class QNetworkReply;
class StelButton;

typedef QSharedPointer<MeteorShower> MeteorShowerP;

/*! @defgroup meteorShowers Meteor Showers Plug-in
@{
The %Meteor Showers plugin displays meteor showers and a marker for each
active and inactive radiant, showing real information about its activity.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [MeteorShowers]).

@}
*/

//! @class MeteorShowers
//! Main class of the %Meteor Showers plugin.
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MeteorShowers : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool msVisible READ getFlagShowMS WRITE setFlagShowMS)
	Q_PROPERTY(bool labelsVisible READ getFlagLabels WRITE setFlagLabels)
public:
	//! @enum UpdateState
	//! Used for keeping track of the download/update status
	enum UpdateState
	{
		Updating,             //!< Update in progress
		CompleteNoUpdates,    //!< Update completed, there we no updates
		CompleteUpdates,      //!< Update completed, there were updates
		DownloadError,        //!< Error during download phase
		OtherError            //!< Other error
	};

	MeteorShowers();
	virtual ~MeteorShowers();

	//! Set up the plugin with default values.  This means clearing out the MeteorShower section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also
	//! creates the default showers.json file from the resource embedded in the plugin lib/dll file.
	void restoreDefaults();

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig();

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig();

	//! Determine if the plugin will try to update the catalog from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled() { return updatesEnabled; }

	//! Determine if the plugin will try to update the catalog from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) { updatesEnabled = b; }

	//! Get the datetime of last update
	//! @return QDateTime
	QDateTime getLastUpdate() { return lastUpdate; }

	//! Get the update frequency in hours
	int getUpdateFrequencyHours() { return updateFrequencyHours; }
	void setUpdateFrequencyHours(int hours) { updateFrequencyHours = hours; }

	//! Get the number of seconds till the next update
	int getSecondsToUpdate();

	//! Get the current updateState
	UpdateState getUpdateState() { return updateState; }

	//! Get current color of active radiant based  in generic data
	QColor getColorARG() { return colorARG; }
	void setColorARG(QColor color) { colorARG = color; }

	//! Get current color of active radiant based  in real data
	QColor getColorARR() { return colorARR; }
	void setColorARR(QColor color) { colorARR = color; }

	//! Get current inactive radiant color
	QColor getColorIR() { return colorIR; }
	void setColorIR(QColor color) { colorIR = color; }

	//! Get the label font
	//! @return QFont
	QFont getLabelFont() { return labelFont; }

	//! Get bolide texture
	//! @return texture
	StelTextureSP getBolideTexture() { return m_bolideTexture; }

	//! Get status of labels
	//! @return false: hidden
	bool getFlagLabels();

	//! Get current sky date.
	//! @return current sky date
	QDateTime getSkyDate() { return skyDate; }

	//! Find all meteor_shower events in a given date interval
	//! @param dateFrom
	//! @param dateTo
	//! @return meteor_shower list
	QList<MeteorShowerP> searchEvents(QDate dateFrom, QDate dateTo) const;

	//
	// Methods defined in the StelModule class
	//

	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! Implment this to tell the main Stellarium GUI that there is a GUI element to configure this plugin.
	virtual bool configureGui(bool show=true);

	//
	// Methods defined in StelObjectModule class
	//

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
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;
	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const { Q_UNUSED(objType) Q_UNUSED(inEnglish) return QStringList(); }
	virtual QString getName() const { return "Meteor Showers"; }

signals:
	//! @param state the new update state.
	void updateStateChanged(MeteorShowers::UpdateState state);

	//! emitted after a JSON update has run.
	void jsonUpdateComplete(void);

public slots:
	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

	void setFlagShowMS(bool b);
	bool getFlagShowMS(void) const;

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");
	void messageTimeout(void);

	//! Define whether the button toggling meteor showers should be visible
	void setFlagShowMSButton(bool b);
	bool getFlagShowMSButton(void) { return flagShowMSButton; }

	//! set the label font size.
	//! @param size the pixel size of the font
	void setLabelFontSize(int size);

	//! Set whether text labels should be displayed next to radiant.
	//! @param false: hidden
	void setFlagLabels(bool b);

	bool getFlagActiveRadiant(void)
	{
		return MeteorShower::showActiveRadiantsOnly;
	}
	void setFlagActiveRadiant(bool b)
	{
		MeteorShower::showActiveRadiantsOnly=b;
	}

	bool getEnableAtStartup(void) {return enableAtStartup;}
	void setEnableAtStartup(bool b)	{enableAtStartup=b;}

	bool getFlagRadiant(void) const { return MeteorShower::radiantMarkerEnabled; }
	void setFlagRadiant(bool b) { MeteorShower::radiantMarkerEnabled=b; }

private:
	//! Draw all radiant
	void drawRadiant(StelCore* core, StelPainter& painter);

	//! Draw pointer
	void drawPointer(StelCore* core);

	//! Upgrade config.ini: rename old key settings to new
	void upgradeConfigIni();

	//! Check if the sky date was changed
	//! @param core
	//! @return if changed, return true
	bool changedSkyDate(StelCore* core);

	//! Update the list with information about active meteors
	//! @param core
	void updateActiveInfo(void);

	//! Set up the plugin with default values.
	void restoreDefaultConfigIni();

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file and create the meteor Showers.
	void readJsonFile(void);

	//! Creates a backup of the showers.json file called showers.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! Get the version from the "version of format" value in the showers.json file
	//! @return version, e.g. "1"
	int getJsonFileFormatVersion(void);
	
	//! Check format of the catalog of meteor showers
	//! @return valid boolean, e.g. "true"
	bool checkJsonFileFormat(void);

	//! Parse JSON file and load showers to map
	QVariantMap loadShowersMap(QString path=QString());

	//! Set items for list of struct from data map
	void setShowersMap(const QVariantMap& map);

	//! A fake method for strings marked for translation.
	//! Use it instead of translations.h for N_() strings, except perhaps for
	//! keyboard action descriptions. (It's better for them to be in a single
	//! place.)
	static void translations();

	QFont labelFont;

	QString showersJsonPath;

	StelTextureSP m_bolideTexture;  //! Meteor bolide texture
	StelTextureSP texPointer;
	QList<MeteorShowerP> mShowers;

	// GUI
	MeteorShowerDialog* configDialog;
	bool flagShowMS;
	bool flagShowMSButton;	
	QPixmap* OnIcon;
	QPixmap* OffIcon;
	QPixmap* GlowIcon;
	StelButton* toolbarButton;
	QColor colorARR;			//color of active radiant based on real data
	QColor colorARG;			//color of active radiant based on generic data
	QColor colorIR;			//color of inactive radiant

	// variables and functions for the updater
	UpdateState updateState;
	QNetworkAccessManager* downloadMgr;
	QString updateUrl;
	QString updateFile;
	class StelProgressController* progressBar;
	QTimer* updateTimer;
	QTimer* messageTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyHours;
	bool enableAtStartup;

	QSettings* conf;

	QDateTime skyDate;              //! Current sky date
	QDateTime lastSkyDate;          //! Last sky date

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
class MeteorShowersStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*METEORSHOWERS_HPP_*/
