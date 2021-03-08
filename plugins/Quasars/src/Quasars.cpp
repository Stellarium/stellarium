/*
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


#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "LabelMgr.hpp"
#include "Quasar.hpp"
#include "Quasars.hpp"
#include "QuasarsDialog.hpp"
#include "StelProgressController.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QVariantMap>
#include <QVariant>
#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QDir>
#include <QSettings>
#include <stdexcept>

#define CATALOG_FORMAT_VERSION 1 /* Version of format of catalog */

/*
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*/
StelModule* QuasarsStelPluginInterface::getStelModule() const
{
	return new Quasars();
}

StelPluginInfo QuasarsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Quasars);

	StelPluginInfo info;
	info.id = "Quasars";
	info.displayedName = N_("Quasars");
	info.authors = "Alexander Wolf";
	info.contact = "https://github.com/Stellarium/stellarium";
	info.description = N_("A plugin that shows some quasars brighter than visual magnitude 18. The catalogue of quasars was compiled from 'Quasars and Active Galactic Nuclei' (13th Ed.) (Veron+ 2010)");
	info.version = QUASARS_PLUGIN_VERSION;
	info.license = QUASARS_PLUGIN_LICENSE;
	return info;
}

/*
 Constructor
*/
Quasars::Quasars()
	: QsrCount(0)
	, updateState(CompleteNoUpdates)
	, networkManager(Q_NULLPTR)
	, downloadReply(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
	, messageTimer(Q_NULLPTR)
	, updatesEnabled(false)
	, updateFrequencyDays(0)
	, enableAtStartup(false)
	, flagShowQuasars(false)
	, flagShowQuasarsButton(false)
	, OnIcon(Q_NULLPTR)
	, OffIcon(Q_NULLPTR)
	, GlowIcon(Q_NULLPTR)
	, toolbarButton(Q_NULLPTR)
	, progressBar(Q_NULLPTR)

{
	setObjectName("Quasars");
	configDialog = new QuasarsDialog();
	conf = StelApp::getInstance().getSettings();
	setFontSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
}

/*
 Destructor
*/
Quasars::~Quasars()
{
	delete configDialog;

	if (GlowIcon)
		delete GlowIcon;
	if (OnIcon)
		delete OnIcon;
	if (OffIcon)
		delete OffIcon;
}

void Quasars::deinit()
{
	QSO.clear();
	Quasar::markerTexture.clear();
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Quasars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}

/*
 Init our module
*/
void Quasars::init()
{
	upgradeConfigIni();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Quasars");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Quasars"))
		{
			qDebug() << "[Quasars] No Quasars section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		catalogJsonPath = StelFileMgr::findFile("modules/Quasars", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable)) + "/quasars.json";
		if (catalogJsonPath.isEmpty())
			return;

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");
		Quasar::markerTexture = StelApp::getInstance().getTextureManager().createTexture(":/Quasars/quasar.png");

		// key bindings and other actions
		addAction("actionShow_Quasars", N_("Quasars"), N_("Show quasars"), "quasarsVisible", "Ctrl+Alt+Q");
		addAction("actionShow_Quasars_ConfigDialog", N_("Quasars"), N_("Quasars configuration window"), configDialog, "visible", ""); // Allow assign shortkey

		GlowIcon = new QPixmap(":/graphicGui/miscGlow32x32.png");
		OnIcon = new QPixmap(":/Quasars/btQuasars-on.png");
		OffIcon = new QPixmap(":/Quasars/btQuasars-off.png");

		setFlagShowQuasars(getEnableAtStartup());
		setFlagShowQuasarsButton(flagShowQuasarsButton);
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[Quasars] init error:" << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(catalogJsonPath).exists())
	{
		if (!checkJsonFileFormat() || getJsonFileFormatVersion()<CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "[Quasars] quasars.json does not exist - copying default file to" << QDir::toNativeSeparators(catalogJsonPath);
		restoreDefaultJsonFile();
	}

	qDebug() << "[Quasars] Loading catalog file:" << QDir::toNativeSeparators(catalogJsonPath);

	readJsonFile();

	// Set up download manager and the update schedule
	networkManager = StelApp::getInstance().getNetworkAccessManager();
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	connect(this, SIGNAL(jsonUpdateComplete(void)), this, SLOT(reloadCatalog()));
	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first QSO in the main window
*/
void Quasars::draw(StelCore* core)
{
	if (!flagShowQuasars)
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);

	for (const auto& quasar : QSO)
	{
		if (quasar && quasar->initialized)
			quasar->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
}

void Quasars::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Quasar");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.setBlending(true);
		painter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

QList<StelObjectP> Quasars::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	if (!flagShowQuasars)
		return result;

	Vec3d v(av);
	v.normalize();
	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& quasar : QSO)
	{
		if (quasar->initialized)
		{
			equPos = quasar->XYZ;
			equPos.normalize();
			if (equPos.dot(v) >= cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(quasar));
			}
		}
	}

	return result;
}

