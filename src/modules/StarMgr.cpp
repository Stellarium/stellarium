/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 *
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006
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

// class used to manage groups of Stars

#include <config.h>
#include <QTextStream>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QRegExp>
#include <QDebug>

#include "StelProjector.hpp"
#include "StarMgr.hpp"
#include "StelObject.hpp"
#include "StelTexture.hpp"
#include "StelNavigator.hpp"
#include "StelUtils.hpp"
#include "StelToneReproducer.hpp"
#include "StelTranslator.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelLoadingBar.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "bytes.h"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelIniParser.hpp"
#include "StelStyle.hpp"
#include "StelPainter.hpp"

#include "ZoneArray.hpp"

#include <list>

#include <errno.h>
#include <unistd.h>

using namespace BigStarCatalogExtension;

static QStringList spectral_array;
static QStringList component_array;

// Initialise statics
bool StarMgr::flagSciNames = true;
double StarMgr::currentJDay = 0;
std::map<int,QString> StarMgr::commonNamesMap;
std::map<int,QString> StarMgr::commonNamesMapI18n;
std::map<QString,int> StarMgr::commonNamesIndex;
std::map<QString,int> StarMgr::commonNamesIndexI18n;
std::map<int,QString> StarMgr::sciNamesMapI18n;
std::map<QString,int> StarMgr::sciNamesIndexI18n;

QStringList initStringListFromFile(const QString& file_name)
{
	QStringList list;
	QFile f(file_name);
	if (f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		while (!f.atEnd())
		{
			QString s = QString::fromUtf8(f.readLine());
			s.chop(1);
			list << s;
		}
		f.close();
	}
	return list;
}

QString StarMgr::convertToSpectralType(int index)
{
	if (index < 0 || index >= spectral_array.size())
	{
		qDebug() << "convertToSpectralType: bad index: " << index << ", max: " << spectral_array.size();
    	return "";
	}
	return spectral_array.at(index);
}

QString StarMgr::convertToComponentIds(int index)
{
	if (index < 0 || index >= component_array.size())
	{
		qDebug() << "convertToComponentIds: bad index: " << index << ", max: " << component_array.size();
		return "";
	}
	return component_array.at(index);
}


void StarMgr::initTriangle(int lev,int index,
                           const Vec3d &c0,
                           const Vec3d &c1,
                           const Vec3d &c2) {
  ZoneArrayMap::const_iterator it(zoneArrays.find(lev));
  if (it!=zoneArrays.end()) it->second->initTriangle(index,c0,c1,c2);
}


StarMgr::StarMgr(void) : hipIndex(new HipIndexStruct[NR_OF_HIP+1]), fontSize(13.), starFont(NULL)
{
	setObjectName("StarMgr");
	if (hipIndex == 0)
	{
    	qFatal("ERROR: StarMgr::StarMgr: no memory");
	}
	maxGeodesicGridLevel = -1;
	lastMaxSearchLevel = -1;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double StarMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10;
	return 0;
}


StarMgr::~StarMgr(void)
{
	delete starSettings;
	starSettings=NULL;
	ZoneArrayMap::iterator it(zoneArrays.end());
	while (it!=zoneArrays.begin())
	{
		--it;
		delete it->second;
		it->second = NULL;
	}
	zoneArrays.clear();
	if (hipIndex)
		delete[] hipIndex;
}

QString StarMgr::getCommonName(int hip)
{
	std::map<int,QString>::const_iterator it(commonNamesMapI18n.find(hip));
	if (it!=commonNamesMapI18n.end())
		return it->second;
	return QString();
}

QString StarMgr::getSciName(int hip)
{
	std::map<int,QString>::const_iterator it(sciNamesMapI18n.find(hip));
	if (it!=sciNamesMapI18n.end())
		return it->second;
	return QString();
}

void StarMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	loadStarSettings();
	loadData();
	double fontSize = 12;
	starFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);

	setFlagStars(conf->value("astro/flag_stars", true).toBool());
	setFlagLabels(conf->value("astro/flag_star_name",true).toBool());
	setLabelsAmount(conf->value("stars/labels_amount",3).toDouble());
	
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);

	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setMinFilter(GL_LINEAR);
	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur2.png");   // Load pointer texture
	
	StelApp::getInstance().getCore()->getGeodesicGrid(maxGeodesicGridLevel)->visitTriangles(maxGeodesicGridLevel,initTriangleFunc,this);
	for (ZoneArrayMap::const_iterator it(zoneArrays.begin()); it!=zoneArrays.end();it++)
	{
		it->second->scaleAxis();
	}
}


