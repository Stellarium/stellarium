/*
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

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelIniParser.hpp"
#include "StelFileMgr.hpp"
#include "LabelMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "MeteorShower.hpp"
#include "MeteorShowers.hpp"
#include "MeteorShowerDialog.hpp"
#include "MeteorStream.hpp"
#include "StelProgressController.hpp"

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
#include <QSharedPointer>
#include <QStringList>
#include <QDir>

#define CATALOG_FORMAT_VERSION 1 /* Version of format of catalog */

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
	info.description = N_("This plugin show information about meteor showers and displays marker for radiants near maximum activity for each meteor showers.");
	return info;
}

/*
 Constructor
*/
MeteorShowers::MeteorShowers()
	: flagShowMS(false)
	, OnIcon(NULL)
	, OffIcon(NULL)
	, GlowIcon(NULL)
	, toolbarButton(NULL)
	, progressBar(NULL)
{
	setObjectName("MeteorShowers");
	configDialog = new MeteorShowerDialog();
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(conf->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
MeteorShowers::~MeteorShowers()
{
	delete configDialog;

	if(GlowIcon)
		delete GlowIcon;
	if(OnIcon)
		delete OnIcon;
	if(OffIcon)
		delete OffIcon;

	active.clear();
	activeInfo.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double MeteorShowers::getCallOrder(StelModuleActionName actionName) const
{
	if(actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName)+0;
	return 0;
}

void MeteorShowers::init()
{
	upgradeConfigIni();

	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/MeteorShowers");

		// If no settings in the main config file, create with defaults
		if(!conf->childGroups().contains("MeteorShowers"))
		{
			qDebug() << "MeteorShowers::init no MeteorShower section exists in main config file - creating with defaults";
			restoreDefaultConfigIni();
		}

		// populate settings from main config file.
		readSettingsFromConfig();

		showersJsonPath = StelFileMgr::findFile("modules/MeteorShowers", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/showers.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");
		MeteorShower::radiantTexture = StelApp::getInstance().getTextureManager().createTexture(":/MeteorShowers/radiant.png");

		// key bindings and other actions
		addAction("actionShow_MeteorShower", N_("Meteor Shower"), N_("Show meteor showers"), "msVisible", "Ctrl+Alt+M");
		addAction("actionShow_MeteorShower_ConfigDialog", N_("Meteor Shower"), N_("Meteor Shower configuration window"), configDialog, "visible");

		GlowIcon = new QPixmap(":/graphicsGui/glow32x32.png");
		OnIcon = new QPixmap(":/MeteorShowers/btMS-on.png");
		OffIcon = new QPixmap(":/MeteorShowers/btMS-off.png");

		setFlagShowMSButton(flagShowMSButton);
		setFlagShowMS(getEnableAtStartup());
	}
	catch(std::runtime_error &e)
	{
		qWarning() << "MeteorShowers::init error: " << e.what();
		return;
	}

	// A timer for hiding alert messages
	messageTimer = new QTimer(this);
	messageTimer->setSingleShot(true);   // recurring check for update
	messageTimer->setInterval(9000);      // 6 seconds should be enough time
	messageTimer->stop();
	connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

	// If the json file does not already exist, create it from the resource in the QT resource
	if(!QFileInfo(showersJsonPath).exists())
	{
		if(!checkJsonFileFormat() || getJsonFileFormatVersion()<CATALOG_FORMAT_VERSION)
		{
			displayMessage(q_("The old showers.json file is no longer compatible - using default file"), "#bb0000");
			restoreDefaultJsonFile();
		}
		else
		{
			qDebug() << "MeteorShowers::init showers.json does not exist - copying default file to " << QDir::toNativeSeparators(showersJsonPath);
			restoreDefaultJsonFile();
		}
	}

	qDebug() << "MeteorShowers::init using file: " << QDir::toNativeSeparators(showersJsonPath);

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
	if(toolbarButton != NULL)
		toolbarButton->setChecked(b);

	flagShowMS=b;
}

// Define whether the button toggling meteor showers should be visible
void MeteorShowers::setFlagShowMSButton(bool b)
{
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(b==true)
	{
		if(toolbarButton==NULL)
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

bool MeteorShowers::changedSkyDate(StelCore* core)
{
	double JD = core->getJDay();
	skyDate = StelUtils::jdToQDateTime(JD+StelUtils::getGMTShiftFromQT(JD)/24-core->getDeltaT(JD)/86400);
	if(skyDate.toString("MM.dd.yyyy") != lastSkyDate.toString("MM.dd.yyyy"))  //if the sky date changed
		return true;
	else
		return false;
}

void MeteorShowers::draw(StelCore* core)
{
	if(!flagShowMS)
		return;

	StelPainter painter(core->getProjection(StelCore::FrameJ2000));
	drawMarker(core, painter);

	if(GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

	painter.setProjector(core->getProjection(StelCore::FrameAltAz));
	drawStream(core, painter);
}

void MeteorShowers::drawStream(StelCore* core, StelPainter& painter)
{
	LandscapeMgr* landmgr = (LandscapeMgr*)StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr");
	if(landmgr->getFlagAtmosphere() && landmgr->getLuminance()>5)
		return;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	painter.enableTexture2d(false);

	int index = 0;
	if(active.size() > 0)
	{
		foreach(const activeData &a, activeInfo)
		{
			(void)a;
			// step through and draw all active meteors
			for(std::vector<MeteorStream*>::iterator iter = active[index].begin(); iter != active[index].end(); ++iter)
			{
				(*iter)->draw(core, painter);
			}
			index++;
		}
	}
}

void MeteorShowers::drawMarker(StelCore* core, StelPainter& painter)
{
	painter.setFont(font);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if(ms && ms->initialized)
			ms->draw(painter);
	}
	glDisable(GL_TEXTURE_2D);
}

void MeteorShowers::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("MeteorShower");
	if(!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if(!painter.getProjector()->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();
		size+=20.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());

		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]-size/2, 10.f, 90);
		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]+size/2, 10.f, 0);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]+size/2, 10.f, -90);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]-size/2, 10.f, -180);
		painter.setColor(1,1,1,0);
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
	if(zhr != -1)  //isn't variable
	{
		highZHR = zhr;
		lowZHR = 0;
	}
	else
	{
		QStringList varZHR = variable.split("-");
		lowZHR = varZHR.at(0).toInt();
		if(varZHR.at(1).contains("*"))
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

	double gaussian = highZHR * std::exp( - std::pow(currentJD - peakJD, 2) / (sd*sd) ) + lowZHR;

	return (int) ((int) ((gaussian - (int) gaussian) * 10) >= 5 ? gaussian+1 : gaussian);
}