StelObjectP Quasars::searchByName(const QString& englishName) const
{
	if (!flagShowQuasars)
		return Q_NULLPTR;

	for (const auto& quasar : QSO)
	{
		if (quasar->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(quasar);
	}

	return Q_NULLPTR;
}

StelObjectP Quasars::searchByNameI18n(const QString& nameI18n) const
{
	if (!flagShowQuasars)
		return Q_NULLPTR;

	for (const auto& quasar : QSO)
	{
		if (quasar->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(quasar);
	}

	return Q_NULLPTR;
}

QStringList Quasars::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (flagShowQuasars)
		result = StelObjectModule::listMatchingObjects(objPrefix, maxNbItem, useStartOfWords);
	return result;
}


QStringList Quasars::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (!flagShowQuasars)
		return result;

	if (inEnglish)
	{
		for (const auto& quasar : QSO)
		{
			result << quasar->getEnglishName();
		}
	}
	else
	{
		for (const auto& quasar : QSO)
		{
			result << quasar->getNameI18n();
		}
	}
	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Quasars::restoreDefaultJsonFile(void)
{
	if (QFileInfo(catalogJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/Quasars/quasars.json");
	if (!src.copy(catalogJsonPath))
	{
		qWarning() << "[Quasars] Cannot copy json resource to" + QDir::toNativeSeparators(catalogJsonPath);
	}
	else
	{
		qDebug() << "[Quasars] Copied default quasars.json to" << QDir::toNativeSeparators(catalogJsonPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(catalogJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("Quasars/last_update");
		lastUpdate = QDateTime::fromString("2012-05-24T12:00:00", Qt::ISODate);
	}
}

/*
  Creates a backup of the quasars.json file called quasars.json.old
*/
bool Quasars::backupJsonFile(bool deleteOriginal)
{
	QFile old(catalogJsonPath);
	if (!old.exists())
	{
		qWarning() << "[Quasars] No file to backup";
		return false;
	}

	QString backupPath = catalogJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "[Quasars] WARNING - could not remove old quasars.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "[Quasars] WARNING - failed to copy quasars.json to quasars.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of quasars.
*/
void Quasars::readJsonFile(void)
{
	setQSOMap(loadQSOMap());
}

/*
  Parse JSON file and load quasarss to map
*/
QVariantMap Quasars::loadQSOMap(QString path)
{
	if (path.isEmpty())
	    path = catalogJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[Quasars] Cannot open" << QDir::toNativeSeparators(path);
	else
	{
		try
		{
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qDebug() << "[Quasars] File format is wrong! Error: " << e.what();
			return QVariantMap();
		}
	}
	return map;
}

/*
  Set items for list of struct from data map
*/
void Quasars::setQSOMap(const QVariantMap& map)
{
	QSO.clear();
	QsrCount = 0;
	QVariantMap qsoMap = map.value("quasars").toMap();
	for (auto qsoKey : qsoMap.keys())
	{
		QVariantMap qsoData = qsoMap.value(qsoKey).toMap();
		qsoData["designation"] = qsoKey;

		QsrCount++;

		QuasarP quasar(new Quasar(qsoData));
		if (quasar->initialized)
			QSO.append(quasar);
	}
}

int Quasars::getJsonFileFormatVersion(void)
{
	int jsonVersion = -1;
	QFile catalogJsonFile(catalogJsonPath);
	if (!catalogJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Quasars] Cannot open" << QDir::toNativeSeparators(catalogJsonPath);
		return jsonVersion;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&catalogJsonFile).toMap();
		catalogJsonFile.close();
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "[Quasars] File format is wrong! Error: " << e.what();
		return jsonVersion;
	}
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}
	qDebug() << "[Quasars] Version of the format of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool Quasars::checkJsonFileFormat()
{
	QFile catalogJsonFile(catalogJsonPath);
	if (!catalogJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Quasars] Cannot open" << QDir::toNativeSeparators(catalogJsonPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&catalogJsonFile).toMap();
		catalogJsonFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "[Quasars] File format is wrong! Error:" << e.what();
		return false;
	}

	return true;
}

QuasarP Quasars::getByID(const QString& id) const
{
	for (const auto& quasar : QSO)
	{
		if (quasar->initialized && quasar->designation == id)
			return quasar;
	}
	return QuasarP();
}

bool Quasars::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Quasars::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void Quasars::restoreDefaultConfigIni(void)
{
	conf->beginGroup("Quasars");

	// delete all existing Quasars settings...
	conf->remove("");

	conf->setValue("distribution_enabled", false);
	conf->setValue("enable_at_startup", false);
	conf->setValue("updates_enabled", true);
	conf->setValue("url", "https://stellarium.org/json/quasars.json");
	conf->setValue("update_frequency_days", 100);
	conf->setValue("flag_show_quasars_button", true);
	conf->setValue("marker_color", "1.0,0.5,0.4");
	conf->endGroup();
}

void Quasars::readSettingsFromConfig(void)
{
	conf->beginGroup("Quasars");

	updateUrl = conf->value("url", "https://stellarium.org/json/quasars.json").toString();
	updateFrequencyDays = conf->value("update_frequency_days", 100).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-05-24T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	setDisplayMode(conf->value("distribution_enabled", false).toBool());
	setFlagUseQuasarMarkers(conf->value("flag_use_markers", false).toBool());
	setMarkerColor(Vec3f(conf->value("marker_color", "1.0,0.5,0.4").toString()));
	enableAtStartup = conf->value("enable_at_startup", false).toBool();
	flagShowQuasarsButton = conf->value("flag_show_quasars_button", true).toBool();

	conf->endGroup();
}

void Quasars::saveSettingsToConfig(void)
{
	conf->beginGroup("Quasars");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_days", updateFrequencyDays);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("distribution_enabled", getDisplayMode());
	conf->setValue("enable_at_startup", enableAtStartup);
	conf->setValue("flag_show_quasars_button", getFlagShowQuasarsButton());
	conf->setValue("flag_use_markers", getFlagUseQuasarMarkers());
	conf->setValue("marker_color", getMarkerColor().toStr());

	conf->endGroup();
}

int Quasars::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyDays * 3600 * 24);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Quasars::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyDays * 3600 * 24) <= QDateTime::currentDateTime() && networkManager->networkAccessible()==QNetworkAccessManager::Accessible)
		updateJSON();
}

