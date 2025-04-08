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
#include "StelPainter.hpp"
#include "StelJsonParser.hpp"
#include "ZoneArray.hpp"
#include "StelSkyDrawer.hpp"
#include "StelModuleMgr.hpp"
#include "ConstellationMgr.hpp"
#include "Planet.hpp"
#include "StelUtils.hpp"
#include "StelHealpix.hpp"
#include "StelObject.hpp"

#include <QGlobalStatic>
#include <QTextStream>
#include <QFile>
#include <QBuffer>
#include <QSettings>
#include <QString>
#include <QRegularExpression>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>

#include <cstdlib>

Q_GLOBAL_STATIC(QStringList, spectral_array);
Q_GLOBAL_STATIC(QStringList, component_array);
Q_GLOBAL_STATIC(QStringList, objtype_array);

// This number must be incremented each time the content or file format of the stars catalogs change
// It can also be incremented when the defaultStarsConfig.json file change.
// It should always match the version field of the defaultStarsConfig.json file
static const int StarCatalogFormatVersion = 23;

// Initialise statics
bool StarMgr::flagSciNames = true;
bool StarMgr::flagAdditionalStarNames = true;
bool StarMgr::flagDesignations = false;
bool StarMgr::flagDblStarsDesignation = false;
bool StarMgr::flagVarStarsDesignation = false;
bool StarMgr::flagHIPDesignation = false;

QHash<StarId,QString> StarMgr::commonNamesMap;
QHash<StarId,QString> StarMgr::commonNamesMapI18n;
QHash<QString,StarId> StarMgr::commonNamesUppercaseIndexI18n;
QHash<QString,StarId> StarMgr::commonNamesUppercaseIndex;
//QHash<StarId,QString> StarMgr::additionalNamesMap;
//QHash<StarId,QString> StarMgr::additionalNamesMapI18n;
//QMap<QString,StarId> StarMgr::additionalNamesIndex;     // ATTN: some names are not unique! map target type may need to become QList<StarId>
//QMap<QString,StarId> StarMgr::additionalNamesIndexI18n; // ATTN: Stores uppercase variant of additionalNameI18n, BUT WHY? Also, some names are not unique! map target type may need to be QList<StarId>

// Cultural names: We must store all data here, and have even 4 indices to native names&spelling, pronunciation, english and user-language spelling
QMultiHash<StarId, StelObject::CulturalName> StarMgr::culturalNamesMap; // cultural names
QMultiMap<QString, StarId> StarMgr::culturalNamesIndex; // reverse mappings. For names, unfortunately multiple results are possible!

QHash<StarId,QString> StarMgr::sciDesignationsMap;      // Bayer/Flamsteed. TODO: Convert map target to QStringList?
QHash<QString,StarId> StarMgr::sciDesignationsIndex;
QHash<StarId,QString> StarMgr::sciExtraDesignationsMap; // Other sci designations. TODO: Convert map target to QStringList?
QHash<QString,StarId> StarMgr::sciExtraDesignationsIndex;
QHash<StarId, varstar> StarMgr::varStarsMap;
QHash<QString, StarId> StarMgr::varStarsIndex;
QHash<StarId, wds> StarMgr::wdsStarsMap;
QHash<QString, StarId> StarMgr::wdsStarsIndex;
QMap<QString, crossid> StarMgr::crossIdMap;
QHash<int, StarId> StarMgr::saoStarsIndex;
QHash<int, StarId> StarMgr::hdStarsIndex;
QHash<int, StarId> StarMgr::hrStarsIndex;
QHash<StarId, QString> StarMgr::referenceMap;
QHash<StarId, binaryorbitstar> StarMgr::binaryOrbitStarMap;

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
	if (index < 0 || index >= spectral_array->size())
	{
		qDebug() << "convertToSpectralType: bad index: " << index << ", max: " << spectral_array->size();
		return "";
	}
	return spectral_array->at(index);
}

QString StarMgr::convertToComponentIds(int index)
{
	if (index < 0 || index >= component_array->size())
	{
		qDebug() << "convertToComponentIds: bad index: " << index << ", max: " << component_array->size();
		return "";
	}
	return component_array->at(index);
}

QString StarMgr::convertToOjectTypes(int index)
{
	if (index < 0 || index >= objtype_array->size())
	{
		qDebug() << "convertToObjTypeIds: bad index: " << index << ", max: " << objtype_array->size();
		return "";
	}
	return objtype_array->at(index).trimmed();
}


void StarMgr::initTriangle(int lev,int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2)
{
	gridLevels[lev]->initTriangle(index,c0,c1,c2);
}


StarMgr::StarMgr(void)
	: StelObjectModule()
	, flagStarName(false)
	, labelsAmount(0.)
	, gravityLabel(false)
	, maxGeodesicGridLevel(-1)
	, lastMaxSearchLevel(-1)
	, hipIndex(new HipIndexStruct[NR_OF_HIP+1])
{
	setObjectName("StarMgr");
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
	for (auto* z : std::as_const(gridLevels))
		delete z;
	gridLevels.clear();
	if (hipIndex)
		delete[] hipIndex;
}

// Allow untranslated name here if set in constellationMgr!
QString StarMgr::getCommonNameI18n(StarId hip)
{
	//static StelSkyCultureMgr* cmgr=GETSTELMODULE(StelSkyCultureMgr);
	//// TODO: This function will likely go away, or prepare for use in InfoString context
	//if (cmgr->getScreenLabelStyle() == StelObject::CulturalDisplayStyle::Native)
	//	return getCommonEnglishName(hip);

	return commonNamesMapI18n.value(hip, QString());
}

//QString StarMgr::getAdditionalNames(StarId hip)
//{
//	return additionalNamesMapI18n.value(hip, QString());
//}
//
//QString StarMgr::getAdditionalEnglishNames(StarId hip)
//{
//	return additionalNamesMap.value(hip, QString());
//}

// Get the cultural names for a star with a specified
// Hipparcos or Gaia catalogue number.
// @param hip The Hipparcos/Gaia number of star
// @return cultural names of star
QList<StelObject::CulturalName> StarMgr::getCulturalNames(StarId hip)
{
	// As returned, the latest added comes first!
	// Our SC JSONs are typically human generated, with most common first, so reverse the results.

	QList<StelObject::CulturalName>cNames=culturalNamesMap.values(hip);
	std::reverse(cNames.begin(), cNames.end());
	return cNames;
}

QString StarMgr::getCommonEnglishName(StarId hip)
{
	return commonNamesMap.value(hip, QString());
}

QString StarMgr::getSciDesignation(StarId hip)
{
	return sciDesignationsMap.value(hip, QString());
}

QString StarMgr::getSciExtraDesignation(StarId hip)
{
	return sciExtraDesignationsMap.value(hip, QString());
}

QString StarMgr::getCrossIdentificationDesignations(const QString &hip)
{
	Q_ASSERT(hip==hip.trimmed());
	QStringList designations;
	//auto cr = crossIdMap.find(hip);
	//if (cr==crossIdMap.end() && hip.right(1).toUInt()==0) // What was the purpose of this? check for 0 (bug) or space (just trim!)?
	//	cr = crossIdMap.find(hip.left(hip.size()-1));

	if (crossIdMap.contains(hip))
	{
		crossid crossIdData = crossIdMap.value(hip);
		if (crossIdData.hr>0)
			designations << QString("HR %1").arg(crossIdData.hr);

		if (crossIdData.hd>0)
			designations << QString("HD %1").arg(crossIdData.hd);

		if (crossIdData.sao>0)
			designations << QString("SAO %1").arg(crossIdData.sao);
	}

	return designations.join(" - ");
}

QString StarMgr::getWdsDesignation(StarId hip)
{
	return (wdsStarsMap.contains(hip) ? QString("WDS J%1").arg(wdsStarsMap.value(hip).designation) : QString());
}

int StarMgr::getWdsLastObservation(StarId hip)
{
	return (wdsStarsMap.contains(hip) ? wdsStarsMap.value(hip).observation : 0);
}

float StarMgr::getWdsLastPositionAngle(StarId hip)
{
	return (wdsStarsMap.contains(hip) ? wdsStarsMap.value(hip).positionAngle : 0);
}

float StarMgr::getWdsLastSeparation(StarId hip)
{
	return (wdsStarsMap.contains(hip) ? wdsStarsMap.value(hip).separation : 0);
}

QString StarMgr::getGcvsDesignation(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).designation : QString());
}

QString StarMgr::getGcvsVariabilityType(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).vtype : QString());
}

float StarMgr::getGcvsMaxMagnitude(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).maxmag : -99.f);
}

int StarMgr::getGcvsMagnitudeFlag(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).mflag : 0);
}


float StarMgr::getGcvsMinMagnitude(StarId hip, bool firstMinimumFlag)
{
	if (varStarsMap.contains(hip))
	{
		varstar var=varStarsMap.value(hip);
		return (firstMinimumFlag ? var.min1mag : var.min2mag);
	}
	return -99.f;
}

QString StarMgr::getGcvsPhotometricSystem(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).photosys : QString());
}

double StarMgr::getGcvsEpoch(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).epoch : -99.);
}

double StarMgr::getGcvsPeriod(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).period : -99.);
}

int StarMgr::getGcvsMM(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).Mm : -99);
}

QString StarMgr::getGcvsSpectralType(StarId hip)
{
	return (varStarsMap.contains(hip) ? varStarsMap.value(hip).stype : QString());
}

binaryorbitstar StarMgr::getBinaryOrbitData(StarId hip)
{
	return binaryOrbitStarMap.value(hip, binaryorbitstar());
}

