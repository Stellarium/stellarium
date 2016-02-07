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


#include "StelProjector.hpp"
#include "StarMgr.hpp"
#include "StelObject.hpp"
#include "StelTexture.hpp"

#include "StelUtils.hpp"
#include "StelToneReproducer.hpp"
#include "StelTranslator.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelIniParser.hpp"
#include "StelPainter.hpp"
#include "StelJsonParser.hpp"
#include "ZoneArray.hpp"
#include "StelSkyDrawer.hpp"
#include "RefractionExtinction.hpp"
#include "StelModuleMgr.hpp"
#include "ConstellationMgr.hpp"

#include <QTextStream>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QRegExp>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>

#include <errno.h>

static QStringList spectral_array;
static QStringList component_array;

// This number must be incremented each time the content or file format of the stars catalogs change
// It can also be incremented when the defaultStarsConfig.json file change.
// It should always matchs the version field of the defaultStarsConfig.json file
static const int StarCatalogFormatVersion = 7;

// Initialise statics
bool StarMgr::flagSciNames = true;
QHash<int,QString> StarMgr::commonNamesMap;
QHash<int,QString> StarMgr::commonNamesMapI18n;
QMap<QString,int> StarMgr::commonNamesIndexI18n;
QMap<QString,int> StarMgr::commonNamesIndex;
QHash<int,QString> StarMgr::sciNamesMapI18n;
QMap<QString,int> StarMgr::sciNamesIndexI18n;
QHash<int,QString> StarMgr::sciAdditionalNamesMapI18n;
QMap<QString,int> StarMgr::sciAdditionalNamesIndexI18n;
QHash<int, varstar> StarMgr::varStarsMapI18n;
QMap<QString, int> StarMgr::varStarsIndexI18n;
QHash<int, int> StarMgr::saoStarsMap;
QMap<int, int> StarMgr::saoStarsIndex;
QHash<int, int> StarMgr::hdStarsMap;
QMap<int, int> StarMgr::hdStarsIndex;

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
	gridLevels[lev]->initTriangle(index,c0,c1,c2);
}


StarMgr::StarMgr(void)
	: flagStarName(false)
	, labelsAmount(0.)
	, gravityLabel(false)
	, hipIndex(new HipIndexStruct[NR_OF_HIP+1])
{
	setObjectName("StarMgr");
	if (hipIndex == 0)
	{
		qFatal("ERROR: StarMgr::StarMgr: no memory");
	}
	maxGeodesicGridLevel = -1;
	lastMaxSearchLevel = -1;	
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
	foreach(ZoneArray* z, gridLevels)
		delete z;
	gridLevels.clear();
	if (hipIndex)
		delete[] hipIndex;
}

