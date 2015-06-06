/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013 Marcos Cardinot
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

#include <QDir>
#include <QtMath>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

#include "LabelMgr.hpp"
#include "LandscapeMgr.hpp"
#include "MeteorMgr.hpp"
#include "MeteorShowerDialog.hpp"
#include "MeteorShowers.hpp"
#include "Planet.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelJsonParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelProgressController.hpp"
#include "StelTextureMgr.hpp"

#define CATALOG_FORMAT_VERSION 1 /* Version of format of catalog */

const double MeteorShowers::zhrToWsr = MeteorMgr::zhrToWsr;

/*
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*/
StelModule* MeteorShowersStelPluginInterface::getStelModule() const
{
	return new MeteorShowers();
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

/*
 Constructor
*/
MeteorShowers::MeteorShowers()
	: flagShowMS(false)
	, flagShowMSButton(true)
	, OnIcon(NULL)
	, OffIcon(NULL)
	, GlowIcon(NULL)
	, toolbarButton(NULL)
	, updateState(CompleteNoUpdates)
	, downloadMgr(NULL)
	, progressBar(NULL)
	, updateTimer(0)
	, messageTimer(0)
	, updatesEnabled(false)
	, updateFrequencyHours(0)
	, enableAtStartup(false)
	, flagShowARG(true)
	, flagShowARR(true)
	, flagShowIR(true)
{
	setObjectName("MeteorShowers");
	configDialog = new MeteorShowerDialog();
	conf = StelApp::getInstance().getSettings();
	qsrand(QDateTime::currentMSecsSinceEpoch());
}

/*
 Destructor
*/
MeteorShowers::~MeteorShowers()
{
	delete configDialog;

	if (GlowIcon)
	{
		delete GlowIcon;
	}

	if (OnIcon)
	{
		delete OnIcon;
	}

	if (OffIcon)
	{
		delete OffIcon;
	}

	active.clear();
	activeInfo.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double MeteorShowers::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
	{
		return GETSTELMODULE(MeteorMgr)->getCallOrder(actionName)+10.;
	}

	return 0;
}

void MeteorShowers::init()
{
	upgradeConfigIni();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/MeteorShowers");

		// If no settings in the main config file, create with defaults
		if (!conf->childGroups().contains("MeteorShowers"))
		{
			qDebug() << "MeteorShowers: no MeteorShower section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		showersJsonPath = StelFileMgr::findFile("modules/MeteorShowers", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/showers.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");
		MeteorShower::radiantTexture = StelApp::getInstance().getTextureManager().createTexture(":/MeteorShowers/radiant.png");

		// key bindings and other actions
		QString msGroup = N_("Meteor Showers");
		addAction("actionShow_MeteorShower", msGroup, N_("Show meteor showers"), "msVisible", "Ctrl+Alt+M");
		addAction("actionShow_radiant_Labels", msGroup, N_("Radiant labels"), "labelsVisible", "Shift+M");
		addAction("actionShow_MeteorShower_ConfigDialog", msGroup, N_("Meteor Shower configuration window"), configDialog, "visible", "Ctrl+Shift+M");

		GlowIcon = new QPixmap(":/graphicGui/glow32x32.png");
		OnIcon = new QPixmap(":/MeteorShowers/btMS-on.png");
		OffIcon = new QPixmap(":/MeteorShowers/btMS-off.png");

		setFlagShowMSButton(flagShowMSButton);
		setFlagShowMS(getEnableAtStartup());
	}
	catch(std::runtime_error &e)
	{
		qWarning() << "MeteorShowers: init error:" << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the QT resource
	if (!QFileInfo(showersJsonPath).exists())
	{
		qDebug() << "MeteorShowers: showers.json does not exist - copying default file to" << QDir::toNativeSeparators(showersJsonPath);
		restoreDefaultJsonFile();
	}
	// Validate JSON format and data
	if (!checkJsonFileFormat() || getJsonFileFormatVersion()<CATALOG_FORMAT_VERSION)
	{
		displayMessage(q_("The old showers.json file is no longer compatible - using default file"), "#bb0000");
		restoreDefaultJsonFile();
	}

	qDebug() << "MeteorShowers: loading catalog file:" << QDir::toNativeSeparators(showersJsonPath);

	// create meteor Showers according to content os Showers.json file
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

	// skyDate startup
	skyDate = StelUtils::jdToQDateTime(StelApp::getInstance().getCore()->getJDay());

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void MeteorShowers::deinit()
{
	MeteorShower::radiantTexture.clear();
	texPointer.clear();
}

void MeteorShowers::upgradeConfigIni(void)
{
	// Upgrade settings for MeteorShower plugin
	if (conf->contains("MeteorShowers/flag_show_ms"))
	{
		bool b = conf->value("MeteorShowers/flag_show_ms", false).toBool();
		if (!conf->contains("MeteorShowers/enable_at_startup"))
			conf->setValue("MeteorShowers/enable_at_startup", b);
		conf->remove("MeteorShowers/flag_show_ms");
	}
}

void MeteorShowers::setFlagShowMS(bool b)
{
	if (toolbarButton != NULL)
	{
		toolbarButton->setChecked(b);
	}

	flagShowMS=b;
}

// Define whether the button toggling meteor showers should be visible
void MeteorShowers::setFlagShowMSButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui != NULL)
	{
		if (b)
		{
			if (toolbarButton == NULL)
			{
				// Create the MeteorShowers button
				toolbarButton = new StelButton(NULL, *OnIcon, *OffIcon, *GlowIcon, "actionShow_MeteorShower");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
		else
		{
			gui->getButtonBar()->hideButton("actionShow_MeteorShower");
		}
		flagShowMSButton = b;
	}
}

bool MeteorShowers::changedSkyDate(StelCore* core)
{
	double JD = core->getJDay();
	skyDate = StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);
	if (skyDate.toString("MM.dd.yyyy") != lastSkyDate.toString("MM.dd.yyyy"))  //if the sky date changed
	{
		return true;
	}
	else
	{
		return false;
	}
}

void MeteorShowers::draw(StelCore* core)
{
	if (!getFlagShowMS())
	{
		return;
	}

	StelPainter painter(core->getProjection(StelCore::FrameJ2000));
	drawMarker(core, painter);

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, painter);
	}

	painter.setProjector(core->getProjection(StelCore::FrameAltAz));
	drawStream(core, painter);
}

