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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSettings>
#include <QString>
#include <QTextStream>

#include "StelProjector.hpp"
#include "StarMgr.hpp"
#include "StelObject.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"

#include "StelUtils.hpp"
#include "StelToneReproducer.hpp"
#include "StelTranslator.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelIniParser.hpp"
#include "StelJsonParser.hpp"
#include "ZoneArray.hpp"
#include "StelSkyDrawer.hpp"
#include "RefractionExtinction.hpp"

#include <errno.h>
#include <unistd.h>

using namespace BigStarCatalogExtension;

static QStringList spectral_array;
static QStringList component_array;

// This number must be incremented each time the content or file format of the stars catalogs change
// It can also be incremented when the defaultStarsConfig.json file change.
// It should always matchs the version field of the defaultStarsConfig.json file
static const int StarCatalogFormatVersion = 2;

// Initialise statics
bool StarMgr::flagSciNames = true;
QHash<int,QString> StarMgr::commonNamesMap;
QHash<int,QString> StarMgr::commonNamesMapI18n;
QMap<QString,int> StarMgr::commonNamesIndexI18n;
QHash<int,QString> StarMgr::sciNamesMapI18n;
QMap<QString,int> StarMgr::sciNamesIndexI18n;

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


void StarMgr::initTriangle(int lev,int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2)
{
	ZoneArrayMap::const_iterator it(zoneArrays.find(lev));
	if (it!=zoneArrays.constEnd())
		it.value()->initTriangle(index,c0,c1,c2);
}


StarMgr::StarMgr(void) 
	: hipIndex(new HipIndexStruct[NR_OF_HIP+1])
	, texPointer(NULL)
{
	setObjectName("StarMgr");
	if (hipIndex == 0)
	{
		qFatal("ERROR: StarMgr::StarMgr: no memory");
	}
	maxGeodesicGridLevel = -1;
	lastMaxSearchLevel = -1;
	starFont.setPixelSize(StelApp::getInstance().getSettings()->value("gui/base_font_size", 13).toInt());
	objectMgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objectMgr);
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
	ZoneArrayMap::iterator it(zoneArrays.end());
	while (it!=zoneArrays.begin())
	{
		--it;
		delete it.value();
		it.value() = NULL;
	}
	zoneArrays.clear();
	if (hipIndex)
		delete[] hipIndex;
	if(NULL != texPointer)
	{
		delete texPointer;
	}
}

QString StarMgr::getCommonName(int hip)
{
	QHash<int,QString>::const_iterator it(commonNamesMapI18n.find(hip));
	if (it!=commonNamesMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getSciName(int hip)
{
	QHash<int,QString>::const_iterator it(sciNamesMapI18n.find(hip));
	if (it!=sciNamesMapI18n.end())
		return it.value();
	return QString();
}

void StarMgr::copyDefaultConfigFile()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/stars/default");
		starConfigFileFullPath = StelFileMgr::getUserDir()+"/stars/default/starsConfig.json";
		qDebug() << "Creates file " << starConfigFileFullPath;
		QFile::copy(StelFileMgr::findFile("stars/default/defaultStarsConfig.json"), starConfigFileFullPath);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << e.what();
		qFatal("Could not create configuration file stars/default/starsConfig.json");
	}
}

void StarMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	try
	{
		starConfigFileFullPath = StelFileMgr::findFile("stars/default/starsConfig.json", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Could not find the starsConfig.json file: will copy the default one.";
		copyDefaultConfigFile();
	}

	QFile fic(starConfigFileFullPath);
	fic.open(QIODevice::ReadOnly);
	starSettings = StelJsonParser::parse(&fic).toMap();
	fic.close();

	// Increment the 1 each time any star catalog file change
	if (starSettings.value("version").toInt()!=StarCatalogFormatVersion)
	{
		qWarning() << "Found an old starsConfig.json file, upgrade..";
		fic.remove();
		copyDefaultConfigFile();
		QFile fic2(starConfigFileFullPath);
		fic2.open(QIODevice::ReadOnly);
		starSettings = StelJsonParser::parse(&fic2).toMap();
		fic2.close();
	}

	loadData(starSettings);
	starFont.setPixelSize(StelApp::getInstance().getSettings()->value("gui/base_font_size", 13).toInt());

	setFlagStars(conf->value("astro/flag_stars", true).toBool());
	setFlagLabels(conf->value("astro/flag_star_name",true).toBool());
	setLabelsAmount(conf->value("stars/labels_amount",3.f).toFloat());

	objectMgr->registerStelObjectMgr(this);

	StelApp::getInstance().getCore()->getGeodesicGrid(maxGeodesicGridLevel)->visitTriangles(maxGeodesicGridLevel,initTriangleFunc,this);
	for (ZoneArrayMap::const_iterator it(zoneArrays.begin()); it!=zoneArrays.end();it++)
	{
		it.value()->scaleAxis();
	}
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(skyCultureChanged(const QString&)), this, SLOT(updateSkyCulture(const QString&)));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
}