void StarMgr::copyDefaultConfigFile()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/stars/hip_gaia3");
		starConfigFileFullPath = StelFileMgr::getUserDir()+"/stars/hip_gaia3/starsConfig.json";
		qDebug() << "Creates file " << QDir::toNativeSeparators(starConfigFileFullPath);
		QFile::copy(StelFileMgr::getInstallationDir()+"/stars/hip_gaia3/defaultStarsConfig.json", starConfigFileFullPath);
		QFile::setPermissions(starConfigFileFullPath, QFile::permissions(starConfigFileFullPath) | QFileDevice::WriteOwner);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << e.what();
		qFatal("Could not create configuration file stars/hip_gaia3/starsConfig.json");
	}
}

void StarMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	starConfigFileFullPath = StelFileMgr::findFile("stars/hip_gaia3/starsConfig.json", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
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

	populateStarsDesignations();
	populateHipparcosLists();

	setFontSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));

	setFlagStars(conf->value("astro/flag_stars", true).toBool());
	setFlagLabels(conf->value("astro/flag_star_name",true).toBool());
	setLabelsAmount(conf->value("stars/labels_amount",3.).toDouble());
	setFlagAdditionalNames(conf->value("astro/flag_star_additional_names",true).toBool());
	setDesignationUsage(conf->value("astro/flag_star_designation_usage", false).toBool());
	setFlagDblStarsDesignation(conf->value("astro/flag_star_designation_dbl", false).toBool());
	setFlagVarStarsDesignation(conf->value("astro/flag_star_designation_var", false).toBool());
	setFlagHIPDesignation(conf->value("astro/flag_star_designation_hip", false).toBool());

	objectMgr->registerStelObjectMgr(this);
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");   // Load pointer texture

	StelApp::getInstance().getCore()->getGeodesicGrid(maxGeodesicGridLevel)->visitTriangles(maxGeodesicGridLevel,initTriangleFunc,this);
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), &StelSkyCultureMgr::currentSkyCultureChanged, this, &StarMgr::updateSkyCulture);

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Stars", displayGroup, N_("Stars"), "flagStarsDisplayed", "S");
	addAction("actionShow_Stars_Labels", displayGroup, N_("Stars labels"), "flagLabelsDisplayed", "Alt+S");
	// Details: https://github.com/Stellarium/stellarium/issues/174
	addAction("actionShow_Stars_MagnitudeLimitIncrease", displayGroup, N_("Increase the magnitude limit for stars"), "increaseStarsMagnitudeLimit()");
	addAction("actionShow_Stars_MagnitudeLimitReduce", displayGroup, N_("Reduce the magnitude limit for stars"), "reduceStarsMagnitudeLimit()");
}


void StarMgr::drawPointer(StelPainter& sPainter, const StelCore* core)
{
	const QList<StelObjectP> newSelected = objectMgr->getSelectedObject("Star");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3f screenpos;
		// Compute 2D pos and return if outside screen
		if (!sPainter.getProjector()->project(pos, screenpos))
			return;

		sPainter.setColor(obj->getInfoColor());
		texPointer->bind();
		sPainter.setBlending(true);
		sPainter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, static_cast<float>(StelApp::getInstance().getAnimationTime())*40.f);
	}
}