// Allow untranslated name here if set in constellationMgr!
QString StarMgr::getCommonName(int hip)
{
	ConstellationMgr* cmgr=GETSTELMODULE(ConstellationMgr);
	if (cmgr->getConstellationDisplayStyle() == ConstellationMgr::constellationsNative)
		return getCommonEnglishName(hip);

	QHash<int,QString>::const_iterator it(commonNamesMapI18n.find(hip));
	if (it!=commonNamesMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getCommonEnglishName(int hip)
{
	QHash<int,QString>::const_iterator it(commonNamesMap.find(hip));
	if (it!=commonNamesMap.end())
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

QString StarMgr::getSciAdditionalName(int hip)
{
	QHash<int,QString>::const_iterator it(sciAdditionalNamesMapI18n.find(hip));
	if (it!=sciAdditionalNamesMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getCrossIndexDesignations(int hip)
{
	QString designations;
	QHash<int,int>::const_iterator it(saoStarsMap.find(hip));
	if (it!=saoStarsMap.end())
		designations = QString("SAO %1").arg(it.value());

	QHash<int,int>::const_iterator it2(hdStarsMap.find(hip));
	if (it2!=hdStarsMap.end())
	{
		if (designations.isEmpty())
			designations = QString("HD %1").arg(it2.value());
		else
			designations += QString(" - HD %1").arg(it2.value());
	}

	return designations;
}

QString StarMgr::getGcvsName(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().designation;
	return QString();
}

QString StarMgr::getGcvsVariabilityType(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().vtype;
	return QString();
}

float StarMgr::getGcvsMaxMagnitude(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().maxmag;
	return -99.f;
}

int StarMgr::getGcvsMagnitudeFlag(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().mflag;
	return 0;
}


float StarMgr::getGcvsMinMagnitude(int hip, bool firstMinimumFlag)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
	{
		if (firstMinimumFlag)
		{
			return it.value().min1mag;
		}
		else
		{
			return it.value().min2mag;
		}
	}
	return -99.f;
}

QString StarMgr::getGcvsPhotometricSystem(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().photosys;
	return QString();
}

double StarMgr::getGcvsEpoch(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().epoch;
	return -99.f;
}

double StarMgr::getGcvsPeriod(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().period;
	return -99.f;
}

int StarMgr::getGcvsMM(int hip)
{
	QHash<int,varstar>::const_iterator it(varStarsMapI18n.find(hip));
	if (it!=varStarsMapI18n.end())
		return it.value().Mm;
	return -99;
}

void StarMgr::copyDefaultConfigFile()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/stars/default");
		starConfigFileFullPath = StelFileMgr::getUserDir()+"/stars/default/starsConfig.json";
		qDebug() << "Creates file " << QDir::toNativeSeparators(starConfigFileFullPath);
		QFile::copy(StelFileMgr::getInstallationDir()+"/stars/default/defaultStarsConfig.json", starConfigFileFullPath);
		QFile::setPermissions(starConfigFileFullPath, QFile::permissions(starConfigFileFullPath) | QFileDevice::WriteOwner);
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

	starConfigFileFullPath = StelFileMgr::findFile("stars/default/starsConfig.json", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	if (starConfigFileFullPath.isEmpty())
	{
		qWarning() << "Could not find the starsConfig.json file: will copy the default one.";
		copyDefaultConfigFile();
	}

	QFile fic(starConfigFileFullPath);
	if(fic.open(QIODevice::ReadOnly))
	{
		starSettings = StelJsonParser::parse(&fic).toMap();
		fic.close();
	}

	// Increment the 1 each time any star catalog file change
	if (starSettings.value("version").toInt()!=StarCatalogFormatVersion)
	{
		qWarning() << "Found an old starsConfig.json file, upgrade..";
		fic.remove();
		copyDefaultConfigFile();
		QFile fic2(starConfigFileFullPath);
		if(fic2.open(QIODevice::ReadOnly))
		{
			starSettings = StelJsonParser::parse(&fic2).toMap();
			fic2.close();
		}
	}

	loadData(starSettings);
	starFont.setPixelSize(StelApp::getInstance().getBaseFontSize());

	setFlagStars(conf->value("astro/flag_stars", true).toBool());
	setFlagLabels(conf->value("astro/flag_star_name",true).toBool());
	setLabelsAmount(conf->value("stars/labels_amount",3.f).toFloat());

	objectMgr->registerStelObjectMgr(this);
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");   // Load pointer texture

	StelApp::getInstance().getCore()->getGeodesicGrid(maxGeodesicGridLevel)->visitTriangles(maxGeodesicGridLevel,initTriangleFunc,this);
	foreach(ZoneArray* z, gridLevels)
		z->scaleAxis();
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(skyCultureChanged(const QString&)), this, SLOT(updateSkyCulture(const QString&)));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Stars", displayGroup, N_("Stars"), "flagStarsDisplayed", "S");
	addAction("actionShow_Stars_Labels", displayGroup, N_("Stars labels"), "flagLabelsDisplayed", "Alt+S");
	registerProperty("prop_StarMgr_labelsAmount","labelsAmount");
}


void StarMgr::drawPointer(StelPainter& sPainter, const StelCore* core)
{
	const QList<StelObjectP> newSelected = objectMgr->getSelectedObject("Star");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!sPainter.getProjector()->project(pos, screenpos))
			return;

		Vec3f c(obj->getInfoColor());
		sPainter.setColor(c[0], c[1], c[2]);
		texPointer->bind();
		sPainter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		sPainter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, StelApp::getInstance().getAnimationTime()*40.);
	}
}

void StarMgr::setStelStyle(const QString& section)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();

	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelColor(StelUtils::strToVec3f(conf->value(section+"/star_label_color", defaultColor).toString()));
}