void StarMgr::drawPointer(const StelProjectorP& prj, const StelNavigator * nav)
{
	const QList<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Star");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(nav);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos))
			return;
	
		StelPainter sPainter(prj);
		glColor3fv(obj->getInfoColor());
		float diameter = 26.f;
		texPointer->bind();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		sPainter.drawSprite2dMode(screenpos[0], screenpos[1], diameter, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

void StarMgr::setStelStyle(const StelStyle& style)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();
	QString section = style.confSectionName;
	
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelColor(StelUtils::strToVec3f(conf->value(section+"/star_label_color", defaultColor).toString()));
}

void StarMgr::loadStarSettings()
{
	QString iniFile;
	try
	{
		iniFile = StelApp::getInstance().getFileMgr().findFile("stars/default/stars.ini");
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR - could not find stars/default/stars.ini : " << e.what() << iniFile;
		return;
	}

	starSettings = new QSettings(iniFile, StelIniFormat);
	if (starSettings->status() != QSettings::NoError)
	{
		qWarning() << "ERROR while parsing " << iniFile;
		return;
	}
	starSettings->beginGroup("stars");
}

/***************************************************************************
 Load star catalogue data from files.
 If a file is not found, it will be skipped.
***************************************************************************/
void StarMgr::loadData()
{
	StelLoadingBar& lb = *StelApp::getInstance().getStelLoadingBar();
			
	// Please do not init twice:
	Q_ASSERT(maxGeodesicGridLevel < 0);

	qDebug() << "Loading star data ...";
	
	qulonglong memoryUsed = 0;
	const qulonglong maxMemory = StelApp::getInstance().getSettings()->value("stars/max_memory", 128).toULongLong() * 1024*1024;
	
	QStringList cats = starSettings->childGroups();
	QListIterator<QString> it(cats);
	StelFileMgr& fileMgr = StelApp::getInstance().getFileMgr();
	while(it.hasNext())
	{
		QString cat = it.next();
		QString cat_file_name = starSettings->value(cat+"/path").toString();
		QString cat_file_path = fileMgr.findFile("stars/default/"+cat_file_name);
		
		lb.SetMessage(q_("Loading catalog %1 from file %2").arg(cat, cat_file_name));
		memoryUsed += fileMgr.size(cat_file_path);
		ZoneArray *const z = ZoneArray::create(cat_file_name, memoryUsed > maxMemory, lb);
		if (z)
		{
			if (maxGeodesicGridLevel < z->level)
			{
				maxGeodesicGridLevel = z->level;
			}
			ZoneArray *&pos(zoneArrays[z->level]);
			if (pos)
			{
				qDebug() << cat_file_name << ", " << z->level << ": duplicate level";
				delete z;
			}
			else
			{
				pos = z;
			}
		}
	}

	for (int i=0; i<=NR_OF_HIP; i++)
	{
		hipIndex[i].a = 0;
		hipIndex[i].z = 0;
		hipIndex[i].s = 0;
	}
	for (ZoneArrayMap::const_iterator it(zoneArrays.begin());
	                it != zoneArrays.end();it++)
	{
		it->second->updateHipIndex(hipIndex);
	}

	const QString cat_hip_sp_file_name = starSettings->value("cat_hip_sp_file_name","").toString();
	if (cat_hip_sp_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_sp_file_name not found";
	}
	else
	{
		try
		{
			spectral_array = initStringListFromFile(fileMgr.findFile("stars/default/" + cat_hip_sp_file_name));
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "ERROR while loading data from "
			           << ("stars/default/" + cat_hip_sp_file_name)
			           << ": " << e.what();
		}
	}

	const QString cat_hip_cids_file_name = starSettings->value("cat_hip_cids_file_name","").toString();
	if (cat_hip_cids_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_cids_file_name not found";
	}
	else
	{
		try
		{
			component_array = initStringListFromFile(fileMgr.findFile("stars/default/" + cat_hip_cids_file_name));
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "ERROR while loading data from "
			           << ("stars/default/" + cat_hip_cids_file_name)
			           << ": " << e.what();
		}
	}

	lastMaxSearchLevel = maxGeodesicGridLevel;
	qDebug() << "Finished loading star catalogue data, max_geodesic_level: " << maxGeodesicGridLevel;
}

// Load common names from file 
int StarMgr::loadCommonNames(const QString& commonNameFile)
{
	commonNamesMap.clear();
	commonNamesMapI18n.clear();
	commonNamesIndex.clear();
	commonNamesIndexI18n.clear();

	qDebug() << "Loading star names from" << commonNameFile;
	QFile cnFile(commonNameFile);
	if (!cnFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << commonNameFile;
		return 0;
	}

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	QString record;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	// record structure is delimited with a | character.  We will
	// use a QRegExp to extract the fields. with whitespace padding permitted 
	// (i.e. it will be stripped automatically) Example record strings:
	// " 10819|c_And"
	// "113726|1_And"
	QRegExp recordRx("^\\s*(\\d+)\\s*\\|(.*)\\n");

	while(!cnFile.atEnd())
	{
		record = QString::fromUtf8(cnFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;
		if (!recordRx.exactMatch(record))
		{
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << commonNameFile 
				   << " - record does not match record pattern";
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok;
			unsigned int hip = recordRx.capturedTexts().at(1).toUInt(&ok);
			if (!ok)
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << commonNameFile 
					   << " - failed to convert " << recordRx.capturedTexts().at(1) << "to a number";
				continue;
			}
			QString englishCommonName = recordRx.capturedTexts().at(2).trimmed();
			if (englishCommonName.isEmpty())
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << commonNameFile 
					   << " - empty name field";
				continue;
			}

			englishCommonName.replace('_', ' ');
			const QString commonNameI18n = q_(englishCommonName);
			QString commonNameI18n_cap = commonNameI18n.toUpper();

			commonNamesMap[hip] = englishCommonName;
			commonNamesIndex[englishCommonName] = hip;
			commonNamesMapI18n[hip] = commonNameI18n;
			commonNamesIndexI18n[commonNameI18n_cap] = hip;
			readOk++;
		}
	}
	cnFile.close();

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "common star names";
	return 1;
}