bool StarMgr::checkAndLoadCatalog(const QVariantMap& catDesc, const bool load)
{
	const bool checked = catDesc.value("checked").toBool();
	QString catalogFileName = catDesc.value("fileName").toString();

	// See if it is an absolute path, else prepend default path
	if (!(StelFileMgr::isAbsolute(catalogFileName)))
		catalogFileName = "stars/hip_gaia3/"+catalogFileName;

	QString catalogFilePath = StelFileMgr::findFile(catalogFileName);
	if (catalogFilePath.isEmpty())
	{
		// The file is supposed to be checked, but we can't find it
		if (checked)
		{
			qWarning().noquote() << "Could not find star catalog" << QDir::toNativeSeparators(catalogFileName);
			setCheckFlag(catDesc.value("id").toString(), false);
		}
		return false;
	}
	// Possibly fixes crash on Vista
	if (!StelFileMgr::isReadable(catalogFilePath))
	{
		qWarning().noquote() << "User does not have permissions to read catalog" << QDir::toNativeSeparators(catalogFilePath);
		return false;
	}

	if (!checked)
	{
		// The file is not checked but we found it, maybe from a previous download/version
		qInfo().noquote() << "Found file" << QDir::toNativeSeparators(catalogFilePath) << ", checking md5sum...";

		QFile file(catalogFilePath);
		if(file.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
		{
			// Compute the MD5 sum
			QCryptographicHash md5Hash(QCryptographicHash::Md5);
			const qint64 cat_sz = file.size();
			qint64 maxStarBufMd5 = qMin(cat_sz, 9223372036854775807LL);
			uchar *cat = maxStarBufMd5 ? file.map(0, maxStarBufMd5) : Q_NULLPTR;
			if (!cat)
			{
				// The OS was not able to map the file, revert to slower not mmap based method
				static const qint64 maxStarBufMd5 = 1024*1024*8;
				char* mmd5buf = static_cast<char*>(malloc(maxStarBufMd5));
				while (!file.atEnd())
				{
					qint64 sz = file.read(mmd5buf, maxStarBufMd5);
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
					md5Hash.addData(QByteArrayView(mmd5buf, static_cast<int>(sz)));
#else
					md5Hash.addData(mmd5buf, static_cast<int>(sz));
#endif
				}
				free(mmd5buf);
			}
			else
			{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
				md5Hash.addData(QByteArrayView(reinterpret_cast<const char*>(cat), static_cast<int>(cat_sz)));
#else
				md5Hash.addData(reinterpret_cast<const char*>(cat), static_cast<int>(cat_sz));
#endif
				file.unmap(cat);
			}
			file.close();
			if (md5Hash.result().toHex()!=catDesc.value("checksum").toByteArray())
			{
				qWarning().noquote() << "Error: File" << QDir::toNativeSeparators(catalogFileName) << "is corrupt, MD5 mismatch! Found" << md5Hash.result().toHex() << "expected" << catDesc.value("checksum").toByteArray();
				file.remove();
				return false;
			}
			qInfo() << "MD5 sum correct!";
			setCheckFlag(catDesc.value("id").toString(), true);
		}
	}

	if (load)
	{
		ZoneArray* z = ZoneArray::create(catalogFilePath, true);
		if (z)
		{
			if (z->level<gridLevels.size())
			{
				qWarning().noquote() << QDir::toNativeSeparators(catalogFileName) << ", " << z->level << ": duplicate level";
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
	else
	{
		qWarning().noquote() << "Star catalog: " << QDir::toNativeSeparators(catalogFileName) << "is found but not loaded because at least one of the lower levels is missing!";
		return false;
	}
}

void StarMgr::setCheckFlag(const QString& catId, bool b)
{
	// Update the starConfigFileFullPath file to take into account that we now have a new catalog
	int idx=0;
	for (const auto& catV : std::as_const(catalogsDescription))
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
	}
	starSettings["catalogs"]=catalogsDescription;
	QFile tmp(starConfigFileFullPath);
	if(tmp.open(QIODevice::WriteOnly))
	{
		StelJsonParser::write(starSettings, &tmp);
		tmp.close();
	}
}

void StarMgr::loadData(QVariantMap starsConfig)
{
	// Please do not init twice:
	Q_ASSERT(maxGeodesicGridLevel < 0);

	qInfo() << "Loading star data ...";

	catalogsDescription = starsConfig.value("catalogs").toList();
	bool isSuccessing = true;
	foreach (const QVariant& catV, catalogsDescription)
	{
		QVariantMap m = catV.toMap();
		isSuccessing = checkAndLoadCatalog(m, isSuccessing);
	}

	for (int i=0; i<=NR_OF_HIP; i++)
	{
		hipIndex[i].a = Q_NULLPTR;
		hipIndex[i].z = Q_NULLPTR;
		hipIndex[i].s = Q_NULLPTR;
	}
	for (auto* z : std::as_const(gridLevels))
		z->updateHipIndex(hipIndex);

	const QString cat_hip_sp_file_name = starsConfig.value("hipSpectralFile").toString();
	if (cat_hip_sp_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_hip_sp_file_name not found";
	}
	else
	{
		QString tmpFic = StelFileMgr::findFile("stars/hip_gaia3/" + cat_hip_sp_file_name);
		if (tmpFic.isEmpty())
			qWarning() << "ERROR while loading data from" << QDir::toNativeSeparators(("stars/hip_gaia3/" + cat_hip_sp_file_name));
		else
			*spectral_array = initStringListFromFile(tmpFic);
	}

	const QString cat_objtype_file_name = starsConfig.value("objecttypesFile").toString();
	if (cat_objtype_file_name.isEmpty())
	{
		qWarning() << "ERROR: stars:cat_objtype_file_name not found";
	}
	else
	{
		QString tmpFic = StelFileMgr::findFile("stars/hip_gaia3/" + cat_objtype_file_name);
		if (tmpFic.isEmpty())
			qWarning() << "ERROR while loading data from" << QDir::toNativeSeparators(("stars/hip_gaia3/" + cat_objtype_file_name));
		else
			*objtype_array = initStringListFromFile(tmpFic);
	}

	// create an array with the first element being an empty string, and then contain strings A,B,C,...,Z
	component_array->append("");
	for (int i=0; i<26; i++)
		component_array->append(QString(QChar('A'+i)));

	lastMaxSearchLevel = maxGeodesicGridLevel;
	qInfo().noquote() << "Finished loading star catalogue data, max_geodesic_level:" << maxGeodesicGridLevel;
}

void StarMgr::populateHipparcosLists()
{
	hipparcosStars.clear();
	hipStarsHighPM.clear();
	doubleHipStars.clear();
	variableHipStars.clear();
	algolTypeStars.clear();
	classicalCepheidsTypeStars.clear();
	carbonStars.clear();
	bariumStars.clear();
	const int pmLimit = 1000; // arc-second per year!
	for (int hip=0; hip<=NR_OF_HIP; hip++)
	{
		const Star1 *const s = hipIndex[hip].s;
		if (s)
		{
			const SpecialZoneArray<Star1> *const a = hipIndex[hip].a;
			const SpecialZoneData<Star1> *const z = hipIndex[hip].z;
			StelObjectP so = s->createStelObject(a,z);
			hipparcosStars.push_back(so);
			QString spectrum = convertToSpectralType(s->getSpInt());
			// Carbon stars have spectral type, which start with C letter
			if (spectrum.startsWith("C", Qt::CaseInsensitive))
				carbonStars.push_back(so);

			// Barium stars have spectral class G to K and contains "Ba" string
			if ((spectrum.startsWith("G", Qt::CaseInsensitive) || spectrum.startsWith("K", Qt::CaseInsensitive)) && spectrum.contains("Ba", Qt::CaseSensitive))
				bariumStars.push_back(so);

			if (!getGcvsVariabilityType(s->getHip()).isEmpty())
			{
				QPair<StelObjectP, float> sa(so, static_cast<float>(getGcvsPeriod(s->getHip())));
				variableHipStars.push_back(sa);
				
				QString vartype = getGcvsVariabilityType(s->getHip());
				if (vartype.contains("EA"))
					algolTypeStars.push_back(QPair<StelObjectP, float>(so, sa.second));
				else if (vartype.contains("DCEP") && !vartype.contains("DCEPS"))
					classicalCepheidsTypeStars.push_back(QPair<StelObjectP, float>(so, sa.second));
			}
			if (!getWdsDesignation(s->getHip()).isEmpty())
			{
				doubleHipStars.push_back(QPair<StelObjectP, float>(so, getWdsLastSeparation(s->getHip())));
			}
			float pm = s->getPMTotal();
			if (qAbs(pm)>=pmLimit)
			{
				QPair<StelObjectP, float> spm(so, pm / 1000.f);  // in arc-seconds per year
				hipStarsHighPM.push_back(spm);
			}
		}
	}
	qInfo().nospace() << "Of " << hipparcosStars.count() << " HIP stars, " << doubleHipStars.count() << " are binaries, " <<
		   variableHipStars.count() << " variable (" << algolTypeStars.count() << " Algol-type, " << classicalCepheidsTypeStars.count() << " Cepheids), " <<
		   carbonStars.count() << " Carbon stars, " << bariumStars.count() << " Barium stars, and " << hipStarsHighPM.count() << " have PM>1000mas/yr.";

	//qInfo() << classicalCepheidsTypeStars;
}

// Load common names from file
auto StarMgr::loadCommonNames(const QString& commonNameFile) const -> CommonNames
{
	CommonNames commonNames;

	if (commonNameFile.isEmpty()) return commonNames;

	qInfo().noquote() << "Loading star names from" << QDir::toNativeSeparators(commonNameFile);
	QFile cnFile(commonNameFile);
	if (!cnFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "Could not open" << QDir::toNativeSeparators(commonNameFile);
		return commonNames;
	}

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	QString record;
	// Allow empty and comment lines where first char (after optional blanks) is #
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	// record structure is delimited with a | character.  We will
	// use a QRegularExpression to extract the fields. with white-space padding permitted
	// (i.e. it will be stripped automatically) Example record strings:
	// "   677|_("Alpheratz")"
	// "113368|_("Fomalhaut")"
	// Note: Stellarium doesn't support sky cultures made prior to version 0.10.6 now!
	static const QRegularExpression recordRx("^\\s*(\\d+)\\s*\\|[_]*[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)");

	while(!cnFile.atEnd())
	{
		record = QString::fromUtf8(cnFile.readLine()).trimmed();
		lineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;
		QRegularExpressionMatch recMatch=recordRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(commonNameFile)
					     << " - record does not match record pattern";
			qWarning().noquote() << "Problematic record:" << record;
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			StarId hip = recMatch.captured(1).toULongLong(&ok);
#else
			StarId hip = recMatch.captured(1).toInt(&ok);
#endif
			if (!ok)
			{
				qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(commonNameFile)
						     << " - failed to convert " << recMatch.captured(1) << "to a number";
				continue;
			}
			QString englishCommonName = recMatch.captured(2).trimmed();
			if (englishCommonName.isEmpty())
			{
				qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(commonNameFile)
						     << " - empty name field";
				continue;
			}

			const QString englishNameCap = englishCommonName.toUpper();
			commonNames.byHIP[hip] = englishCommonName;
			commonNames.hipByName[englishNameCap] = hip;

			QString reference = recMatch.captured(3).trimmed();
			if (!reference.isEmpty())
			{
				if (referenceMap.contains(hip))
					referenceMap[hip] = referenceMap[hip].append("," + reference);
				else
					referenceMap[hip] = reference;
			}

			readOk++;
		}
	}
	cnFile.close();

	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "common star names";
	return commonNames;
}

// Identify a star HIP/Gaia StarId from its common name and the commonnames list.
// @param data the JSON array assigned to the "NAME commonName" entry of the current skyculture's index.json
// @param commonName the name given in the common_names entry in the current skyculture's index.json
// @param commonNamesIndexToSearchWhileLoading the part of CommonNames that provides HIPO for a given name.
void StarMgr::loadCultureSpecificNameForNamedObject(const QJsonArray& data, const QString& commonName,
                                                    const QMap<QString, int>& commonNamesIndexToSearchWhileLoading,
                                                    const QSet<int> &excludedRefs)
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	const QString commonNameUpper=commonName.toUpper();
	if (!commonNamesIndexToSearchWhileLoading.contains(commonNameUpper))
	{
		// This may actually not even be a star, so we shouldn't emit any warning, just return
		return;
	}
	const int HIP = commonNamesIndexToSearchWhileLoading.value(commonNameUpper);

	for (const QJsonValue& entry : data)
	{
		// See if we have configured unwanted references. We can only attempt to exclude entries which actually have references.
		QVariantList refsVariants=entry["references"].toArray().toVariantList();
		if (!refsVariants.isEmpty())
		{
			QSet<int> refs;
			foreach(const QVariant &v, refsVariants) {
			    refs << v.value<int>();
			}
			if (refs.subtract(excludedRefs).isEmpty())
				continue;
		}

		StelObject::CulturalName cName{entry["native"].toString(), entry["pronounce"].toString(), trans.qTranslateStar(entry["pronounce"].toString()),
					entry["transliteration"].toString(), entry["english"].toString(), trans.qTranslateStar(entry["english"].toString()), entry["IPA"].toString()};

		//if (culturalNamesMap.contains(HIP))
		//	qInfo() << "Adding additional cultural name for HIP" << HIP << ":" <<  cName.native << "/" << cName.pronounceI18n << "/" << cName.translated;
		culturalNamesMap.insert(HIP, cName); // add as possibly multiple entry to HIP.
		if (!cName.native.isEmpty())
			culturalNamesIndex.insert(cName.native.toUpper(), HIP);
		if (!cName.pronounceI18n.isEmpty())
			culturalNamesIndex.insert(cName.pronounceI18n.toUpper(), HIP);
		if (!cName.transliteration.isEmpty())
			culturalNamesIndex.insert(cName.transliteration.toUpper(), HIP);
		if (!cName.translatedI18n.isEmpty())
			culturalNamesIndex.insert(cName.translatedI18n.toUpper(), HIP);
	}
}

// data: the array containing one or more name dicts
void StarMgr::loadCultureSpecificNameForStar(const QJsonArray& data, const StarId HIP, const QSet<int> &excludedRefs)
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	for (const QJsonValue& entry : data)
	{
		// See if we have configured unwanted references. We can only attempt to exclude entries which actually have references.
		QVariantList refsVariants=entry["references"].toArray().toVariantList();
		if (!refsVariants.isEmpty())
		{
			QSet<int> refs;
			foreach(const QVariant &v, refsVariants) {
			    refs << v.value<int>();
			}
			//qInfo() << "Star" << entry["native"].toString() << "has refs" << refs;
			if (refs.subtract(excludedRefs).isEmpty())
			{
				//qInfo() << "Star name" << entry["native"].toString() << "has lost support. Skipping.";
				continue;
			}
			//else
			//	qInfo() << "Star" << entry["native"].toString() << "still has refs" << refs.subtract(excludedRefs);
		}

		StelObject::CulturalName cName{entry["native"].toString(), entry["pronounce"].toString(), trans.qTranslateStar(entry["pronounce"].toString()),
					entry["transliteration"].toString(), entry["english"].toString(), trans.qTranslateStar(entry["english"].toString()), entry["IPA"].toString()};
		//if (culturalNamesMap.contains(HIP))
		//	qInfo() << "Adding additional cultural name for HIP" << HIP << ":" <<  cName.native << "/" << cName.pronounceI18n << "/" << cName.translated << "/" << cName.translatedI18n;
		culturalNamesMap.insert(HIP, cName); // add as possibly multiple entry to HIP.
		if (!cName.native.isEmpty())
			culturalNamesIndex.insert(cName.native.toUpper(), HIP);
		if (!cName.pronounceI18n.isEmpty())
			culturalNamesIndex.insert(cName.pronounceI18n.toUpper(), HIP);
		if (!cName.transliteration.isEmpty())
			culturalNamesIndex.insert(cName.transliteration.toUpper(), HIP);
		if (!cName.translatedI18n.isEmpty())
			culturalNamesIndex.insert(cName.translatedI18n.toUpper(), HIP);
	}
	//qInfo() << "Skyculture has " << culturalNamesMap.size() << "entries, index has" << culturalNamesIndex.size();
}

void StarMgr::loadCultureSpecificNames(const QJsonObject& data, const QMap<QString, int>& commonNamesIndexToSearchWhileLoading, const QSet<int> &excludedRefs)
{
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		const auto key = it.key();
		// Let's allow Hipparcos and Gaia designations only
		if (key.startsWith("HIP "))
		{
			loadCultureSpecificNameForStar(it.value().toArray(), key.mid(4).toUInt(), excludedRefs);
		}
		else if (key.startsWith("Gaia DR3 "))
		{
			loadCultureSpecificNameForStar(it.value().toArray(), key.mid(9).toULongLong(), excludedRefs);
		}
		else if (key.startsWith("NAME "))
		{
			loadCultureSpecificNameForNamedObject(it.value().toArray(), key.mid(5), commonNamesIndexToSearchWhileLoading, excludedRefs);
		}
	}
}

