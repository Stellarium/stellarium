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

#include <QDir>
#include <QJsonDocument>
#include <QSettings>
#include <QTimer>

#include "LabelMgr.hpp"
#include "MSConfigDialog.hpp"
#include "MeteorShowersMgr.hpp"
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
	: m_meteorShowers(NULL)
	, m_configDialog(NULL)
	, m_toolbarEnableButton(NULL)
	, m_conf(StelApp::getInstance().getSettings())
	, m_enablePlugin(false)
	, m_activeRadiantOnly(false)
	, m_enableAtStartup(true)
	, m_enableButtons(true)
	, m_enableLabels(true)
	, m_enableMarker(true)
	, m_messageTimer(NULL)
	, m_isUpdating(false)
	, m_enableUpdates(true)
	, m_updateFrequencyHours(0)
	, m_downloadMgr(NULL)
	, m_progressBar(NULL)
{
	setObjectName("MeteorShowers");
	qsrand(QDateTime::currentMSecsSinceEpoch());
}

MeteorShowersMgr::~MeteorShowersMgr()
{
	delete m_configDialog;
	delete m_downloadMgr;
}

void MeteorShowersMgr::init()
{
	loadTextures();

	m_meteorShowers = new MeteorShowers(this);
	m_configDialog = new MSConfigDialog(this);

	createActions();
	createToolbarButtons();
	loadConfig();

	// timer to hide the alert messages
	m_messageTimer = new QTimer(this);
	m_messageTimer->setSingleShot(true);
	m_messageTimer->setInterval(9000);
	m_messageTimer->stop();
	connect(m_messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// MeteorShowers directory
	QString userDir = StelFileMgr::getUserDir() + "/modules/MeteorShowers";
	StelFileMgr::makeSureDirExistsAndIsWritable(userDir);

	// Loads the JSON catalog
	m_catalogPath = userDir + "/showers.json";
	if (!loadCatalog(m_catalogPath))
	{
		displayMessage(q_("The current catalog of Meteor Showers is invalid!"), "#bb0000");
		restoreDefaultCatalog(m_catalogPath);
	}

	// Sets up the download manager
	m_downloadMgr = new QNetworkAccessManager(this);
	connect(m_downloadMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(updateFinished(QNetworkReply*)));

	// every 5 min, check if it's time to update
	QTimer* updateTimer = new QTimer(this);
	updateTimer->setInterval(300000);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdates()));
	updateTimer->start();

	// enable at startup?
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	StelAction* action = actionMgr->findAction("actionShow_MeteorShowers");
	action->setChecked(getEnableAtStartup());
}

void MeteorShowersMgr::deinit()
{
	m_bolideTexture.clear();
	m_radiantTexture.clear();
	m_pointerTexture.clear();
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
	addAction("actionShow_MeteorShowers",               msGroup, N_("Toogle Meteor Showers"), this,           "enablePlugin", "Ctrl+Alt+M");
	addAction("actionShow_MeteorShowers_labels",        msGroup, N_("Radiant Labels"),        this,           "enableLabels", "Shift+M");
	addAction("actionShow_MeteorShowers_config_dialog", msGroup, N_("Show Settings Dialog"),  m_configDialog, "visible",      "Ctrl+Shift+M");
}

void MeteorShowersMgr::createToolbarButtons()
{
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui)
		{
			m_toolbarEnableButton = new StelButton(NULL,
							       QPixmap(":/MeteorShowers/btMS-on.png"),
							       QPixmap(":/MeteorShowers/btMS-off.png"),
							       QPixmap(":/graphicGui/glow32x32.png"),
							       "actionShow_MeteorShowers");
			gui->getButtonBar()->addButton(m_toolbarEnableButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "MeteorShowersMgr : unable to create toolbar buttons for MeteorShowers plugin!"
			   << e.what();
	}
}