// Load scientific names from file 
void StarMgr::loadSciNames(const QString& sciNameFile)
{
	sciNamesMapI18n.clear();
	sciNamesIndexI18n.clear();

	qDebug() << "Loading star names from" << sciNameFile;
	QFile snFile(sciNameFile);
	if (!snFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << sciNameFile;
		return;
	}

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	QString record;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	// record structure is delimited with a | character.  We will
	// use a QRegExp to extract the fields. with whitespace padding permitted 
	// (i.e. it will be stripped automatically) Example record strings:
	// " 10819|c_And"
	// "113726|1_And"
	QRegExp recordRx("^\\s*(\\d+)\\s*\\|(.*)\\n");

	while(!snFile.atEnd())
	{
		record = QString::fromUtf8(snFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;
		if (!recordRx.exactMatch(record))
		{
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << sciNameFile 
				   << " - record does not match record pattern";
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok;
			unsigned int hip = recordRx.capturedTexts().at(1).toUInt(&ok);
			if (!ok)
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << sciNameFile 
					   << " - failed to convert " << recordRx.capturedTexts().at(1) << "to a number";
				continue;
			}

			// Don't set the sci name if it's already set
			if (sciNamesMapI18n.find(hip)!=sciNamesMapI18n.end())
			{
				//qWarning() << "WARNING - duplicate name for HP" << hip << "at line" 
				//           << lineNumber << "in" << sciNameFile << "SKIPPING";
				continue;
			}

			QString sci_name_i18n = recordRx.capturedTexts().at(2).trimmed();
			if (sci_name_i18n.isEmpty())
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << sciNameFile 
					   << " - empty name field";
				continue;
			}

			sci_name_i18n.replace('_',' ');
			QString sci_name_i18n_cap = sci_name_i18n.toUpper();
			sciNamesMapI18n[hip] = sci_name_i18n;
			sciNamesIndexI18n[sci_name_i18n_cap] = hip;
			readOk++;
		}
	}
	snFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "scientific star names";
}


int StarMgr::getMaxSearchLevel() const
{
	int rval = -1;
	for (ZoneArrayMap::const_iterator it(zoneArrays.begin());it!=zoneArrays.end();++it)
	{
		const float mag_min = 0.001f*it->second->mag_min;
		float rcmag[2];
		if (StelApp::getInstance().getCore()->getSkyDrawer()->computeRCMag(mag_min,rcmag)==false)
			break;
		rval = it->first;
	}
	return rval;
}