// Load scientific designations from file
void StarMgr::loadSciDesignations(const QString& sciNameFile, QHash<StarId, QString>map, QHash<QString, StarId>index)
{
	map.clear();
	index.clear();
	qInfo().noquote() << "Loading scientific star designations from" << QDir::toNativeSeparators(sciNameFile);

	QFile snFile(sciNameFile);
	if (!snFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "Could not open" << QDir::toNativeSeparators(sciNameFile);
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
	for (const QString& record : allRecords)
	{
		++lineNumber;
		// skip comments and empty lines
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;

		++totalRecords;
		const QStringList& fields = record.split('|');
		if (fields.size()!=2)
		{
			qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(sciNameFile)
					     << " - record does not match record pattern. Skipping line.";
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok;
			StarId hip = fields.at(0).toLongLong(&ok);
			if (!ok)
			{
				qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(sciNameFile)
						     << " - failed to convert " << fields.at(0) << "to a number";
				continue;
			}

			QString sci_name = fields.at(1).trimmed();
			if (sci_name.isEmpty())
			{
				qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(sciNameFile)
						     << " - empty name field";
				continue;
			}

			sci_name.replace('_',' ');
			// Don't set the main sci name if it's already set - it's additional sci name
			if (map.contains(hip))
			{
				map[hip].append(" - " + sci_name);
			}
			else
				map.insert(hip, sci_name);
			index[sci_name] = hip;

			++readOk;
		}
	}
	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "scientific designations";
}

// Load GCVS from file
void StarMgr::loadGcvs(const QString& GcvsFileName)
{
	varStarsMap.clear();
	varStarsIndex.clear();

	qInfo().noquote() << "Loading variable stars data from" << QDir::toNativeSeparators(GcvsFileName);

	QFile vsFile(GcvsFileName);
	if (!vsFile.open(QIODevice::ReadOnly))
	{
		qDebug().noquote() << "Cannot open file" << QDir::toNativeSeparators(GcvsFileName);
		return;
	}
	QByteArray data = StelUtils::uncompress(vsFile);
	vsFile.close();
	//check if decompressing was successful
	if(data.isEmpty())
	{
		qDebug().noquote() << "Could not decompress file" << QDir::toNativeSeparators(GcvsFileName);
		return;
	}
	//create and open a QBuffer for reading
	QBuffer buf(&data);
	buf.open(QIODevice::ReadOnly);

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	// Version of GCVS catalog
	static const QRegularExpression versionRx("\\s*Version:\\s*([\\d\\-\\.]+)\\s*");

	// record structure is delimited with a tab character.
	while (!buf.atEnd())
	{
		QString record = QString::fromUtf8(buf.readLine());
		lineNumber++;

		// skip comments and empty lines
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
		{
			QRegularExpressionMatch versionMatch=versionRx.match(record);
			if (versionMatch.hasMatch())
				qInfo().noquote() << "[...]" << QString("GCVS %1").arg(versionMatch.captured(1).trimmed());
			continue;
		}

		totalRecords++;
		const QStringList& fields = record.split('\t');

		bool ok;
		StarId hip = fields.at(0).toLongLong(&ok);
		if (!ok)
			continue;

		// Don't set the star if it's already set
		if (varStarsMap.contains(hip))
			continue;

		varstar variableStar;

		variableStar.designation = fields.at(1).trimmed();
		variableStar.vtype = fields.at(2).trimmed();
		variableStar.maxmag = fields.at(3).isEmpty() ? 99.f : fields.at(3).toFloat();
		variableStar.mflag = fields.at(4).toInt();
		variableStar.min1mag = fields.at(5).isEmpty() ? 99.f : fields.at(5).toFloat();
		variableStar.min2mag = fields.at(6).isEmpty() ? 99.f : fields.at(6).toFloat();
		variableStar.photosys = fields.at(7).trimmed();
		variableStar.epoch = fields.at(8).toDouble();
		variableStar.period = fields.at(9).toDouble();
		variableStar.Mm = fields.at(10).toInt();
		variableStar.stype = fields.at(11).trimmed();

		varStarsMap[hip] = variableStar;
		varStarsIndex[variableStar.designation.toUpper()] = hip;
		++readOk;
	}

	buf.close();
	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "variable stars";
}

// Load WDS from file
void StarMgr::loadWds(const QString& WdsFileName)
{
	wdsStarsMap.clear();
	wdsStarsIndex.clear();

	qInfo().noquote() << "Loading double stars from" << QDir::toNativeSeparators(WdsFileName);
	QFile dsFile(WdsFileName);
	if (!dsFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "Could not open" << QDir::toNativeSeparators(WdsFileName);
		return;
	}
	const QStringList& allRecords = QString::fromUtf8(dsFile.readAll()).split('\n');
	dsFile.close();

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;

	// record structure is delimited with a tab character.
	for (const auto& record : allRecords)
	{
		++lineNumber;
		// skip comments and empty lines
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;

		++totalRecords;
		const QStringList& fields = record.split('\t');

		bool ok;
		StarId hip = fields.at(0).toLongLong(&ok);
		if (!ok)
		{
			qWarning() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(WdsFileName)
				   << " - failed to convert " << fields.at(0) << "to a number";
			continue;
		}

		// Don't set the star if it's already set
		if (wdsStarsMap.contains(hip))
		{
			qWarning() << "HIP" << hip << "already processed. Ignoring record:" << record;
			continue;
		}

		wds doubleStar = {fields.at(1).trimmed(), fields.at(2).toInt(), fields.at(3).toFloat(),fields.at(4).toFloat()};
		wdsStarsMap[hip] = doubleStar;
		wdsStarsIndex[QString("WDS J%1").arg(doubleStar.designation.toUpper())] = hip;
		++readOk;
	}

	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "double stars";
}

// Load cross-identification data from file
void StarMgr::loadCrossIdentificationData(const QString& crossIdFile)
{
	crossIdMap.clear();
	saoStarsIndex.clear();	
	hdStarsIndex.clear();	
	hrStarsIndex.clear();

	qInfo().noquote() << "Loading cross-identification data from" << QDir::toNativeSeparators(crossIdFile);
	QFile ciFile(crossIdFile);
	if (!ciFile.open(QIODevice::ReadOnly))
	{
		qWarning().noquote() << "Could not open" << QDir::toNativeSeparators(crossIdFile);
		return;
	}

	QDataStream in(&ciFile);
	in.setByteOrder(QDataStream::LittleEndian);

	crossid crossIdData;

	int readOk = 0;
	int totalRecords = 0;
	StarId hip;
	unsigned char component;
	int sao, hd, hr;
	QString hipstar;
	quint64 hipTemp;

	while (!in.atEnd())
	{
		++totalRecords;

		in >> hipTemp >> component >> sao >> hd >> hr;
		hip = static_cast<StarId>(hipTemp);

		if (in.status() != QDataStream::Ok)
		{
			qWarning().noquote() << "Parse error in" << QDir::toNativeSeparators(crossIdFile)
						 << " - failed to read data";
			break;
		}

		hipstar = QString("%1%2").arg(hip).arg(component == 0 ? QString() : QString(QChar('A' + component - 1)));
		crossIdData.sao = sao;
		crossIdData.hd = hd;
		crossIdData.hr = hr;

		crossIdMap[hipstar] = crossIdData;
		if (crossIdData.sao > 0)
			saoStarsIndex[crossIdData.sao] = hip;
		if (crossIdData.hd > 0)
			hdStarsIndex[crossIdData.hd] = hip;
		if (crossIdData.hr > 0)
			hrStarsIndex[crossIdData.hr] = hip;

		++readOk;
	}

	ciFile.close();
	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "cross-identification data records for stars";
}

// load binary system orbital parameters data
void StarMgr::loadBinaryOrbitalData(const QString& orbitalParamFile)
{
	qInfo().noquote() << "Loading orbital parameters data for binary system from" << QDir::toNativeSeparators(orbitalParamFile);
	QFile opFile(orbitalParamFile);
	if (!opFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "Could not open" << QDir::toNativeSeparators(orbitalParamFile);
		return;
	}
	const QStringList& allRecords = QString::fromUtf8(opFile.readAll()).split('\n');
	opFile.close();

	binaryorbitstar orbitalData;

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	StarId hip;
	for (const auto& record : allRecords)
	{
		++lineNumber;
		// skip comments and empty lines
		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty())
			continue;

		++totalRecords;
		const QStringList& fields = record.split('\t');
		if (fields.size()!=18)
		{
			qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(orbitalParamFile)
					     << " - record does not match record pattern";
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok, allOK=true;
			orbitalData.binary_period    = fields.at(2).toDouble(&ok);  allOK &= ok;
			orbitalData.eccentricity     = fields.at(3).toFloat(&ok);   allOK &= ok;
			orbitalData.inclination      = fields.at(4).toFloat(&ok);   allOK &= ok;
			orbitalData.big_omega        = fields.at(5).toFloat(&ok);   allOK &= ok;
			orbitalData.small_omega      = fields.at(6).toFloat(&ok);   allOK &= ok;
			orbitalData.periastron_epoch = fields.at(7).toDouble(&ok);  allOK &= ok;
			orbitalData.semi_major       = fields.at(8).toDouble(&ok);  allOK &= ok;
			orbitalData.bary_distance    = fields.at(9).toDouble(&ok);  allOK &= ok;
			orbitalData.data_epoch       = fields.at(10).toDouble(&ok); allOK &= ok;
			orbitalData.bary_ra          = fields.at(11).toDouble(&ok); allOK &= ok;
			orbitalData.bary_dec         = fields.at(12).toDouble(&ok); allOK &= ok;
			orbitalData.bary_rv          = fields.at(13).toDouble(&ok); allOK &= ok;
			orbitalData.bary_pmra        = fields.at(14).toDouble(&ok); allOK &= ok;
			orbitalData.bary_pmdec       = fields.at(15).toDouble(&ok); allOK &= ok;
			orbitalData.primary_mass     = fields.at(16).toFloat(&ok);  allOK &= ok;
			orbitalData.secondary_mass   = fields.at(17).toFloat(&ok);  allOK &= ok;
			// do primary star
			hip = fields.at(0).toLongLong(&ok); allOK &= ok;
			orbitalData.hip = hip;
			orbitalData.primary = true;
			binaryOrbitStarMap[hip] = orbitalData;
			// do secondary star
			hip = fields.at(1).toLongLong(&ok); allOK &= ok;
			orbitalData.hip = hip;
			orbitalData.primary = false;
			binaryOrbitStarMap[hip] = orbitalData;

			if (!allOK)
			{
				qWarning().noquote() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(orbitalParamFile)
						     << " - failed to convert " << fields.at(0) << "to a number";
				continue;
			}
			++readOk;
		}
	}

	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "orbital parameters data for binary system";
}