void MeteorShowersMgr::loadConfig()
{
	setActiveRadiantOnly(m_conf->value(MS_CONFIG_PREFIX + "/flag_active_radiant_only", true).toBool());
	setEnableButtons(m_conf->value(MS_CONFIG_PREFIX + "/flag_buttons", true).toBool());
	setColorARG(StelUtils::strToVec3f(m_conf->value(MS_CONFIG_PREFIX + "/colorARG", "0,255,240").toString()));
	setColorARR(StelUtils::strToVec3f(m_conf->value(MS_CONFIG_PREFIX + "/colorARR", "255,240,0").toString()));
	setColorIR(StelUtils::strToVec3f(m_conf->value(MS_CONFIG_PREFIX + "/colorIR", "255,255,255").toString()));
	setEnableAtStartup(m_conf->value(MS_CONFIG_PREFIX + "/enable_at_startup", true).toBool());
	setFontSize(m_conf->value(MS_CONFIG_PREFIX + "/font_size", 13).toInt());
	setEnableLabels(m_conf->value(MS_CONFIG_PREFIX + "/flag_radiant_labels", true).toBool());
	setEnableMarker(m_conf->value(MS_CONFIG_PREFIX + "/flag_radiant_marker", true).toBool());
	setUpdateFrequencyHours(m_conf->value(MS_CONFIG_PREFIX + "/update_frequency_hours", 720).toInt());
	setEnableUpdates(m_conf->value(MS_CONFIG_PREFIX + "/updates_enabled", true).toBool());
	setUrl(m_conf->value(MS_CONFIG_PREFIX + "/url", "http://stellarium.org/json/showers.json").toString());
	setLastUpdate(m_conf->value(MS_CONFIG_PREFIX + "/last_update", "2015-07-01T00:00:00").toDateTime());
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
	qDebug() << "MeteorShowersMgr: Loading catalog file:"
		 << QDir::toNativeSeparators(jsonPath);

	QFile jsonFile(jsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "MeteorShowersMgr: Cannot to open the catalog file!";
		return false;
	}

	QJsonObject json(QJsonDocument::fromJson(jsonFile.readAll()).object());
	jsonFile.close();

	if (json["shortName"].toString() != "meteor showers data"
		|| json["version"].toInt() != MS_CATALOG_VERSION)
	{
		qWarning()  << "MeteorShowersMgr: The current catalog is not compatible!";
		return false;
	}

	QVariantMap map = json["showers"].toObject().toVariantMap();
	m_meteorShowers->loadMeteorShowers(map);

	return true;
}

void MeteorShowersMgr::restoreDefaultSettings()
{
	qDebug() << "MeteorShowersMgr: Restoring default settings";
	m_conf->beginGroup(MS_CONFIG_PREFIX);
	m_conf->remove("");
	m_conf->endGroup();
	loadConfig();
}

bool MeteorShowersMgr::restoreDefaultCatalog(const QString& destination)
{
	qDebug() << "MeteorShowersMgr: Trying to restore the default catalog to"
		 << QDir::toNativeSeparators(destination);

	if (!QFile(destination).remove())
	{
		qWarning() << "MeteorShowersMgr: Cannot remove the current catalog file!";
		return false;
	}

	QFile defaultJson(":/MeteorShowers/showers.json");
	if (!defaultJson.copy(destination))
	{
		qWarning() << "MeteorShowersMgr: Cannot copy the default catalog!";
		return false;
	}

	// The resource is read only, and the new file inherits this... make sure the
	// new file is writable by the Stellarium process so that updates can be done.
	QFile dest(destination);
	dest.setPermissions(dest.permissions() | QFile::WriteOwner);

	setLastUpdate(QDateTime::fromString("2015-07-01T00:00:00"));

	qDebug() << "MeteorShowersMgr: The default catalog was copied!";
	displayMessage(q_("Using the default Meteor Showers catalog."), "#bb0000");

	return true;
}