void Quasars::updateJSON(void)
{
	if (updateState==Quasars::Updating)
	{
		qWarning() << "[Quasars] Already updating...  will not start again current update is complete.";
		return;
	}

	qDebug() << "[Quasars] Updating quasars catalog...";
	startDownload(updateUrl);
}

void Quasars::deleteDownloadProgressBar()
{
	disconnect(this, SLOT(updateDownloadProgress(qint64,qint64)));

	if (progressBar)
	{
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
}

void Quasars::startDownload(QString urlString)
{
	QUrl url(urlString);
	if (!url.isValid() || url.isRelative() || !url.scheme().startsWith("http", Qt::CaseInsensitive))
	{
		qWarning() << "[Quasars] Invalid URL:" << urlString;
		return;
	}

	if (progressBar == Q_NULLPTR)
		progressBar = StelApp::getInstance().addProgressBar();
	progressBar->setValue(0);
	progressBar->setRange(0, 0);

	connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	downloadReply = networkManager->get(request);
	connect(downloadReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)));

	updateState = Quasars::Updating;
	emit(updateStateChanged(updateState));
}

void Quasars::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (progressBar == Q_NULLPTR)
		return;

	int currentValue = 0;
	int endValue = 0;

	if (bytesTotal > -1 && bytesReceived <= bytesTotal)
	{
		//Round to the greatest possible derived unit
		while (bytesTotal > 1024)
		{
			bytesReceived = std::floor(bytesReceived / 1024.);
			bytesTotal    = std::floor(bytesTotal / 1024.);
		}
		currentValue = bytesReceived;
		endValue = bytesTotal;
	}

	progressBar->setValue(currentValue);
	progressBar->setRange(0, endValue);
}