int StarMgr::getMaxSearchLevel() const
{
	int rval = -1;
	for (const auto* z : gridLevels)
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
	if (!static_cast<bool>(starsFader.getInterstate()))
		return;

	int maxSearchLevel = getMaxSearchLevel();
	double margin = 0.;  // pixel margin to be applied to viewport convex polygon
	if (core->getSkyDrawer()->getFlagHasAtmosphere())
	{
		// Saemundsson's inversion formula for atmospheric refraction is not exact, so need some padding in terms of arcseconds
		margin = 30000. * MAS2RAD * prj->getPixelPerRadAtCenter();  // 0.5 arcmin
	}
	
	QVector<SphericalCap> viewportCaps = prj->getViewportConvexPolygon(margin, margin)->getBoundingSphericalCaps();
	viewportCaps.append(core->getVisibleSkyArea());
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(maxSearchLevel)->search(viewportCaps,maxSearchLevel);

	// Set temporary static variable for optimization
	const float names_brightness = labelsFader.getInterstate() * starsFader.getInterstate();

	// prepare for aberration: Explan. Suppl. 2013, (7.38)
	const bool withAberration=core->getUseAberration();
	Vec3d vel(0.);
	if (withAberration)
	{
		vel = core->getAberrationVec(core->getJDE());
	}

	// Prepare openGL for drawing many stars
	StelPainter sPainter(prj);
	sPainter.setFont(starFont);
	skyDrawer->preDrawPointSource(&sPainter);

	// Prepare a table for storing precomputed RCMag for all ZoneArrays
	RCMag rcmag_table[RCMAG_TABLE_SIZE];
	
	// Draw all the stars of all the selected zones
	for (const auto* z : std::as_const(gridLevels))
	{
		int limitMagIndex=RCMAG_TABLE_SIZE;
		// overshoot by 7 mag because some stars (like HIP 16757) can get brighter than its catalogs min magnitude
		const float mag_min = 0.001f*z->mag_min - 7.f;
		for (int i=0;i<RCMAG_TABLE_SIZE;++i)
		{
			const float mag = mag_min+0.05*i;  // 0.05 mag MagStepIncrement
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

		int maxMagStarName = 0;
		if (labelsFader.getInterstate()>0.f)
		{
			// Adapt magnitude limit of the stars labels according to FOV and labelsAmount
			float maxMag = (skyDrawer->getLimitMagnitude()-6.5f)*0.7f+(static_cast<float>(labelsAmount)*1.2f)-2.f;
			int x = static_cast<int>((maxMag-mag_min)/0.05);  // 0.05 mag MagStepIncrement
			if (x > 0)
				maxMagStarName = x;
		}
		int zone;
		double withParallax = core->getUseParallax() * core->getParallaxFactor();
		Vec3d diffPos(0., 0., 0.);
		if (withParallax) {
			diffPos = core->getParallaxDiff(core->getJDE());
		}
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,z->level);(zone = it1.next()) >= 0;)
			z->draw(&sPainter, zone, true, rcmag_table, limitMagIndex, core, maxMagStarName, names_brightness, viewportCaps, withAberration, vel, withParallax, diffPos);
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,z->level);(zone = it1.next()) >= 0;)
			z->draw(&sPainter, zone, false, rcmag_table, limitMagIndex, core, maxMagStarName,names_brightness, viewportCaps, withAberration, vel, withParallax, diffPos);
		// always check the last zone because it is a global zone
		z->draw(&sPainter, (20<<(z->level<<1)), false, rcmag_table, limitMagIndex, core, maxMagStarName, names_brightness, viewportCaps, withAberration, vel, withParallax, diffPos);
	}
	exit_loop:

	// Finish drawing many stars
	skyDrawer->postDrawPointSource(&sPainter);

	if (objectMgr->getFlagSelectedObjectPointer())
		drawPointer(sPainter, core);
}


// Return a QList containing the stars located
// inside the limFov circle around position vv (in J2000 frame without aberration)
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
	f = 1.0/e0.norm();
	e0 *= f;
	e1 *= f;
	e2 *= f;
	e3 *= f;
	// Search the triangles
	SphericalConvexPolygon c(e3, e2, e2, e0);
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid(lastMaxSearchLevel)->search(c.getBoundingSphericalCaps(),lastMaxSearchLevel);

	double withParallax = core->getUseParallax() * core->getParallaxFactor();
	Vec3d diffPos(0., 0., 0.);
	if (withParallax) {
		diffPos = core->getParallaxDiff(core->getJDE());
	}

	// Iterate over the stars inside the triangles
	f = cos(limFov * M_PI/180.);
	for (auto* z : gridLevels)
	{
		//qDebug() << "search inside(" << it->first << "):";
		int zone;
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,z->level);(zone = it1.next()) >= 0;)
		{
			z->searchAround(core, zone, v, withParallax, diffPos, f, result);
			//qDebug() << " " << zone;
		}
		//qDebug() << StelUtils::getEndLineChar() << "search border(" << it->first << "):";
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,z->level); (zone = it1.next()) >= 0;)
		{
			z->searchAround(core, zone, v, withParallax, diffPos, f, result);
			//qDebug() << " " << zone;
		}
		// always search the last zone because it is a global zone
		z->searchAround(core, (20<<(z->level<<1)), v, withParallax, diffPos, f, result);
	}
	return result;
}


//! Update i18 names from english names according to passed translator.
//! The translation is done using gettext with translated strings defined in translations.h
void StarMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	commonNamesMapI18n.clear();
	commonNamesUppercaseIndexI18n.clear();
	//additionalNamesMapI18n.clear();
	//additionalNamesIndexI18n.clear();
	for (QHash<StarId,QString>::ConstIterator it(commonNamesMap.constBegin());it!=commonNamesMap.constEnd();it++)
	{
		const StarId i = it.key();
		const QString t(trans.qTranslateStar(it.value()));
		commonNamesMapI18n[i] = t;
		commonNamesUppercaseIndexI18n[t.toUpper()] = i;
	}
	//for (QHash<StarId,QString>::ConstIterator ita(additionalNamesMap.constBegin());ita!=additionalNamesMap.constEnd();ita++)
	//{
	//	const StarId i = ita.key();
	//	const QStringList a = ita.value().split(" - ");
	//	QStringList tn;
	//	for (const auto& str : a)
	//	{
	//		QString tns = trans.qTranslateStar(str);
	//		tn << tns;
	//		additionalNamesIndexI18n[tns.toUpper()] = i;
	//	}
	//	const QString r = tn.join(" - ");
	//	additionalNamesMapI18n[i] = r;
	//}
	culturalNamesIndex.clear();
	for (QMultiHash<StarId,StelObject::CulturalName>::iterator it(culturalNamesMap.begin());it!=culturalNamesMap.end();++it)
	{
		StarId HIP=it.key();
		StelObject::CulturalName &cName=it.value();
		cName.pronounceI18n=trans.qTranslateStar(cName.pronounce);
		cName.translatedI18n=trans.qTranslateStar(cName.translated);
		it.value() = cName;

		// rebuild index
		if (!cName.native.isEmpty())
			culturalNamesIndex.insert(cName.native.toUpper(), HIP);
		if (!cName.pronounceI18n.isEmpty())
			culturalNamesIndex.insert(cName.pronounceI18n.toUpper(), HIP);
		if (!cName.transliteration.isEmpty())
			culturalNamesIndex.insert(cName.transliteration.toUpper(), HIP);
		if (!cName.translatedI18n.isEmpty())
			culturalNamesIndex.insert(cName.translatedI18n.toUpper(), HIP);
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

// Search the star by Gaia source_id
StelObjectP StarMgr::searchGaia(StarId source_id) const
{
	int maxSearchLevel = getMaxSearchLevel();
	int matched = 0;
	int index = 0;
	// get the level 12 HEALPix index of the source
	int lv12_pix = source_id / 34359738368;
	Vec3d v;
	StelObjectP so;
	healpix_pix2vec(pow(2, 12), lv12_pix, v.v);  // search which pixel the source is in and turn to coordinates
	Vec3f vf = v.toVec3f();

	for (const auto* z : gridLevels)
	{
		// search the zone where the source is in
		index = StelApp::getInstance().getCore()->getGeodesicGrid(maxSearchLevel)->getZoneNumberForPoint(vf, z->level);
		so = z->searchGaiaID(index, source_id, matched);
		if (matched)
			return so;
		
		// then search the global zone 
		so = z->searchGaiaID((20<<(z->level<<1)), source_id, matched);
		if (matched)
			return so;
	}
	return StelObjectP();
}

StelObjectP StarMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString nameI18nUpper = nameI18n.toUpper();

	// Search by I18n common name
	if (commonNamesUppercaseIndexI18n.contains(nameI18nUpper))
		return searchHP(commonNamesUppercaseIndexI18n.value(nameI18nUpper));

	//// Search by I18n additional common names?
	//if (getFlagAdditionalNames() && additionalNamesIndexI18n.contains(nameI18nUpper))
	//	return searchHP(additionalNamesIndexI18n.value(nameI18nUpper));

	// Search by cultural names? (Any names: native/pronounceI18n/translatedI18n/transliteration
	if (getFlagAdditionalNames() && culturalNamesIndex.contains(nameI18nUpper))
	{
		// This only returns the first-found number.
		StarId starId=culturalNamesIndex.value(nameI18nUpper);
		return (starId <= NR_OF_HIP ? searchHP(starId) : searchGaia(starId));
	}

	return searchByName(nameI18n);
}