void MeteorShowersMgr::update(double deltaTime)
{
	if (!m_enablePlugin)
	{
		return;
	}

	StelCore* core = StelApp::getInstance().getCore();

	double tspeed = core->getTimeRate() * 86400.0;  // sky seconds per actual second
	if (!tspeed) { // is paused?
		return; // freeze meteors at the current position
	}

	deltaTime *= 1000.0;

	// if stellarium has been suspended, don't create huge number of meteors to
	// make up for lost time!
	if (deltaTime > 500.0)
	{
		deltaTime = 500.0;
	}

/* TODO
	// is GUI visible? refresh dates
	if (m_configDialog->visible())
	{
		m_configDialog->refreshRangeDates(core);
	}
*/
	m_meteorShowers->update(core, deltaTime);
}

void MeteorShowersMgr::draw(StelCore* core)
{
	if (m_enablePlugin)
	{
		m_meteorShowers->draw(core);
	}
}

void MeteorShowersMgr::checkForUpdates()
{
	if (m_enableUpdates && m_lastUpdate.addSecs(m_updateFrequencyHours * 3600.) <= QDateTime::currentDateTime())
	{
		updateCatalog();
	}
}

void MeteorShowersMgr::updateCatalog()
{
	if (m_isUpdating)
	{
		qWarning() << "MeteorShowersMgr: The catalog is being updated already!";
		return;
	}

	qDebug() << "MeteorShowersMgr: Starting to update the catalog...";
	m_isUpdating = true;

	setLastUpdate(QDateTime::currentDateTime());

	m_progressBar = StelApp::getInstance().addProgressBar();
	m_progressBar->setValue(0);
	m_progressBar->setRange(0, 100);
	m_progressBar->setFormat("Meteor Showers Catalog");

	QNetworkRequest request;
	request.setUrl(QUrl(m_url));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Meteor Showers Plugin %1; http://stellarium.org/)").arg(METEORSHOWERS_PLUGIN_VERSION).toUtf8());
	m_downloadMgr->get(request);
}

void MeteorShowersMgr::updateFinished(QNetworkReply* reply)
{
	if (m_progressBar)
	{
		m_progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(m_progressBar);
		m_progressBar = NULL;
	}

	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "MeteorShowersMgr: Failed to download!" << reply->url();
		qWarning() << "MeteorShowersMgr: Error " << reply->errorString();
		emit(failedToUpdate());
		m_isUpdating = false;
		return;
	}

	QString tempPath(m_catalogPath + "_new.json");
	QFile newCatalog(tempPath);
	newCatalog.remove(); // always overwrites
	if (!newCatalog.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		qWarning() << "MeteorShowersMgr: Cannot write the downloaded catalog!";
		emit(failedToUpdate());
		m_isUpdating = false;
		return;
	}

	newCatalog.write(reply->readAll());
	newCatalog.close();

	if (!loadCatalog(tempPath))
	{
		emit(failedToUpdate());
		m_isUpdating = false;
		return;
	}

	QFile(m_catalogPath).remove();
	newCatalog.rename(tempPath.remove("_new.json"));

	qDebug() << "MeteorShowersMgr: The catalog was updated!";
	m_isUpdating = false;
	emit(updated());
}

void MeteorShowersMgr::setActiveRadiantOnly(const bool& b)
{
	m_activeRadiantOnly = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/flag_active_radiant_only", b);
}

void MeteorShowersMgr::setEnableButtons(const bool& b)
{
	if (b)
	{
		m_toolbarEnableButton->show();
	}
	else
	{
		m_toolbarEnableButton->hide();
	}
	m_conf->setValue(MS_CONFIG_PREFIX + "/flag_buttons", b);
}

void MeteorShowersMgr::setColorARG(const Vec3f& rgb)
{
	m_colorARG = rgb;
	QString rgbStr = QString("%1,%2,%3").arg(rgb[0]).arg(rgb[1]).arg(rgb[2]);
	m_conf->setValue(MS_CONFIG_PREFIX + "/colorARG", rgbStr);
}

void MeteorShowersMgr::setColorARR(const Vec3f& rgb)
{
	m_colorARR = rgb;
	QString rgbStr = QString("%1,%2,%3").arg(rgb[0]).arg(rgb[1]).arg(rgb[2]);
	m_conf->setValue(MS_CONFIG_PREFIX + "/colorARR", rgbStr);
}

