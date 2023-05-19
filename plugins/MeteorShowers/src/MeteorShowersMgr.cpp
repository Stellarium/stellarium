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

#include <stdexcept>
#include <QDir>
#include <QJsonDocument>
#include <QSettings>
#include <QTimer>

#include "LabelMgr.hpp"
#include "MSConfigDialog.hpp"
#include "MSSearchDialog.hpp"
#include "MeteorShowersMgr.hpp"
#include "Planet.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelProgressController.hpp"
#include "StelTextureMgr.hpp"
#include "StelUtils.hpp"
#include "SporadicMeteorMgr.hpp"

MeteorShowersMgr::MeteorShowersMgr()
	: m_meteorShowers(Q_NULLPTR)
	, m_configDialog(Q_NULLPTR)
	, m_searchDialog(Q_NULLPTR)
	, m_conf(StelApp::getInstance().getSettings())
	, m_onEarth(false)
	, m_enablePlugin(false)
	, m_activeRadiantOnly(false)
	, m_enableAtStartup(true)
	, m_enableLabels(true)
	, m_enableMarker(true)
	, m_showEnableButton(true)
	, m_showSearchButton(true)
	//, m_isUpdating(false)
	, m_enableAutoUpdates(true)
	, m_updateFrequencyHours(0)
	, m_updateState(CompleteNoUpdates)
	, m_networkManager(Q_NULLPTR)
	, m_downloadReply(Q_NULLPTR)
	, m_progressBar(Q_NULLPTR)
	, m_updateTimer(Q_NULLPTR)
{
	setObjectName("MeteorShowers");
}

MeteorShowersMgr::~MeteorShowersMgr()
{
	delete m_configDialog;
	delete m_searchDialog;
}

void MeteorShowersMgr::init()
{
	loadTextures();

	m_meteorShowers = new MeteorShowers(this);
	m_configDialog = new MSConfigDialog(this);
	m_searchDialog = new MSSearchDialog(this);

	createActions();
	loadConfig();

	// MeteorShowers directory
	QString userDir = StelFileMgr::getUserDir() + "/modules/MeteorShowers";
	StelFileMgr::makeSureDirExistsAndIsWritable(userDir);

	// Loads the JSON catalog
	m_catalogPath = userDir + "/MeteorShowers.json";
	if (!loadCatalog(m_catalogPath))
	{
		displayMessage(q_("The current catalog of Meteor Showers is invalid!"), "#bb0000");
		restoreDefaultCatalog(m_catalogPath);
	}

	// Set up download manager and the update schedule
	//m_networkManager = StelApp::getInstance().getNetworkAccessManager();
	m_networkManager = new QNetworkAccessManager(this);
	m_updateState = CompleteNoUpdates;
	m_updateTimer = new QTimer(this);
	m_updateTimer->setSingleShot(false);   // recurring check for update
	m_updateTimer->setInterval(300000);    // every 5 min, check if it's time to update
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdates()));
	m_updateTimer->start();

	// always check if we are on Earth
	StelCore* core = StelApp::getInstance().getCore();
	m_onEarth = core->getCurrentPlanet()->getEnglishName() == "Earth";
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(locationChanged(StelLocation)));
	connect(core, SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));

	// enable at startup?
	setEnablePlugin(getEnableAtStartup());
}

void MeteorShowersMgr::deinit()
{
	m_bolideTexture.clear();
	m_radiantTexture.clear();
	m_pointerTexture.clear();
	delete m_meteorShowers;
	m_meteorShowers = Q_NULLPTR;
	delete m_configDialog;
	m_configDialog = Q_NULLPTR;
	delete m_searchDialog;
	m_searchDialog = Q_NULLPTR;
}

double MeteorShowersMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
	{
		return GETSTELMODULE(SporadicMeteorMgr)->getCallOrder(actionName) + 10.;
	}
	return 0.;
}

bool MeteorShowersMgr::configureGui(bool show)
{
	if (show)
	{
		m_configDialog->setVisible(show);
	}
	return true;
}

void MeteorShowersMgr::createActions()
{
	QString msGroup = N_("Meteor Showers");
	addAction("actionShow_MeteorShowers",               msGroup, N_("Toggle meteor showers"), this,           "enablePlugin", "Ctrl+Shift+M");
	addAction("actionShow_MeteorShowers_labels",        msGroup, N_("Toggle radiant labels"), this,           "enableLabels", "Shift+M");
	addAction("actionShow_MeteorShowers_config_dialog", msGroup, N_("Show settings dialog"),  m_configDialog, "visible",      "Ctrl+Alt+Shift+M");
	addAction("actionShow_MeteorShowers_search_dialog", msGroup, N_("Show meteor showers search dialog"),    m_searchDialog, "visible",      "Ctrl+Alt+M");
}