void MeteorShowers::drawMarker(StelCore* core, StelPainter& painter)
{
	Q_UNUSED(core);
	painter.setFont(labelFont);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	foreach (const MeteorShowerP& ms, mShowers)
	{
		ms->updateCurrentData(skyDate);

		if (ms && ms->initialized && ms->active)
		{
			ms->draw(painter);
		}
	}
	glDisable(GL_TEXTURE_2D);
}

void MeteorShowers::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("MeteorShower");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
		{
			return;
		}

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();
		size += 20.f + 10.f * qSin(2.f * StelApp::getInstance().getTotalRunTime());

		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]-size/2, 10.f, 90);
		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]+size/2, 10.f, 0);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]+size/2, 10.f, -90);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]-size/2, 10.f, -180);
		painter.setColor(1,1,1,0);
	}
}

void MeteorShowers::drawStream(StelCore* core, StelPainter& painter)
{
	if (!core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		return;
	}

	LandscapeMgr* landmgr = GETSTELMODULE(LandscapeMgr);
	if (landmgr->getFlagAtmosphere() && landmgr->getLuminance()>5)
	{
		return;
	}

	int index = 0;
	if (active.size() > 0)
	{
		foreach (const activeData &a, activeInfo)
		{
			Q_UNUSED(a);
			// step through and draw all active meteors
			std::vector<MeteorStream*>::iterator iter;
			for (iter = active[index].begin(); iter != active[index].end(); ++iter)
			{
				(*iter)->draw(core, painter);
			}
			index++;
		}
	}
}