void MeteorShowersMgr::setColorIR(const Vec3f& rgb)
{
	m_colorIR = rgb;
	QString rgbStr = QString("%1,%2,%3").arg(rgb[0]).arg(rgb[1]).arg(rgb[2]);
	m_conf->setValue(MS_CONFIG_PREFIX + "/colorIR", rgbStr);
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
	m_enableLabels = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/flag_radiant_labels", b);
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

void MeteorShowersMgr::setEnableUpdates(const bool& b)
{
	m_enableUpdates = b;
	m_conf->setValue(MS_CONFIG_PREFIX + "/updates_enabled", b);
}

void MeteorShowersMgr::setUrl(const QString& url)
{
	m_url = url;
	m_conf->setValue(MS_CONFIG_PREFIX + "/url", url);
}

void MeteorShowersMgr::setLastUpdate(const QDateTime &datetime)
{
	m_lastUpdate = datetime;
	m_conf->setValue(MS_CONFIG_PREFIX + "/last_update",
			 m_lastUpdate.toString(Qt::ISODate));
}

QDateTime MeteorShowersMgr::getNextUpdate()
{
	return m_lastUpdate.addSecs(m_updateFrequencyHours * 3600);
}

void MeteorShowersMgr::displayMessage(const QString& message, const QString hexColor)
{
	m_messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20 * m_messageIDs.count()), true, 16, hexColor);
	m_messageTimer->start();
}