void MeteorShowersMgr::loadConfig()
{
	setActiveRadiantOnly(m_conf->value(MS_CONFIG_PREFIX + "/flag_active_radiant_only", true).toBool());
	setShowEnableButton(m_conf->value(MS_CONFIG_PREFIX + "/show_enable_button", true).toBool());
	setShowSearchButton(m_conf->value(MS_CONFIG_PREFIX + "/show_search_button", true).toBool());
	Vec3f color = Vec3f(m_conf->value(MS_CONFIG_PREFIX + "/colorARG", "0.0,1.0,0.94").toString());
	if (color[0]>1.f || color[1]>1.f || color[2]>1.f) { color /= 255.f; }
	setColorARG(color);
	color = Vec3f(m_conf->value(MS_CONFIG_PREFIX + "/colorARC", "1.0,0.94,0.0").toString());
	if (color[0]>1.f || color[1]>1.f || color[2]>1.f) { color /= 255.f; }
	setColorARC(color);
	color = Vec3f(m_conf->value(MS_CONFIG_PREFIX + "/colorIR", "1.0,1.0,1.0").toString());
	if (color[0]>1.f || color[1]>1.f || color[2]>1.f) { color /= 255.f; }
	setColorIR(color);
	setEnableAtStartup(m_conf->value(MS_CONFIG_PREFIX + "/enable_at_startup", true).toBool());	
	setFontSize(m_conf->value(MS_CONFIG_PREFIX + "/font_size", 13).toInt());
	setEnableLabels(m_conf->value(MS_CONFIG_PREFIX + "/flag_radiant_labels", true).toBool());
	setEnableMarker(m_conf->value(MS_CONFIG_PREFIX + "/flag_radiant_marker", true).toBool());
	setUpdateFrequencyHours(m_conf->value(MS_CONFIG_PREFIX + "/update_frequency_hours", 720).toInt());
	setEnableAutoUpdates(m_conf->value(MS_CONFIG_PREFIX + "/automatic_updates_enabled", true).toBool());
	if (m_conf->contains(MS_CONFIG_PREFIX + "/url"))
	{
		m_conf->remove(MS_CONFIG_PREFIX + "/url");
	}
	setUrl(m_conf->value(MS_CONFIG_PREFIX + "/url", "https://stellarium.org/json/MeteorShowers.json").toString());
	setLastUpdate(m_conf->value(MS_CONFIG_PREFIX + "/last_update", "2022-04-27T00:00:00").toDateTime());
}

void MeteorShowersMgr::saveSettings()
{
	// Let's interpret hidden radiants as "disabled at startup" when main settings are saving
	m_conf->setValue(MS_CONFIG_PREFIX + "/enable_at_startup", getEnablePlugin());
}

void MeteorShowersMgr::loadTextures()
{
	m_bolideTexture = StelApp::getInstance().getTextureManager().createTextureThread(
				StelFileMgr::getInstallationDir() + "/textures/cometComa.png",
				StelTexture::StelTextureParams(true, GL_LINEAR, GL_CLAMP_TO_EDGE));

	m_pointerTexture = StelApp::getInstance().getTextureManager().createTexture(
				StelFileMgr::getInstallationDir() + "/textures/pointeur5.png");

	m_radiantTexture = StelApp::getInstance().getTextureManager().createTexture(
				":/MeteorShowers/radiant.png");
}

bool MeteorShowersMgr::loadCatalog(const QString& jsonPath)
{
	qDebug() << "[MeteorShowersMgr] Loading catalog file:"
		 << QDir::toNativeSeparators(jsonPath);

	QFile jsonFile(jsonPath);
	if (!jsonFile.exists())
	{
		restoreDefaultCatalog(jsonPath);
	}

	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[MeteorShowersMgr] Cannot to open the catalog file!";
		return false;
	}

	QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
	jsonFile.close();

	if (json["shortName"].toString() != "meteor showers data"
		|| json["version"].toInt() != MS_CATALOG_VERSION)
	{
		qWarning()  << "[MeteorShowersMgr] The current catalog is not compatible!";
		return false;
	}

	QVariantMap map = json["showers"].toObject().toVariantMap();
	m_meteorShowers->loadMeteorShowers(map);
	qDebug() << "[MeteorShowersMgr] Version of the format of the catalog:" << json["version"].toInt();

	return true;
}