int MeteorShowers::calculateZHR(int zhr, QString variable, QDateTime start, QDateTime finish, QDateTime peak)
{
	/***************************************
	 * Get ZHR ranges
	 ***************************************/
	int highZHR;
	int lowZHR;
	//bool multPeak = false; //multiple peaks
	if (zhr != -1)  //isn't variable
	{
		highZHR = zhr;
		lowZHR = 0;
	}
	else
	{
		QStringList varZHR = variable.split("-");
		lowZHR = varZHR.at(0).toInt();
		if (varZHR.at(1).contains("*"))
		{
			//multPeak = true;
			highZHR = varZHR[1].replace("*", "").toInt();
		}
		else
		{
			highZHR = varZHR.at(1).toInt();
		}
	}

	/***************************************
	 * Convert time intervals
	 ***************************************/
	double startJD = StelUtils::qDateTimeToJd(start);
	double finishJD = StelUtils::qDateTimeToJd(finish);
	double peakJD = StelUtils::qDateTimeToJd(peak);
	double currentJD = StelUtils::qDateTimeToJd(skyDate);

	/***************************************
	 * Gaussian distribution
	 ***************************************/
	double sd; //standard deviation
	if (currentJD >= startJD && currentJD < peakJD) //left side of gaussian
		sd = (peakJD - startJD)/2;
	else
		sd = (finishJD - peakJD)/2;

	double gaussian = highZHR * qExp( - qPow(currentJD - peakJD, 2) / (sd*sd) ) + lowZHR;

	return (int) ((int) ((gaussian - (int) gaussian) * 10) >= 5 ? gaussian+1 : gaussian);
}

void MeteorShowers::updateActiveInfo(void)
{
	foreach (const MeteorShowerP& ms, mShowers)
	{
		if (ms && ms->initialized)
		{
			//if the meteor shower is active, get data
			if (ms->getStatus())
			{
				//First, check if there is already data about the constellation in "activeInfo"
				//The var "index" will be updated to show the correct place do put the new information
				int index = 0;
				foreach(const activeData &a, activeInfo)
				{
					if (a.showerID == ms->showerID)  //exists
						break;
					index++;
				}

				if (activeInfo.size() < index + 1) //new?, put in the end
				{
					activeData newData;
					newData.showerID = ms->showerID;
					newData.speed = ms->speed;
					newData.radiantAlpha = ms->radiantAlpha;
					newData.radiantDelta = ms->radiantDelta;
					newData.pidx = ms->pidx;
					newData.zhr = ms->zhr;
					newData.variable = ms->variable;
					newData.start = ms->start;
					newData.finish = ms->finish;
					newData.peak = ms->peak;
					newData.status = ms->status;
					newData.colors = ms->colors;
					activeInfo.append(newData);
				}
				else //just overwrites
				{
					activeInfo[index].zhr = ms->zhr;
					activeInfo[index].variable = ms->variable;
					activeInfo[index].start = ms->start;
					activeInfo[index].finish = ms->finish;
					activeInfo[index].peak = ms->peak;
					activeInfo[index].status = ms->status;
				}
			}
		}
	}
	lastSkyDate = skyDate;
}