void StarMgr::drawPointer(StelRenderer* renderer, StelProjectorP projector, const StelCore* core)
{
	const QList<StelObjectP> newSelected = objectMgr->getSelectedObject("Star");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d win;
		// Compute 2D pos and return if outside screen
		if (!projector->project(pos, win))
		{
			return;
		}

		if(NULL == texPointer)
		{
			texPointer = renderer->createTexture("textures/pointeur2.png");   // Load pointer texture
		}

		Vec3f c(obj->getInfoColor());
		if (StelApp::getInstance().getVisionModeNight())
			c = StelUtils::getNightColor(c);

		renderer->setGlobalColor(c[0], c[1], c[2]);

		texPointer->bind();
		renderer->setBlendMode(BlendMode_Alpha);
		const float angle = StelApp::getInstance().getTotalRunTime() * 40.0f;
		renderer->drawTexturedRect(win[0] - 13.0f, win[1] - 13.0f, 26.0f, 26.0f, angle);
	}
}

void StarMgr::setStelStyle(const QString& section)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();

	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelColor(StelUtils::strToVec3f(conf->value(section+"/star_label_color", defaultColor).toString()));
}

bool StarMgr::checkAndLoadCatalog(QVariantMap catDesc)
{
	const bool checked = catDesc.value("checked").toBool();
	QString catalogFileName = catDesc.value("fileName").toString();

	// See if it is an absolute path, else prepend default path
	if (!(StelFileMgr::isAbsolute(catalogFileName)))
		catalogFileName = "stars/default/"+catalogFileName;

	QString catalogFilePath;
	try
	{
		catalogFilePath = StelFileMgr::findFile(catalogFileName);
	}
	catch (std::runtime_error e)
	{
		// The file is supposed to be checked, but we can't find it
		if (checked)
		{
			qWarning() << QString("Warning: could not find star catalog %1").arg(catalogFileName);
			setCheckFlag(catDesc.value("id").toString(), false);
		}
		return false;
	}
	// Possibly fixes crash on Vista
	if (!StelFileMgr::isReadable(catalogFilePath))
	{
		qWarning() << QString("Warning: User does not have permissions to read catalog %1").arg(catalogFilePath);
		return false;
	}

	if (!checked)
	{
		// The file is not checked but we found it, maybe from a previous download/version
		qWarning() << "Found file " << catalogFilePath << ", checking md5sum..";

		QFile fic(catalogFilePath);
		fic.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
		// Compute the MD5 sum
		QCryptographicHash md5Hash(QCryptographicHash::Md5);
		const qint64 cat_sz = fic.size();
		qint64 maxStarBufMd5 = qMin(cat_sz, 9223372036854775807LL);
		uchar *cat = maxStarBufMd5 ? fic.map(0, maxStarBufMd5) : NULL;
		if (!cat)
		{
			// The OS was not able to map the file, revert to slower not mmap based method
			static const qint64 maxStarBufMd5 = 1024*1024*8;
			char* mmd5buf = (char*)malloc(maxStarBufMd5);
			while (!fic.atEnd())
			{
				qint64 sz = fic.read(mmd5buf, maxStarBufMd5);
				md5Hash.addData(mmd5buf, sz);
			}
			free(mmd5buf);
		}
		else
		{
			md5Hash.addData((const char*)cat, cat_sz);
			fic.unmap(cat);
		}
		fic.close();
		if (md5Hash.result().toHex()!=catDesc.value("checksum").toByteArray())
		{
			qWarning() << "Error: File " << catalogFileName << " is corrupt, MD5 mismatch! Found " << md5Hash.result().toHex() << " expected " << catDesc.value("checksum").toByteArray();
			fic.remove();
			return false;
		}
		qWarning() << "MD5 sum correct!";
		setCheckFlag(catDesc.value("id").toString(), true);
	}

//workaround: mmap is failing with "unknown error" on Android
#ifdef ANDROID
        ZoneArray* const z = ZoneArray::create(catalogFilePath, false);
#else
	ZoneArray* const z = ZoneArray::create(catalogFilePath, true);
#endif
	if (z)
	{
		if (maxGeodesicGridLevel < z->level)
		{
			maxGeodesicGridLevel = z->level;
		}
		ZoneArray *&pos(zoneArrays[z->level]);
		if (pos)
		{
			qWarning() << catalogFileName << ", " << z->level << ": duplicate level";
			delete z;
		}
		else
		{
			pos = z;
		}
	}
	return true;
}