void MeteorShowers::updateActiveInfo(StelCore* core)
{
	//check if the sky date changed
	bool changedDate = changedSkyDate(core);

	if(changedDate)
		activeInfo.clear(); //clear data of all MS active

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if(ms && ms->initialized)
		{
			//if the meteor shower is active, get data
			if(ms->isActive)
			{
				//First, check if there is already data about the constellation in "activeInfo"
				//The var "index" will be updated to show the correct place do put the new information
				int index = 0;
				foreach(const activeData &a, activeInfo)
				{
					if(a.showerID == ms->showerID)  //exists
						break;
					index++;
				}

				if(activeInfo.size() < index + 1) //new?, put in the end
				{
					activeData newData;
					newData.showerID = ms->showerID;
					newData.speed = ms->speed;
					newData.radiantAlpha = ms->radiantAlpha;
					newData.radiantDelta = ms->radiantDelta;
					newData.zhr = ms->zhr;
					newData.variable = ms->variable;
					newData.start = ms->start;
					newData.finish = ms->finish;
					newData.peak = ms->peak;
					activeInfo.append(newData);
				}
				else //just overwrites
				{
					activeInfo[index].zhr = ms->zhr;
					activeInfo[index].variable = ms->variable;
					activeInfo[index].start = ms->start;
					activeInfo[index].finish = ms->finish;
					activeInfo[index].peak = ms->peak;
				}
			}
		}
	}
	lastSkyDate = skyDate;
}