void MeteorShowersMgr::restoreDefaultSettings()
{
	qDebug() << "[MeteorShowersMgr] Restoring default settings";
	m_conf->beginGroup(MS_CONFIG_PREFIX);
	m_conf->remove("");
	m_conf->endGroup();
	loadConfig();
	restoreDefaultCatalog(m_catalogPath);
	m_configDialog->init();
}

bool MeteorShowersMgr::restoreDefaultCatalog(const QString& destination)
{
	qDebug() << "[MeteorShowersMgr] Trying to restore the default catalog to"
		 << QDir::toNativeSeparators(destination);

	QFile d(destination);
	if (d.exists() && !d.remove())
	{
		qWarning() << "[MeteorShowersMgr] Cannot remove the current catalog file!";
		return false;
	}

	QFile defaultJson(":/MeteorShowers/MeteorShowers.json");
	if (!defaultJson.copy(destination))
	{
		qWarning() << "[MeteorShowersMgr] Cannot copy the default catalog!";
		return false;
	}

	// The resource is read only, and the new file inherits this... make sure the
	// new file is writable by the Stellarium process so that updates can be done.
	QFile dest(destination);
	dest.setPermissions(dest.permissions() | QFile::WriteOwner);

	setLastUpdate(QDateTime::fromString("2015-07-01T00:00:00"));

	qDebug() << "[MeteorShowersMgr] The default catalog was copied!";
	displayMessage(q_("Using the default Meteor Showers catalog."), "#bb0000");

	return true;
}

void MeteorShowersMgr::update(double deltaTime)
{
	if (!m_enablePlugin || !m_onEarth)
	{
		return;
	}

	m_meteorShowers->update(deltaTime);
}

void MeteorShowersMgr::draw(StelCore* core)
{
	if (m_enablePlugin && m_onEarth)
	{
		m_meteorShowers->draw(core);
	}
}

void MeteorShowersMgr::repaint()
{
	update(1.0);
	draw(StelApp::getInstance().getCore());
}

void MeteorShowersMgr::checkForUpdates()
{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	if (m_enableAutoUpdates && m_lastUpdate.addSecs(static_cast<qint64>(m_updateFrequencyHours) * 3600) <= QDateTime::currentDateTime())
#else
	if (m_enableAutoUpdates && m_lastUpdate.addSecs(static_cast<qint64>(m_updateFrequencyHours) * 3600) <= QDateTime::currentDateTime() && m_networkManager->networkAccessible()==QNetworkAccessManager::Accessible)
#endif
	{
		updateCatalog();
	}
}

void MeteorShowersMgr::actionEnablePlugin(const bool &b)
{
	if (m_enablePlugin != b)
	{
		m_enablePlugin = b;
		emit enablePluginChanged(b);
		emit StelApp::getInstance().getCore()->updateSearchLists();
	}
}

void MeteorShowersMgr::deleteDownloadProgressBar()
{
	disconnect(this, SLOT(updateDownloadProgress(qint64,qint64)));

	if (m_progressBar)
	{
		StelApp::getInstance().removeProgressBar(m_progressBar);
		m_progressBar = Q_NULLPTR;
	}
}

void MeteorShowersMgr::startDownload(QString urlString)
{
	QUrl url(urlString);
	if (!url.isValid() || url.isRelative() || !url.scheme().startsWith("http", Qt::CaseInsensitive))
	{
		qWarning() << "[MeteorShowersMgr] Invalid URL:" << urlString;
		return;
	}

	if (m_progressBar == Q_NULLPTR)
		m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, 0);

	connect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	QNetworkRequest request;
	request.setUrl(QUrl(m_url));
	request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
#endif
	m_downloadReply = m_networkManager->get(request);
	connect(m_downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)));
	m_updateState = MeteorShowersMgr::Updating;
	emit updateStateChanged(m_updateState);
}