void StarMgr::setCheckFlag(const QString& catId, bool b)
{
	// Update the starConfigFileFullPath file to take into account that we now have a new catalog
	int idx=0;
	foreach (const QVariant& catV, catalogsDescription)
	{
		++idx;
		QVariantMap m = catV.toMap();
		if (m.value("id").toString()!=catId)
			continue;
		const bool checked = m.value("checked").toBool();
		if (checked==b)
			return;
		m["checked"]=b;
		catalogsDescription[idx-1]=m;
		starSettings["catalogs"]=catalogsDescription;
		QFile tmp(starConfigFileFullPath);
		tmp.open(QIODevice::WriteOnly);
		StelJsonParser::write(starSettings, &tmp);
		tmp.close();
	}
}

void StarMgr::loadData(QVariantMap starsConfig)
{
	// Please do not init twice:
	Q_ASSERT(maxGeodesicGridLevel < 0);

	qDebug() << "Loading star data ...";

	catalogsDescription = starsConfig.value("catalogs").toList();
	foreach (const QVariant& catV, catalogsDescription)
	{
		QVariantMap m = catV.toMap();
		checkAndLoadCatalog(m);
	}

	for (int i=0; i<=NR_OF_HIP; i++)
	{
		hipIndex[i].a = 0;
		hipIndex[i].z = 0;
		hipIndex[i].s = 0;
	}
	for (ZoneArrayMap::const_iterator it(zoneArrays.constBegin()); it != zoneArrays.constEnd();++it)
	{
		it.value()->updateHipIndex(hipIndex);
	}

	const QString cat_hip_sp_file_name = starsConfig.value("hipSpectralFile").toString();
	if (cat_hip_sp_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_sp_file_name not found";
	}
	else
	{
		try
		{
			spectral_array = initStringListFromFile(StelFileMgr::findFile("stars/default/" + cat_hip_sp_file_name));
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "ERROR while loading data from "
					   << ("stars/default/" + cat_hip_sp_file_name)
					   << ": " << e.what();
		}
	}

	const QString cat_hip_cids_file_name = starsConfig.value("hipComponentsIdsFile").toString();
	if (cat_hip_cids_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_cids_file_name not found";
	}
	else
	{
		try
		{
			component_array = initStringListFromFile(StelFileMgr::findFile("stars/default/" + cat_hip_cids_file_name));
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "ERROR while loading data from "
					   << ("stars/default/" + cat_hip_cids_file_name) << ": " << e.what();
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

			// Fix for translate star names 
			// englishCommonName.replace('_', ' ');
			const QString commonNameI18n = q_(englishCommonName);
			QString commonNameI18n_cap = commonNameI18n.toUpper();

			commonNamesMap[hip] = englishCommonName;
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
	const QStringList& allRecords = QString::fromUtf8(snFile.readAll()).split('\n');
	snFile.close();

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	// record structure is delimited with a | character. Example record strings:
	// " 10819|c_And"
	// "113726|1_And"
	foreach(const QString& record, allRecords)
	{
		++lineNumber;
		if (record.isEmpty())
			continue;

		++totalRecords;
		const QStringList& fields = record.split('|');
		if (fields.size()!=2)
		{
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << sciNameFile
				   << " - record does not match record pattern";
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok;
			unsigned int hip = fields.at(0).toUInt(&ok);
			if (!ok)
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << sciNameFile
					   << " - failed to convert " << fields.at(0) << "to a number";
				continue;
			}

			// Don't set the sci name if it's already set
			if (sciNamesMapI18n.find(hip)!=sciNamesMapI18n.end())
				continue;

			QString sci_name_i18n = fields.at(1).trimmed();
			if (sci_name_i18n.isEmpty())
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << sciNameFile
					   << " - empty name field";
				continue;
			}

			sci_name_i18n.replace('_',' ');
			sciNamesMapI18n[hip] = sci_name_i18n;
			sciNamesIndexI18n[sci_name_i18n.toUpper()] = hip;
			++readOk;
		}
	}

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "scientific star names";
}


