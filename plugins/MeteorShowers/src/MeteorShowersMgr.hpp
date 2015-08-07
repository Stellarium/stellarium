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
class MSSearchDialog;

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
	Q_PROPERTY(bool enablePlugin READ getEnablePlugin WRITE actionEnablePlugin)
	Q_PROPERTY(bool enableLabels READ getEnableLabels WRITE setEnableLabels)

public:
	//! @enum DownloadStatus
	enum DownloadStatus {
		OUTDATED,
		UPDATING,
		UPDATED,
		FAILED
	};

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

	//! Gets the MeteorShowers instance
	//! @return MeteorShowers instance
	MeteorShowers* getMeteorShowers() { return m_meteorShowers; }

	//! Enable/disable the meteor showers plugin.
	void setEnablePlugin(const bool& b);
	bool getEnablePlugin() { return m_enablePlugin; }

	//! True if user wants to see the active radiants only.
	bool getActiveRadiantOnly() { return m_activeRadiantOnly; }

	//! Get the status of the enable button on the toolbar.
	//! @return true if it's visible
	bool getShowEnableButton() { return m_showEnableButton; }

	//! Get the status of the search button on the toolbar.
	//! @return true if it's visible
	bool getShowSearchButton() { return m_showSearchButton; }

	//! Set the color of the active radiant based on generic data.
	void setColorARG(const Vec3f& rgb);
	Vec3f getColorARG() { return m_colorARG; }

	//! Set the color of the active radiant based on confirmed data.
	void setColorARC(const Vec3f& rgb);
	Vec3f getColorARC() { return m_colorARC; }

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
	bool getEnableAutoUpdates() { return m_enableAutoUpdates; }

	//! Set the URL for downloading the meteor showers catalog.
	void setUrl(const QString& url);
	QString getUrl() { return m_url; }

	//! Set the date and time of last update.
	void setLastUpdate(const QDateTime& datetime);
	QDateTime getLastUpdate() { return m_lastUpdate; }

	//! Set the status of the last update
	void setStatusOfLastUpdate(const int &downloadStatus);
	DownloadStatus getStatusOfLastUpdate() { return m_statusOfLastUpdate; }

	//! Gets the date of the next update.
	QDateTime getNextUpdate();

	//! It's useful to force the update() and draw().
	void repaint();

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
	void downloadStatusChanged(DownloadStatus);

public slots:
	//! Enable the meteor showers plugin at Stellarium startup.
	void setEnableAtStartup(const bool& b);

	//! Show/hide the button that enable/disable the meteor showers plugin.
	void setShowEnableButton(const bool& show);

	//! Show/hide the button that opens the search dialog.
	void setShowSearchButton(const bool& show);

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

	//! Enable/disable automatic catalog updates from the internet.
	void setEnableAutoUpdates(const bool& b);

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
	void locationChanged(StelLocation location);

private:
	MeteorShowers* m_meteorShowers;
	MSConfigDialog* m_configDialog;
	MSSearchDialog* m_searchDialog;

	QFont m_font;
	QSettings* m_conf;
	QString m_catalogPath;

	bool m_onEarth;
	bool m_enablePlugin;
	bool m_activeRadiantOnly;
	bool m_enableAtStartup;
	bool m_enableLabels;
	bool m_enableMarker;
	bool m_showEnableButton;
	bool m_showSearchButton;

	Vec3f m_colorARG;        //! color of active radiant based on generic data
	Vec3f m_colorARC;        //! color of active radiant based on confirmed data
	Vec3f m_colorIR;         //! color of inactive radiant

	QTimer* m_messageTimer;
	QList<int> m_messageIDs;

	StelTextureSP m_bolideTexture;  //! Meteor bolide texture
	StelTextureSP m_pointerTexture; //! Pointer texture
	StelTextureSP m_radiantTexture; //! Radiant texture

	bool m_isUpdating;
	bool m_enableAutoUpdates;
	int m_updateFrequencyHours;
	QString m_url;
	QDateTime m_lastUpdate;
	DownloadStatus m_statusOfLastUpdate;
	QNetworkAccessManager* m_downloadMgr;
	class StelProgressController* m_progressBar;

	void createActions();
	void loadConfig();
	void loadTextures();
	bool loadCatalog(const QString& jsonPath);

	//! Enable/disable the Meteor Showers plugin.
	//! It'll be triggered by a StelAction! So, it should NOT be called directly!
	void actionEnablePlugin(const bool& b) { m_enablePlugin = b; }
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
