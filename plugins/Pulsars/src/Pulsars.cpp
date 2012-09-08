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

#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "LabelMgr.hpp"
#include "Pulsar.hpp"
#include "Pulsars.hpp"
#include "PulsarsDialog.hpp"
#include "renderer/StelRenderer.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QAction>
#include <QProgressBar>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QVariantMap>
#include <QVariant>
#include <QList>
#include <QSettings>
#include <QSharedPointer>
#include <QStringList>

#define CATALOG_FORMAT_VERSION 2 /* Version of format of catalog */

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* PulsarsStelPluginInterface::getStelModule() const
{
	return new Pulsars();
}

StelPluginInfo PulsarsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Pulsars);

	StelPluginInfo info;
	info.id = "Pulsars";
	info.displayedName = N_("Pulsars");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("This plugin plots the position of various pulsars, with object information about each one.");
	return info;
}

Q_EXPORT_PLUGIN2(Pulsars, PulsarsStelPluginInterface)


/*
 Constructor
*/
Pulsars::Pulsars()
	: texPointer(NULL)
	, markerTexture(NULL)
	, progressBar(NULL)
{
	setObjectName("Pulsars");
	configDialog = new PulsarsDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(conf->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Pulsars::~Pulsars()
{
	delete configDialog;
}

void Pulsars::deinit()
{
	psr.clear();
	if(NULL != markerTexture)
	{
		delete markerTexture;
	}
	if(NULL != texPointer)
	{
		delete texPointer;
	}
}

/*
 Reimplementation of the getCallOrder method
*/
double Pulsars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Pulsars::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Pulsars");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Pulsars"))
		{
			qDebug() << "Pulsars::init no Pulsars section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		jsonCatalogPath = StelFileMgr::findFile("modules/Pulsars", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/pulsars.json";

		// key bindings and other actions
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		connect(gui->getGuiAction("actionShow_Pulsars_ConfigDialog"), SIGNAL(toggled(bool)), configDialog, SLOT(setVisible(bool)));
		connect(configDialog, SIGNAL(visibleChanged(bool)), gui->getGuiAction("actionShow_Pulsars_ConfigDialog"), SLOT(setChecked(bool)));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Pulsars::init error: " << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(jsonCatalogPath).exists())
	{
		if (getJsonFileVersion() < CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Pulsars::init pulsars.json does not exist - copying default file to " << jsonCatalogPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Pulsars::init using pulsars.json file: " << jsonCatalogPath;

	readJsonFile();

	// Set up download manager and the update schedule
	downloadMgr = new QNetworkAccessManager(this);
	connect(downloadMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(updateDownloadComplete(QNetworkReply*)));
	updateState = CompleteNoUpdates;
	updateTimer = new QTimer(this);
	updateTimer->setSingleShot(false);   // recurring check for update
	updateTimer->setInterval(13000);     // check once every 13 seconds to see if it is time for an update
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
	updateTimer->start();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first PSR in the main window
*/
void Pulsars::draw(StelCore* core, StelRenderer* renderer)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	renderer->setFont(font);
	
	foreach (const PulsarP& pulsar, psr)
	{
		if (pulsar && pulsar->initialized)
		{
			if(NULL == markerTexture)
			{
				markerTexture = renderer->createTexture(":/Pulsars/pulsar.png");
			}
			pulsar->draw(core, renderer, prj, markerTexture);
		}
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, renderer, prj);
	}
}

void Pulsars::drawPointer(StelCore* core, StelRenderer* renderer, StelProjectorP projector)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Pulsar");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!projector->project(pos, screenpos))
		{
			 return;
		}

		const Vec3f& c(obj->getInfoColor());
		renderer->setGlobalColor(c[0], c[1], c[2]);
		if(NULL == texPointer)
		{
			texPointer = renderer->createTexture("textures/pointeur2.png");
		}
		texPointer->bind();
		renderer->setBlendMode(BlendMode_Alpha);
		renderer->drawTexturedRect(screenpos[0] - 13.0f, screenpos[1] - 13.0f, 26.0f, 26.0f,
		                           StelApp::getInstance().getTotalRunTime() * 40.0f);
	}
}

QList<StelObjectP> Pulsars::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->initialized)
		{
			equPos = pulsar->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(pulsar));
			}
		}
	}

	return result;
}

StelObjectP Pulsars::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(pulsar);
	}

	return NULL;
}

StelObjectP Pulsars::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(pulsar);
	}

	return NULL;
}

