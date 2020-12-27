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
#include "Supernova.hpp"
#include "Supernovae.hpp"
#include "SupernovaeDialog.hpp"
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
StelModule* SupernovaeStelPluginInterface::getStelModule() const
{
	return new Supernovae();
}

StelPluginInfo SupernovaeStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Supernovae);

	StelPluginInfo info;
	info.id = "Supernovae";
	info.displayedName = N_("Historical Supernovae");
	info.authors = "Alexander Wolf";
	info.contact = "https://github.com/Stellarium/stellarium";
	info.description = N_("This plugin allows you to see some bright historical supernovae.");
	info.version = SUPERNOVAE_PLUGIN_VERSION;
	info.license = SUPERNOVAE_PLUGIN_LICENSE;
	return info;
}

/*
 Constructor
*/
Supernovae::Supernovae()
	: SNCount(0)
	, updateState(CompleteNoUpdates)
	, networkManager(Q_NULLPTR)
	, downloadReply(Q_NULLPTR)
	, progressBar(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
	, updatesEnabled(false)
	, updateFrequencyDays(0)
{
	setObjectName("Supernovae");
	configDialog = new SupernovaeDialog();
	conf = StelApp::getInstance().getSettings();
	setFontSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
}

/*
 Destructor
*/
Supernovae::~Supernovae()
{
	delete configDialog;
}

void Supernovae::deinit()
{
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Supernovae::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Supernovae::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Supernovae");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Supernovae"))
		{
			qDebug() << "[Supernovae] no Supernovae section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		sneJsonPath = StelFileMgr::findFile("modules/Supernovae", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable)) + "/supernovae.json";
		if (sneJsonPath.isEmpty())
			return;

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

		// key bindings and other actions
		addAction("actionShow_Supernovae_ConfigDialog", N_("Historical Supernovae"), N_("Historical Supernovae configuration window"), configDialog, "visible", ""); // Allow assign shortkey
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Supernovas: init error:" << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(sneJsonPath).exists())
	{
		if (!checkJsonFileFormat() || getJsonFileVersion()<CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "[Supernovae] supernovae.json does not exist - copying default file to" << QDir::toNativeSeparators(sneJsonPath);
		restoreDefaultJsonFile();
	}

	qDebug() << "[Supernovae] loading catalog file:" << QDir::toNativeSeparators(sneJsonPath);

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
 Draw our module. This should print name of first SNe in the main window
*/
void Supernovae::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	for (const auto& sn : snstar)
	{
		if (sn && sn->initialized)
			sn->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
}

void Supernovae::drawPointer(StelCore* core, StelPainter& painter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Supernova");
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
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]), static_cast<float>(screenpos[1]), 13.f, static_cast<float>(StelApp::getInstance().getTotalRunTime())*40.f);
	}
}

QList<StelObjectP> Supernovae::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& sn : snstar)
	{
		if (sn->initialized)
		{
			equPos = sn->XYZ;
			equPos.normalize();
			if (equPos.dot(v) >= cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(sn));
			}
		}
	}

	return result;
}