int StarMgr::getMaxSearchLevel() const
{
	int rval = -1;
	for (ZoneArrayMap::const_iterator it(zoneArrays.constBegin());it!=zoneArrays.constEnd();++it)
	{
		const float mag_min = 0.001f*it.value()->mag_min;
		float rcmag[2];
		if (StelApp::getInstance().getCore()->getSkyDrawer()->computeRCMag(mag_min,rcmag)==false)
			break;
		rval = it.key();
	}
	return rval;
}


// Draw all the stars
void StarMgr::draw(StelCore* core, StelRenderer* renderer)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelSkyDrawer* skyDrawer = core->getSkyDrawer();
	// If stars are turned off don't waste time below
	// projecting all stars just to draw disembodied labels
	if (!starsFader.getInterstate())
		return;

	int maxSearchLevel = getMaxSearchLevel();
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(maxSearchLevel)->search(prj->getViewportConvexPolygon()->getBoundingSphericalCaps(),maxSearchLevel);

	// Set temporary static variable for optimization
	const float names_brightness = labelsFader.getInterstate() * starsFader.getInterstate();

	// Prepare for drawing many stars
	renderer->setFont(starFont);
	skyDrawer->preDrawPointSource();

	// draw all the stars of all the selected zones
	// GZ: This table must be enlarged from 2x256 to many more entries. CORRELATE IN Zonearray.cpp!
	//float rcmag_table[2*256];
	//float rcmag_table[2*16384];
	float rcmag_table[2 * RCMAG_TABLE_SIZE];

	for (ZoneArrayMap::const_iterator it(zoneArrays.constBegin()); it!=zoneArrays.constEnd();++it)
	{
		const float mag_min = 0.001f*it.value()->mag_min;
		const float k = (0.001f*it.value()->mag_range)/it.value()->mag_steps; // MagStepIncrement
		// GZ: add a huge number of entries to rcMag
		//for (int i=it.value()->mag_steps-1;i>=0;--i)
		for (int i=RCMAG_TABLE_SIZE-1;i>=0;--i)
		{
			const float mag = mag_min+k*i;
			if (skyDrawer->computeRCMag(mag,rcmag_table + 2*i)==false)
			{
				if (i==0) goto exit_loop;
			}
			if (skyDrawer->getDrawStarsAsPoints())
			{
				rcmag_table[2*i+1] *= starsFader.getInterstate();
			}
			else
			{
				rcmag_table[2*i] *= starsFader.getInterstate();
			}
		}
		lastMaxSearchLevel = it.key();

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
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it.key());(zone = it1.next()) >= 0;)
			it.value()->draw(prj, renderer, zone, true, rcmag_table, core, maxMagStarName, names_brightness);
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it.key());(zone = it1.next()) >= 0;)
			it.value()->draw(prj, renderer, zone, false, rcmag_table, core, maxMagStarName,names_brightness);
	}
	exit_loop:
	// Finish drawing many stars
	skyDrawer->postDrawPointSource(prj);

	if (objectMgr->getFlagSelectedObjectPointer())
		drawPointer(renderer, prj, core);
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
	SphericalConvexPolygon c(e3, e2, e2, e0);
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(lastMaxSearchLevel)->search(c.getBoundingSphericalCaps(),lastMaxSearchLevel);

	// Iterate over the stars inside the triangles
	f = cos(limFov * M_PI/180.);
	for (ZoneArrayMap::const_iterator it(zoneArrays.constBegin());it!=zoneArrays.constEnd();it++)
	{
		//qDebug() << "search inside(" << it->first << "):";
		int zone;
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it.key());(zone = it1.next()) >= 0;)
		{
			it.value()->searchAround(core, zone,v,f,result);
			//qDebug() << " " << zone;
		}
		//qDebug() << endl << "search border(" << it->first << "):";
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it.key()); (zone = it1.next()) >= 0;)
		{
			it.value()->searchAround(core, zone,v,f,result);
			//qDebug() << " " << zone;
		}
	}
	return result;
}