void MeteorShowers::update(double deltaTime)
{
	if(!flagShowMS)
		return;

	StelCore* core = StelApp::getInstance().getCore();

	double timeRate = core->getTimeRate();
	if(timeRate > 0.2)
		return;

	updateActiveInfo(core);

	deltaTime*=1000;

	std::vector<std::vector<MeteorStream*> >::iterator iterOut;
	std::vector<MeteorStream*>::iterator iterIn;
	int index = 0;
	if(active.size() > 0)
	{
		// step through and update all active meteors
		for(iterOut = active.begin(); iterOut != active.end(); ++iterOut)
		{
			for(iterIn = active[index].begin(); iterIn != active[index].end(); ++iterIn)
			{
				if(!(*iterIn)->update(deltaTime))
				{
					delete *iterIn;
					active[index].erase(iterIn);
					iterIn--;
				}
			}
			if(active[index].empty())
			{
				active.erase(iterOut);
				iterOut--;
				index--;
			}
			index++;
		}
	}

	index = 0;
	foreach(const activeData &a, activeInfo)
	{
		ZHR = calculateZHR(a.zhr, a.variable, a.start, a.finish, a.peak);

		// only makes sense given lifetimes of meteors to draw when timeSpeed is realtime
		// otherwise high overhead of large numbers of meteors
		double tspeed = timeRate*86400;  // sky seconds per actual second
		if(tspeed<=0 || fabs(tspeed)>1.)
			return; // don't start any more meteors

		// if stellarium has been suspended, don't create huge number of meteors to
		// make up for lost time!
		if(deltaTime > 500)
			deltaTime = 500;

		// determine average meteors per frame needing to be created
		int mpf = (int)((double)ZHR*zhrToWsr*deltaTime/1000.0 + 0.5);
		if(mpf<1)
			mpf = 1;

		std::vector<MeteorStream*> aux;
		if(active.empty() || active.size() < (unsigned) index+1)
			active.push_back(aux);

		for(int i=0; i<mpf; ++i)
		{
			// start new meteor based on ZHR time probability
			double prob = ((double)rand())/RAND_MAX;
			if(ZHR>0 && prob<((double)ZHR*zhrToWsr*deltaTime/1000.0/(double)mpf))
			{
				MeteorStream *m = new MeteorStream(StelApp::getInstance().getCore(), a.speed, a.radiantAlpha, a.radiantDelta);
				active[index].push_back(m);
			}
		}
		index++;
	}
}

QList<StelObjectP> MeteorShowers::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	if(!flagShowMS)
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if(ms->initialized)
		{
			equPos = ms->XYZ;
			equPos.normalize();
			if(equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(ms));
			}
		}
	}

	return result;
}

StelObjectP MeteorShowers::searchByName(const QString& englishName) const
{
	if(!flagShowMS)
		return NULL;

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if(ms->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(ms);
	}

	return NULL;
}