void MeteorShowersMgr::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (m_progressBar == Q_NULLPTR)
		return;

	int currentValue = 0;
	int endValue = 0;

	if (bytesTotal > -1 && bytesReceived <= bytesTotal)
	{
		//Round to the greatest possible derived unit
		while (bytesTotal > 1024)
		{
			bytesReceived = static_cast<qint64>(std::floor(static_cast<double>(bytesReceived) / 1024.));
			bytesTotal    = static_cast<qint64>(std::floor(static_cast<double>(bytesTotal)    / 1024.));
		}
		currentValue = static_cast<int>(bytesReceived);
		endValue = static_cast<int>(bytesTotal);
	}

	m_progressBar->setValue(currentValue);
	m_progressBar->setRange(0, endValue);
}

void MeteorShowersMgr::downloadComplete(QNetworkReply *reply)
{
	if (reply == Q_NULLPTR)
		return;

	disconnect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "[MeteorShowersMgr] Download error: While trying to access"
			   << reply->url().toString()
			   << "the following error occurred:"
			   << reply->errorString();

		reply->deleteLater();
		m_downloadReply = Q_NULLPTR;
		return;
	}

	// download completed successfully.
	try
	{
		QString jsonFilePath = StelFileMgr::findFile("modules/MeteorShowers", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/MeteorShowers.json";
		QFile jsonFile(jsonFilePath);
		if (jsonFile.exists())
			jsonFile.remove();

		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}

		m_updateState = MeteorShowersMgr::CompleteUpdates;
		qDebug() << "[MeteorShowersMgr] Updating Meteor Showers catalog is complete...";
		m_lastUpdate = QDateTime::currentDateTime();
		m_conf->setValue("MeteorShowers/last_update", m_lastUpdate.toString(Qt::ISODate));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[MeteorShowersMgr] Cannot write JSON data to file:" << e.what();
		m_updateState = MeteorShowersMgr::DownloadError;
	}

	emit updateStateChanged(m_updateState);
	emit jsonUpdateComplete();
	reply->deleteLater();
	m_downloadReply = Q_NULLPTR;
}

void MeteorShowersMgr::updateCatalog()
{
	if (m_updateState==MeteorShowersMgr::Updating)
	{
		qWarning() << "[MeteorShowersMgr] already updating...  will not start again current update is complete.";
		return;
	}

	qDebug() << "[MeteorShowersMgr] Updating meteor showers catalog...";
	startDownload(m_url);
}

void MeteorShowersMgr::setEnablePlugin(const bool& b)
{
	// we should never change the 'm_enablePlugin' member directly!
	// as it's a button on the toolbar, it must be sync with its StelAction
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	StelAction* action = actionMgr->findAction("actionShow_MeteorShowers");
	action->setChecked(b);
}

void MeteorShowersMgr::setActiveRadiantOnly(const bool& b)
{
	m_activeRadiantOnly = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/flag_active_radiant_only", b);
}

void MeteorShowersMgr::setShowEnableButton(const bool& show)
{
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (!gui)
		{
			return;
		}

		if (show)
		{
			StelButton* enablePlugin = new StelButton(Q_NULLPTR,
								  QPixmap(":/MeteorShowers/btMS-on.png"),
								  QPixmap(":/MeteorShowers/btMS-off.png"),
								  QPixmap(":/graphicGui/miscGlow32x32.png"),
								  "actionShow_MeteorShowers",
								  false,
								  "actionShow_MeteorShowers_config_dialog");
			gui->getButtonBar()->addButton(enablePlugin, "065-pluginsGroup");
		}
		else
		{
			gui->getButtonBar()->hideButton("actionShow_MeteorShowers");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "MeteorShowersMgr : unable to create toolbar buttons for MeteorShowers plugin!"
			   << e.what();
	}
	m_showEnableButton = show;
	m_conf->setValue(MS_CONFIG_PREFIX + "/show_enable_button", show);
}

void MeteorShowersMgr::setShowSearchButton(const bool& show)
{
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (!gui)
		{
			return;
		}

		if (show)
		{
			StelButton* searchMS = new StelButton(Q_NULLPTR,
							      QPixmap(":/MeteorShowers/btMS-search-on.png"),
							      QPixmap(":/MeteorShowers/btMS-search-off.png"),
							      QPixmap(":/graphicGui/miscGlow32x32.png"),
							      "actionShow_MeteorShowers_search_dialog");
			gui->getButtonBar()->addButton(searchMS, "065-pluginsGroup");
		}
		else
		{
			gui->getButtonBar()->hideButton("actionShow_MeteorShowers_search_dialog");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "MeteorShowersMgr : unable to create toolbar buttons for MeteorShowers plugin!"
			   << e.what();
	}
	m_showSearchButton = show;
	m_conf->setValue(MS_CONFIG_PREFIX + "/show_search_button", show);
}