//! Update i18 names from english names according to passed translator.
//! The translation is done using gettext with translated strings defined in translations.h
void StarMgr::updateI18n()
{
	QRegExp transRx("_[(]\"(.*)\"[)]");
	StelTranslator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	commonNamesMapI18n.clear();
	commonNamesIndexI18n.clear();
	for (QHash<int,QString>::ConstIterator it(commonNamesMap.constBegin());it!=commonNamesMap.constEnd();it++)
	{
		const int i = it.key();
		transRx.exactMatch(it.value());
		QString tt = transRx.capturedTexts().at(1);
		const QString t = trans.qtranslate(tt);
		//const QString t(trans.qtranslate(it.value()));
		commonNamesMapI18n[i] = t;
		commonNamesIndexI18n[t.toUpper()] = i;
	}
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
	QMap<QString,int>::const_iterator it(commonNamesIndexI18n.find(objw));
	if (it!=commonNamesIndexI18n.end())
	{
		return searchHP(it.value());
	}

	// Search by sci name
	QMap<QString,int>::const_iterator it2 = sciNamesIndexI18n.find(objw);
	if (it2!=sciNamesIndexI18n.end())
	{
		return searchHP(it2.value());
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
	QMap<QString,int>::const_iterator it = sciNamesIndexI18n.find(objw);
	if (it!=sciNamesIndexI18n.end())
	{
		return searchHP(it.value());
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
	for (QMap<QString,int>::const_iterator it(commonNamesIndexI18n.lowerBound(objw)); it!=commonNamesIndexI18n.end(); ++it)
	{
		if (it.key().startsWith(objw))
		{
			if (maxNbItem==0)
				break;
			result << getCommonName(it.value());
			--maxNbItem;
		}
		else
			break;
	}

	// Search for sci names
	QString bayerPattern = objw;
	QRegExp bayerRegEx(bayerPattern);

	// if the first character is a Greek letter, check if there's an index
	// after it, such as "alpha1 Cen".
	if (objw.at(0).unicode() >= 0x0391 && objw.at(0).unicode() <= 0x03A9)
		bayerRegEx.setPattern(bayerPattern.insert(1,"\\d?"));

	for (QMap<QString,int>::const_iterator it(sciNamesIndexI18n.lowerBound(objw)); it!=sciNamesIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegEx)==0)
		{
			if (maxNbItem==0)
				break;
			result << getSciName(it.value());
			--maxNbItem;
		}
		else if (it.key().at(0) != objw.at(0))
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
	starFont.setPixelSize(newFontSize);
}

void StarMgr::updateSkyCulture(const QString& skyCultureDir)
{
	// Load culture star names in english
	try
	{
		loadCommonNames(StelFileMgr::findFile("skycultures/" + skyCultureDir + "/star_names.fab"));
	}
	catch(std::runtime_error& e)
	{
		qDebug() << "Could not load star_names.fab for sky culture " << skyCultureDir << ": " << e.what();
	}

	try
	{
		loadSciNames(StelFileMgr::findFile("stars/default/name.fab"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: could not load scientific star names file: " << e.what();
	}

	// Turn on sci names/catalog names for western culture only
	setFlagSciNames(skyCultureDir.startsWith("western"));
	updateI18n();
}