// Draw all the stars
void StarMgr::draw(StelCore* core)
{
	StelNavigator* nav = core->getNavigator();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelSkyDrawer* skyDrawer = core->getSkyDrawer();
	
    currentJDay = nav->getJDay();

    // If stars are turned off don't waste time below
    // projecting all stars just to draw disembodied labels
    if (!starsFader.getInterstate())
		return;

	int maxSearchLevel = getMaxSearchLevel();
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(maxSearchLevel)->search(prj->unprojectViewport(),maxSearchLevel);

    // Set temporary static variable for optimization
    const float names_brightness = labelsFader.getInterstate() * starsFader.getInterstate();

	// Prepare openGL for drawing many stars
	StelPainter* sPainter = new StelPainter(prj);
	skyDrawer->preDrawPointSource(sPainter);
	Q_ASSERT(sPainter);

    // draw all the stars of all the selected zones
    float rcmag_table[2*256];
	
    for (ZoneArrayMap::const_iterator it(zoneArrays.begin()); it!=zoneArrays.end();++it)
	{
		const float mag_min = 0.001f*it->second->mag_min;
		const float k = (0.001f*it->second->mag_range)/it->second->mag_steps;
		for (int i=it->second->mag_steps-1;i>=0;--i)
		{
			const float mag = mag_min+k*i;
			if (skyDrawer->computeRCMag(mag,rcmag_table + 2*i)==false)
			{
				if (i==0) goto exit_loop;
			}
			if (skyDrawer->getFlagPointStar())
			{
				rcmag_table[2*i+1] *= starsFader.getInterstate();
			}
			else
			{
				rcmag_table[2*i] *= starsFader.getInterstate();
			}
		}
		lastMaxSearchLevel = it->first;
	
		unsigned int maxMagStarName = 0;
		if (labelsFader.getInterstate()>0.f)
		{
			// Adapt magnitude limit of the stars labels according to FOV and labelsAmount
			float maxMag = (skyDrawer->getLimitMagnitude()-6.5)*0.7+(labelsAmount*1.2f)-2.f;
			int x = (int)((maxMag-mag_min)/k);
			if (x > 0)
				maxMagStarName = x;
		}
		int zone;
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it->first);(zone = it1.next()) >= 0;)
			it->second->draw(zone, true, rcmag_table, prj, maxMagStarName, names_brightness, starFont);
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it->first);(zone = it1.next()) >= 0;)
			it->second->draw(zone, false, rcmag_table, prj, maxMagStarName,names_brightness, starFont);
    }
    exit_loop:
	// Finish drawing many stars
	skyDrawer->postDrawPointSource();
	
	delete sPainter;
	sPainter = NULL;
	
	drawPointer(prj, nav);
}


// Return a stl vector containing the stars located
// inside the limFov circle around position v
QList<StelObjectP > StarMgr::searchAround(const Vec3d& vv, double limFov, const StelCore* core) const
{
	QList<StelObjectP > result;
	if (!getFlagStars())
		return result;
		
	Vec3d v(vv);
	v.normalize();
	
	// find any vectors h0 and h1 (length 1), so that h0*v=h1*v=h0*h1=0
	int i;
	{
		const double a0 = fabs(v[0]);
		const double a1 = fabs(v[1]);
		const double a2 = fabs(v[2]);
		if (a0 <= a1)
		{
			if (a0 <= a2) i = 0;
			else i = 2;
		} else
		{
			if (a1 <= a2) i = 1;
			else i = 2;
		}
	}
	Vec3d h0(0.0,0.0,0.0);
	h0[i] = 1.0;
	Vec3d h1 = h0 ^ v;
	h1.normalize();
	h0 = h1 ^ v;
	h0.normalize();
	
	// Now we have h0*v=h1*v=h0*h1=0.
	// Construct a region with 4 corners e0,e1,e2,e3 inside which all desired stars must be:
	double f = 1.4142136 * tan(limFov * M_PI/180.0);
	h0 *= f;
	h1 *= f;
	Vec3d e0 = v + h0;
	Vec3d e1 = v + h1;
	Vec3d e2 = v - h0;
	Vec3d e3 = v - h1;
	f = 1.0/e0.length();
	e0 *= f;
	e1 *= f;
	e2 *= f;
	e3 *= f;
	// Search the triangles
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(lastMaxSearchLevel)->search(e3,e2,e1,e0,lastMaxSearchLevel);
	
	// Iterate over the stars inside the triangles
	f = cos(limFov * M_PI/180.);
	for (ZoneArrayMap::const_iterator it(zoneArrays.begin());it!=zoneArrays.end();it++)
	{
		//qDebug() << "search inside(" << it->first << "):";
		int zone;
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it->first);(zone = it1.next()) >= 0;)
		{
			it->second->searchAround(zone,v,f,result);
			//qDebug() << " " << zone;
		}
		//qDebug() << endl << "search border(" << it->first << "):";
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it->first); (zone = it1.next()) >= 0;)
		{
			it->second->searchAround(zone,v,f,result);
			//qDebug() << " " << zone;
		}
	}
	return result;
}