void MeteorShowers::update(double deltaTime)
{
	if (!getFlagShowMS())
	{
		return;
	}

	StelCore* core = StelApp::getInstance().getCore();

	double tspeed = core->getTimeRate()*86400;  // sky seconds per actual second
	if (!tspeed) { // is paused?
		return; // freeze meteors at the current position
	}

	deltaTime*=1000;
	// if stellarium has been suspended, don't create huge number of meteors to
	// make up for lost time!
	if (deltaTime > 500)
	{
		deltaTime = 500;
	}

	QList<activeData> old_activeInfo;

	//check if the sky date changed
	if (changedSkyDate(core))
	{
		// clear data of all MS active
		old_activeInfo = activeInfo;
		activeInfo.clear();

		// Is GUI visible and the year changed? refresh ranges
		if (configDialog->visible() && lastSkyDate.toString("yyyy") != skyDate.toString("yyyy"))
			configDialog->refreshRangeDates();
	}

	// fill `activeInfo` with all current active meteor showers
	updateActiveInfo();
	// counting index of `active`
	int index = 0;
	// something changed? check if we have some MS to remove from `active`
	if (!old_activeInfo.isEmpty())
	{
		for (int i=0; i<old_activeInfo.size(); i++)
		{
			bool found = false;
			for (int j=0; j<activeInfo.size(); j++)
			{
				if (old_activeInfo[i].showerID == activeInfo[j].showerID)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				active.erase(active.begin().operator +(index));
				index--;
			}
			index++;
		}
	}

	std::vector<std::vector<MeteorStream*> >::iterator iterOut;
	std::vector<MeteorStream*>::iterator iterIn;
	index = 0;
	if (active.size() > 0)
	{
		// step through and update all active meteors
		for (iterOut = active.begin(); iterOut != active.end(); ++iterOut)
		{
			for (iterIn = active[index].begin(); iterIn != active[index].end(); ++iterIn)
			{
				if (!(*iterIn)->update(deltaTime))
				{
					delete *iterIn;
					iterIn = active[index].erase(iterIn);
					iterIn--;
				}
			}
			if (active[index].empty())
			{
				iterOut = active.erase(iterOut);
				iterOut--;
				index--;
			}
			index++;
		}
	}

	index = 0;
	foreach (const activeData &current, activeInfo)
	{
		std::vector<MeteorStream*> aux;
		if (active.empty() || active.size() < (unsigned) activeInfo.size())
		{
			if(tspeed<0 || fabs(tspeed)>1.)
			{
				activeInfo.removeAt(index);
				continue; // don't create new meteors
			}
			active.push_back(aux);
		}

		int ZHR = calculateZHR(current.zhr,
				       current.variable,
				       current.start,
				       current.finish,
				       current.peak);

		// determine average meteors per frame needing to be created
		int mpf = (int)((double)ZHR*zhrToWsr*deltaTime/1000.0 + 0.5);
		if (mpf<1)
		{
			mpf = 1;
		}

		for (int i=0; i<mpf; ++i)
		{
			// start new meteor based on ZHR time probability
			double prob = ((double)qrand())/RAND_MAX;
			if (ZHR>0 && prob<((double)ZHR*zhrToWsr*deltaTime/1000.0/(double)mpf))
			{
				MeteorStream *m = new MeteorStream(core,
								   current.speed,
								   current.radiantAlpha,
								   current.radiantDelta,
								   current.pidx,
								   current.colors);
				active[index].push_back(m);
			}
		}
		index++;
	}
}

QList<MeteorShowerP> MeteorShowers::searchEvents(QDate dateFrom, QDate dateTo) const
{
	QList<MeteorShowerP> result;
	QDate date;

	foreach(const MeteorShowerP& ms, mShowers)
	{
		date = dateFrom;
		while(date.operator <=(dateTo))
		{
			ms->updateCurrentData((QDateTime) date);
			if (ms->initialized && ms->status)
			{
				result.append(ms);
				break;
			}
			date = date.addDays(1);
		}
	}

	return result;
}

QList<StelObjectP> MeteorShowers::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	if (!getFlagShowMS() || !getFlagRadiant())
	{
		return result;
	}

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if (ms->initialized && ms->active)
		{
			equPos = ms->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(ms));
			}
		}
	}

	return result;
}

StelObjectP MeteorShowers::searchByName(const QString& englishName) const
{
	if (!getFlagShowMS() || !getFlagRadiant())
	{
		return NULL;
	}

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if (ms->initialized)
		{
			bool sameEngName = ms->getEnglishName().toUpper() == englishName.toUpper();
			bool desigIsEngName = ms->getDesignation().toUpper() == englishName.toUpper();
			bool emptyDesig = ms->getDesignation().isEmpty();
			if (sameEngName || (desigIsEngName && !emptyDesig))
			{
				return qSharedPointerCast<StelObject>(ms);
			}
		}
	}

	return NULL;
}

StelObjectP MeteorShowers::searchByNameI18n(const QString& nameI18n) const
{
	if (!getFlagShowMS() || !getFlagRadiant())
	{
		return NULL;
	}

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if (ms->initialized)
		{
			if (ms->getNameI18n().toUpper() == nameI18n.toUpper())
			{
				return qSharedPointerCast<StelObject>(ms);
			}
		}
	}

	return NULL;
}

