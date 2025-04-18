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

#ifndef METEORSHOWERSMGR_HPP
#define METEORSHOWERSMGR_HPP

#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "StelGuiItems.hpp"
#include "StelModule.hpp"
#include "StelLocation.hpp"
#include "StelTextureTypes.hpp"
#include "VecMath.hpp"

#define MS_CATALOG_VERSION 2
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
//! Main class of the %Meteor Showers plugin, inherits from StelModule.
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MeteorShowersMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enablePlugin READ getEnablePlugin WRITE actionEnablePlugin NOTIFY enablePluginChanged)
	Q_PROPERTY(bool enableLabels READ getEnableLabels WRITE setEnableLabels NOTIFY enableLabelsChanged)
	Q_PROPERTY(Vec3f colorARG READ getColorARG WRITE setColorARG NOTIFY colorARGChanged)
	Q_PROPERTY(Vec3f colorARC READ getColorARC WRITE setColorARC NOTIFY colorARCChanged)
	Q_PROPERTY(Vec3f colorIR  READ getColorIR  WRITE setColorIR  NOTIFY colorIRChanged)

public:
	//! @enum UpdateState
	//! Used for keeping for track of the download/update status
	enum UpdateState {
		Updating,			//!< Update in progress
		CompleteNoUpdates,	//!< Update completed, there we no updates
		CompleteUpdates,	//!< Update completed, there were updates
		DownloadError,		//!< Error during download phase
		OtherError			//!< Other error
	};

	//! Constructor.
	MeteorShowersMgr();

	//! Destructor.
	~MeteorShowersMgr() override;

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

	//! Get the font.
	QFont getFont() { return m_font; }

	//! Set the URL for downloading the meteor showers catalog.
	void setUrl(const QString& url);
	QString getUrl() { return m_url; }

	//! Set the date and time of last update.
	void setLastUpdate(const QDateTime& datetime);
	QDateTime getLastUpdate() { return m_lastUpdate; }

	//! Get the current updateState
	UpdateState getUpdateState(void) const {return m_updateState;}

	//! Gets the date of the next update.
	QDateTime getNextUpdate();

	//! It's useful to force the update() and draw().
	void repaint();

	//
	// Methods defined in the StelModule class
	//
	void init() override;
	void deinit() override;
	void update(double deltaTime) override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;
	bool configureGui(bool show=true) override;

signals:
	//! @param state the new update state.
	void updateStateChanged(MeteorShowersMgr::UpdateState state);

	//! Emitted after a JSON update has run.
	void jsonUpdateComplete(void);
	void enablePluginChanged(bool b);
	void enableLabelsChanged(bool b);
	void colorARGChanged(Vec3f c);
	void colorARCChanged(Vec3f c);
	void colorIRChanged(Vec3f c);