bool StarMgr::checkAndLoadCatalog(const QVariantMap& catDesc)
{
	const bool checked = catDesc.value("checked").toBool();
	QString catalogFileName = catDesc.value("fileName").toString();

	// See if it is an absolute path, else prepend default path
	if (!(StelFileMgr::isAbsolute(catalogFileName)))
		catalogFileName = "stars/default/"+catalogFileName;

	QString catalogFilePath = StelFileMgr::findFile(catalogFileName);
	if (catalogFilePath.isEmpty())
	{
		// The file is supposed to be checked, but we can't find it
		if (checked)
		{
			qWarning() << QString("Warning: could not find star catalog %1").arg(QDir::toNativeSeparators(catalogFileName));
			setCheckFlag(catDesc.value("id").toString(), false);
		}
		return false;
	}
	// Possibly fixes crash on Vista
	if (!StelFileMgr::isReadable(catalogFilePath))
	{
		qWarning() << QString("Warning: User does not have permissions to read catalog %1").arg(QDir::toNativeSeparators(catalogFilePath));
		return false;
	}

	if (!checked)
	{
		// The file is not checked but we found it, maybe from a previous download/version
		qWarning() << "Found file " << QDir::toNativeSeparators(catalogFilePath) << ", checking md5sum..";

		QFile fic(catalogFilePath);
		if(fic.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
		{
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
				qWarning() << "Error: File " << QDir::toNativeSeparators(catalogFileName) << " is corrupt, MD5 mismatch! Found " << md5Hash.result().toHex() << " expected " << catDesc.value("checksum").toByteArray();
				fic.remove();
				return false;
			}
			qWarning() << "MD5 sum correct!";
			setCheckFlag(catDesc.value("id").toString(), true);
		}
	}

	ZoneArray* z = ZoneArray::create(catalogFilePath, true);
	if (z)
	{
		if (z->level<gridLevels.size())
		{
			qWarning() << QDir::toNativeSeparators(catalogFileName) << ", " << z->level << ": duplicate level";
			delete z;
			return true;
		}
		Q_ASSERT(z->level==maxGeodesicGridLevel+1);
		Q_ASSERT(z->level==gridLevels.size());
		++maxGeodesicGridLevel;
		gridLevels.append(z);
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
		if(tmp.open(QIODevice::WriteOnly))
		{
			StelJsonParser::write(starSettings, &tmp);
			tmp.close();
		}
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
	foreach(ZoneArray* z, gridLevels)
		z->updateHipIndex(hipIndex);

	const QString cat_hip_sp_file_name = starsConfig.value("hipSpectralFile").toString();
	if (cat_hip_sp_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_sp_file_name not found";
	}
	else
	{
		QString tmpFic = StelFileMgr::findFile("stars/default/" + cat_hip_sp_file_name);
		if (tmpFic.isEmpty())
			qWarning() << "ERROR while loading data from " << QDir::toNativeSeparators(("stars/default/" + cat_hip_sp_file_name));
		else
			spectral_array = initStringListFromFile(tmpFic);
	}

	const QString cat_hip_cids_file_name = starsConfig.value("hipComponentsIdsFile").toString();
	if (cat_hip_cids_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_cids_file_name not found";
	}
	else
	{
		QString tmpFic = StelFileMgr::findFile("stars/default/" + cat_hip_cids_file_name);
		if (tmpFic.isEmpty())
			qWarning() << "ERROR while loading data from " << QDir::toNativeSeparators(("stars/default/" + cat_hip_cids_file_name));
		else
			component_array = initStringListFromFile(tmpFic);
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
	commonNamesIndex.clear();

	qDebug() << "Loading star names from" << QDir::toNativeSeparators(commonNameFile);
	QFile cnFile(commonNameFile);
	if (!cnFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(commonNameFile);
		return 0;
	}

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	QString record;
	// Allow empty and comment lines where first char (after optional blanks) is #
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	// record structure is delimited with a | character.  We will
	// use a QRegExp to extract the fields. with whitespace padding permitted
	// (i.e. it will be stripped automatically) Example record strings:
	// "   677|_("Alpheratz")"
	// "113368|_("Fomalhaut")"
	// Note: Stellarium doesn't support sky cultures made prior version 0.10.6 now!
	QRegExp recordRx("^\\s*(\\d+)\\s*\\|_[(]\"(.*)\"[)]\\s*\\n");

	while(!cnFile.atEnd())
	{
		record = QString::fromUtf8(cnFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;
		if (!recordRx.exactMatch(record))
		{
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(commonNameFile)
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
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(commonNameFile)
					   << " - failed to convert " << recordRx.capturedTexts().at(1) << "to a number";
				continue;
			}
			QString englishCommonName = recordRx.capturedTexts().at(2).trimmed();
			if (englishCommonName.isEmpty())
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(commonNameFile)
					   << " - empty name field";
				continue;
			}

			const QString commonNameI18n = englishCommonName;
			QString commonNameI18n_cap = commonNameI18n.toUpper();

			commonNamesMap[hip] = englishCommonName;
			commonNamesMapI18n[hip] = commonNameI18n;
			commonNamesIndexI18n[commonNameI18n_cap] = hip;
			commonNamesIndex[englishCommonName.toUpper()] = hip;
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
	sciAdditionalNamesMapI18n.clear();
	sciAdditionalNamesIndexI18n.clear();

	qDebug() << "Loading star names from" << QDir::toNativeSeparators(sciNameFile);
	QFile snFile(sciNameFile);
	if (!snFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(sciNameFile);
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
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(sciNameFile)
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
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(sciNameFile)
					   << " - failed to convert " << fields.at(0) << "to a number";
				continue;
			}

			QString sci_name_i18n = fields.at(1).trimmed();
			if (sci_name_i18n.isEmpty())
			{
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(sciNameFile)
					   << " - empty name field";
				continue;
			}

			sci_name_i18n.replace('_',' ');
			// Don't set the main sci name if it's already set - it's additional sci name
			if (sciNamesMapI18n.find(hip)!=sciNamesMapI18n.end())
			{
				sciAdditionalNamesMapI18n[hip] = sci_name_i18n;
				sciAdditionalNamesIndexI18n[sci_name_i18n.toUpper()] = hip;
			}
			else
			{
				sciNamesMapI18n[hip] = sci_name_i18n;
				sciNamesIndexI18n[sci_name_i18n.toUpper()] = hip;
			}
			++readOk;
		}
	}

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "scientific star names";
}

// Load GCVS from file
void StarMgr::loadGcvs(const QString& GcvsFile)
{
	varStarsMapI18n.clear();
	varStarsIndexI18n.clear();

	qDebug() << "Loading variable stars from" << QDir::toNativeSeparators(GcvsFile);
	QFile vsFile(GcvsFile);
	if (!vsFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(GcvsFile);
		return;
	}
	const QStringList& allRecords = QString::fromUtf8(vsFile.readAll()).split('\n');
	vsFile.close();

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;

	// record structure is delimited with a tab character.
	foreach(const QString& record, allRecords)
	{
		++lineNumber;
		if (record.isEmpty())
			continue;

		++totalRecords;
		const QStringList& fields = record.split('\t');

		bool ok;
		unsigned int hip = fields.at(0).toUInt(&ok);
		if (!ok)
		{
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(GcvsFile)
				   << " - failed to convert " << fields.at(0) << "to a number";
			continue;
		}

		// Don't set the star if it's already set
		if (varStarsMapI18n.find(hip)!=varStarsMapI18n.end())
			continue;

		varstar variableStar;

		variableStar.designation = fields.at(1).trimmed();
		variableStar.vtype = fields.at(2).trimmed();
		if (fields.at(3).isEmpty())
			variableStar.maxmag = 99.f;
		else
			variableStar.maxmag = fields.at(3).toFloat();
		variableStar.mflag = fields.at(4).toInt();
		if (fields.at(5).isEmpty())
			variableStar.min1mag = 99.f;
		else
			variableStar.min1mag = fields.at(5).toFloat();
		if (fields.at(6).isEmpty())
			variableStar.min2mag = 99.f;
		else
			variableStar.min2mag = fields.at(6).toFloat();
		variableStar.photosys = fields.at(7).trimmed();
		variableStar.epoch = fields.at(8).toDouble();
		variableStar.period = fields.at(9).toDouble();
		variableStar.Mm = fields.at(10).toInt();
		variableStar.stype = fields.at(11).trimmed();

		varStarsMapI18n[hip] = variableStar;
		varStarsIndexI18n[variableStar.designation.toUpper()] = hip;
		++readOk;
	}

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "variable stars";
}

// Load cross-index data from file
void StarMgr::loadCrossIndex(const QString& crossIndexFile)
{
	saoStarsMap.clear();
	saoStarsIndex.clear();
	hdStarsMap.clear();
	hdStarsIndex.clear();

	qDebug() << "Loading cross-index data from" << QDir::toNativeSeparators(crossIndexFile);
	QFile ciFile(crossIndexFile);
	if (!ciFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(crossIndexFile);
		return;
	}
	const QStringList& allRecords = QString::fromUtf8(ciFile.readAll()).split('\n');
	ciFile.close();

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	// record structure is delimited with a 'tab' character. Example record strings:
	// "1	128522	224700"
	// "2	165988	224690"
	foreach(const QString& record, allRecords)
	{
		++lineNumber;
		if (record.isEmpty())
			continue;

		++totalRecords;
		const QStringList& fields = record.split('\t');
		if (fields.size()!=3)
		{
			qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(crossIndexFile)
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
				qWarning() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(crossIndexFile)
					   << " - failed to convert " << fields.at(0) << "to a number";
				continue;
			}

			unsigned int sao = fields.at(1).toUInt(&ok);
			unsigned int hd = fields.at(2).toUInt(&ok);

			if (sao>0)
			{
				saoStarsMap[hip] = sao;
				saoStarsIndex[sao] = hip;
			}

			if (hd>0)
			{
				hdStarsMap[hip] = hd;
				hdStarsIndex[hd] = hip;
			}

			++readOk;
		}
	}

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "cross-index data records";
}