void Quasars::downloadComplete(QNetworkReply *reply)
{
	if (reply == Q_NULLPTR)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "[Quasars] Download error: While trying to access"
			   << reply->url().toString()
			   << "the following error occured:"
			   << reply->errorString();

		reply->deleteLater();
		downloadReply = Q_NULLPTR;
		return;
	}

	// download completed successfully.
	try
	{
		QString jsonFilePath = StelFileMgr::findFile("modules/Quasars", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/quasars.json";
		QFile jsonFile(jsonFilePath);
		if (jsonFile.exists())
			jsonFile.remove();

		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}

		updateState = Quasars::CompleteUpdates;

		lastUpdate = QDateTime::currentDateTime();
		conf->setValue("Quasars/last_update", lastUpdate.toString(Qt::ISODate));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[Quasars] Cannot write JSON data to file:" << e.what();
		updateState = Quasars::DownloadError;
	}

	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());

	reply->deleteLater();
	downloadReply = Q_NULLPTR;

	readJsonFile();
}

void Quasars::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor, false, 9000);
}

void Quasars::upgradeConfigIni(void)
{
	// Upgrade settings for Quasars plugin
	if (conf->contains("Quasars/flag_show_quasars"))
	{
		bool b = conf->value("Quasars/flag_show_quasars", false).toBool();
		if (!conf->contains("Quasars/enable_at_startup"))
			conf->setValue("Quasars/enable_at_startup", b);
		conf->remove("Quasars/flag_show_quasars");
	}
}

// Define whether the button toggling quasars should be visible
void Quasars::setFlagShowQuasarsButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=Q_NULLPTR)
	{
		if (b==true) {
			if (toolbarButton==Q_NULLPTR) {
				// Create the quasars button
				toolbarButton = new StelButton(Q_NULLPTR, *OnIcon, *OffIcon, *GlowIcon, "actionShow_Quasars");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Quasars");
		}
	}
	flagShowQuasarsButton = b;
}

bool Quasars::getDisplayMode()
{
	return Quasar::distributionMode;
}

void Quasars::setDisplayMode(bool b)
{
	Quasar::distributionMode=b;
}

bool Quasars::getFlagUseQuasarMarkers()
{
	return Quasar::useMarkers;
}

void Quasars::setFlagUseQuasarMarkers(bool b)
{
	Quasar::useMarkers = b;
}

Vec3f Quasars::getMarkerColor()
{
	return Quasar::markerColor;
}

void Quasars::setMarkerColor(const Vec3f &c)
{
	Quasar::markerColor = c;
	emit quasarsColorChanged(c);
}

void Quasars::reloadCatalog(void)
{
	bool hasSelection = false;
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	// Whether any quasar are selected? Save the current selection...
	const QList<StelObjectP> selectedObject = objMgr->getSelectedObject("Quasar");
	if (!selectedObject.isEmpty())
	{
		// ... unselect current quasar.
		hasSelection = true;
		objMgr->unSelect();
	}

	readJsonFile();

	if (hasSelection)
	{
		// Restore selection...
		objMgr->setSelectedObject(selectedObject);
	}
}

void Quasars::setFlagShowQuasars(bool b)
{
	if (b!=flagShowQuasars)
	{
		flagShowQuasars=b;
		emit flagQuasarsVisibilityChanged(b);
		emit StelApp::getInstance().getCore()->updateSearchLists();
	}
}