//! Update i18 names from english names according to passed translator.
//! The translation is done using gettext with translated strings defined in translations.h
void StarMgr::updateI18n()
{
	StelTranslator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	commonNamesMapI18n.clear();
	commonNamesIndexI18n.clear();
	for (std::map<int,QString>::iterator it(commonNamesMap.begin());it!=commonNamesMap.end();it++)
	{
		const int i = it->first;
		const QString t(trans.qtranslate(it->second));
		commonNamesMapI18n[i] = t;
		commonNamesIndexI18n[t.toUpper()] = i;
	}
	starFont = &StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}

// Search the star by HP number
StelObjectP StarMgr::searchHP(int hp) const
{
	if (0 < hp && hp <= NR_OF_HIP)
	{
		const Star1 *const s = hipIndex[hp].s;
		if (s)
		{
			const SpecialZoneArray<Star1> *const a = hipIndex[hp].a;
			const SpecialZoneData<Star1> *const z = hipIndex[hp].z;
			return s->createStelObject(a,z);
		}
	}
	return StelObjectP();
}

StelObjectP StarMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	// Search by HP number if it's an HP formated number
	QRegExp rx("^\\s*(HIP|HP)\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx.exactMatch(objw))
	{
		return searchHP(rx.capturedTexts().at(2).toInt());
	}

	// Search by I18n common name
	std::map<QString,int>::const_iterator it(commonNamesIndexI18n.find(objw));
	if (it!=commonNamesIndexI18n.end()) 
	{
		return searchHP(it->second);
	}

	// Search by sci name
	it = sciNamesIndexI18n.find(objw);
	if (it!=sciNamesIndexI18n.end()) 
	{
		return searchHP(it->second);
	}

	return StelObjectP();
}


StelObjectP StarMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();

	// Search by HP number if it's an HP formated number
	QRegExp rx("^\\s*(HP|HIP)\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx.exactMatch(objw))
	{
		return searchHP(rx.capturedTexts().at(2).toInt());
	}

	// Search by sci name
	std::map<QString,int>::const_iterator it = sciNamesIndexI18n.find(objw);
	if (it!=sciNamesIndexI18n.end())
	{
		return searchHP(it->second);
	}

	return StelObjectP();
}

//! Find and return the list of at most maxNbItem objects auto-completing
//! the passed object I18n name.
QStringList StarMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const 
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	// Search for common names
	for (std::map<QString,int>::const_iterator it(commonNamesIndexI18n.lower_bound(objw)); it!=commonNamesIndexI18n.end(); ++it) 
	{
		const QString constw(it->first.mid(0,objw.size()));
		if (constw==objw)
		{
			if (maxNbItem==0)
				break;
			result << getCommonName(it->second);
			--maxNbItem;
		} 
		else 
			break;
	}

	// Search for sci names
	for (std::map<QString,int>::const_iterator it(sciNamesIndexI18n.lower_bound(objw)); it!=sciNamesIndexI18n.end(); ++it) 
	{
		const QString constw(it->first.mid(0,objw.size()));
		if (constw==objw)
		{
			if (maxNbItem==0)
				break;
			result << getSciName(it->second);
			--maxNbItem;
		} 
		else 
			break;
	}

	// Add exact Hp catalogue numbers
	QRegExp hpRx("^(HIP|HP)\\s*(\\d+)\\s*$");
	hpRx.setCaseSensitivity(Qt::CaseInsensitive);
	if (hpRx.exactMatch(objw))
	{
		bool ok;
		int hpNum = hpRx.capturedTexts().at(2).toInt(&ok);
		if (ok)
		{
			StelObjectP s = searchHP(hpNum);
			if (s && maxNbItem>0)
			{
				result << QString("HIP%1").arg(hpNum);
				maxNbItem--;
			}
		}
	}

	result.sort();
	return result;
}


//! Define font file name and size to use for star names display
void StarMgr::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	starFont = &StelApp::getInstance().getFontManager().getStandardFont(
		StelApp::getInstance().getLocaleMgr().getSkyLanguage(),fontSize);
}

void StarMgr::updateSkyCulture(const QString& skyCultureDir)
{
	// Load culture star names in english
	try
	{
		loadCommonNames(StelApp::getInstance().getFileMgr().findFile("skycultures/" + skyCultureDir + "/star_names.fab"));
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "WARNING: could not load star_names.fab for sky culture " 
		           << skyCultureDir << ": " << e.what();	
	}
	
	try
	{
		loadSciNames(StelApp::getInstance().getFileMgr().findFile("stars/default/name.fab"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not load scientific star names file: " << e.what();
	}

	// Turn on sci names/catalog names for western culture only
	setFlagSciNames(skyCultureDir.startsWith("western"));
	updateI18n();
}
