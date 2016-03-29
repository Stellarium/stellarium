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
#include "Pulsar.hpp"
#include "Pulsars.hpp"
#include "PulsarsDialog.hpp"
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
	info.version = PULSARS_PLUGIN_VERSION;
	return info;
}

/*
 Constructor
*/
Pulsars::Pulsars()
	: PsrCount(0)
	, updateState(CompleteNoUpdates)
	, downloadMgr(NULL)
	, updateTimer(0)
	, messageTimer(0)
	, updatesEnabled(false)
	, updateFrequencyDays(0)
	, enableAtStartup(false)
	, flagShowPulsars(false)
	, flagShowPulsarsButton(false)
	, OnIcon(NULL)
	, OffIcon(NULL)
	, GlowIcon(NULL)
	, toolbarButton(NULL)
	, progressBar(NULL)
{
	setObjectName("Pulsars");
	configDialog = new PulsarsDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(StelApp::getInstance().getBaseFontSize());
}

/*
 Destructor
*/
Pulsars::~Pulsars()
{
	delete configDialog;

	if (GlowIcon)
		delete GlowIcon;
	if (OnIcon)
		delete OnIcon;
	if (OffIcon)
		delete OffIcon;
}

void Pulsars::deinit()
{
	psr.clear();
	Pulsar::markerTexture.clear();
	texPointer.clear();
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
	upgradeConfigIni();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Pulsars");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Pulsars"))
		{
			qDebug() << "Pulsars: no Pulsars section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		jsonCatalogPath = StelFileMgr::findFile("modules/Pulsars", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/pulsars.json";
		if (jsonCatalogPath.isEmpty())
			return;

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");
		Pulsar::markerTexture = StelApp::getInstance().getTextureManager().createTexture(":/Pulsars/pulsar.png");

		// key bindings and other actions
		addAction("actionShow_Pulsars", N_("Pulsars"), N_("Show pulsars"), "pulsarsVisible", "Ctrl+Alt+P");
		addAction("actionShow_Pulsars_ConfigDialog", N_("Pulsars"), N_("Pulsars configuration window"), configDialog, "visible");

		GlowIcon = new QPixmap(":/graphicGui/glow32x32.png");
		OnIcon = new QPixmap(":/Pulsars/btPulsars-on.png");
		OffIcon = new QPixmap(":/Pulsars/btPulsars-off.png");

		setFlagShowPulsars(getEnableAtStartup());
		setFlagShowPulsarsButton(flagShowPulsarsButton);
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Pulsars: init error:" << e.what();
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
		if (!checkJsonFileFormat() || getJsonFileFormatVersion()<CATALOG_FORMAT_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Pulsars: pulsars.json does not exist - copying default file to" << QDir::toNativeSeparators(jsonCatalogPath);
		restoreDefaultJsonFile();
	}

	qDebug() << "Pulsars: loading catalog file:" << QDir::toNativeSeparators(jsonCatalogPath);

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
void Pulsars::draw(StelCore* core)
{
	if (!flagShowPulsars)
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const PulsarP& pulsar, psr)
	{
		if (pulsar && pulsar->initialized)
			pulsar->draw(core, &painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void Pulsars::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Pulsar");
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
		painter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		painter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

QList<StelObjectP> Pulsars::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	if (!flagShowPulsars)
		return result;

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
	if (!flagShowPulsars)
		return NULL;

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(pulsar);
	}

	return NULL;
}

StelObjectP Pulsars::searchByNameI18n(const QString& nameI18n) const
{
	if (!flagShowPulsars)
		return NULL;

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(pulsar);
	}

	return NULL;
}

