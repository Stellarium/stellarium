/*
 * Copyright (C) 2009 Matthew Gates
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocation.hpp"
#include "StelNavigator.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelIniParser.hpp"
#include "Satellites.hpp"
#include "Satellite.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelJsonParser.hpp"
#include "SatellitesDialog.hpp"

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

StelModule* SatellitesStelPluginInterface::getStelModule() const
{
	return new Satellites();
}

StelPluginInfo SatellitesStelPluginInterface::getPluginInfo() const
{
		// Allow to load the resources when used as a static plugin
		Q_INIT_RESOURCE(Satellites);

		StelPluginInfo info;
		info.id = "Satellites";
		info.displayedName = "Satellites";
		info.authors = "Matthew Gates";
		info.contact = "http://stellarium.org/";
		info.description = "Prediction of artificial satellite positions in Earth orbit based on NORAD TLE data";
		return info;
}

Q_EXPORT_PLUGIN2(Satellites, SatellitesStelPluginInterface)

Satellites::Satellites()
	: pxmapGlow(NULL), pxmapOnIcon(NULL), pxmapOffIcon(NULL), toolbarButton(NULL), earth(NULL), defaultHintColor(0.,0.4,0.6), progressBar(NULL)
{
	setObjectName("Satellites");
	configDialog = new SatellitesDialog();
}

void Satellites::deinit()
{
	Satellite::hintTexture.clear();
	texPointer.clear();
	configDialog->setVisible(false);
}

Satellites::~Satellites()
{
	delete configDialog;
}


void Satellites::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Satellites");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("Satellites"))
		{
			qDebug() << "Stellites::init no Satellites section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		satellitesJsonPath = StelFileMgr::findFile("modules/Satellites", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/satellites.json";

		// Load and find resources used in the plugin
		texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur5.png");
		Satellite::hintTexture = StelApp::getInstance().getTextureManager().createTexture(":/satellites/satellite_hint.png");

		// key bindings and other actions
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->addGuiActions("actionShow_Satellite_ConfigDialog", "Satellite Config Dialog", "Shift+Ctrl+Z", "Plugin Key Bindings", true, false);
		gui->addGuiActions("actionShow_Satellite_Hints", "Satellite Hints", "Ctrl+Z", "Plugin Key Bindings", true, false);
		gui->getGuiActions("actionShow_Satellite_Hints")->setChecked(hintFader);
		gui->addGuiActions("actionShow_Satellite_Labels", "Satellite Labels", "Shift+Z", "Plugin Key Bindings", true, false);
		gui->getGuiActions("actionShow_Satellite_Labels")->setChecked(Satellite::showLabels);

		// Gui toolbar button
		pxmapGlow = new QPixmap(":/graphicGui/gui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/satellites/bt_satellites_on.png");
		pxmapOffIcon = new QPixmap(":/satellites/bt_satellites_off.png");
		toolbarButton = new StelButton(NULL, *pxmapOnIcon, *pxmapOffIcon, *pxmapGlow, gui->getGuiActions("actionShow_Satellite_Hints"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");

		connect(gui->getGuiActions("actionShow_Satellite_ConfigDialog"), SIGNAL(toggled(bool)), configDialog, SLOT(setVisible(bool)));
		connect(gui->getGuiActions("actionShow_Satellite_Hints"), SIGNAL(toggled(bool)), this, SLOT(setFlagHints(bool)));
		connect(gui->getGuiActions("actionShow_Satellite_Labels"), SIGNAL(toggled(bool)), this, SLOT(setFlagLabels(bool)));
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Satellites::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the QT resource
	if(!QFileInfo(satellitesJsonPath).exists())
	{
		qDebug() << "Satellites::init satellites.json does not exist - copying default file to " << satellitesJsonPath;
		restoreDefaultJsonFile();
	}
	else
	{
		qDebug() << "Satellites::init using satellite.json file: " << satellitesJsonPath;
	}

	// create satellites according to content os satellites.json file
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

	earth = GETSTELMODULE(SolarSystem)->getEarth();
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	// Handle changes to the observer location:
	connect(StelApp::getInstance().getCore()->getNavigator(), SIGNAL(locationChanged(StelLocation)), this, SLOT(observerLocationChanged(StelLocation)));
	
	//Load the module's custom style sheets
	QFile styleSheetFile;
	styleSheetFile.setFileName(":/satellites/normalStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		normalStyleSheet = new QByteArray(styleSheetFile.readAll());
	}
	styleSheetFile.close();
	styleSheetFile.setFileName(":/satellites/nightStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		nightStyleSheet = new QByteArray(styleSheetFile.readAll());
	}
	styleSheetFile.close();
}

void Satellites::setStelStyle(const StelStyle& style)
{
	configDialog->setStelStyle(style);
}

const StelStyle Satellites::getModuleStyleSheet(const StelStyle& style)
{
	StelStyle pluginStyle(style);
	if (style.confSectionName == "color")
	{
		pluginStyle.qtStyleSheet.append(*normalStyleSheet);
	}
	else
	{
		pluginStyle.qtStyleSheet.append(*nightStyleSheet);
	}
	return pluginStyle;
}

double Satellites::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)+1.;
	return 0;
}

QList<StelObjectP> Satellites::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!hintFader || StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().planetName != earth->getEnglishName())
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			equPos = sat->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(sat));
			}
		}
	}
	return result;
}

StelObjectP Satellites::searchByNameI18n(const QString& nameI18n) const
{
	if (!hintFader || StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().planetName != earth->getEnglishName())
		return NULL;

	QString objw = nameI18n.toUpper();

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			if (sat->getNameI18n().toUpper() == nameI18n)
				return qSharedPointerCast<StelObject>(sat);
		}
	}

	return NULL;
}

StelObjectP Satellites::searchByName(const QString& englishName) const
{
	if (!hintFader || StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().planetName != earth->getEnglishName())
		return NULL;

	QString objw = englishName.toUpper();
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			if (sat->getEnglishName().toUpper() == englishName)
				return qSharedPointerCast<StelObject>(sat);
		}
	}

	return NULL;
}

QStringList Satellites::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (!hintFader || StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().planetName != earth->getEnglishName())
		return result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
		{
			if (sat->getNameI18n().toUpper().left(objw.length()) == objw)
			{
				result << sat->getNameI18n().toUpper();
			}
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

bool Satellites::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiActions("actionShow_Satellite_ConfigDialog")->setChecked(true);
	}

	return true;
}

void Satellites::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readSettingsFromConfig();
}

void Satellites::restoreDefaultConfigIni(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// delete all existing Satellite settings...
	conf->remove("");

	conf->setValue("show_satellite_hints", true);
	conf->setValue("show_satellite_labels", true);
	conf->setValue("updates_enabled", true);
	conf->setValue("show_satellites", true);
	conf->setValue("hint_color", "0.0,0.4,0.6");
	conf->setValue("hint_font_size", 10);
	conf->setValue("tle_url0", "http://celestrak.com/NORAD/elements/noaa.txt");
	conf->setValue("tle_url1", "http://celestrak.com/NORAD/elements/goes.txt");
	conf->setValue("tle_url2", "http://celestrak.com/NORAD/elements/gps-ops.txt");
	conf->setValue("tle_url3", "http://celestrak.com/NORAD/elements/galileo.txt");
	conf->setValue("tle_url4", "http://celestrak.com/NORAD/elements/visual.txt");
	conf->setValue("tle_url5", "http://celestrak.com/NORAD/elements/amateur.txt");
	conf->setValue("tle_url6", "http://celestrak.com/NORAD/elements/iridium.txt");
	conf->setValue("tle_url7", "http://celestrak.com/NORAD/elements/tle-new.txt");
	conf->setValue("update_frequency_hours", 72);
	conf->endGroup();
}

void Satellites::restoreDefaultJsonFile(void)
{
	QFile src(":/satellites/satellites.json");
	if (!src.copy(satellitesJsonPath))
	{
		qWarning() << "Satellites::restoreDefaultJsonFile cannot copy json resource to " + satellitesJsonPath;
	}
	else
	{
		qDebug() << "Satellites::init copied default satellites.json to " << satellitesJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(satellitesJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		StelApp::getInstance().getSettings()->remove("Satellites/last_update");
	}
}

void Satellites::readSettingsFromConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// populate updateUrls from tle_url? keys
	QRegExp keyRE("^tle_url\\d+$");
	updateUrls.clear();
	foreach(QString key, conf->childKeys())
	{
		if (keyRE.exactMatch(key))
		{
			QString s = conf->value(key, "").toString();
			if (!s.isEmpty() && s!="")
				updateUrls << s;
		}
	}

	// updater related settings...
	updateFrequencyHours = conf->value("update_frequency_hours", 72).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2009-01-01 12:00:00").toString(), "yyyy-MM-dd HH:mm:ss");
	hintFader = conf->value("show_satellite_hints", true).toBool();
	Satellite::showLabels = conf->value("show_satellite_labels", true).toBool();
	updatesEnabled = conf->value("updates_enabled", true).toBool();

	// Get a font for labels
	labelFont.setPixelSize(conf->value("hint_font_size", 10).toInt());

	conf->endGroup();
}

int Satellites::readJsonFile(void)
{
	// First, delete all existing Satellite objects...
	satellites.clear();

	QFile satelliteJsonFile(satellitesJsonPath);
	if (!satelliteJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Satellites::init cannot open " << satellitesJsonPath;
		return 0;
	}

	int numReadOk = 0;
	QVariantMap map;
	map = StelJsonParser::parse(&satelliteJsonFile).toMap();
	QVariantList defaultHintColorMap;
	defaultHintColorMap << defaultHintColor[0] << defaultHintColor[1] << defaultHintColor[2];
	if (map.contains("hintColor"))
	{
		defaultHintColorMap = map.value("hintColor").toList();
		defaultHintColor.set(defaultHintColorMap.at(0).toDouble(), defaultHintColorMap.at(1).toDouble(), defaultHintColorMap.at(2).toDouble());
	}

	QVariantMap satMap = map.value("satellites").toMap();
	foreach(QString designation, satMap.keys())
	{
		QVariantMap satData = satMap.value(designation).toMap();
		satData["designation"] = designation;

		if (!satData.contains("hintColor"))
			satData["hintColor"] = defaultHintColorMap;

		SatelliteP sat(new Satellite(satData));
		if (sat->initialized)
		{
			satellites.append(sat);
			numReadOk++;
		}
	}
	satelliteJsonFile.close();
	return numReadOk;
}

QStringList Satellites::getGroups(void) const
{
	QStringList groups;
	foreach (const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
		{
			foreach(QString group, sat->groupIDs)
			{
				if (!groups.contains(group))
					groups << group;
			}
		}
	}
	return groups;
}

QStringList Satellites::getSatellites(const QString& group, Visibility vis)
{
	QStringList result;

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized)
			if ((group.isEmpty() || sat->groupIDs.contains(group)) && ! result.contains(sat->designation))
			{
				if (vis==Both || (vis==Visible && sat->visible) || (vis==NotVisible && !sat->visible))
					result << sat->designation;
			}
	}
	return result;
}

SatelliteP Satellites::getByID(const QString& id)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->designation == id)
			return sat;
	}
	return SatelliteP();
}

int Satellites::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyHours * 3600);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void Satellites::setTleSources(QStringList tleSources)
{
	updateUrls = tleSources;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Satellites");

	// clear old source list
	QRegExp keyRE("^tle_url\\d+$");
	foreach(QString key, conf->childKeys())
	{
		if (keyRE.exactMatch(key))
			conf->remove(key);
	}

	// set the new sources list
	int i=0;
	foreach (QString url, updateUrls)
	{
		conf->setValue(QString("tle_url%1").arg(i++), url);
	}

	conf->endGroup();
}

bool Satellites::getFlagLabels(void)
{
	return Satellite::showLabels;
}

void Satellites::setFlagLabels(bool b)
{
	Satellite::showLabels = b;
}

void Satellites::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime())
		updateTLEs();
}

void Satellites::updateTLEs(void)
{
	if (updateState==Satellites::Updating)
	{
		qWarning() << "Satellites::updateTLEs already updating...  will not start again current update is complete";
		return;
	}
	else
	{
		qDebug() << "Satellites::updateTLEs starting update";
	}

	lastUpdate = QDateTime::currentDateTime();
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("Satellites/last_update", lastUpdate.toString("yyyy-MM-dd HH:mm:ss"));

	if (updateUrls.size() == 0)
	{
		qWarning() << "Satellites::updateTLEs no update URLs are defined... nothing to do";
		emit(TleUpdateComplete(0));
		return;
	}

	updateState = Satellites::Updating;
	emit(updateStateChanged(updateState));
	updateFiles.clear();
	numberDownloadsComplete = 0;

	if (progressBar==NULL)
		progressBar = StelApp::getInstance().getGui()->addProgressBar();

	progressBar->setValue(0);
	progressBar->setMaximum(updateUrls.size());
	progressBar->setVisible(true);
	progressBar->setFormat("TLE download %v/%m");

	// set off the downloads
	for(int i=0; i<updateUrls.size(); i++)
	{
		downloadMgr->get(QNetworkRequest(QUrl(updateUrls.at(i))));
	}
}

void Satellites::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "Satellites::updateDownloadComplete FAILED to download" << reply->url() << " Error: " << reply->error();
	}
	else
	{
		// download completed successfully.
		try
		{
			QString partialName = QString("tle%1.txt").arg(numberDownloadsComplete);
			QString tleTmpFilePath = StelFileMgr::findFile("modules/Satellites", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/" + partialName;
			QFile tmpFile(tleTmpFilePath);
			tmpFile.remove();
			tmpFile.open(QIODevice::WriteOnly | QIODevice::Text);
			tmpFile.write(reply->readAll());
			tmpFile.close();
			updateFiles << tleTmpFilePath;
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "Satellites::updateDownloadComplete: cannot write TLE data to file:" << e.what();
		}
	}
	numberDownloadsComplete++;
	if (progressBar)
		progressBar->setValue(numberDownloadsComplete);

	// all downloads are complete...  do the update.
	if (numberDownloadsComplete >= updateUrls.size())
	{
		updateFromFiles();
	}
}

void Satellites::observerLocationChanged(StelLocation loc)
{
	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
			sat->setObserverLocation(&loc);
	}
}

void Satellites::updateFromFiles(void)
{
	// define a map of new TLE data - the key is the satellite designation
	QMap< QString, QPair<QString, QString> > newTLE;

	if (progressBar)
	{
		progressBar->setValue(0);
		progressBar->setMaximum(updateFiles.size() + 1);
		progressBar->setFormat("TLE updating %v/%m");
	}

	foreach(QString tleFilePath, updateFiles)
	{
		QFile tleFile(tleFilePath);
		if (tleFile.open(QIODevice::ReadOnly|QIODevice::Text))
		{
			int lineNumber = 0;
			QString thisSatId("");
			QPair<QString, QString> tleLines;
			while (!tleFile.atEnd()) {
				QString line = QString(tleFile.readLine()).trimmed();
				if (line.length() < 65) // this is a new designation
				{
					if (thisSatId!="" && !tleLines.first.isEmpty() && !tleLines.second.isEmpty())
					{
						newTLE[thisSatId] = tleLines;
					}
					thisSatId = line;
					thisSatId.replace(QRegExp("\\[[^\\]]*\\]\\s*$"),"");  // remove things in square brackets
					tleLines.first = QString();
					tleLines.second = QString();
				}
				else
				{
					if (QRegExp("^1 .*").exactMatch(line))
						tleLines.first = line;
					else if (QRegExp("^2 .*").exactMatch(line))
						tleLines.second = line;
					else
						qDebug() << "Satellites::updateFromFiles unprocessed line " << lineNumber <<  " in file " << tleFilePath;
				}
			}
			if (thisSatId!="" && !tleLines.first.isEmpty() && !tleLines.second.isEmpty())
			{
				newTLE[thisSatId] = tleLines;
			}
			tleFile.close();
			if (progressBar)
				progressBar->setValue(progressBar->value() + 1);
		}
		tleFile.remove();  // clean up downloaded TLE files
	}

	QFile satelliteJsonFile(satellitesJsonPath);
	StelJsonParser parser;
	QVariantMap map;
	if (!satelliteJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Satellites::updateFromFiles cannot open for reading " << satellitesJsonPath;
	}
	else
	{
		map = parser.parse(&satelliteJsonFile).toMap();
		satelliteJsonFile.close();
	}
	// Right, we should now have a map of all the elements we downloaded.  For each satellite
	// which this module is managing, see if it exists with an updated element, and update it if so...
	int numUpdated = 0;
	QVariantList defCol;
	defCol << (double)defaultHintColor[0] << (double)defaultHintColor[1] << (double)defaultHintColor[2];
	map["creator"] = "Satellites plugin (update)";
	map["hintColor"] = defCol;
	map["shortName"] = "satellite orbital data";
	QVariantMap sats = map["satellites"].toMap();
	foreach(const SatelliteP& sat, satellites)
	{

		if (newTLE.contains(sat->designation))
		{
			if (sat->elements[1] != newTLE[sat->designation].first || sat->elements[2] != newTLE[sat->designation].second)
			{
				QVariantMap satMap = sat->getMap();
				satMap.remove("designation");
				if (satMap["hintColor"].toList() == defCol)
					satMap.remove("hintColor");

				// We have updated TLE elements for this satellite
				// update the satellites.json file
				satMap["tle1"] = newTLE[sat->designation].first;
				satMap["tle2"] = newTLE[sat->designation].second;
				strncpy(sat->elements[1], qPrintable(newTLE[sat->designation].first), 80);
				strncpy(sat->elements[2], qPrintable(newTLE[sat->designation].second), 80);
				numUpdated++;
				qDebug() << "updated orbital elements for: " << sat->designation;
				sats[sat->designation] = satMap;
			}
		}
	}
	map["satellites"] = sats;

	if (numUpdated>0)
	{
		satelliteJsonFile.remove();
		if (!satelliteJsonFile.open(QIODevice::WriteOnly))
		{
			qWarning() << "Satellites::updateFromFiles cannot open for writing " << satellitesJsonPath;
		}
		else
		{
			qDebug() << "Satellites::updateFromFiles Writing updated JSON file";
			parser.write(map, &satelliteJsonFile);
			satelliteJsonFile.close();
		}
	}

	delete progressBar;
	progressBar = NULL;

	qDebug() << "Satellites::updateFromFiles updated orbital elements for " << numUpdated << " satellites";
	if (numUpdated==0)
		updateState = CompleteNoUpdates;
	else
		updateState = CompleteUpdates;

	emit(updateStateChanged(updateState));
	emit(TleUpdateComplete(numUpdated));
}

void Satellites::update(double deltaTime)
{
	if (StelApp::getInstance().getCore()->getNavigator()->getCurrentLocation().planetName != earth->getEnglishName() || (!hintFader && hintFader.getInterstate() <= 0.))
		return;

	hintFader.update((int)(deltaTime*1000));

	foreach(const SatelliteP& sat, satellites)
	{
		if (sat->initialized && sat->visible)
			sat->update(deltaTime);
	}
}

void Satellites::draw(StelCore* core)
{
	if (core->getNavigator()->getCurrentLocation().planetName != earth->getEnglishName() || (!hintFader && hintFader.getInterstate() <= 0.))
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);
	painter.setFont(labelFont);
	Satellite::hintBrightness = hintFader.getInterstate();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	Satellite::hintTexture->bind();
	foreach (const SatelliteP& sat, satellites)
	{
		if (sat && sat->initialized && sat->visible)
			sat->draw(core, painter, 1.0);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
}

void Satellites::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelNavigator* nav = core->getNavigator();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Satellite");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(nav);
		Vec3d screenpos;

		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos))
			return;
		glColor3f(0.4f,0.5f,0.8f);
		texPointer->bind();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		// Size on screen
		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();
		size += 12.f + 3.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		// size+=20.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]-size/2, 20, 90);
		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]+size/2, 20, 0);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]+size/2, 20, -90);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]-size/2, 20, -180);
	}
}


