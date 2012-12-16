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
#include "Quasar.hpp"
#include "Quasars.hpp"
#include "renderer/StelRenderer.hpp"
#include "QuasarsDialog.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QAction>
#include <QProgressBar>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QList>
#include <QSettings>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

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
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("A plugin that shows some quasars brighter than 16 visual magnitude. A catalogue of quasars compiled from 'Quasars and Active Galactic Nuclei' (13th Ed.) (Veron+ 2010) =2010A&A...518A..10V");
	return info;
}

Q_EXPORT_PLUGIN2(Quasars, QuasarsStelPluginInterface)


/*
 Constructor
*/
Quasars::Quasars()
	: texPointer(NULL)
	, markerTexture(NULL)
	, progressBar(NULL)
{
	setObjectName("Quasars");
	configDialog = new QuasarsDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(conf->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Quasars::~Quasars()
{
	delete configDialog;
}

void Quasars::deinit()
{
	if(NULL != texPointer)    {delete texPointer;}
	if(NULL != markerTexture) {delete markerTexture;}
	texPointer = markerTexture = NULL;
	QSO.clear();
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
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Quasars");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Quasars"))
		{
			qDebug() << "Quasars::init no Quasars section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		catalogJsonPath = StelFileMgr::findFile("modules/Quasars", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/quasars.json";

		// key bindings and other actions
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		connect(gui->getGuiAction("actionShow_Quasars_ConfigDialog"), SIGNAL(toggled(bool)), configDialog, SLOT(setVisible(bool)));
		connect(configDialog, SIGNAL(visibleChanged(bool)), gui->getGuiAction("actionShow_Quasars_ConfigDialog"), SLOT(setChecked(bool)));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Quasars::init error: " << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(catalogJsonPath).exists())
	{
		if (getJsonFileVersion() < CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Quasars::init catalog.json does not exist - copying default file to " << catalogJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Quasars::init using catalog.json file: " << catalogJsonPath;

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
 Draw our module. This should print name of first QSO in the main window
*/
void Quasars::draw(StelCore* core, class StelRenderer* renderer)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	renderer->setFont(font);

	if(NULL == markerTexture)
	{
		markerTexture = renderer->createTexture(":/Quasars/quasar.png");
	}
	
	foreach (const QuasarP& quasar, QSO)
	{
		if (quasar && quasar->initialized)
		{
			quasar->draw(core, renderer, prj, markerTexture);
		}
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, renderer, prj);
	}
}

void Quasars::drawPointer(StelCore* core, StelRenderer* renderer, StelProjectorP projector)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Quasar");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenPos;
		// Compute 2D pos and return if outside screen
		if (!projector->project(pos, screenPos))
		{
			return;
		}
		if(NULL == texPointer)
		{
			texPointer = renderer->createTexture("textures/pointeur2.png");
		}

		const Vec3f& c(obj->getInfoColor());
		renderer->setGlobalColor(c[0], c[1], c[2]);
		texPointer->bind();
		renderer->setBlendMode(BlendMode_Alpha);
		renderer->drawTexturedRect(screenPos[0] - 13.0f, screenPos[1] - 13.0f, 26.0f, 26.0f, 
		                           StelApp::getInstance().getTotalRunTime() * 40.0f);
	}
}

QList<StelObjectP> Quasars::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->initialized)
		{
			equPos = quasar->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(quasar));
			}
		}
	}

	return result;
}

StelObjectP Quasars::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(quasar);
	}

	return NULL;
}

StelObjectP Quasars::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(quasar);
	}

	return NULL;
}

