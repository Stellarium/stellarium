/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#ifndef METEORSHOWERSMGR_HPP_
#define METEORSHOWERSMGR_HPP_

#include <QNetworkReply>

#include "StelGuiItems.hpp"
#include "StelObjectModule.hpp"

#define MS_CATALOG_VERSION 1
#define MS_CONFIG_PREFIX QString("MeteorShowers")

class MeteorShowers;
class MSConfigDialog;

/*! @defgroup meteorShowers Meteor Showers Plug-in
@{
The %Meteor Showers plugin displays meteor showers and a marker for each
active and inactive radiant, showing real information about its activity.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [MeteorShowers]).

@}
*/

//! @class MeteorShowersMgr
//! Main class of the %Meteor Showers plugin, inherits from StelObjectModule.
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MeteorShowersMgr : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool enablePlugin READ getEnablePlugin WRITE setEnablePlugin)
	Q_PROPERTY(bool enableLabels READ getEnableLabels WRITE setEnableLabels)

public:
	//! Constructor.
	MeteorShowersMgr();

	//! Destructor.
	virtual ~MeteorShowersMgr();

	//! Restore default catalog.
	bool restoreDefaultCatalog(const QString& destination);

	//! Gets the bolide texture.
	//! @return texture
	StelTextureSP getBolideTexture() { return m_bolideTexture; }

	//! Gets the pointer texture.
	//! @return texture
	StelTextureSP getPointerTexture() { return m_pointerTexture; }

	//! Gets the radiant texture
	//! @return texture
	StelTextureSP getRadiantTexture() { return m_radiantTexture; }

	//! Returns the plugin status.
	//! @return true if the Meteor Showers plugin is enabled.
	bool getEnablePlugin() { return m_enablePlugin; }

	//! True if user wants to see the active radiants only.
	bool getActiveRadiantOnly() { return m_activeRadiantOnly; }

	//! Enable/disable the meteor showers buttons on the toolbar.
	bool getEnableButtons() { return m_enableButtons; }

	//! Set the color of the active radiant based on generic data.
	void setColorARG(const Vec3f& rgb);
	Vec3f getColorARG() { return m_colorARG; }

	//! Set the color of the active radiant based on real data.
	void setColorARR(const Vec3f& rgb);
	Vec3f getColorARR() { return m_colorARR; }

	//! Set the color of the inactive radiant.
	void setColorIR(const Vec3f& rgb);
	Vec3f getColorIR() { return m_colorIR; }

	//! True if the plugin is enabled at Stellarium startup.
	bool getEnableAtStartup() { return m_enableAtStartup; }

	//! Set the font size (used on radiant labels).
	int getFontSize() { return m_font.pixelSize(); }

	//! Get the font.
	QFont getFont() { return m_font; }

	//! Enable/disable radiant labels
	bool getEnableLabels() { return m_enableLabels; }

	//! Enable/disable radiant marker.
	bool getEnableMarker() { return m_enableMarker; }

	//! Gets the update frequency in hours.
	int getUpdateFrequencyHours() { return m_updateFrequencyHours; }

	//! Enable/disable catalog updates from the internet.
	bool getEnableUpdates() { return m_enableUpdates; }

	//! Set the URL for downloading the meteor showers catalog.
	void setUrl(const QString& url);
	QString getUrl() { return m_url; }

	//! Set the date and time of last update.
	void setLastUpdate(const QDateTime& datetime);
	QDateTime getLastUpdate() { return m_lastUpdate; }

	//! Gets the date of the next update.
	QDateTime getNextUpdate();

	//!
	bool isUpdating() { return m_isUpdating; }

	//
	// Methods defined in the StelModule class
	//
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show=true);

	//
	// Methods defined in StelObjectModule class
	//
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const { return QList<StelObjectP>();}
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const { return StelObjectP();}
	virtual StelObjectP searchByName(const QString& name) const { return StelObjectP();}
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const { return QStringList();}
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const { return QStringList();}
	virtual QStringList listAllObjects(bool inEnglish) const { return QStringList();}
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const { Q_UNUSED(objType) Q_UNUSED(inEnglish) return QStringList(); }
	virtual QString getName() const { return QString();}

signals:
	void updated();
	void failedToUpdate();

public slots:
	//! Enable the meteor showers plugin at Stellarium startup.
	void setEnableAtStartup(const bool& b);

	//! Enable/disable the meteor showers buttons on the toolbar.
	void setEnableButtons(const bool& b);

	//! Enable/disable radiant marker.
	void setEnableMarker(const bool& b);

	//! True if user wants to see the active radiants only.
	void setActiveRadiantOnly(const bool& b);

	//! Enable/disable radiant labels
	void setEnableLabels(const bool& b);

	//! Set the font size (used on radiant labels).
	void setFontSize(int pixelSize);

	//! Set the update frequency in hours.
	void setUpdateFrequencyHours(const int& hours);

	//! Enable/disable catalog updates from the internet.
	void setEnableUpdates(const bool& b);

	//! Download the Meteor Showers catalog from the Internet.
	void updateCatalog();

	//! Restore default settings.
	void restoreDefaultSettings();

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString hexColor="#999999");
	void messageTimeout();

private slots:
	void checkForUpdates();
	void updateFinished(QNetworkReply* reply);

private:
	MeteorShowers* m_meteorShowers;
	MSConfigDialog* m_configDialog;

	StelButton* m_toolbarEnableButton;
	QFont m_font;
	QSettings* m_conf;
	QString m_catalogPath;

	bool m_enablePlugin;
	bool m_activeRadiantOnly;
	bool m_enableAtStartup;
	bool m_enableButtons;
	bool m_enableLabels;
	bool m_enableMarker;

	Vec3f m_colorARG;        //! color of active radiant based on generic data
	Vec3f m_colorARR;        //! color of active radiant based on real data
	Vec3f m_colorIR;         //! color of inactive radiant

	QTimer* m_messageTimer;
	QList<int> m_messageIDs;

	StelTextureSP m_bolideTexture;  //! Meteor bolide texture
	StelTextureSP m_pointerTexture; //! Pointer texture
	StelTextureSP m_radiantTexture; //! Radiant texture

	bool m_isUpdating;
	bool m_enableUpdates;
	int m_updateFrequencyHours;
	QString m_url;
	QDateTime m_lastUpdate;
	QNetworkAccessManager* m_downloadMgr;
	class StelProgressController* m_progressBar;

	void createActions();
	void createToolbarButtons();
	void loadConfig();
	void loadTextures();
	bool loadCatalog(const QString& jsonPath);

	//! Enable/disable the Meteor Showers plugin.
	//! It'll be triggered by a StelAction! So, it should NOT be called directly!
	void setEnablePlugin(const bool& b) { m_enablePlugin = b; }

	//! A fake method for strings marked for translation.
	//! Use it instead of translations.h for N_() strings, except perhaps for
	//! keyboard action descriptions. (It's better for them to be in a single
	//! place.)
	static void translations();
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

#endif /*METEORSHOWERSMGR_HPP_*/