int StarMgr::getMaxSearchLevel() const
{
	int rval = -1;
	foreach(const ZoneArray* z, gridLevels)
	{
		const float mag_min = 0.001f*z->mag_min;
		RCMag rcmag;
		if (StelApp::getInstance().getCore()->getSkyDrawer()->computeRCMag(mag_min, &rcmag)==false)
			break;
		rval = z->level;
	}
	return rval;
}


// Draw all the stars
void StarMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelSkyDrawer* skyDrawer = core->getSkyDrawer();
	// If stars are turned off don't waste time below
	// projecting all stars just to draw disembodied labels
	if (!starsFader.getInterstate())
		return;

	int maxSearchLevel = getMaxSearchLevel();
	QVector<SphericalCap> viewportCaps = prj->getViewportConvexPolygon()->getBoundingSphericalCaps();
	viewportCaps.append(core->getVisibleSkyArea());
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(maxSearchLevel)->search(viewportCaps,maxSearchLevel);

	// Set temporary static variable for optimization
	const float names_brightness = labelsFader.getInterstate() * starsFader.getInterstate();

	// Prepare openGL for drawing many stars
	StelPainter sPainter(prj);
	sPainter.setFont(starFont);
	skyDrawer->preDrawPointSource(&sPainter);

	// Prepare a table for storing precomputed RCMag for all ZoneArrays
	RCMag rcmag_table[RCMAG_TABLE_SIZE];
	
	// Draw all the stars of all the selected zones
	foreach(const ZoneArray* z, gridLevels)
	{
		int limitMagIndex=RCMAG_TABLE_SIZE;
		const float mag_min = 0.001f*z->mag_min;
		const float k = (0.001f*z->mag_range)/z->mag_steps; // MagStepIncrement
		for (int i=0;i<RCMAG_TABLE_SIZE;++i)
		{
			const float mag = mag_min+k*i;
			if (skyDrawer->computeRCMag(mag, &rcmag_table[i])==false)
			{
				if (i==0)
					goto exit_loop;
				
				// The last magnitude at which the star is visible
				limitMagIndex = i-1;
				
				// We reached the point where stars are not visible anymore
				// Fill the rest of the table with zero and leave.
				for (;i<RCMAG_TABLE_SIZE;++i)
				{
					rcmag_table[i].luminance=0;
					rcmag_table[i].radius=0;
				}
				break;
			}
			rcmag_table[i].radius *= starsFader.getInterstate();
		}
		lastMaxSearchLevel = z->level;

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
		
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,z->level);(zone = it1.next()) >= 0;)
			z->draw(&sPainter, zone, true, rcmag_table, limitMagIndex, core, maxMagStarName, names_brightness, viewportCaps);
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,z->level);(zone = it1.next()) >= 0;)
			z->draw(&sPainter, zone, false, rcmag_table, limitMagIndex, core, maxMagStarName,names_brightness, viewportCaps);
	}
	exit_loop:

	// Finish drawing many stars
	skyDrawer->postDrawPointSource(&sPainter);

	if (objectMgr->getFlagSelectedObjectPointer())
		drawPointer(sPainter, core);
}