void MeteorShowersMgr::setColorARG(const Vec3f& rgb)
{
	m_colorARG = rgb;	
	m_conf->setValue(MS_CONFIG_PREFIX + "/colorARG", rgb.toStr());
	emit colorARGChanged(rgb);
}

void MeteorShowersMgr::setColorARC(const Vec3f& rgb)
{
	m_colorARC = rgb;	
	m_conf->setValue(MS_CONFIG_PREFIX + "/colorARC", rgb.toStr());
	emit colorARCChanged(rgb);
}

void MeteorShowersMgr::setColorIR(const Vec3f& rgb)
{
	m_colorIR = rgb;	
	m_conf->setValue(MS_CONFIG_PREFIX + "/colorIR", rgb.toStr());
	emit colorIRChanged(rgb);
}

void MeteorShowersMgr::setEnableAtStartup(const bool& b)
{
	m_enableAtStartup = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/enable_at_startup", b);
}

void MeteorShowersMgr::setFontSize(int pixelSize)
{
	pixelSize = pixelSize < 1 ? 13 : pixelSize;
	m_font.setPixelSize(pixelSize);
	m_conf->setValue(MS_CONFIG_PREFIX + "/font_size", pixelSize);
}

void MeteorShowersMgr::setEnableLabels(const bool& b)
{
	if (m_enableLabels != b)
	{
		m_enableLabels = b;
		m_conf->setValue(MS_CONFIG_PREFIX + "/flag_radiant_labels", b);
		emit enableLabelsChanged(b);
	}
}

void MeteorShowersMgr::setEnableMarker(const bool& b)
{
	m_enableMarker = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/flag_radiant_marker", b);
}

void MeteorShowersMgr::setUpdateFrequencyHours(const int& hours)
{
	m_updateFrequencyHours = hours;
	m_conf->setValue(MS_CONFIG_PREFIX + "/update_frequency_hours", hours);
}

void MeteorShowersMgr::setEnableAutoUpdates(const bool& b)
{
	m_enableAutoUpdates = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/automatic_updates_enabled", b);
}

void MeteorShowersMgr::setUrl(const QString& url)
{
	m_url = url;
	m_conf->setValue(MS_CONFIG_PREFIX + "/url", url);
}

void MeteorShowersMgr::setLastUpdate(const QDateTime &datetime)
{
	m_lastUpdate = datetime;
	m_conf->setValue(MS_CONFIG_PREFIX + "/last_update", m_lastUpdate.toString(Qt::ISODate));
}

QDateTime MeteorShowersMgr::getNextUpdate()
{
	return m_lastUpdate.addSecs(m_updateFrequencyHours * 3600);
}

void MeteorShowersMgr::displayMessage(const QString& message, const QString hexColor)
{
	m_messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20 * m_messageIDs.count()), true, 16, hexColor, false, 9000);
}

void MeteorShowersMgr::locationChanged(const StelLocation &location)
{
	m_onEarth = (location.planetName == "Earth");
}

/////////////////////////////////////////////////////////////////////////////////////////

StelModule* MeteorShowersStelPluginInterface::getStelModule() const
{
	return new MeteorShowersMgr();
}

StelPluginInfo MeteorShowersStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(MeteorShower);

	StelPluginInfo info;
	info.id = "MeteorShowers";
	info.displayedName = N_("Meteor Showers");
	info.authors = "Marcos Cardinot";
	info.contact = STELLARIUM_DEV_URL;
	info.acknowledgements = N_("This plugin was initially created in the 2013 campaign of the ESA Summer of Code in Space programme.");
	info.description = N_(
	"<p>"
		"This plugin enables you to simulate periodic meteor showers and "
		"to display a marker for each active and inactive radiant."
	"</p>"
	"<p>"
		"By a single click on the radiant's marker, you can see all the "
		"details about its position and activity. Most data used on this "
		"plugin comes from the official <a href=\"http://imo.net\">International "
		"Meteor Organization</a> catalog and <a href=\"https://www.ta3.sk/IAUC22DB/MDC2007/\">IAU Meteor Data Center</a>."
	"</p>");
	info.version = METEORSHOWERS_PLUGIN_VERSION;
	info.license = METEORSHOWERS_PLUGIN_LICENSE;
	return info;
}