QStringList Pulsars::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << pulsar->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Pulsars::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach(const PulsarP& pulsar, psr)
		{
			result << pulsar->getEnglishName();
		}
	}
	else
	{
		foreach(const PulsarP& pulsar, psr)
		{
			result << pulsar->getNameI18n();
		}
	}
	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Pulsars::restoreDefaultJsonFile(void)
{
	if (QFileInfo(jsonCatalogPath).exists())
		backupJsonFile(true);

	QFile src(":/Pulsars/pulsars.json");
	if (!src.copy(jsonCatalogPath))
	{
		qWarning() << "Pulsars::restoreDefaultJsonFile cannot copy json resource to " + jsonCatalogPath;
	}
	else
	{
		qDebug() << "Pulsars::init copied default pulsars.json to " << jsonCatalogPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(jsonCatalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("Pulsars/last_update");
		lastUpdate = QDateTime::fromString("2012-05-24T12:00:00", Qt::ISODate);
	}
}

/*
  Creates a backup of the pulsars.json file called pulsars.json.old
*/
bool Pulsars::backupJsonFile(bool deleteOriginal)
{
	QFile old(jsonCatalogPath);
	if (!old.exists())
	{
		qWarning() << "Pulsars::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = jsonCatalogPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Pulsars::backupJsonFile WARNING - could not remove old pulsars.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Pulsars::backupJsonFile WARNING - failed to copy pulsars.json to pulsars.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of pulsars.
*/
void Pulsars::readJsonFile(void)
{
	setPSRMap(loadPSRMap());
}

/*
  Parse JSON file and load pulsars to map
*/
QVariantMap Pulsars::loadPSRMap(QString path)
{
	if (path.isEmpty())
	    path = jsonCatalogPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "Pulsars::loadPSRMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Pulsars::setPSRMap(const QVariantMap& map)
{
	psr.clear();
	QVariantMap psrMap = map.value("pulsars").toMap();
	foreach(QString psrKey, psrMap.keys())
	{
		QVariantMap psrData = psrMap.value(psrKey).toMap();
		psrData["designation"] = psrKey;

		PulsarP pulsar(new Pulsar(psrData));
		if (pulsar->initialized)
			psr.append(pulsar);

	}
}

int Pulsars::getJsonFileVersion(void)
{
	int jsonVersion = -1;
	QFile jsonPSRCatalogFile(jsonCatalogPath);
	if (!jsonPSRCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Pulsars::init cannot open " << jsonCatalogPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&jsonPSRCatalogFile).toMap();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	jsonPSRCatalogFile.close();
	qDebug() << "Pulsars::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

PulsarP Pulsars::getByID(const QString& id)
{
	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->initialized && pulsar->designation == id)
			return pulsar;
	}
	return PulsarP();
}

bool Pulsars::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiAction("actionShow_Pulsars_ConfigDialog")->setChecked(true);
	}

	return true;
}

void Pulsars::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void Pulsars::restoreDefaultConfigIni(void)
{
	conf->beginGroup("Pulsars");

	// delete all existing Pulsars settings...
	conf->remove("");

	conf->setValue("distribution_enabled", false);
	conf->setValue("updates_enabled", true);
	conf->setValue("url", "http://stellarium.org/json/pulsars.json");
	conf->setValue("update_frequency_days", 100);
	conf->endGroup();
}

void Pulsars::readSettingsFromConfig(void)
{
	conf->beginGroup("Pulsars");

	updateUrl = conf->value("url", "http://stellarium.org/json/pulsars.json").toString();
	updateFrequencyDays = conf->value("update_frequency_days", 100).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-05-24T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	distributionEnabled = conf->value("distribution_enabled", false).toBool();

	conf->endGroup();
}

void Pulsars::saveSettingsToConfig(void)
{
	conf->beginGroup("Pulsars");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_days", updateFrequencyDays);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("distribution_enabled", distributionEnabled);

	conf->endGroup();
}

int Pulsars::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyDays * 3600 * 24);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Pulsars::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyDays * 3600 * 24) <= QDateTime::currentDateTime())
		updateJSON();
}

void Pulsars::updateJSON(void)
{
	if (updateState==Pulsars::Updating)
	{
		qWarning() << "Pulsars: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "Pulsars: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	conf->setValue("Pulsars/last_update", lastUpdate.toString(Qt::ISODate));

	emit(jsonUpdateComplete());

	updateState = Pulsars::Updating;

	emit(updateStateChanged(updateState));
	updateFile.clear();

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();

	progressBar->setValue(0);
	progressBar->setMaximum(updateUrl.size());
	progressBar->setVisible(true);
	progressBar->setFormat("Update pulsars");

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Pulsars Plugin %1; http://stellarium.org/)").arg(PULSARS_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	progressBar->setValue(100);
	delete progressBar;
	progressBar = NULL;

	updateState = CompleteUpdates;

	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void Pulsars::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Pulsars::updateDownloadComplete FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		try
		{
			QString jsonFilePath = StelFileMgr::findFile("modules/Pulsars", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/pulsars.json";
			QFile jsonFile(jsonFilePath);
			if (jsonFile.exists())
				jsonFile.remove();

			jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "Pulsars::updateDownloadComplete: cannot write JSON data to file:" << e.what();
		}

	}

	if (progressBar)
		progressBar->setValue(100);
}

void Pulsars::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Pulsars::messageTimeout(void)
{
	foreach(int i, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}