StelObjectP Supernovae::searchByName(const QString& englishName) const
{
	for (const auto& sn : snstar)
	{
		if (sn->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(sn);
	}

	return Q_NULLPTR;
}

StelObjectP Supernovae::searchByNameI18n(const QString& nameI18n) const
{
	for (const auto& sn : snstar)
	{
		if (sn->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(sn);
	}

	return Q_NULLPTR;
}

QStringList Supernovae::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		for (const auto& sn : snstar)
		{
			result << sn->getEnglishName();
		}
	}
	else
	{
		for (const auto& sn : snstar)
		{
			result << sn->getNameI18n();
		}
	}
	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Supernovae::restoreDefaultJsonFile(void)
{
	if (QFileInfo(sneJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/Supernovae/supernovae.json");
	if (!src.copy(sneJsonPath))
	{
		qWarning() << "[Supernovae] cannot copy JSON resource to" + QDir::toNativeSeparators(sneJsonPath);
	}
	else
	{
		qDebug() << "[Supernovae] copied default supernovae.json to" << QDir::toNativeSeparators(sneJsonPath);
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(sneJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("Supernovae/last_update");
		lastUpdate = QDateTime::fromString("2012-05-24T12:00:00", Qt::ISODate);
	}
}

/*
  Creates a backup of the supernovae.json file called supernovae.json.old
*/
bool Supernovae::backupJsonFile(bool deleteOriginal)
{
	QFile old(sneJsonPath);
	if (!old.exists())
	{
		qWarning() << "[Supernovae] no file to backup";
		return false;
	}

	QString backupPath = sneJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "[Supernovae] WARNING - could not remove old supernovas.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "[Supernovae] WARNING - failed to copy supernovae.json to supernovae.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of supernovaes.
*/
void Supernovae::readJsonFile(void)
{
	setSNeMap(loadSNeMap());
}

/*
  Parse JSON file and load supernovaes to map
*/
QVariantMap Supernovae::loadSNeMap(QString path)
{
	if (path.isEmpty())
	    path = sneJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "[Supernovae] cannot open" << QDir::toNativeSeparators(path);
	else
	{
		try
		{
			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qDebug() << "[Supernovae] File format is wrong! Error: " << e.what();
			return QVariantMap();
		}
	}
	return map;
}

/*
  Set items for list of struct from data map
*/
void Supernovae::setSNeMap(const QVariantMap& map)
{
	snstar.clear();
	snlist.clear();
	SNCount = 0;
	QVariantMap sneMap = map.value("supernova").toMap();
	for (auto sneKey : sneMap.keys())
	{
		QVariantMap sneData = sneMap.value(sneKey).toMap();
		sneData["designation"] = QString("SN %1").arg(sneKey);

		snlist.insert(sneData.value("designation").toString(), sneData.value("peakJD").toDouble());
		SNCount++;

		SupernovaP sn(new Supernova(sneData));
		if (sn->initialized)
			snstar.append(sn);
	}
}

int Supernovae::getJsonFileVersion(void) const
{	
	int jsonVersion = -1;
	QFile sneJsonFile(sneJsonPath);
	if (!sneJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Supernovae] cannot open" << QDir::toNativeSeparators(sneJsonPath);
		return jsonVersion;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&sneJsonFile).toMap();
		sneJsonFile.close();
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "[Supernovae] File format is wrong! Error: " << e.what();
		return jsonVersion;
	}
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	qDebug() << "[Supernovae] version of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool Supernovae::checkJsonFileFormat() const
{
	QFile sneJsonFile(sneJsonPath);
	if (!sneJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Supernovae] cannot open " << QDir::toNativeSeparators(sneJsonPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&sneJsonFile).toMap();
		sneJsonFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "[Supernovae] file format is wrong! Error:" << e.what();
		return false;
	}

	return true;
}

float Supernovae::getLowerLimitBrightness() const
{
	float lowerLimit = 10.f;
	QFile sneJsonFile(sneJsonPath);
	if (!sneJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Supernovae] cannot open" << QDir::toNativeSeparators(sneJsonPath);
		return lowerLimit;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&sneJsonFile).toMap();
		sneJsonFile.close();
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "[Supernovae] File format is wrong! Error: " << e.what();
		return lowerLimit;
	}
	if (map.contains("limit"))
	{
		lowerLimit = map.value("limit").toFloat();
	}

	return lowerLimit;
}

SupernovaP Supernovae::getByID(const QString& id) const
{
	for (const auto& sn : snstar)
	{
		if (sn->initialized && sn->designation == id)
			return sn;
	}
	return SupernovaP();
}

bool Supernovae::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Supernovae::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void Supernovae::restoreDefaultConfigIni(void)
{
	conf->beginGroup("Supernovae");

	// delete all existing Supernovae settings...
	conf->remove("");

	conf->setValue("updates_enabled", true);
	conf->setValue("url", "https://stellarium.org/json/supernovae.json");
	conf->setValue("update_frequency_days", 100);
	conf->endGroup();
}

void Supernovae::readSettingsFromConfig(void)
{
	conf->beginGroup("Supernovae");

	updateUrl = conf->value("url", "https://stellarium.org/json/supernovae.json").toString();
	updateFrequencyDays = conf->value("update_frequency_days", 100).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-06-11T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();

	conf->endGroup();
}

void Supernovae::saveSettingsToConfig(void)
{
	conf->beginGroup("Supernovae");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_days", updateFrequencyDays);
	conf->setValue("updates_enabled", updatesEnabled );

	conf->endGroup();
}

int Supernovae::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyDays * 3600 * 24);
	return static_cast<int>(QDateTime::currentDateTime().secsTo(nextUpdate));
}