StelObjectP StarMgr::searchByName(const QString& name) const
{
	QString nameUpper = name.toUpper();
	StarId sid;

	// Search by HP number if it's an HP formatted number. The final part (A/B/...) is ignored
	static const QRegularExpression rx("^\\s*(HP|HIP)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch match=rx.match(nameUpper);
	if (match.hasMatch())
		return searchHP(match.captured(2).toInt());

	// Search by SAO number if it's an SAO formatted number
	static const QRegularExpression rx2("^\\s*(SAO)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=rx2.match(nameUpper);
	if (match.hasMatch())
	{
		const int saoIndex=match.captured(2).toInt();
		if (saoStarsIndex.contains(saoIndex))
		{
			sid = saoStarsIndex.value(saoIndex);
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by HD number if it's an HD formatted number
	static const QRegularExpression rx3("^\\s*(HD)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=rx3.match(nameUpper);
	if (match.hasMatch())
	{
		const int hdIndex=match.captured(2).toInt();
		if (hdStarsIndex.contains(hdIndex))
		{
			sid = hdStarsIndex.value(hdIndex);
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by HR number if it's an HR formatted number
	static const QRegularExpression rx4("^\\s*(HR)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=rx4.match(nameUpper);
	if (match.hasMatch())
	{
		const int hrIndex=match.captured(2).toInt();
		if (hrStarsIndex.contains(hrIndex))
		{
			sid = hrStarsIndex.value(hrIndex);
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by Gaia number if it's an Gaia formatted number.
	static const QRegularExpression rx5("^\\s*(Gaia DR3)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	match=rx5.match(nameUpper);
	if (match.hasMatch())
		return searchGaia(match.captured(2).toLongLong());

	// Search by English common name
	if (commonNamesUppercaseIndex.contains(nameUpper))
	{
		sid = commonNamesUppercaseIndex.value(nameUpper);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	if (getFlagAdditionalNames())
	{
		//// Search by English additional common names
		//if (additionalNamesIndex.contains(nameUpper))
		//{
		//	sid = additionalNamesIndex.value(nameUpper);
		//	return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		//}

		if (culturalNamesIndex.contains(nameUpper))
		{
			// This only returns the first-found number.
			sid=culturalNamesIndex.value(nameUpper);
			return (sid <= NR_OF_HIP ? searchHP(sid) : searchGaia(sid));
		}
	}

	// Search by scientific name
	if (sciDesignationsIndex.contains(name)) // case sensitive!
	{
		sid = sciDesignationsIndex.value(name);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}
	if (sciDesignationsIndex.contains(nameUpper)) // case insensitive!
	{
		sid = sciDesignationsIndex.value(nameUpper);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	// Search by scientific name
	if (sciExtraDesignationsIndex.contains(name)) // case sensitive!
	{
		sid = sciExtraDesignationsIndex.value(name);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}
	if (sciExtraDesignationsIndex.contains(nameUpper)) // case insensitive!
	{
		sid = sciExtraDesignationsIndex.value(nameUpper);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	// Search by GCVS name
	if (varStarsIndex.contains(nameUpper))
	{
		sid = varStarsIndex.value(nameUpper);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	// Search by WDS name
	if (wdsStarsIndex.contains(nameUpper))
	{
		sid = wdsStarsIndex.value(nameUpper);
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	return StelObjectP();
}

StelObjectP StarMgr::searchByID(const QString &id) const
{
	return searchByName(id);
}

//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
QStringList StarMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem <= 0 || !getFlagStars())
		return result;

	const QString objPrefixUpper = objPrefix.toUpper();
	bool found;

	// Search for common names
	QHashIterator<QString, StarId> i(commonNamesUppercaseIndexI18n);
	while (i.hasNext())
	{
		i.next();
		if (useStartOfWords && i.key().startsWith(objPrefixUpper))
			found = true;
		else found = (!useStartOfWords && i.key().contains(objPrefixUpper));

		if (found)
		{
			if (maxNbItem<=0)
				break;
			result.append(getCommonNameI18n(i.value()));
			--maxNbItem;
		}
	}

	QHashIterator<QString, StarId> j(commonNamesUppercaseIndex);
	while (j.hasNext())
	{
		j.next();
		if (useStartOfWords && j.key().startsWith(objPrefixUpper))
			found = true;
		else found = (!useStartOfWords && j.key().contains(objPrefixUpper));

		if (found)
		{
			if (maxNbItem<=0)
				break;
			result.append(getCommonEnglishName(j.value()));
			--maxNbItem;
		}
	}

	if (getFlagAdditionalNames())
	{
		//QMapIterator<QString, StarId> k(additionalNamesIndexI18n);
		//while (k.hasNext())
		//{
		//	k.next();
		//	QStringList names = getAdditionalNames(k.value()).split(" - ");
		//	for (const auto &name : std::as_const(names))
		//	{
		//		if (useStartOfWords && name.startsWith(objPrefixUpper, Qt::CaseInsensitive))
		//			found = true;
		//		else if (!useStartOfWords && name.contains(objPrefixUpper, Qt::CaseInsensitive))
		//			found = true;
		//		else
		//			found = false;

		//		if (found)
		//		{
		//			if (maxNbItem<=0)
		//				break;
		//			result.append(name);
		//			--maxNbItem;
		//		}
		//	}
		//}

		//QMapIterator<QString, StarId> l(additionalNamesIndex);
		//while (l.hasNext())
		//{
		//	l.next();
		//	QStringList names = getAdditionalEnglishNames(l.value()).split(" - ");
		//	for (const auto &name : std::as_const(names))
		//	{
		//		if (useStartOfWords && name.startsWith(objPrefixUpper, Qt::CaseInsensitive))
		//			found = true;
		//		else if (!useStartOfWords && name.contains(objPrefixUpper, Qt::CaseInsensitive))
		//			found = true;
		//		else
		//			found = false;

		//		if (found)
		//		{
		//			if (maxNbItem<=0)
		//				break;
		//			result.append(name);
		//			--maxNbItem;
		//		}
		//	}
		//}

#if  (QT_VERSION<QT_VERSION_CHECK(6,0,0))
		QMapIterator<QString, StarId>it(culturalNamesIndex);
#else
		QMultiMapIterator<QString, StarId>it(culturalNamesIndex);
#endif
		while (it.hasNext())
		{
			it.next();
			QString name=it.key();

			if (useStartOfWords && name.startsWith(objPrefixUpper, Qt::CaseInsensitive))
				found = true;
			else found = (!useStartOfWords && name.contains(objPrefixUpper, Qt::CaseInsensitive));

			if (found)
			{
				if (maxNbItem<=0)
					break;
				result.append(name);
				--maxNbItem;
			}
		}
	}

	// Search for sci names
	// need special character escape because many stars have name starting with "*"
	QString bayerPattern = objPrefix;
	QRegularExpression bayerRegEx(QRegularExpression::escape(bayerPattern));
	QString bayerPatternCI = objPrefixUpper;
	QRegularExpression bayerRegExCI(QRegularExpression::escape(bayerPatternCI));

	// if the first character is a Greek letter, check if there's an index
	// after it, such as "alpha1 Cen".
	if (objPrefix.at(0).unicode() >= 0x0391 && objPrefix.at(0).unicode() <= 0x03A9)
		bayerRegEx.setPattern(bayerPattern.insert(1,"\\d?"));	
	if (objPrefixUpper.at(0).unicode() >= 0x0391 && objPrefixUpper.at(0).unicode() <= 0x03A9)
		bayerRegExCI.setPattern(bayerPatternCI.insert(1,"\\d?"));

	//for (auto it = sciDesignationsIndex.begin(objPrefix); it != sciDesignationsIndex.end(); ++it)
	QHashIterator<QString,StarId>it(sciDesignationsIndex);
	while (it.hasNext())
	{
		it.next();
		if (it.key().indexOf(bayerRegEx)==0 || it.key().indexOf(objPrefix)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciDesignation(it.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else found = (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive));

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
		else if (it.key().at(0) != objPrefix.at(0))
			break;
	}

	//for (auto it = sciDesignationsIndex.lowerBound(objPrefixUpper); it != sciDesignationsIndex.end(); ++it)
	it.toFront(); // reset
	while (it.hasNext())
	{
		it.next();
		if (it.key().indexOf(bayerRegExCI)==0 || it.key().indexOf(objPrefixUpper)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciDesignation(it.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else found = (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive));

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
		else if (it.key().at(0) != objPrefixUpper.at(0))
			break;
	}

	//for (auto it = sciExtraDesignationsIndex.lowerBound(objPrefix); it != sciExtraDesignationsIndex.end(); ++it)
	QHashIterator<QString,StarId>ite(sciExtraDesignationsIndex);
	while (ite.hasNext())
	{
		ite.next();
		if (ite.key().indexOf(bayerRegEx)==0 || ite.key().indexOf(objPrefix)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciExtraDesignation(ite.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else found = (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive));

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
		else if (ite.key().at(0) != objPrefix.at(0))
			break;
	}

	//for (auto it = sciExtraDesignationsIndex.lowerBound(objPrefixUpper); it != sciExtraDesignationsIndex.end(); ++it)
	ite.toFront(); // reset
	while (ite.hasNext())
	{
		ite.next();
		if (ite.key().indexOf(bayerRegExCI)==0 || ite.key().indexOf(objPrefixUpper)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciExtraDesignation(ite.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else found = (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive));

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
		else if (ite.key().at(0) != objPrefixUpper.at(0))
			break;
	}

	// Search for sci names for var stars
	//for (auto it = varStarsIndex.lowerBound(objPrefixUpper); it != varStarsIndex.end(); ++it)
	QHashIterator<QString,StarId>itv(varStarsIndex);
	while (itv.hasNext())
	{
		itv.next();
		if (itv.key().startsWith(objPrefixUpper))
		{
			if (maxNbItem<=0)
				break;
			result << getGcvsDesignation(itv.value());
			--maxNbItem;
		}
		else
			break;
	}

	StarId sid;
	// Add exact Hp catalogue numbers. The final part (A/B/...) is ignored
	static const QRegularExpression hpRx("^(HIP|HP)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch match=hpRx.match(objPrefixUpper);
	if (match.hasMatch())
	{
		bool ok;
		int hpNum = match.captured(2).toInt(&ok);
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
	static const QRegularExpression saoRx("^(SAO)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=saoRx.match(objPrefixUpper);
	if (match.hasMatch())
	{
		int saoNum = match.captured(2).toInt();
		if (saoStarsIndex.contains(saoNum))
		{
			sid = saoStarsIndex.value(saoNum);
			StelObjectP s =  (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
			if (s && maxNbItem>0)
			{
				result << QString("SAO%1").arg(saoNum);
				maxNbItem--;
			}
		}
	}

	// Add exact HD catalogue numbers
	static const QRegularExpression hdRx("^(HD)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=hdRx.match(objPrefixUpper);
	if (match.hasMatch())
	{
		int hdNum = match.captured(2).toInt();
		if (hdStarsIndex.contains(hdNum))
		{
			sid = hdStarsIndex.value(hdNum);
			StelObjectP s =  (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
			if (s && maxNbItem>0)
			{
				result << QString("HD%1").arg(hdNum);
				maxNbItem--;
			}
		}
	}

	// Add exact HR catalogue numbers
	static const QRegularExpression hrRx("^(HR)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=hrRx.match(objPrefixUpper);
	if (match.hasMatch())
	{
		int hrNum = match.captured(2).toInt();
		if (hrStarsIndex.contains(hrNum))
		{
			sid = hrStarsIndex.value(hrNum);
			StelObjectP s =  (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
			if (s && maxNbItem>0)
			{
				result << QString("HR%1").arg(hrNum);
				maxNbItem--;
			}
		}
	}

	// Add exact WDS catalogue numbers
	static const QRegularExpression wdsRx("^(WDS)\\s*(\\S+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	if (wdsRx.match(objPrefixUpper).hasMatch())
	{
		//for (auto wds = wdsStarsIndex.lowerBound(objPrefixUpper); wds != wdsStarsIndex.end(); ++wds)
		QHashIterator<QString, StarId>wds(wdsStarsIndex);
		while (wds.hasNext())
		{
			wds.next();
			if (wds.key().startsWith(objPrefixUpper))
			{
				if (maxNbItem==0)
					break;
				result << getWdsDesignation(wds.value());
				--maxNbItem;
			}
			else
				break;
		}
	}

	// Add exact Gaia DR3 catalogue numbers.
	static const QRegularExpression gaiaRx("^\\s*(Gaia DR3)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	match=gaiaRx.match(objPrefixUpper);
	if (match.hasMatch())
	{
		bool ok;
		StarId gaiaNum = match.captured(2).toLongLong(&ok);
		if (ok)
		{
			StelObjectP s = searchGaia(gaiaNum);
			if (s && maxNbItem>0)
			{
				result << QString("Gaia DR3 %1").arg(gaiaNum);
				maxNbItem--;
			}
		}
	}

	result.sort();	
	return result;
}

// Set display flag for Stars.
void StarMgr::setFlagStars(bool b)
{
	starsFader=b;
	StelApp::immediateSave("astro/flag_stars", b);
	emit starsDisplayedChanged(b);
}
// Set display flag for Star names (labels).
void StarMgr::setFlagLabels(bool b) {labelsFader=b; StelApp::immediateSave("astro/flag_star_name", b); emit starLabelsDisplayedChanged(b);}
// Set the amount of star labels. The real amount is also proportional with FOV.
// The limit is set in function of the stars magnitude
// @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
void StarMgr::setLabelsAmount(double a)
{
	if(!qFuzzyCompare(a,labelsAmount))
	{
		labelsAmount=a;
		StelApp::immediateSave("stars/labels_amount", a);
		emit labelsAmountChanged(a);
	}
}

// Define font file name and size to use for star names display
void StarMgr::setFontSize(int newFontSize)
{
	starFont.setPixelSize(newFontSize);
}

// Set flag for usage designations of stars for their labels instead common names.
void StarMgr::setDesignationUsage(const bool flag)
{
	if(flagDesignations!=flag)
	{
		flagDesignations=flag;
		StelApp::immediateSave("astro/flag_star_designation_usage", flag);
		emit designationUsageChanged(flag);
	}
}
// Set flag for usage traditional designations of double stars.
void StarMgr::setFlagDblStarsDesignation(const bool flag)
{
	if(flagDblStarsDesignation!=flag)
	{
		flagDblStarsDesignation=flag;
		StelApp::immediateSave("astro/flag_star_designation_dbl", flag);
		emit flagDblStarsDesignationChanged(flag);
	}
}
// Set flag for usage designations of variable stars.
void StarMgr::setFlagVarStarsDesignation(const bool flag)
{
	if(flagVarStarsDesignation!=flag)
	{
		flagVarStarsDesignation=flag;
		StelApp::immediateSave("astro/flag_star_designation_var", flag);
		emit flagVarStarsDesignationChanged(flag);
	}
}
// Set flag for usage Hipparcos catalog designations of stars.
void StarMgr::setFlagHIPDesignation(const bool flag)
{
	if(flagHIPDesignation!=flag)
	{
		flagHIPDesignation=flag;
		StelApp::immediateSave("astro/flag_star_designation_hip", flag);
		emit flagHIPDesignationChanged(flag);
	}
}
// Show additional star names.
void StarMgr::setFlagAdditionalNames(bool flag)
{
	if (flagAdditionalStarNames!=flag)
	{
		flagAdditionalStarNames=flag;
		StelApp::immediateSave("astro/flag_star_additional_names", flag);
		emit flagAdditionalNamesDisplayedChanged(flag);
	}
}

void StarMgr::updateSkyCulture(const StelSkyCulture& skyCulture)
{
	const QString fic = StelFileMgr::findFile("skycultures/common_star_names.fab");
	CommonNames commonNames;
	if (fic.isEmpty())
		qWarning() << "Could not load common_star_names.fab";
	else
		commonNames = loadCommonNames(fic);

	QMap<QString, int> commonNamesIndexToSearchWhileLoading = commonNames.hipByName;
	commonNamesMap.clear();
	commonNamesMapI18n.clear();
	//additionalNamesMap.clear();
	//additionalNamesMapI18n.clear();
	commonNamesUppercaseIndexI18n.clear();
	commonNamesUppercaseIndex.clear();
	//additionalNamesIndex.clear();
	//additionalNamesIndexI18n.clear();
	culturalNamesMap.clear();
	culturalNamesIndex.clear();

	static QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	QString exclude=conf->value("SCExcludeReferences/"+skyCulture.id, QString()).toString();
	QSet<int>excludeRefs;
	if (!exclude.isEmpty())
	{
#if  (QT_VERSION<QT_VERSION_CHECK(5,14,0))
		const QStringList excludeRefStrings = exclude.split(",", QString::SkipEmptyParts);
#else
		const QStringList excludeRefStrings = exclude.split(",", Qt::SkipEmptyParts);
#endif
		//qInfo() << "Skyculture" << skyCulture.id << "configured to exclude references" << excludeRefStrings;
		for (const QString &s: excludeRefStrings)
		{
			excludeRefs.insert(s.toInt());
		}
		qInfo() << "Skyculture" << skyCulture.id << "configured to exclude references" << excludeRefs;
	}

	if (!skyCulture.names.isEmpty())
		loadCultureSpecificNames(skyCulture.names, commonNamesIndexToSearchWhileLoading, excludeRefs);

	if (skyCulture.fallbackToInternationalNames)
	{
		for (auto it = commonNames.hipByName.begin(); it != commonNames.hipByName.end(); ++it)
		{
			const StarId HIP = it.value();
			const QString& englishName = commonNames.byHIP[HIP];
			const QString englishNameUpper = englishName.toUpper();
			if (!commonNamesMap.contains(HIP))
			{
				commonNamesMap[HIP] = englishName;
				commonNamesMapI18n[HIP] = englishName;
				commonNamesUppercaseIndexI18n[englishNameUpper] = HIP;
				commonNamesUppercaseIndex[englishNameUpper] = HIP;
			}
			// TODO: GZ: Not sure why this filled additionalNames with the actual commonNames?
			//else if (!additionalNamesMap.contains(HIP))
			//{
			//	additionalNamesMap[HIP] = englishName;
			//	additionalNamesMapI18n[HIP] = englishName;
			//	additionalNamesIndex[englishNameCap] = HIP;
			//	additionalNamesIndexI18n[englishNameCap] = HIP;
			//}
			//else
			//{
			//	const auto newName = additionalNamesMap[HIP] + (" - " + englishName);
			//	additionalNamesMap[HIP] = newName;
			//	additionalNamesMapI18n[HIP] = newName;
			//	additionalNamesIndex[englishNameCap] = HIP;
			//	additionalNamesIndexI18n[englishNameCap] = HIP;
			//}
		}
	}

	// Turn on sci names/catalog names for modern cultures only
	setFlagSciNames(skyCulture.englishName.contains("modern", Qt::CaseInsensitive));
	updateI18n();
}

void StarMgr::increaseStarsMagnitudeLimit()
{
	static StelCore* core = StelApp::getInstance().getCore();
	core->getSkyDrawer()->setCustomStarMagnitudeLimit(core->getSkyDrawer()->getCustomStarMagnitudeLimit() + 0.1);
}

void StarMgr::reduceStarsMagnitudeLimit()
{
	static StelCore* core = StelApp::getInstance().getCore();
	core->getSkyDrawer()->setCustomStarMagnitudeLimit(core->getSkyDrawer()->getCustomStarMagnitudeLimit() - 0.1);
}

void StarMgr::populateStarsDesignations()
{
	QString filePath;
	filePath = StelFileMgr::findFile("stars/hip_gaia3/name.fab");
	if (filePath.isEmpty())
		qWarning() << "Could not load scientific star names file: stars/hip_gaia3/name.fab";
	else
		loadSciDesignations(filePath, sciDesignationsMap, sciDesignationsIndex);

	filePath = StelFileMgr::findFile("stars/hip_gaia3/extra_name.fab");
	if (filePath.isEmpty())
		qWarning() << "Could not load scientific star extra names file: stars/hip_gaia3/extra_name.fab";
	else
		loadSciDesignations(filePath, sciExtraDesignationsMap, sciExtraDesignationsIndex);

	filePath = StelFileMgr::findFile("stars/hip_gaia3/gcvs.cat");
	if (filePath.isEmpty())
		qWarning() << "Could not load variable stars file: stars/hip_gaia3/gcvs.cat";
	else
		loadGcvs(filePath);

	filePath = StelFileMgr::findFile("stars/hip_gaia3/wds_hip_part.dat");
	if (filePath.isEmpty())
		qWarning() << "Could not load double stars file: stars/hip_gaia3/wds_hip_part.dat";
	else
		loadWds(filePath);

	filePath = StelFileMgr::findFile("stars/hip_gaia3/cross-id.cat");
	if (filePath.isEmpty())
		qWarning() << "Could not load cross-identification data file: stars/hip_gaia3/cross-id.cat";
	else
		loadCrossIdentificationData(filePath);
	
	filePath = StelFileMgr::findFile("stars/hip_gaia3/binary_orbitparam.dat");
	if (filePath.isEmpty())
		qWarning() << "Could not load binary orbital parameters data file: stars/hip_gaia3/binary_orbitparam.dat";
	else
		loadBinaryOrbitalData(filePath);
}

QStringList StarMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		//QHashIterator<QString, StarId> i(commonNamesUppercaseIndex);
		//while (i.hasNext())
		//{
		//	i.next();
		//	result << getCommonEnglishName(i.value());
		//}
		// TBD: This should do the same, right?
		//result=commonNamesUppercaseIndex.keys(); // No, it's uppercase
		result = commonNamesMap.values(); // probably yes!

		QMultiHashIterator<StarId, StelObject::CulturalName> ci(culturalNamesMap);
		while (ci.hasNext())
		{
			ci.next();
			result << ci.value().native;
			result << ci.value().translated;
			result << ci.value().pronounce;
			result << ci.value().transliteration;
		}
	}
	else
	{
		//QMapIterator<QString, StarId> i(commonNamesUppercaseIndexI18n);
		//while (i.hasNext())
		//{
		//	i.next();
		//	result << getCommonName(i.value());
		//}
		// TBD: This should do the same, right?
		//result=commonNamesUppercaseIndexI18n.keys(); // No, it's uppercase
		result=commonNamesMapI18n.values(); // probably yes!

		QMultiHashIterator<StarId, StelObject::CulturalName> ci(culturalNamesMap);
		while (ci.hasNext())
		{
			ci.next();
			result << ci.value().native;
			result << ci.value().translatedI18n;
			result << ci.value().pronounceI18n;
			result << ci.value().transliteration;
		}
	}
	result.removeDuplicates();
	result.removeAll(QString(""));
	result.removeAll(QString());
	return result;
}

QStringList StarMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;	
	// type 1
	bool isStarT1 = false;
	QList<StelObjectP> starsT1;
	// type 2
	bool isStarT2 = false;
	QList<StelACStarData> starsT2;
	int type = objType.toInt();	
	switch (type)
	{
		case 0: // Interesting double stars
		{
			static const QStringList doubleStars = {
				"21 Tau", "27 Tau", "77 Tau", "δ1 Tau", "V1016 Ori",
				"42 Ori", "ι Ori", "ζ Crv", "ζ UMa", "α2 Lib", "α1 Cru",
				"ω1 Sco", "λ Sco", "μ1 Sco", "ζ1 Sco", "ε1 Lyr", "ε2 Lyr",
				"δ1 Lyr", 	"ν1 Sgr", "ο1 Cyg", "ο2 Cyg", "α2 Cap", "β1 Cyg",
				"β Ori", "γ1 And", "ξ Boo", "α1 Her", "T Dra", "ν1 Dra",
				"70 Oph", "α Gem", "ζ Her", "ο2 Eri", "γ1 Ari", "γ Vir",
				"γ1 Leo", "β Mon", "ε Boo", "44 Boo", "β1 Sco", "ζ1 Cnc",
				"φ2 Cnc", "α Leo", "α2 CVn", "ι Cas", "ε Ari", "κ Vel", "γ1 Del",
				"61 Cyg B", "55 Aqr", "σ Cas", "η Cas", "α UMi", "36 Oph",
				"α1 Cen",  "65 UMa", "σ2 UMa", "55 Cnc", "16 Cyg A",
				"HIP 28393", "HIP 84709"};
			result = doubleStars;
			break;
		}
		case 1: // Interesting variable stars
		{
			static const QStringList variableStars = {
				"δ Cep", "β Per", "ο Cet", "λ Tau", "β Lyr", "ζ Gem", "μ Cep",
				"α1 Her", "η Gem", "η Aql", "γ Cas", "α Ori", "R And",
				"U Ant", "θ Aps", "R Aql", "V Aql", "R Aqr", "ε Aur", "R Aur",
				"AE Aur", "W Boo", "VZ Cam", "l Car", "WZ Cas",	"S Cen",
				"α Cen C", "T Cep", "U Cep", "R CMa", "VY CMa",
				"S Cnc", "α CrB", "R CrB", "T CrB", "U CrB", "R Cru",
				"SU Cyg", "EU Del", "β Dor", "R Gem", "30 Her", "68 Her",
				"R Hor", "R Lep", "R Leo", "RR Lyr", "U Mon", "R Hya", "χ Cyg",
				"δ Ori", "VV Ori", "κ Pav", "β Peg", "ε Peg", "ζ Phe", "R Sct",
				"U Sgr", "RY Sgr", "W UMa", "α UMi"};
			result = variableStars;
			break;
		}
		case 2: // Bright double stars
		{
			starsT2 = doubleHipStars;
			isStarT2 = true;
			break;
		}
		case 3: // Bright variable stars
		{
			starsT2 = variableHipStars;
			isStarT2 = true;
			break;
		}
		case 4:
		{
			starsT2 = hipStarsHighPM;
			isStarT2 = true;
			break;
		}
		case 5: // Variable stars: Algol-type eclipsing systems
		{
			starsT2 = algolTypeStars;
			isStarT2 = true;
			break;
		}
		case 6: // Variable stars: the classical cepheids
		{
			starsT2 = classicalCepheidsTypeStars;
			isStarT2 = true;
			break;
		}
		case 7: // Bright carbon stars
		{
			starsT1 = carbonStars;
			isStarT1 = true;
			break;
		}
		case 8: // Bright barium stars
		{
			starsT1 = bariumStars;
			isStarT1 = true;
			break;
		}
		default:
		{
			// No stars yet?
			break;
		}
	}

	QString starName;
	if (isStarT1)
	{
		for (const auto& star : std::as_const(starsT1))
		{
			starName = inEnglish ? star->getEnglishName() : star->getNameI18n();
			if (!starName.isEmpty())
				result << starName;
			else
				result << star->getID();
		}
	}

	if (isStarT2)
	{
		for (const auto& star : std::as_const(starsT2))
		{
			starName = inEnglish ? star.first->getEnglishName() : star.first->getNameI18n();
			if (!starName.isEmpty())
				result << starName;
			else
				result << star.first->getID();
		}
	}

	result.removeDuplicates();
	return result;
}

QString StarMgr::getStelObjectType() const
{
	return STAR_TYPE;
}

//! cultural names
//! Return screen label (to be used in the sky display. Most users will use some short label)
QString StarMgr::getCulturalScreenLabel(StarId hip)
{
	QStringList list=getCultureLabels(hip, GETSTELMODULE(StelSkyCultureMgr)->getScreenLabelStyle());
	//qDebug() << "culturalScreenLabel: " << list;
	return list.isEmpty() ? "" : list.constFirst();
}

//! Return InfoString label (to be used in the InfoString).
//! When dealing with foreign skycultures, many users will want this to be longer, with more name components.
QString StarMgr::getCulturalInfoLabel(StarId hip)
{
	QStringList list=getCultureLabels(hip, GETSTELMODULE(StelSkyCultureMgr)->getInfoLabelStyle());
	return list.isEmpty() ? "" : list.join(" - ");
}

QStringList StarMgr::getCultureLabels(StarId hip, StelObject::CulturalDisplayStyle style)
{
	// Retrieve list in order as read from JSON
	QList<StelObject::CulturalName>culturalNames=getCulturalNames(hip);
	if (culturalNames.isEmpty())
	{
		return QStringList();
	}

	QStringList labels;
	for (auto &cName: culturalNames)
		{
			QString label=StelSkyCultureMgr::createCulturalLabel(cName, style, getCommonNameI18n(hip));
			labels << label;
		}
	labels.removeDuplicates();
	labels.removeAll(QString(""));
	labels.removeAll(QString());
	return labels;
}