QStringList Pulsars::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!flagShowPulsars)
		return result;

	if (maxNbItem==0)
		return result;

	QString psrn;
	bool find;
	foreach(const PulsarP& pulsar, psr)
	{
		psrn = pulsar->getNameI18n();
		find = false;
		if (useStartOfWords)
		{
			if (psrn.toUpper().left(objPrefix.length()) == objPrefix.toUpper())
				find = true;
		}
		else
		{
			if (psrn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
			result << psrn;
		}
	}

	result.sort();
	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Pulsars::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!flagShowPulsars)
		return result;

	if (maxNbItem==0)
		return result;

	QString psrn;
	bool find;
	foreach(const PulsarP& pulsar, psr)
	{
		psrn = pulsar->getEnglishName();
		find = false;
		if (useStartOfWords)
		{
			if (psrn.toUpper().left(objPrefix.length()) == objPrefix.toUpper())
				find = true;
		}
		else
		{
			if (psrn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
		{
			result << psrn;
		}
	}

	result.sort();
	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList Pulsars::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (!flagShowPulsars)
		return result;

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
		qWarning() << "Pulsars: cannot copy JSON resource to" + QDir::toNativeSeparators(jsonCatalogPath);
	}
	else
	{
		qDebug() << "Pulsars: copied default pulsars.json to" << QDir::toNativeSeparators(jsonCatalogPath);
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
		qWarning() << "Pulsars: no file to backup";
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
				qWarning() << "Pulsars: WARNING - could not remove old pulsars.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Pulsars: WARNING - failed to copy pulsars.json to pulsars.json.old";
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
		qWarning() << "Pulsars: cannot open" << QDir::toNativeSeparators(path);
	else
	{
		map = StelJsonParser::parse(jsonFile.readAll()).toMap();
		jsonFile.close();
	}
	return map;
}

/*
  Set items for list of struct from data map
*/
void Pulsars::setPSRMap(const QVariantMap& map)
{
	psr.clear();
	PsrCount = 0;
	QVariantMap psrMap = map.value("pulsars").toMap();
	foreach(QString psrKey, psrMap.keys())
	{
		QVariantMap psrData = psrMap.value(psrKey).toMap();
		psrData["designation"] = psrKey;

		PsrCount++;

		PulsarP pulsar(new Pulsar(psrData));
		if (pulsar->initialized)
			psr.append(pulsar);

	}
}

int Pulsars::getJsonFileFormatVersion(void)
{
	int jsonVersion = -1;
	QFile jsonPSRCatalogFile(jsonCatalogPath);
	if (!jsonPSRCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Pulsars: cannot open" << QDir::toNativeSeparators(jsonCatalogPath);
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&jsonPSRCatalogFile).toMap();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	jsonPSRCatalogFile.close();
	qDebug() << "Pulsars: version of the format of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool Pulsars::checkJsonFileFormat()
{
	QFile jsonPSRCatalogFile(jsonCatalogPath);
	if (!jsonPSRCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Pulsars: cannot open" << QDir::toNativeSeparators(jsonCatalogPath);
		return false;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&jsonPSRCatalogFile).toMap();
		jsonPSRCatalogFile.close();
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "Pulsars: file format is wrong! Error:" << e.what();
		return false;
	}

	return true;
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
		configDialog->setVisible(true);
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
	conf->setValue("enable_at_startup", false);
	conf->setValue("updates_enabled", true);	
	conf->setValue("url", "http://stellarium.org/json/pulsars.json");
	conf->setValue("update_frequency_days", 100);
	conf->setValue("flag_show_pulsars_button", true);
	conf->setValue("marker_color", "0.4,0.5,1.0");
	conf->setValue("glitch_color", "0.2,0.3,1.0");
	conf->setValue("use_separate_colors", false);
	conf->endGroup();
}

void Pulsars::readSettingsFromConfig(void)
{
	conf->beginGroup("Pulsars");

	updateUrl = conf->value("url", "http://stellarium.org/json/pulsars.json").toString();
	updateFrequencyDays = conf->value("update_frequency_days", 100).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2012-05-24T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	setDisplayMode(conf->value("distribution_enabled", false).toBool());
	setGlitchFlag(conf->value("use_separate_colors", false).toBool());
	setMarkerColor(conf->value("marker_color", "0.4,0.5,1.0").toString(), true);
	setMarkerColor(conf->value("glitch_color", "0.2,0.3,1.0").toString(), false);
	enableAtStartup = conf->value("enable_at_startup", false).toBool();
	flagShowPulsarsButton = conf->value("flag_show_pulsars_button", true).toBool();

	conf->endGroup();
}

void Pulsars::saveSettingsToConfig(void)
{
	conf->beginGroup("Pulsars");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_days", updateFrequencyDays);
	conf->setValue("updates_enabled", updatesEnabled );
	conf->setValue("distribution_enabled", getDisplayMode());
	conf->setValue("use_separate_colors", getGlitchFlag());
	conf->setValue("enable_at_startup", enableAtStartup);
	conf->setValue("flag_show_pulsars_button", flagShowPulsarsButton);
	conf->setValue("marker_color", getMarkerColor(true));
	conf->setValue("glitch_color", getMarkerColor(false));

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

	updateState = Pulsars::Updating;
	emit(updateStateChanged(updateState));	

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().addProgressBar();

	progressBar->setValue(0);
	progressBar->setRange(0, 100);
	progressBar->setFormat("Update pulsars");

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Pulsars Plugin %1; http://stellarium.org/)").arg(PULSARS_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	updateState = Pulsars::CompleteUpdates;
	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void Pulsars::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Pulsars: FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		QString jsonFilePath = StelFileMgr::findFile("modules/Pulsars", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/pulsars.json";
		if (jsonFilePath.isEmpty())
		{
			qWarning() << "Pulsars: cannot write JSON data to file:" << QDir::toNativeSeparators(jsonCatalogPath);
			return;
		}
		QFile jsonFile(jsonFilePath);
		if (jsonFile.exists())
			jsonFile.remove();

		if(jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}
	}

	if (progressBar)
	{
		progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar=NULL;
	}
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

void Pulsars::upgradeConfigIni(void)
{
	// Upgrade settings for Pulsars plugin
	if (conf->contains("Pulsars/flag_show_pulsars"))
	{
		bool b = conf->value("Pulsars/flag_show_pulsars", false).toBool();
		if (!conf->contains("Pulsars/enable_at_startup"))
			conf->setValue("Pulsars/enable_at_startup", b);
		conf->remove("Pulsars/flag_show_pulsars");
	}
}

// Define whether the button toggling pulsars should be visible
void Pulsars::setFlagShowPulsarsButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=NULL)
	{
		if (b==true) {
			if (toolbarButton==NULL) {
				// Create the pulsars button
				toolbarButton = new StelButton(NULL, *OnIcon, *OffIcon, *GlowIcon, "actionShow_Pulsars");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		} else {
			gui->getButtonBar()->hideButton("actionShow_Pulsars");
		}
	}
	flagShowPulsarsButton = b;
}

bool Pulsars::getDisplayMode()
{
	return Pulsar::distributionMode;
}

void Pulsars::setDisplayMode(bool b)
{
	Pulsar::distributionMode=b;
}

bool Pulsars::getGlitchFlag()
{
	return Pulsar::glitchFlag;
}

void Pulsars::setGlitchFlag(bool b)
{
	Pulsar::glitchFlag=b;
}

QString Pulsars::getMarkerColor(bool mtype)
{
	Vec3f c;
	if (mtype)
		c = Pulsar::markerColor;
	else
		c = Pulsar::glitchColor;
	return QString("%1,%2,%3").arg(c[0]).arg(c[1]).arg(c[2]);
}

void Pulsars::setMarkerColor(QString c, bool mtype)
{
	if (mtype)
		Pulsar::markerColor = StelUtils::strToVec3f(c);
	else
		Pulsar::glitchColor = StelUtils::strToVec3f(c);
}

void Pulsars::reloadCatalog(void)
{
	bool hasSelection = false;
	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	// Whether any pulsar are selected? Save the current selection...
	const QList<StelObjectP> selectedObject = objMgr->getSelectedObject("Pulsar");
	if (!selectedObject.isEmpty())
	{
		// ... unselect current pulsar.
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