void Supernovae::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyDays * 3600 * 24) <= QDateTime::currentDateTime() && networkManager->networkAccessible()==QNetworkAccessManager::Accessible)
		updateJSON();
}

void Supernovae::updateJSON(void)
{
	if (updateState==Supernovae::Updating)
	{
		qWarning() << "[Supernovae] already updating...  will not start again current update is complete.";
		return;
	}

	qDebug() << "[Supernovae] Updating supernovae catalog...";
	startDownload(updateUrl);
}

void Supernovae::deleteDownloadProgressBar()
{
	disconnect(this, SLOT(updateDownloadProgress(qint64,qint64)));

	if (progressBar)
	{
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
}

void Supernovae::startDownload(QString urlString)
{
	QUrl url(urlString);
	if (!url.isValid() || url.isRelative() || !url.scheme().startsWith("http", Qt::CaseInsensitive))
	{
		qWarning() << "[Supernovae] Invalid URL:" << urlString;
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

	updateState = Supernovae::Updating;
	emit(updateStateChanged(updateState));
}

void Supernovae::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (progressBar == Q_NULLPTR)
		return;

	qint64 currentValue = 0;
	qint64 endValue = 0;

	if (bytesTotal > -1 && bytesReceived <= bytesTotal)
	{
		//Round to the greatest possible derived unit
		while (bytesTotal > 1024)
		{
			bytesReceived = qRound(std::floor(bytesReceived / 1024.));
			bytesTotal    = qRound(std::floor(bytesTotal / 1024.));
		}
		currentValue = bytesReceived;
		endValue = bytesTotal;
	}

	progressBar->setValue(static_cast<int>(currentValue));
	progressBar->setRange(0, static_cast<int>(endValue));
}

void Supernovae::downloadComplete(QNetworkReply *reply)
{
	if (reply == Q_NULLPTR)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
	deleteDownloadProgressBar();

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "[Supernovae] Download error: While trying to access"
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
		QString jsonFilePath = StelFileMgr::findFile("modules/Supernovae", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/supernovae.json";
		QFile jsonFile(jsonFilePath);
		if (jsonFile.exists())
			jsonFile.remove();

		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}

		updateState = Supernovae::CompleteUpdates;

		lastUpdate = QDateTime::currentDateTime();
		conf->setValue("Supernovae/last_update", lastUpdate.toString(Qt::ISODate));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[Supernovae] Cannot write JSON data to file:" << e.what();
		updateState = Supernovae::DownloadError;
	}

	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());

	reply->deleteLater();
	downloadReply = Q_NULLPTR;

	readJsonFile();
}

void Supernovae::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor, false, 9000);
}

QString Supernovae::getSupernovaeList() const
{
	QStringList out;
	int year, month, day;
	QList<double> vals = snlist.values();
	std::sort(vals.begin(), vals.end());
	for (auto val : vals)
	{
		StelUtils::getDateFromJulianDay(val, &year, &month, &day);
		out << QString("%1 (%2 %3)").arg(snlist.key(val)).arg(day).arg(StelLocaleMgr::longGenitiveMonthName(month));
	}

	return out.join(", ");
}

void Supernovae::reloadCatalog(void)
{
	bool hasSelection = false;
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	// Whether any supernova are selected? Save the current selection...
	const QList<StelObjectP> selectedObject = objMgr->getSelectedObject("Supernova");
	if (!selectedObject.isEmpty())
	{
		// ... unselect current supernova.
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
