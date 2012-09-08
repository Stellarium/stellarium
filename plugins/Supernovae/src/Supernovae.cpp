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
#include "Supernova.hpp"
#include "Supernovae.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "SupernovaeDialog.hpp"

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
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("A plugin that shows some historical supernovae brighter than 10 visual magnitude.");
	return info;
}

Q_EXPORT_PLUGIN2(Supernovae, SupernovaeStelPluginInterface)


/*
 Constructor
*/
Supernovae::Supernovae()
	: texPointer(NULL)
	, progressBar(NULL)
{
	setObjectName("Supernovae");
	configDialog = new SupernovaeDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(conf->value("gui/base_font_size", 13).toInt());
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
	if(NULL != texPointer)
	{
		delete texPointer;
	}
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
			qDebug() << "Supernovae::init no Supernovae section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		sneJsonPath = StelFileMgr::findFile("modules/Supernovae", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/supernovae.json";
		// key bindings and other actions
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		connect(gui->getGuiAction("actionShow_Supernovae_ConfigDialog"), SIGNAL(toggled(bool)), configDialog, SLOT(setVisible(bool)));
		connect(configDialog, SIGNAL(visibleChanged(bool)), gui->getGuiAction("actionShow_Supernovae_ConfigDialog"), SLOT(setChecked(bool)));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Supernovas::init error: " << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(sneJsonPath).exists())
	{
		if (getJsonFileVersion() < CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Supernovae::init supernovae.json does not exist - copying default file to " << sneJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Supernovae::init using supernovae.json file: " << sneJsonPath;

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
 Draw our module. This should print name of first SNe in the main window
*/
void Supernovae::draw(StelCore* core, StelRenderer* renderer)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	renderer->setFont(font);
	
	foreach (const SupernovaP& sn, snstar)
	{
		if (sn && sn->initialized)
		{
			sn->draw(core, renderer, prj);
		}
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, renderer, prj);
	}
}

void Supernovae::drawPointer(StelCore* core, StelRenderer* renderer, StelProjectorP projector)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Supernova");
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
		renderer->setGlobalColor(c[0],c[1],c[2]);
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

QList<StelObjectP> Supernovae::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->initialized)
		{
			equPos = sn->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(sn));
			}
		}
	}

	return result;
}

StelObjectP Supernovae::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(sn);
	}

	return NULL;
}

StelObjectP Supernovae::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(sn);
	}

	return NULL;
}

QStringList Supernovae::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << sn->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Supernovae::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach (const SupernovaP& sn, snstar)
		{
			result << sn->getEnglishName();
		}
	}
	else
	{
		foreach (const SupernovaP& sn, snstar)
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
		qWarning() << "Supernovae::restoreDefaultJsonFile cannot copy json resource to " + sneJsonPath;
	}
	else
	{
		qDebug() << "Supernovae::init copied default supernovae.json to " << sneJsonPath;
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
		qWarning() << "Supernovae::backupJsonFile no file to backup";
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
				qWarning() << "Supernovae::backupJsonFile WARNING - could not remove old supernovas.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Supernovae::backupJsonFile WARNING - failed to copy supernovae.json to supernovae.json.old";
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
	    qWarning() << "Supernovae::loadSNeMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Supernovae::setSNeMap(const QVariantMap& map)
{
	snstar.clear();
	snlist.clear();
	QVariantMap sneMap = map.value("supernova").toMap();
	foreach(QString sneKey, sneMap.keys())
	{
		QVariantMap sneData = sneMap.value(sneKey).toMap();
		sneData["designation"] = QString("SN %1").arg(sneKey);

		snlist.insert(sneData.value("designation").toString(), sneData.value("peakJD").toDouble());

		SupernovaP sn(new Supernova(sneData));
		if (sn->initialized)
			snstar.append(sn);

	}
}

int Supernovae::getJsonFileVersion(void)
{	
	int jsonVersion = -1;
	QFile sneJsonFile(sneJsonPath);
	if (!sneJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Supernovae::init cannot open " << sneJsonPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&sneJsonFile).toMap();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	sneJsonFile.close();
	qDebug() << "Supernovae::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

SupernovaP Supernovae::getByID(const QString& id)
{
	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->initialized && sn->designation == id)
			return sn;
	}
	return SupernovaP();
}

bool Supernovae::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiAction("actionShow_Supernovae_ConfigDialog")->setChecked(true);
	}

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
	conf->setValue("url", "http://stellarium.org/json/supernovae.json");
	conf->setValue("update_frequency_days", 100);
	conf->endGroup();
}

void Supernovae::readSettingsFromConfig(void)
{
	conf->beginGroup("Supernovae");

	updateUrl = conf->value("url", "http://stellarium.org/json/supernovae.json").toString();
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
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Supernovae::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyDays * 3600 * 24) <= QDateTime::currentDateTime())
		updateJSON();
}

void Supernovae::updateJSON(void)
{
	if (updateState==Supernovae::Updating)
	{
		qWarning() << "Supernovae: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "Supernovae: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	conf->setValue("Supernovae/last_update", lastUpdate.toString(Qt::ISODate));

	emit(jsonUpdateComplete());

	updateState = Supernovae::Updating;

	emit(updateStateChanged(updateState));
	updateFile.clear();

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();

	progressBar->setValue(0);
	progressBar->setMaximum(updateUrl.size());
	progressBar->setVisible(true);
	progressBar->setFormat("Update historical supernovae");

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Historical Supernovae Plugin %1; http://stellarium.org/)").arg(SUPERNOVAE_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	progressBar->setValue(100);
	delete progressBar;
	progressBar = NULL;

	updateState = CompleteUpdates;

	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void Supernovae::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Supernovae::updateDownloadComplete FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		try
		{
			QString jsonFilePath = StelFileMgr::findFile("modules/Supernovae", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/supernovae.json";
			QFile jsonFile(jsonFilePath);
			if (jsonFile.exists())
				jsonFile.remove();

			jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "Supernovae::updateDownloadComplete: cannot write JSON data to file:" << e.what();
		}

	}

	if (progressBar)
		progressBar->setValue(100);
}

void Supernovae::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void Supernovae::messageTimeout(void)
{
	foreach(int i, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}

QString Supernovae::getSupernovaeList()
{
	QString smonth[] = {q_("January"), q_("February"), q_("March"), q_("April"), q_("May"), q_("June"), q_("July"), q_("August"), q_("September"), q_("October"), q_("November"), q_("December")};
	QStringList out;
	int year, month, day;
	QList<double> vals = snlist.values();
	qSort(vals);
	foreach(double val, vals)
	{
		StelUtils::getDateFromJulianDay(val, &year, &month, &day);
		out << QString("%1 (%2 %3)").arg(snlist.key(val)).arg(day).arg(smonth[month-1]);
	}

	return out.join(", ");
}