QStringList MeteorShowers::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!getFlagShowMS() || !getFlagRadiant())
	{
		return result;
	}

	if (maxNbItem==0)
	{
		return result;
	}

	QString sn;
	bool find;

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if (ms->initialized && ms->active)
		{
			sn = ms->getNameI18n();
			find = false;
			if (useStartOfWords)
			{
				if (sn.toUpper().left(objPrefix.length()) == objPrefix.toUpper())
					find = true;
			}
			else
			{
				if (sn.contains(objPrefix, Qt::CaseInsensitive))
					find = true;
			}
			if (find)
				result << sn;
		}
	}

	result.sort();

	if (result.size()>maxNbItem)
	{
		result.erase(result.begin()+maxNbItem, result.end());
	}

	return result;
}

QStringList MeteorShowers::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (!getFlagShowMS() || !getFlagRadiant())
	{
		return result;
	}

	if (maxNbItem==0)
	{
		return result;
	}

	QString sn;
	bool find;
	foreach(const MeteorShowerP& ms, mShowers)
	{
		if (ms->initialized && ms->active)
		{
			sn = ms->getEnglishName();
			find = false;
			if (useStartOfWords)
			{
				if (objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
					find = true;
			}
			else
			{
				if (sn.contains(objPrefix, Qt::CaseInsensitive))
					find = true;
			}
			if (find)
			{
				result << sn;
			}

			sn = ms->getDesignation();
			find = false;
			if (useStartOfWords)
			{
				if (objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
					find = true;
			}
			else
			{
				if (sn.contains(objPrefix, Qt::CaseInsensitive))
					find = true;
			}
			if (find)
			{
				result << sn;
			}
		}
	}

	result.sort();
	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList MeteorShowers::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach(const MeteorShowerP& ms, mShowers)
		{
			result << ms->getEnglishName();
		}
	}
	else
	{
		foreach(const MeteorShowerP& ms, mShowers)
		{
			result << ms->getNameI18n();
		}
	}
	return result;
}

bool MeteorShowers::configureGui(bool show)
{
	if (show)
	{
		configDialog->setVisible(true);
	}

	return true;
}

QVariantMap MeteorShowers::loadShowersMap(QString path)
{
	if (path.isEmpty())
	{
		path = showersJsonPath;
	}

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "MeteorShowers: cannot open" << path;
	}
	else
	{
		map = StelJsonParser::parse(jsonFile.readAll()).toMap();
		jsonFile.close();
	}
	return map;
}

void MeteorShowers::readJsonFile(void)
{
	setShowersMap(loadShowersMap());
}

void MeteorShowers::setShowersMap(const QVariantMap& map)
{
	mShowers.clear();
	QVariantMap msMap = map.value("showers").toMap();
	foreach(QString msKey, msMap.keys())
	{
		QVariantMap msData = msMap.value(msKey).toMap();
		msData["showerID"] = msKey;

		MeteorShowerP ms(new MeteorShower(msData));
		if (ms->initialized)
		{
			mShowers.append(ms);
		}
	}
}

void MeteorShowers::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	restoreDefaultJsonFile();
	readJsonFile();
	readSettingsFromConfig();
}

void MeteorShowers::restoreDefaultConfigIni(void)
{
	conf->beginGroup("MeteorShowers");

	// delete all existing MeteorShower settings...
	conf->remove("");

	conf->setValue("enable_at_startup", true);
	conf->setValue("updates_enabled", true);
	conf->setValue("url", "http://stellarium.org/json/showers.json");
	conf->setValue("update_frequency_hours", 100);
	conf->setValue("flag_show_ms_button", true);
	conf->setValue("flag_show_radiants", true);
	conf->setValue("flag_active_radiants", true);

	conf->setValue("colorARG", "0, 255, 240");
	conf->setValue("colorARR", "255, 240, 0");
	conf->setValue("colorIR", "255, 255, 255");

	conf->setValue("show_radiant_labels", true);

	conf->endGroup();
}