void MeteorShowersMgr::messageTimeout()
{
	foreach(int i, m_messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}

void MeteorShowersMgr::translations()
{
#if 0
	// Meteor showers
	// TRANSLATORS: Name of meteor shower
	N_("Quadrantids");
	// TRANSLATORS: Name of meteor shower
	N_("Lyrids");
	// TRANSLATORS: Name of meteor shower
	N_("α-Centaurids");
	// TRANSLATORS: Name of meteor shower
	N_("γ-Normids");
	// TRANSLATORS: Name of meteor shower
	N_("η-Aquariids");
	// TRANSLATORS: Name of meteor shower
	N_("June Bootids");
	// TRANSLATORS: Name of meteor shower
	N_("Piscis Austrinids");
	// TRANSLATORS: Name of meteor shower
	N_("Southern δ-Aquariids");
	// TRANSLATORS: Name of meteor shower
	N_("α-Capricornids");
	// TRANSLATORS: Name of meteor shower
	N_("α-Aurigids");
	// TRANSLATORS: Name of meteor shower
	N_("September ε-Perseids");
	// TRANSLATORS: Name of meteor shower
	N_("Draconids");
	// TRANSLATORS: Name of meteor shower
	N_("Leonids");
	// TRANSLATORS: Name of meteor shower
	N_("Phoenicids");
	// TRANSLATORS: Name of meteor shower
	N_("Puppid-Velids");
	// TRANSLATORS: Name of meteor shower
	N_("Ursids");
	// TRANSLATORS: Name of meteor shower
	N_("Perseids");
	// TRANSLATORS: Name of meteor shower
	N_("δ-Leonids");
	// TRANSLATORS: Name of meteor shower
	N_("π-Puppids");
	// TRANSLATORS: Name of meteor shower
	N_("June Lyrids");
	// TRANSLATORS: Name of meteor shower
	N_("κ-Cygnids");
	// TRANSLATORS: Name of meteor shower
	N_("ε-Lyrids");
	// TRANSLATORS: Name of meteor shower
	N_("δ-Aurigids");
	// TRANSLATORS: Name of meteor shower
	N_("ε-Geminids");
	// TRANSLATORS: Name of meteor shower
	N_("Southern Taurids");
	// TRANSLATORS: Name of meteor shower
	N_("Northern Taurids");
	// TRANSLATORS: Name of meteor shower
	N_("Monocerotids");
	// TRANSLATORS: Name of meteor shower
	N_("α-Monocerotids");
	// TRANSLATORS: Name of meteor shower
	N_("σ-Hydrids");
	// TRANSLATORS: Name of meteor shower
	N_("Geminids");
	// TRANSLATORS: Name of meteor shower
	N_("Leonis Minorids");
	// TRANSLATORS: Name of meteor shower
	N_("December Leonis Minorids");
	// TRANSLATORS: Name of meteor shower
	N_("Comae Berenicids");
	// TRANSLATORS: Name of meteor shower
	N_("Orionids");
	// TRANSLATORS: Name of meteor shower
	N_("Andromedids");
	// TRANSLATORS: Name of meteor shower
	N_("η-Lyrids");
	// TRANSLATORS: Name of meteor shower
	N_("α-Scorpiids");
	// TRANSLATORS: Name of meteor shower
	N_("Ophiuchids");
	// TRANSLATORS: Name of meteor shower
	N_("θ-Ophiuchids");
	// TRANSLATORS: Name of meteor shower
	N_("κ-Serpentids");
	// TRANSLATORS: Name of meteor shower
	N_("θ-Centaurids");
	// TRANSLATORS: Name of meteor shower
	N_("ω-Cetids");
	// TRANSLATORS: Name of meteor shower
	N_("Southern ω-Scorpiids");
	// TRANSLATORS: Name of meteor shower
	N_("Northern ω-Scorpiids");
	// TRANSLATORS: Name of meteor shower
	N_("Arietids");
	// TRANSLATORS: Name of meteor shower
	N_("π-Cetids");
	// TRANSLATORS: Name of meteor shower
	N_("δ-Cancrids");
	// TRANSLATORS: Name of meteor shower
	N_("τ-Herculids");
	// TRANSLATORS: Name of meteor shower
	N_("ρ-Geminids");
	// TRANSLATORS: Name of meteor shower
	N_("η-Carinids");
	// TRANSLATORS: Name of meteor shower
	N_("η-Craterids");
	// TRANSLATORS: Name of meteor shower
	N_("π-Virginids");
	// TRANSLATORS: Name of meteor shower
	N_("θ-Virginids");
	// TRANSLATORS: Name of meteor shower
	N_("May Librids");
	// TRANSLATORS: Name of meteor shower
	N_("June Scutids");
	// TRANSLATORS: Name of meteor shower
	N_("α-Pisces Australids");
	// TRANSLATORS: Name of meteor shower
	N_("Southern ι-Aquariids");
	// TRANSLATORS: Name of meteor shower
	N_("Northern ι-Aquariids");
	// TRANSLATORS: Name of meteor shower
	N_("γ-Aquariids");
	// TRANSLATORS: Name of meteor shower
	N_("Autumn Arietids");
	// TRANSLATORS: Name of meteor shower
	N_("χ-Orionids");

	// List of parent objects for meteor showers
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 1P/Halley");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 7P/Pons-Winnecke");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 55P/Tempel-Tuttle");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 96P/Machholz");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 109P/Swift-Tuttle");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet Thatcher (1861 I)");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 26P/Grigg-Skjellerup");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 21P/Giacobini-Zinner");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 169P/NEAT");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 289P/Blanpain");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 8P/Tuttle");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 3D/Biela");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet C/1917 F1 (Mellish)");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet C/1964 N1 (Ikeya)");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet Schwassmann-Wachmann 3");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Minor planet 2003 EH1 and Comet C/1490 Y1");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Minor planet (4450) Pan");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Minor planet 2008 ED69");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 2P/Encke");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Minor planet 2004 TG10");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Minor planet (3200) Phaethon");

	/* For copy/paste:
	// TRANSLATORS: Name of meteor shower
	N_("");
	*/

#endif
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
	info.contact = "mcardinot@gmail.com";
	info.description = N_("This plugin displays meteor showers and a marker"
			      " for each active and inactive radiant, showing"
			      " real information about its activity.");
	info.version = METEORSHOWERS_PLUGIN_VERSION;
	return info;
}