QStringList Quasars::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << quasar->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Quasars::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach (const QuasarP& quasar, QSO)
		{
			result << quasar->getEnglishName();
		}
	}
	else
	{
		foreach (const QuasarP& quasar, QSO)
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
		qWarning() << "Quasars::restoreDefaultJsonFile cannot copy json resource to " + catalogJsonPath;
	}
	else
	{
		qDebug() << "Quasars::init copied default catalog.json to " << catalogJsonPath;
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
  Creates a backup of the Quasars.json file called Quasars.json.old
*/
bool Quasars::backupJsonFile(bool deleteOriginal)
{
	QFile old(catalogJsonPath);
	if (!old.exists())
	{
		qWarning() << "Quasars::backupJsonFile no file to backup";
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
				qWarning() << "Quasars::backupJsonFile WARNING - could not remove old quasars.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Quasars::backupJsonFile WARNING - failed to copy quasars.json to quasars.json.old";
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
	    qWarning() << "Quasars::loadQSOMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Quasars::setQSOMap(const QVariantMap& map)
{
	QSO.clear();
	QVariantMap qsoMap = map.value("quasars").toMap();
	foreach(QString qsoKey, qsoMap.keys())
	{
		QVariantMap qsoData = qsoMap.value(qsoKey).toMap();
		qsoData["designation"] = qsoKey;

		QuasarP quasar(new Quasar(qsoData));
		if (quasar->initialized)
			QSO.append(quasar);

	}
}

int Quasars::getJsonFileVersion(void)
{
	int jsonVersion = -1;
	QFile catalogJsonFile(catalogJsonPath);
	if (!catalogJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Quasars::init cannot open " << catalogJsonPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&catalogJsonFile).toMap();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	catalogJsonFile.close();
	qDebug() << "Quasars::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

QuasarP Quasars::getByID(const QString& id)
{
	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->initialized && quasar->designation == id)
			return quasar;
	}
	return QuasarP();
}

bool Quasars::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiAction("actionShow_Quasars_ConfigDialog")->setChecked(true);
	}

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
	conf->setValue("updates_enabled", true);
	conf->setValue("url", "http://stellarium.org/json/quasars.json");
	conf->setValue("update_frequency_days", 100);
	conf->endGroup();
}

void Quasars::readSettingsFromConfig(void)
{
	conf->beginGroup("Quasars");

	updateUrl = conf->value("url", "http://stellarium.org/json/quasars.json").toString();
	updateFrequencyDays = conf->value("update_frequency_days", 100).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-05-24T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	distributionEnabled = conf->value("distribution_enabled", false).toBool();

	conf->endGroup();
}

void Quasars::saveSettingsToConfig(void)
{
	conf->beginGroup("Quasars");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_days", updateFrequencyDays);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("distribution_enabled", distributionEnabled);

	conf->endGroup();
}

int Quasars::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyDays * 3600 * 24);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Quasars::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyDays * 3600 * 24) <= QDateTime::currentDateTime())
		updateJSON();
}

void Quasars::updateJSON(void)
{
	if (updateState==Quasars::Updating)
	{
		qWarning() << "Quasars: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "Quasars: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	conf->setValue("Quasars/last_update", lastUpdate.toString(Qt::ISODate));

	emit(jsonUpdateComplete());

	updateState = Quasars::Updating;

	emit(updateStateChanged(updateState));
	updateFile.clear();

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();

	progressBar->setValue(0);
	progressBar->setMaximum(updateUrl.size());
	progressBar->setVisible(true);
	progressBar->setFormat("Update quasars");

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Quasars Plugin %1; http://stellarium.org/)").arg(QUASARS_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	progressBar->setValue(100);
	delete progressBar;
	progressBar = NULL;

	updateState = CompleteUpdates;

	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void Quasars::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Quasars::updateDownloadComplete FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		try
		{
			QString jsonFilePath = StelFileMgr::findFile("modules/Quasars", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/quasars.json";
			QFile jsonFile(jsonFilePath);
			if (jsonFile.exists())
				jsonFile.remove();

			jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "Quasars::updateDownloadComplete: cannot write JSON data to file:" << e.what();
		}

	}

	if (progressBar)
		progressBar->setValue(100);
}

void Quasars::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Quasars::messageTimeout(void)
{
	foreach(int i, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}