StelObjectP MeteorShowers::searchByNameI18n(const QString& nameI18n) const
{
	if(!flagShowMS)
		return NULL;

	foreach(const MeteorShowerP& ms, mShowers)
	{
		if(ms->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(ms);
	}

	return NULL;
}

QStringList MeteorShowers::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if(!flagShowMS)
		return result;

	if(maxNbItem==0)
		return result;

	QString sn;
	bool find;

	foreach(const MeteorShowerP& ms, mShowers)
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

	result.sort();

	if(result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList MeteorShowers::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if(!flagShowMS)
		return result;

	if(maxNbItem==0)
		return result;

	QString sn;
	bool find;
	foreach(const MeteorShowerP& ms, mShowers)
	{
		sn = ms->getEnglishName();
		find = false;
		if(useStartOfWords)
		{
			if(objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
				find = true;
		}
		else
		{
			if(sn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if(find)
		{
			result << sn;
		}

		sn = ms->getDesignation();
		find = false;
		if(useStartOfWords)
		{
			if(objPrefix.toUpper()==sn.toUpper().left(objPrefix.length()))
				find = true;
		}
		else
		{
			if(sn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if(find)
		{
			result << sn;
		}
	}

	result.sort();
	if(result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

QStringList MeteorShowers::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if(inEnglish)
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
	if(show)
		configDialog->setVisible(true);

	return true;
}

QVariantMap MeteorShowers::loadShowersMap(QString path)
{
	if(path.isEmpty())
		path = showersJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if(!jsonFile.open(QIODevice::ReadOnly))
		qWarning() << "MeteorShowers::loadShowersMap cannot open " << path;
	else
		map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
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
		if(ms->initialized)
			mShowers.append(ms);
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

	conf->setValue("enable_at_startup", false);
	conf->setValue("updates_enabled", true);
	conf->setValue("url", "http://stellarium.org/json/showers.json");
	conf->setValue("update_frequency_hours", 100);
	conf->setValue("flag_show_ms_button", true);
	conf->endGroup();
}

void MeteorShowers::restoreDefaultJsonFile(void)
{
	if(QFileInfo(showersJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/MeteorShowers/showers.json");
	if(!src.copy(showersJsonPath))
	{
		qWarning() << "MeteorShowers::restoreDefaultJsonFile cannot copy json resource to " + showersJsonPath;
	}
	else
	{
		qDebug() << "MeteorShowers::init copied default showers.json to " << showersJsonPath;
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
	if(!old.exists())
	{
		qWarning() << "MeteorShowers::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = showersJsonPath + ".old";
	if(QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if(old.copy(backupPath))
	{
		if(deleteOriginal)
		{
			if(!old.remove())
			{
				qWarning() << "MeteorShowers::backupJsonFile WARNING - could not remove old showers.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "MeteorShowers::backupJsonFile WARNING - failed to copy showers.json to showers.json.old";
		return false;
	}

	return true;
}

int MeteorShowers::getJsonFileFormatVersion(void)
{
	int jsonVersion = -1;
	QFile showersJsonFile(showersJsonPath);
	if(!showersJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "MeteorShowers::init cannot open " << QDir::toNativeSeparators(showersJsonPath);
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&showersJsonFile).toMap();
	if(map.contains("version"))
	{
		jsonVersion = map.value("version").toInt();
	}

	showersJsonFile.close();
	qDebug() << "MeteorShowers::getJsonFileFormatVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

bool MeteorShowers::checkJsonFileFormat()
{
	QFile showersJsonFile(showersJsonPath);
	if(!showersJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "MeteorShowers::init cannot open " << QDir::toNativeSeparators(showersJsonPath);
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
		qDebug() << "MeteorShowers::checkJsonFileFormat(): file format is wrong!";
		qDebug() << "MeteorShowers::checkJsonFileFormat() error:" << e.what();
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
	enableAtStartup = conf->value("enable_at_startup", false).toBool();
	flagShowMSButton = conf->value("flag_show_ms_button", true).toBool();

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

	conf->endGroup();
}

int MeteorShowers::getSecondsToUpdate(void)
{
	QDateTime nextUpdate = lastUpdate.addSecs(updateFrequencyHours * 3600);
	return QDateTime::currentDateTime().secsTo(nextUpdate);
}

void MeteorShowers::checkForUpdate(void)
{
	if(updatesEnabled && lastUpdate.addSecs(updateFrequencyHours * 3600) <= QDateTime::currentDateTime())
		updateJSON();
}

void MeteorShowers::updateJSON(void)
{
	if(updateState==MeteorShowers::Updating)
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

	if(progressBar==NULL)
		progressBar = StelApp::getInstance().addProgressBar();

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
	if(reply->error() != QNetworkReply::NoError)
	{
		qWarning() << "MeteorShowers::updateDownloadComplete FAILED to download" << reply->url() << " Error: " << reply->errorString();
	}
	else
	{
		// download completed successfully.
		QString jsonFilePath = StelFileMgr::findFile("modules/MeteorShowers", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory)) + "/showers.json";
		if(jsonFilePath.isEmpty())
		{
			qWarning() << "MeteorShowers::updateDownloadComplete: cannot write JSON data to file";
		}
		else
		{
			QFile jsonFile(jsonFilePath);
			if(jsonFile.exists())
				jsonFile.remove();

			jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
			jsonFile.write(reply->readAll());
			jsonFile.close();
		}
	}

	if(progressBar)
	{
		progressBar->setValue(100);
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = NULL;
	}
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

	// List of parent objects for meteor showers
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Asteroid 2003 EH1");
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
	N_("Asteroid (4450) Pan");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 26P/Grigg-Skjellerup");
	// TRANSLATORS: Name of parent object for meteor shower
	N_("Comet 21P/Giacobini-Zinner");

	/* For copy/paste:
	// TRANSLATORS: Name of meteor shower
	N_("");
	*/

#endif
}