// Return a QList containing the stars located
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
	foreach(ZoneArray* z, gridLevels)
	{
		//qDebug() << "search inside(" << it->first << "):";
		int zone;
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,z->level);(zone = it1.next()) >= 0;)
		{
			z->searchAround(core, zone,v,f,result);
			//qDebug() << " " << zone;
		}
		//qDebug() << endl << "search border(" << it->first << "):";
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,z->level); (zone = it1.next()) >= 0;)
		{
			z->searchAround(core, zone,v,f,result);
			//qDebug() << " " << zone;
		}
	}
	return result;
}


//! Update i18 names from english names according to passed translator.
//! The translation is done using gettext with translated strings defined in translations.h
void StarMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	commonNamesMapI18n.clear();
	commonNamesIndexI18n.clear();
	for (QHash<int,QString>::ConstIterator it(commonNamesMap.constBegin());it!=commonNamesMap.constEnd();it++)
	{
		const int i = it.key();
		const QString t(trans.qtranslate(it.value()));
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

	// Search by SAO number if it's an SAO formated number
	QRegExp rx2("^\\s*(SAO)\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx2.exactMatch(objw))
	{
		QMap<int,int>::const_iterator sao(saoStarsIndex.find(rx2.capturedTexts().at(2).toInt()));
		if (sao!=saoStarsIndex.end())
		{
			return searchHP(sao.value());
		}
	}

	// Search by HD number if it's an HD formated number
	QRegExp rx3("^\\s*(HD)\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx3.exactMatch(objw))
	{
		QMap<int,int>::const_iterator hd(hdStarsIndex.find(rx3.capturedTexts().at(2).toInt()));
		if (hd!=hdStarsIndex.end())
		{
			return searchHP(hd.value());
		}
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


	// Search by additional sci name
	QMap<QString,int>::const_iterator it3 = sciAdditionalNamesIndexI18n.find(objw);
	if (it3!=sciAdditionalNamesIndexI18n.end())
	{
		return searchHP(it3.value());
	}

	// Search by GCVS name
	QMap<QString,int>::const_iterator it4 = varStarsIndexI18n.find(objw);
	if (it4!=varStarsIndexI18n.end())
	{
		return searchHP(it4.value());
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

	// Search by SAO number if it's an SAO formated number
	QRegExp rx2("^\\s*(SAO)\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx2.exactMatch(objw))
	{
		QMap<int,int>::const_iterator sao(saoStarsIndex.find(rx2.capturedTexts().at(2).toInt()));
		if (sao!=saoStarsIndex.end())
		{
			return searchHP(sao.value());
		}
	}

	// Search by HD number if it's an HD formated number
	QRegExp rx3("^\\s*(HD)\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx3.exactMatch(objw))
	{
		QMap<int,int>::const_iterator hd(hdStarsIndex.find(rx3.capturedTexts().at(2).toInt()));
		if (hd!=hdStarsIndex.end())
		{
			return searchHP(hd.value());
		}
	}

	// Search by I18n common name
	QMap<QString,int>::const_iterator it(commonNamesIndex.find(objw));
	if (it!=commonNamesIndex.end())
	{
		return searchHP(it.value());
	}

	// Search by sci name
	QMap<QString,int>::const_iterator it2 = sciNamesIndexI18n.find(objw);
	if (it2!=sciNamesIndexI18n.end())
	{
		return searchHP(it2.value());
	}

	// Search by additional sci name
	QMap<QString,int>::const_iterator it3 = sciAdditionalNamesIndexI18n.find(objw);
	if (it3!=sciAdditionalNamesIndexI18n.end())
	{
		return searchHP(it3.value());
	}

	return StelObjectP();
}

//! Find and return the list of at most maxNbItem objects auto-completing
//! the passed object I18n name.
QStringList StarMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	// Search for common names
	if (useStartOfWords) {
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
	}
	else
	{
		QMapIterator<QString, int> i(commonNamesIndexI18n);
		while (i.hasNext())
		{
			i.next();
			if (i.key().contains(objw))
			{
				if (maxNbItem==0)
					break;
				result << getCommonName(i.value());
				--maxNbItem;
			}
		}
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

	for (QMap<QString,int>::const_iterator it(sciAdditionalNamesIndexI18n.lowerBound(objw)); it!=sciAdditionalNamesIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegEx)==0)
		{
			if (maxNbItem==0)
				break;
			result << getSciAdditionalName(it.value());
			--maxNbItem;
		}
		else if (it.key().at(0) != objw.at(0))
			break;
	}

	for (QMap<QString,int>::const_iterator it(varStarsIndexI18n.lowerBound(objw)); it!=varStarsIndexI18n.end(); ++it)
	{
		if (it.key().startsWith(objw))
		{
			if (maxNbItem==0)
				break;
			result << getGcvsName(it.value());
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

	// Add exact SAO catalogue numbers
	QRegExp saoRx("^(SAO)\\s*(\\d+)\\s*$");
	saoRx.setCaseSensitivity(Qt::CaseInsensitive);
	if (saoRx.exactMatch(objw))
	{
		bool ok;
		int saoNum = saoRx.capturedTexts().at(2).toInt(&ok);
		QMap<int,int>::const_iterator sao(saoStarsIndex.find(saoNum));
		if (sao!=saoStarsIndex.end())
		{
			StelObjectP s = searchHP(sao.value());
			if (s && maxNbItem>0)
			{
				result << QString("SAO%1").arg(saoNum);
				maxNbItem--;
			}
		}
	}

	// Add exact HD catalogue numbers
	QRegExp hdRx("^(HD)\\s*(\\d+)\\s*$");
	hdRx.setCaseSensitivity(Qt::CaseInsensitive);
	if (hdRx.exactMatch(objw))
	{
		bool ok;
		int hdNum = hdRx.capturedTexts().at(2).toInt(&ok);
		QMap<int,int>::const_iterator hd(hdStarsIndex.find(hdNum));
		if (hd!=hdStarsIndex.end())
		{
			StelObjectP s = searchHP(hd.value());
			if (s && maxNbItem>0)
			{
				result << QString("HD%1").arg(hdNum);
				maxNbItem--;
			}
		}
	}

	result.sort();
	return result;
}

//! Find and return the list of at most maxNbItem objects auto-completing
//! the passed object English name.
QStringList StarMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	// Search for common names
	if (useStartOfWords)
	{
		for (QMap<QString,int>::const_iterator it(commonNamesIndex.lowerBound(objw)); it!=commonNamesIndex.end(); ++it)
		{
			if (it.key().startsWith(objw))
			{
				if (maxNbItem==0)
					break;
				result << getCommonEnglishName(it.value());
				--maxNbItem;
			}
			else
				break;
		}
	}
	else
	{
		QMapIterator<QString, int> i(commonNamesIndex);
		while (i.hasNext())
		{
			i.next();
			if (i.key().contains(objw))
			{
				if (maxNbItem==0)
					break;
				result << getCommonEnglishName(i.value());
				--maxNbItem;
			}
		}
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

	for (QMap<QString,int>::const_iterator it(sciAdditionalNamesIndexI18n.lowerBound(objw)); it!=sciAdditionalNamesIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegEx)==0)
		{
			if (maxNbItem==0)
				break;
			result << getSciAdditionalName(it.value());
			--maxNbItem;
		}
		else if (it.key().at(0) != objw.at(0))
			break;
	}

	// Search for sci names for var stars
	for (QMap<QString,int>::const_iterator it(varStarsIndexI18n.lowerBound(objw)); it!=varStarsIndexI18n.end(); ++it)
	{
		if (it.key().startsWith(objw))
		{
			if (maxNbItem==0)
				break;
			result << getGcvsName(it.value());
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

	// Add exact SAO catalogue numbers
	QRegExp saoRx("^(SAO)\\s*(\\d+)\\s*$");
	saoRx.setCaseSensitivity(Qt::CaseInsensitive);
	if (saoRx.exactMatch(objw))
	{
		bool ok;
		int saoNum = saoRx.capturedTexts().at(2).toInt(&ok);
		QMap<int,int>::const_iterator sao(saoStarsIndex.find(saoNum));
		if (sao!=saoStarsIndex.end())
		{
			StelObjectP s = searchHP(sao.value());
			if (s && maxNbItem>0)
			{
				result << QString("SAO%1").arg(saoNum);
				maxNbItem--;
			}
		}
	}

	// Add exact HD catalogue numbers
	QRegExp hdRx("^(HD)\\s*(\\d+)\\s*$");
	hdRx.setCaseSensitivity(Qt::CaseInsensitive);
	if (hdRx.exactMatch(objw))
	{
		bool ok;
		int hdNum = hdRx.capturedTexts().at(2).toInt(&ok);
		QMap<int,int>::const_iterator hd(hdStarsIndex.find(hdNum));
		if (hd!=hdStarsIndex.end())
		{
			StelObjectP s = searchHP(hd.value());
			if (s && maxNbItem>0)
			{
				result << QString("HD%1").arg(hdNum);
				maxNbItem--;
			}
		}
	}

	result.sort();
	return result;
}


//! Define font file name and size to use for star names display
void StarMgr::setFontSize(float newFontSize)
{
	starFont.setPixelSize(newFontSize);
}

void StarMgr::updateSkyCulture(const QString& skyCultureDir)
{
	// Load culture star names in english
	QString fic = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/star_names.fab");
	if (fic.isEmpty())
		qDebug() << "Could not load star_names.fab for sky culture " << QDir::toNativeSeparators(skyCultureDir);
	else
		loadCommonNames(fic);

	fic = StelFileMgr::findFile("stars/default/name.fab");
	if (fic.isEmpty())
		qWarning() << "WARNING: could not load scientific star names file: stars/default/name.fab";
	else
		loadSciNames(fic);
	
	fic = StelFileMgr::findFile("stars/default/gcvs_hip_part.dat");
	if (fic.isEmpty())
		qWarning() << "WARNING: could not load variable stars file: stars/default/gcvs_hip_part.dat";
	else
		loadGcvs(fic);

	fic = StelFileMgr::findFile("stars/default/cross-index.dat");
	if (fic.isEmpty())
		qWarning() << "WARNING: could not load cross-index data file: stars/default/cross-index.dat";
	else
		loadCrossIndex(fic);

	// Turn on sci names/catalog names for western culture only
	setFlagSciNames(skyCultureDir.startsWith("western"));
	updateI18n();
}

QStringList StarMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		QMapIterator<QString, int> i(commonNamesIndex);
		while (i.hasNext())
		{
			i.next();
			result << getCommonEnglishName(i.value());
		}
	}
	else
	{
		QMapIterator<QString, int> i(commonNamesIndexI18n);
		while (i.hasNext())
		{
			i.next();
			result << getCommonName(i.value());
		}
	}
	return result;
}

QStringList StarMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;
	int type = objType.toInt();
	// Use SkyTranslator for translation star names
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	switch (type)
	{
		case 0: // Double stars
		{
			QStringList doubleStars;
			doubleStars  << "Asterope" << "Atlas" << "77 Tau" << "δ1 Tau" << "V1016 Ori"
				     << "42 Ori" << "ι Ori" << "ζ Crv" << "Mizar" << "Zubenelgenubi"
				     << "ω1 Sco" << "λ Sco" << "μ1 Sco" << "ζ1 Sco" << "ε1 Lyr" << "δ1 Lyr"
				     << "ν1 Sgr" << "ο1 Cyg" << "ο2 Cyg" << "Algedi" << "Albireo" << "Rigel"
				     << "Almaak" << "ξ Boo" << "Rasalgethi" << "T Dra" << "Kuma" << "70 Oph"
				     << "Castor" << "ζ Her" << "Keid" << "Mesarthim" << "Porrima" << "Algieba"
				     << "β Mon" << "Izar" << "44 Boo" << "Acrab" << "Tegmine" << "φ2 Cnc"
				     << "Regulus" << "Cor Caroli" << "ι Cas" << "ε Ari" << "Markeb" << "γ1 Del"
				     << "Bessel's Star" << "55 Aqr" << "σ Cas" << "Achird" << "Polaris" << "36 Oph"
				     << "65 UMa" << "σ2 UMa" << "55 Cnc" << "16 Cyg" << "HIP 28393" << "HIP 84709";
			if (inEnglish)
			{
				result = doubleStars;
			}
			else
			{
				foreach (QString star, doubleStars)
				{
					result << trans.qtranslate(star);
				}
			}
			break;
		}
		case 1: // Variable stars
		{
			QStringList variableStars;
			variableStars << "δ Cep" << "Algol" << "Mira" << "λ Tau" << "Sheliak" << "ζ Gem" << "μ Cep"
				      << "Rasalgethi" << "η Gem" << "η Aql" << "γ Cas" << "Betelgeuse" << "R And"
				      << "U Ant" << "θ Aps" << "R Aql" << "V Aql" << "R Aqr" << "ε Aur" << "R Aur"
				      << "AE Aur" << "W Boo" << "VZ Cam" << "l Car" << "γ Cas" << "WZ Cas"
				      << "S Cen" << "Proxima" << "T Cep" << "U Cep" << "R CMa" << "VY CMa"
				      << "S Cnc" << "Alphekka" << "R CrB" << "T CrB" << "U CrB" << "R Cru"
				      << "SU Cyg" << "EU Del" << "β Dor" << "R Gem" << "30 Her" << "68 Her"
				      << "R Hor" << "Hind's Crimson Star" << "R Leo" << "RR Lyr" << "U Mon"
				      << "Mintaka" << "VV Ori" << "κ Pav" << "β Peg" << "Enif" << "ζ Phe" << "R Sct"
				      << "U Sgr" << "RY Sgr" << "W UMa" << "Polaris";
			if (inEnglish)
			{
				result = variableStars;
			}
			else
			{
				foreach (QString star, variableStars)
				{
					result << trans.qtranslate(star);
				}
			}
			break;
		}
		default:
		{
			// No stars yet?
			break;
		}
	}

	result.removeDuplicates();
	return result;
}