void MeteorShowers::restoreDefaultJsonFile(void)
{
	if (QFileInfo(showersJsonPath).exists())
	{
		backupJsonFile(true);
	}

	QFile src(":/MeteorShowers/showers.json");
	if (!src.copy(showersJsonPath))
	{
		qWarning() << "MeteorShowers: cannot copy JSON resource to" << showersJsonPath;
	}
	else
	{
		qDebug() << "MeteorShowers: copied default showers.json to" << showersJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(showersJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);

		// Make sure that in the case where an online update has previously been done, but
		// the json file has been manually removed, that an update is schreduled in a timely
		// manner
		conf->remove("MeteorShowers/last_update");
		lastUpdate = QDateTime::fromString("2013-12-31T12:00:00", Qt::ISODate);
	}
}

bool MeteorShowers::backupJsonFile(bool deleteOriginal)
{
	QFile old(showersJsonPath);
	if (!old.exists())
	{
		qWarning() << "MeteorShowers: no file to backup";
		return false;
	}

	QString backupPath = showersJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
	{
		QFile(backupPath).remove();
	}

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "MeteorShowers: WARNING - could not remove old showers.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "MeteorShowers: WARNING - failed to copy showers.json to showers.json.old";
		return false;
	}

	return true;
}

int MeteorShowers::getJsonFileFormatVersion(void)
{
	int jsonVersion = -1;
	QFile showersJsonFile(showersJsonPath);
	if (!showersJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "MeteorShowers: cannot open" << QDir::toNativeSeparators(showersJsonPath);
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&showersJsonFile).toMap();
	if (map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	showersJsonFile.close();
	qDebug() << "MeteorShowers: version of the format of the catalog:" << jsonVersion;
	return jsonVersion;
}

bool MeteorShowers::checkJsonFileFormat()
{
	QFile showersJsonFile(showersJsonPath);
	if (!showersJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "MeteorShowers: cannot open" << QDir::toNativeSeparators(showersJsonPath);
		return false;
	}
	
	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(&showersJsonFile).toMap();
		showersJsonFile.close();
	}
	catch(std::runtime_error& e)
	{
		qDebug() << "MeteorShowers: file format is wrong! Error:" << e.what();
		return false;
	}
	
	return true;
}

void MeteorShowers::readSettingsFromConfig(void)
{
	conf->beginGroup("MeteorShowers");

	updateUrl = conf->value("url", "http://stellarium.org/json/showers.json").toString();
	updateFrequencyHours = conf->value("update_frequency_hours", 720).toInt();
	lastUpdate = QDateTime::fromString(conf->value("last_update", "2013-12-10T12:00:00").toString(), Qt::ISODate);
	updatesEnabled = conf->value("updates_enabled", true).toBool();
	enableAtStartup = conf->value("enable_at_startup", true).toBool();
	flagShowMSButton = conf->value("flag_show_ms_button", true).toBool();
	setFlagRadiant(conf->value("flag_show_radiants", true).toBool());
	setFlagActiveRadiant(conf->value("flag_active_radiants", true).toBool());

	Vec3f color;
	color = StelUtils::strToVec3f(conf->value("colorARG", "0, 255, 240").toString());
	colorARG = QColor(color[0],color[1],color[2]);
	color = StelUtils::strToVec3f(conf->value("colorARR", "255, 240, 0").toString());
	colorARR = QColor(color[0],color[1],color[2]);
	color = StelUtils::strToVec3f(conf->value("colorIR", "255, 255, 255").toString());
	colorIR = QColor(color[0],color[1],color[2]);

	MeteorShower::showLabels = conf->value("show_radiant_labels", true).toBool();
	labelFont.setPixelSize(conf->value("font_size", 13).toInt());

	conf->endGroup();
}