public slots:
	//! Enable the meteor showers plugin at Stellarium startup.
	//! @param b boolean flag
	void setEnableAtStartup(const bool& b);
	//! True if the plugin is enabled at Stellarium startup.
	//! @return true if it's enabled at startup
	bool getEnableAtStartup() { return m_enableAtStartup; }

	//! Show/hide the button that enable/disable the meteor showers plugin.
	//! @param b boolean flag
	void setShowEnableButton(const bool& show);
	//! Get the status of the enable button on the toolbar.
	//! @return true if it's visible
	bool getShowEnableButton() { return m_showEnableButton; }

	//! Show/hide the button that opens the search dialog.
	//! @param b boolean flag
	void setShowSearchButton(const bool& show);
	//! Get the status of the search button on the toolbar.
	//! @return true if it's visible
	bool getShowSearchButton() { return m_showSearchButton; }

	//! Enable/disable radiant marker.
	//! @param b boolean flag
	//! @code
	//! // example of usage in scripts
	//! MeteorShowers.setEnableMarker(true);
	//! @endcode
	void setEnableMarker(const bool& b);
	//! Enable/disable radiant marker.
	//! @return true if radiant markers visible
	//! @code
	//! // example of usage in scripts
	//! var flag = MeteorShowers.getEnableMarker();
	//! @endcode
	bool getEnableMarker() { return m_enableMarker; }

	//! True if user wants to see the active radiants only.
	//! @param b boolean flag
	//! @code
	//! // example of usage in scripts
	//! MeteorShowers.setActiveRadiantOnly(true);
	//! @endcode
	void setActiveRadiantOnly(const bool& b);
	//! True if user wants to see the active radiants only.
	//! @return true if only active radiants are visible
	//! @code
	//! // example of usage in scripts
	//! var flag = MeteorShowers.getActiveRadiantOnly();
	//! @endcode
	bool getActiveRadiantOnly() { return m_activeRadiantOnly; }

	//! Enable/disable radiant labels
	//! @param b boolean flag
	//! @code
	//! // example of usage in scripts
	//! MeteorShowers.setEnableLabels(true);
	//! @endcode
	void setEnableLabels(const bool& b);
	//! Enable/disable radiant labels
	//! @return true if radiant labels visible
	//! @code
	//! // example of usage in scripts
	//! var flag = MeteorShowers.getEnableLabels();
	//! @endcode
	bool getEnableLabels() { return m_enableLabels; }

	//! Set the font size (used on radiant labels).
	//! @param pixelSize size of font
	//! @code
	//! // example of usage in scripts
	//! MeteorShowers.setFontSize(15);
	//! @endcode
	void setFontSize(int pixelSize);
	//! Set the font size (used on radiant labels).
	//! @return size of font
	//! @code
	//! // example of usage in scripts
	//! var size = MeteorShowers.getFontSize();
	//! @endcode
	int getFontSize() { return m_font.pixelSize(); }

	//! Set the update frequency in hours.
	//! @param hours update frequency in hours
	void setUpdateFrequencyHours(const int& hours);
	//! Gets the update frequency in hours.
	//! @return update frequency in hours
	int getUpdateFrequencyHours() { return m_updateFrequencyHours; }

	//! Enable/disable automatic catalog updates from the internet.
	//! @param b boolean flag
	void setEnableAutoUpdates(const bool& b);
	//! Enable/disable catalog updates from the internet.
	//! @return true if auto updates is enabled
	bool getEnableAutoUpdates() { return m_enableAutoUpdates; }

	//! Set the color of the active radiant based on generic data.
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! MeteorShowers.setColorARG(c.toVec3f());
	//! @endcode
	void setColorARG(const Vec3f& rgb);
	//! @return color of markers of the active radiants based on generic data.
	//! @code
	//! // example of usage in scripts
	//! color = MeteorShowers.getColorARG();
	//! @endcode
	Vec3f getColorARG() { return m_colorARG; }

	//! Set the color of the active radiant based on confirmed data.
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! MeteorShowers.setColorARC(c.toVec3f());
	//! @endcode
	void setColorARC(const Vec3f& rgb);
	//! @return color of markers of the active radiants based on confirmed data.
	//! @code
	//! // example of usage in scripts
	//! color = MeteorShowers.getColorARC();
	//! @endcode
	Vec3f getColorARC() { return m_colorARC; }

	//! Set the color of the inactive radiant.
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! MeteorShowers.setColorIR(c.toVec3f());
	//! @endcode
	void setColorIR(const Vec3f& rgb);
	//! @return color of markers of the inactive radiants.
	//! @code
	//! // example of usage in scripts
	//! color = MeteorShowers.getColorIR();
	//! @endcode
	Vec3f getColorIR() { return m_colorIR; }

	//! Download the Meteor Showers catalog from the Internet.
	void updateCatalog();

	//! Restore default settings.
	void restoreDefaultSettings();

	//! Display a message. This is used for plugin-specific warnings and such
	void displayMessage(const QString& message, const QString &hexColor="#999999");

private slots:
	void checkForUpdates();
	void updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadComplete(QNetworkReply * reply);
	void locationChanged(const StelLocation &location);
	//! Call when button "Save settings" in main GUI are pressed
	void saveSettings();

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

	QList<int> m_messageIDs;

	StelTextureSP m_bolideTexture;  //! Meteor bolide texture
	StelTextureSP m_pointerTexture; //! Pointer texture
	StelTextureSP m_radiantTexture; //! Radiant texture

	//bool m_isUpdating;
	bool m_enableAutoUpdates;
	int m_updateFrequencyHours;
	QString m_url;
	QDateTime m_lastUpdate;
	UpdateState m_updateState;
	QNetworkAccessManager * m_networkManager;
	QNetworkReply * m_downloadReply;
	class StelProgressController* m_progressBar;
	QTimer* m_updateTimer;

	void createActions();
	void loadConfig();	
	void loadTextures();
	bool loadCatalog(const QString& jsonPath);

	void startDownload(const QString &url);
	void deleteDownloadProgressBar();

	//! Enable/disable the Meteor Showers plugin.
	//! It'll be triggered by a StelAction! So, it should NOT be called directly!
	void actionEnablePlugin(const bool& b);
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
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /*METEORSHOWERSMGR_HPP*/