void MeteorShowers::saveSettingsToConfig(void)
{
	conf->beginGroup("MeteorShowers");

	conf->setValue("url", updateUrl);
	conf->setValue("update_frequency_hours", updateFrequencyHours);
	conf->setValue("updates_enabled", updatesEnabled);
	conf->setValue("enable_at_startup", enableAtStartup);
	conf->setValue("flag_show_ms_button", flagShowMSButton);
	conf->setValue("flag_show_radiants", getFlagRadiant());
	conf->setValue("flag_active_radiants", getFlagActiveRadiant());

	int r,g,b;
	colorARG.getRgb(&r,&g,&b);
	conf->setValue("colorARG", QString("%1, %2, %3").arg(r).arg(g).arg(b));
	colorARR.getRgb(&r,&g,&b);
	conf->setValue("colorARR", QString("%1, %2, %3").arg(r).arg(g).arg(b));
	colorIR.getRgb(&r,&g,&b);
	conf->setValue("colorIR", QString("%1, %2, %3").arg(r).arg(g).arg(b));

	conf->setValue("show_radiant_labels", MeteorShower::showLabels);
	conf->setValue("font_size", labelFont.pixelSize());

	conf->endGroup();
}

int MeteorShowers::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyHours * 3600);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void MeteorShowers::checkForUpdate(void)
{
	if (updatesEnabled && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime())
	{
		updateJSON();
	}
}

void MeteorShowers::updateJSON(void)
{
	if (updateState == MeteorShowers::Updating)
	{
		qWarning() << "MeteorShowers: already updating...  will not start again current update is complete.";
		return;
	}
	else
	{
		qDebug() << "MeteorShowers: starting update...";
	}

	lastUpdate = QDateTime::currentDateTime();
	conf->setValue("MeteorShowers/last_update", lastUpdate.toString(Qt::ISODate));

	emit(jsonUpdateComplete());

	updateState = MeteorShowers::Updating;

	emit(updateStateChanged(updateState));
	updateFile.clear();

	if (progressBar == NULL)
	{
		progressBar = StelApp::getInstance().addProgressBar();
	}

	progressBar->setValue(0);
	progressBar->setRange(0, 100);
	progressBar->setFormat("Update meteor showers");

	QNetworkRequest request;
	request.setUrl(QUrl(updateUrl));
	request.setRawHeader("User-Agent", QString("Mozilla/5.0 (Stellarium Meteor Showers Plugin %1; http://stellarium.org/)").arg(METEORSHOWERS_PLUGIN_VERSION).toUtf8());
	downloadMgr->get(request);

	updateState = MeteorShowers::CompleteUpdates;
	emit(updateStateChanged(updateState));
	emit(jsonUpdateComplete());
}

void MeteorShowers::updateDownloadComplete(QNetworkReply* reply)
{
	// check the download worked, and save the data to file if this is the case.
	if (reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "MeteorShowers: FAILED to download" << reply->url() << " Error:" << reply->errorString();
	}
	else
	{
		// download completed successfully.
		QString jsonFilePath = StelFileMgr::findFile("modules/MeteorShowers", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/showers.json";
		if (jsonFilePath.isEmpty())
		{
			qWarning() << "MeteorShowers: cannot write JSON data to file";
		}
		else
		{
			QFile jsonFile(jsonFilePath);
			if (jsonFile.exists())
			{
				jsonFile.remove();
			}

			if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				jsonFile.write(reply->readAll());
				jsonFile.close();
			}
		}
	}

	if (progressBar)
	{
		progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = NULL;
	}

	readJsonFile();
}

void MeteorShowers::displayMessage(const QString& message, const QString hexColor)
{
	messageIDs << GETSTELMODULE(LabelMgr)->labelScreen(message, 30, 30 + (20*messageIDs.count()), true, 16, hexColor);
	messageTimer->start();
}

void MeteorShowers::messageTimeout(void)
{
	foreach(int i, messageIDs)
	{
		GETSTELMODULE(LabelMgr)->deleteLabel(i);
	}
}

bool MeteorShowers::getFlagLabels()
{
	return MeteorShower::showLabels;
}

void MeteorShowers::setFlagLabels(bool b)
{
	if (MeteorShower::showLabels != b)
		MeteorShower::showLabels = b;
}

void MeteorShowers::setLabelFontSize(int size)
{
	if (labelFont.pixelSize() != size)
		labelFont.setPixelSize(size);
}

bool MeteorShowers::getFlagShowMS(void) const
{
	if (StelApp::getInstance().getCore()->getCurrentPlanet().data()->getEnglishName()!="Earth")
		return false;
	else
		return flagShowMS;
}

void MeteorShowers::translations()
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
