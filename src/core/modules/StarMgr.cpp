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

static QStringList spectral_array;
static QStringList component_array;
static QStringList objtype_array;

// This number must be incremented each time the content or file format of the stars catalogs change
// It can also be incremented when the defaultStarsConfig.json file change.
// It should always match the version field of the defaultStarsConfig.json file
static const int StarCatalogFormatVersion = 22;

// Initialise statics
bool StarMgr::flagSciNames = true;
bool StarMgr::flagAdditionalStarNames = true;
bool StarMgr::flagDesignations = false;
bool StarMgr::flagDblStarsDesignation = false;
bool StarMgr::flagVarStarsDesignation = false;
bool StarMgr::flagHIPDesignation = false;
QHash<StarId,QString> StarMgr::commonNamesMap;
QHash<StarId,QString> StarMgr::commonNamesMapI18n;
QHash<StarId,QString> StarMgr::additionalNamesMap;
QHash<StarId,QString> StarMgr::additionalNamesMapI18n;
QMap<QString,StarId> StarMgr::commonNamesIndexI18n;
QMap<QString,StarId> StarMgr::commonNamesIndex;
QMap<QString,StarId> StarMgr::additionalNamesIndex;
QMap<QString,StarId> StarMgr::additionalNamesIndexI18n;
QHash<StarId,QString> StarMgr::sciDesignationsMapI18n;
QMap<QString,StarId> StarMgr::sciDesignationsIndexI18n;
QHash<StarId,QString> StarMgr::sciExtraDesignationsMapI18n;
QMap<QString,StarId> StarMgr::sciExtraDesignationsIndexI18n;
QHash<StarId, varstar> StarMgr::varStarsMapI18n;
QMap<QString, StarId> StarMgr::varStarsIndexI18n;
QHash<StarId, wds> StarMgr::wdsStarsMapI18n;
QMap<QString, StarId> StarMgr::wdsStarsIndexI18n;
QMap<QString, crossid> StarMgr::crossIdMap;
QMap<int, StarId> StarMgr::saoStarsIndex;
QMap<int, StarId> StarMgr::hdStarsIndex;
QMap<int, StarId> StarMgr::hrStarsIndex;
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

QString StarMgr::convertToOjectTypes(int index)
{
	if (index < 0 || index >= objtype_array.size())
	{
		qDebug() << "convertToObjTypeIds: bad index: " << index << ", max: " << objtype_array.size();
		return "";
	}
	return objtype_array.at(index).trimmed();
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
QString StarMgr::getCommonName(StarId hip)
{
	ConstellationMgr* cmgr=GETSTELMODULE(ConstellationMgr);
	if (cmgr->getConstellationDisplayStyle() == ConstellationMgr::constellationsNative)
		return getCommonEnglishName(hip);

	auto it = commonNamesMapI18n.find(hip);
	if (it!=commonNamesMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getAdditionalNames(StarId hip)
{
	auto it = additionalNamesMapI18n.find(hip);
	if (it!=additionalNamesMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getAdditionalEnglishNames(StarId hip)
{
	auto it = additionalNamesMap.find(hip);
	if (it!=additionalNamesMap.end())
		return it.value();
	return QString();
}

QString StarMgr::getCommonEnglishName(StarId hip)
{
	auto it = commonNamesMap.find(hip);
	if (it!=commonNamesMap.end())
		return it.value();
	return QString();
}


QString StarMgr::getSciName(StarId hip)
{
	auto it = sciDesignationsMapI18n.find(hip);
	if (it != sciDesignationsMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getSciExtraName(StarId hip)
{
	auto it = sciExtraDesignationsMapI18n.find(hip);
	if (it!=sciExtraDesignationsMapI18n.end())
		return it.value();
	return QString();
}

QString StarMgr::getCrossIdentificationDesignations(QString hip)
{
	QStringList designations;
	auto cr = crossIdMap.find(hip);
	if (cr==crossIdMap.end() && hip.right(1).toUInt()==0)
		cr = crossIdMap.find(hip.left(hip.size()-1));

	if (cr!=crossIdMap.end())
	{
		crossid crossIdData = cr.value();
		if (crossIdData.hr>0)
			designations << QString("HR %1").arg(crossIdData.hr);

		if (crossIdData.hd>0)
			designations << QString("HD %1").arg(crossIdData.hd);

		if (crossIdData.sao>0)
			designations << QString("SAO %1").arg(crossIdData.sao);
	}

	return designations.join(" - ");
}

QString StarMgr::getWdsName(StarId hip)
{
	auto it = wdsStarsMapI18n.find(hip);
	if (it!=wdsStarsMapI18n.end())
		return QString("WDS J%1").arg(it.value().designation);
	return QString();
}

int StarMgr::getWdsLastObservation(StarId hip)
{
	auto it = wdsStarsMapI18n.find(hip);
	if (it!=wdsStarsMapI18n.end())
		return it.value().observation;
	return 0;
}

float StarMgr::getWdsLastPositionAngle(StarId hip)
{
	auto it = wdsStarsMapI18n.find(hip);
	if (it!=wdsStarsMapI18n.end())
		return it.value().positionAngle;
	return 0;
}

float StarMgr::getWdsLastSeparation(StarId hip)
{
	auto it = wdsStarsMapI18n.find(hip);
	if (it!=wdsStarsMapI18n.end())
		return it.value().separation;
	return 0.f;
}

QString StarMgr::getGcvsName(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().designation;
	return QString();
}

QString StarMgr::getGcvsVariabilityType(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().vtype;
	return QString();
}

float StarMgr::getGcvsMaxMagnitude(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().maxmag;
	return -99.f;
}

int StarMgr::getGcvsMagnitudeFlag(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().mflag;
	return 0;
}


float StarMgr::getGcvsMinMagnitude(StarId hip, bool firstMinimumFlag)
{
	auto it = varStarsMapI18n.find(hip);
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

QString StarMgr::getGcvsPhotometricSystem(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().photosys;
	return QString();
}

double StarMgr::getGcvsEpoch(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().epoch;
	return -99.;
}

double StarMgr::getGcvsPeriod(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().period;
	return -99.;
}

int StarMgr::getGcvsMM(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().Mm;
	return -99;
}

QString StarMgr::getGcvsSpectralType(StarId hip)
{
	auto it = varStarsMapI18n.find(hip);
	if (it!=varStarsMapI18n.end())
		return it.value().stype;
	return QString();
}

binaryorbitstar StarMgr::getBinaryOrbitData(StarId hip)
{
	auto it = binaryOrbitStarMap.find(hip);
	if (it!=binaryOrbitStarMap.end())
		return it.value();
	return binaryorbitstar();
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

bool StarMgr::checkAndLoadCatalog(const QVariantMap& catDesc)
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
		qWarning().noquote() << "Found file" << QDir::toNativeSeparators(catalogFilePath) << ", checking md5sum...";

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
			qWarning() << "MD5 sum correct!";
			setCheckFlag(catDesc.value("id").toString(), true);
		}
	}

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
	foreach (const QVariant& catV, catalogsDescription)
	{
		QVariantMap m = catV.toMap();
		checkAndLoadCatalog(m);
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
			spectral_array = initStringListFromFile(tmpFic);
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
			objtype_array = initStringListFromFile(tmpFic);
	}

	// create an array with the first element being an empty string, and then contain strings A,B,C,...,Z
	component_array.append("");
	for (int i=0; i<26; i++)
		component_array.append(QString(QChar('A'+i)));

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
				QMap<StelObjectP, float> sa;
				sa[so] = static_cast<float>(getGcvsPeriod(s->getHip()));
				variableHipStars.push_back(sa);
				
				auto vartype = getGcvsVariabilityType(s->getHip());
				if (vartype.contains("EA"))
				{
					QMap<StelObjectP, float> sal;
					sal[so] = sa[so];
					algolTypeStars.push_back(sal);
				}
				if (vartype.contains("DCEP") && !vartype.contains("DCEPS"))
				{
					QMap<StelObjectP, float> sacc;
					sacc[so] = sa[so];
					classicalCepheidsTypeStars.push_back(sacc);
				}
			}
			if (!getWdsName(s->getHip()).isEmpty())
			{
				QMap<StelObjectP, float> sd;
				sd[so] = getWdsLastSeparation(s->getHip());
				doubleHipStars.push_back(sd);
			}
			float pm = s->getPMTotal();
			if (qAbs(pm)>=pmLimit)
			{
				QMap<StelObjectP, float> spm;
				spm[so] = pm / 1000.f;  // in arc-seconds per year
				hipStarsHighPM.push_back(spm);
			}
		}
	}
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
				if (referenceMap.find(hip)!=referenceMap.end())
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

void StarMgr::loadCultureSpecificNameForNamedObject(const QJsonArray& data, const QString& commonName,
                                                    const QMap<QString, int>& commonNamesIndexToSearchWhileLoading)
{
	const auto commonNameIndexIt = commonNamesIndexToSearchWhileLoading.find(commonName.toUpper());
	if (commonNameIndexIt == commonNamesIndexToSearchWhileLoading.end())
	{
		// This may actually not even be a star, so we shouldn't emit any warning, just return
		return;
	}
	const int HIP = commonNameIndexIt.value();

	for (const auto& entry : data)
	{
		for (const char*const nameType : {"english", "native"})
		{
			const auto specificName = entry.toObject()[nameType].toString();
			if (specificName.isEmpty())
				continue;

			const auto specificNameCap = specificName.toUpper();
			if (additionalNamesMap.find(HIP) != additionalNamesMap.end())
			{
				const auto newName = additionalNamesMap[HIP].prepend(specificName + " - ");
				additionalNamesMap[HIP] = newName;
				additionalNamesMapI18n[HIP] = newName;
				additionalNamesIndex[specificNameCap] = HIP;
				additionalNamesIndexI18n[specificNameCap] = HIP;
			}
			else
			{
				additionalNamesMap[HIP] = specificName;
				additionalNamesMapI18n[HIP] = specificName;
				additionalNamesIndex[specificNameCap] = HIP;
				additionalNamesIndexI18n[specificNameCap] = HIP;
			}
		}
	}
}

void StarMgr::loadCultureSpecificNameForHIP(const QJsonArray& data, const int HIP)
{
	for (const auto& entry : data)
	{
		for (const char*const nameType : {"english", "native"})
		{
			const auto specificName = entry.toObject()[nameType].toString();
			if (specificName.isEmpty())
				continue;

			const auto englishNameCap = specificName.toUpper();
			if (commonNamesMap.find(HIP) != commonNamesMap.end())
			{
				if (additionalNamesMap.find(HIP) != additionalNamesMap.end())
				{
					const auto newName = additionalNamesMap[HIP].prepend(specificName + " - ");
					additionalNamesMap[HIP] = newName;
					additionalNamesMapI18n[HIP] = newName;
					additionalNamesIndex[englishNameCap] = HIP;
					additionalNamesIndexI18n[englishNameCap] = HIP;
				}
				else
				{
					additionalNamesMap[HIP] = specificName;
					additionalNamesMapI18n[HIP] = specificName;
					additionalNamesIndex[englishNameCap] = HIP;
					additionalNamesIndexI18n[englishNameCap] = HIP;
				}
			}
			else
			{
				commonNamesMap[HIP] = specificName;
				commonNamesMapI18n[HIP] = specificName;
				commonNamesIndexI18n[englishNameCap] = HIP;
				commonNamesIndex[englishNameCap] = HIP;
			}
		}
	}
}

void StarMgr::loadCultureSpecificNames(const QJsonObject& data, const QMap<QString, int>& commonNamesIndexToSearchWhileLoading)
{
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		const auto key = it.key();
		if (key.startsWith("HIP "))
		{
			loadCultureSpecificNameForHIP(it.value().toArray(), key.mid(4).toInt());
		}
		else if (key.startsWith("NAME "))
		{
			loadCultureSpecificNameForNamedObject(it.value().toArray(), key.mid(5), commonNamesIndexToSearchWhileLoading);
		}
	}
}

// Load scientific names from file
void StarMgr::loadSciNames(const QString& sciNameFile, const bool extraData)
{
	if (extraData)
	{
		sciExtraDesignationsMapI18n.clear();
		sciExtraDesignationsIndexI18n.clear();
		qInfo().noquote() << "Loading scientific star extra names from" << QDir::toNativeSeparators(sciNameFile);
	}
	else
	{
		sciDesignationsMapI18n.clear();
		sciDesignationsIndexI18n.clear();
		qInfo().noquote() << "Loading scientific star names from" << QDir::toNativeSeparators(sciNameFile);
	}

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
	for (const auto& record : allRecords)
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
					     << " - record does not match record pattern";
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
			if (extraData)
			{
				// Don't set the main sci name if it's already set - it's additional sci name
				if (sciExtraDesignationsMapI18n.find(hip)!=sciExtraDesignationsMapI18n.end())
				{
					QString sname = sciExtraDesignationsMapI18n[hip].append(" - " + sci_name);
					sciExtraDesignationsMapI18n[hip] = sname;
				}
				else
					sciExtraDesignationsMapI18n[hip] = sci_name;
				sciExtraDesignationsIndexI18n[sci_name] = hip;
			}
			else
			{
				// Don't set the main sci name if it's already set - it's additional sci name
				if (sciDesignationsMapI18n.find(hip)!=sciDesignationsMapI18n.end())
				{
					QString sname = sciDesignationsMapI18n[hip].append(" - " + sci_name);
					sciDesignationsMapI18n[hip] = sname;
				}
				else
					sciDesignationsMapI18n[hip] = sci_name;
				sciDesignationsIndexI18n[sci_name] = hip;
			}
			++readOk;
		}
	}

	if (extraData)
		qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "scientific star extra names";
	else
		qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "scientific star names";
}

// Load GCVS from file
void StarMgr::loadGcvs(const QString& GcvsFile)
{
	varStarsMapI18n.clear();
	varStarsIndexI18n.clear();

	qInfo().noquote() << "Loading variable stars data from" << QDir::toNativeSeparators(GcvsFile);

	QFile vsFile(GcvsFile);
	if (!vsFile.open(QIODevice::ReadOnly))
	{
		qDebug().noquote() << "Cannot open file" << QDir::toNativeSeparators(GcvsFile);
		return;
	}
	QByteArray data = StelUtils::uncompress(vsFile);
	vsFile.close();
	//check if decompressing was successful
	if(data.isEmpty())
	{
		qDebug().noquote() << "Could not decompress file" << QDir::toNativeSeparators(GcvsFile);
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
		if (varStarsMapI18n.find(hip)!=varStarsMapI18n.end())
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

		varStarsMapI18n[hip] = variableStar;
		varStarsIndexI18n[variableStar.designation.toUpper()] = hip;
		++readOk;
	}

	buf.close();
	qInfo().noquote() << "Loaded" << readOk << "/" << totalRecords << "variable stars";
}

// Load WDS from file
void StarMgr::loadWds(const QString& WdsFile)
{
	wdsStarsMapI18n.clear();
	wdsStarsIndexI18n.clear();

	qInfo().noquote() << "Loading double stars from" << QDir::toNativeSeparators(WdsFile);
	QFile dsFile(WdsFile);
	if (!dsFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "Could not open" << QDir::toNativeSeparators(WdsFile);
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
			qWarning() << "Parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(WdsFile)
				   << " - failed to convert " << fields.at(0) << "to a number";
			continue;
		}

		// Don't set the star if it's already set
		if (wdsStarsMapI18n.find(hip)!=wdsStarsMapI18n.end())
			continue;

		wds doubleStar;

		doubleStar.designation = fields.at(1).trimmed();
		doubleStar.observation = fields.at(2).toInt();
		doubleStar.positionAngle = fields.at(3).toFloat();
		doubleStar.separation = fields.at(4).toFloat();

		wdsStarsMapI18n[hip] = doubleStar;
		wdsStarsIndexI18n[QString("WDS J%1").arg(doubleStar.designation.toUpper())] = hip;
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
			bool ok;
			orbitalData.binary_period = fields.at(2).toDouble(&ok);
			orbitalData.eccentricity = fields.at(3).toFloat(&ok);
			orbitalData.inclination = fields.at(4).toFloat(&ok);
			orbitalData.big_omega = fields.at(5).toFloat(&ok);
			orbitalData.small_omega = fields.at(6).toFloat(&ok);
			orbitalData.periastron_epoch = fields.at(7).toDouble(&ok);
			orbitalData.semi_major = fields.at(8).toDouble(&ok);
			orbitalData.bary_distance = fields.at(9).toDouble(&ok);
			orbitalData.data_epoch = fields.at(10).toDouble(&ok);
			orbitalData.bary_ra = fields.at(11).toDouble(&ok);
			orbitalData.bary_dec = fields.at(12).toDouble(&ok);
			orbitalData.bary_rv = fields.at(13).toDouble(&ok);
			orbitalData.bary_pmra = fields.at(14).toDouble(&ok);
			orbitalData.bary_pmdec = fields.at(15).toDouble(&ok);
			orbitalData.primary_mass = fields.at(16).toFloat(&ok);
			orbitalData.secondary_mass = fields.at(17).toFloat(&ok);
			// do primary star
			hip = fields.at(0).toLongLong(&ok);
			orbitalData.hip = hip;
			orbitalData.primary = true;
			binaryOrbitStarMap[hip] = orbitalData;
			// do secondary star
			hip = fields.at(1).toLongLong(&ok);
			orbitalData.hip = hip;
			orbitalData.primary = false;
			binaryOrbitStarMap[hip] = orbitalData;

			if (!ok)
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
	QVector<SphericalCap> viewportCaps = prj->getViewportConvexPolygon()->getBoundingSphericalCaps();
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
	commonNamesIndexI18n.clear();
	additionalNamesMapI18n.clear();
	additionalNamesIndexI18n.clear();
	for (QHash<StarId,QString>::ConstIterator it(commonNamesMap.constBegin());it!=commonNamesMap.constEnd();it++)
	{
		const StarId i = it.key();
		const QString t(trans.qTranslateStar(it.value()));
		commonNamesMapI18n[i] = t;
		commonNamesIndexI18n[t.toUpper()] = i;
	}
	for (QHash<StarId,QString>::ConstIterator ita(additionalNamesMap.constBegin());ita!=additionalNamesMap.constEnd();ita++)
	{
		const StarId i = ita.key();
		QStringList a = ita.value().split(" - ");
		QStringList tn;
		for (const auto& str : a)
		{
			QString tns = trans.qTranslateStar(str);
			tn << tns;
			additionalNamesIndexI18n[tns.toUpper()] = i;
		}
		const QString r = tn.join(" - ");
		additionalNamesMapI18n[i] = r;
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
	QString objw = nameI18n.toUpper();

	// Search by I18n common name
	auto it = commonNamesIndexI18n.find(objw);
	if (it!=commonNamesIndexI18n.end())
		return searchHP(it.value());

	if (getFlagAdditionalNames())
	{
		// Search by I18n additional common names
		auto ita = additionalNamesIndexI18n.find(objw);
		if (ita!=additionalNamesIndexI18n.end())
			return searchHP(ita.value());
	}

	return searchByName(nameI18n);
}


StelObjectP StarMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	StarId sid;

	// Search by HP number if it's an HP formatted number. The final part (A/B/...) is ignored
	static const QRegularExpression rx("^\\s*(HP|HIP)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch match=rx.match(objw);
	if (match.hasMatch())
		return searchHP(match.captured(2).toInt());

	// Search by SAO number if it's an SAO formatted number
	static const QRegularExpression rx2("^\\s*(SAO)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=rx2.match(objw);
	if (match.hasMatch())
	{
		auto sao = saoStarsIndex.find(match.captured(2).toInt());
		if (sao!=saoStarsIndex.end())
		{
			sid = sao.value();
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by HD number if it's an HD formatted number
	static const QRegularExpression rx3("^\\s*(HD)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=rx3.match(objw);
	if (match.hasMatch())
	{
		auto hd = hdStarsIndex.find(match.captured(2).toInt());
		if (hd!=hdStarsIndex.end())
		{
			sid = hd.value();
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by HR number if it's an HR formatted number
	static const QRegularExpression rx4("^\\s*(HR)\\s*(\\d+)\\s*$", QRegularExpression::CaseInsensitiveOption);
	match=rx4.match(objw);
	if (match.hasMatch())
	{
		auto hr = hrStarsIndex.find(match.captured(2).toInt());
		if (hr!=hrStarsIndex.end())
		{
			sid = hr.value();
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by Gaia number if it's an Gaia formatted number.
	static const QRegularExpression rx5("^\\s*(Gaia DR3)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	match=rx5.match(objw);
	if (match.hasMatch())
		return searchGaia(match.captured(2).toLongLong());

	// Search by English common name
	auto it = commonNamesIndex.find(objw);
	if (it!=commonNamesIndex.end())
	{
		sid = it.value();
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	if (getFlagAdditionalNames())
	{
		// Search by English additional common names
		auto ita = additionalNamesIndex.find(objw);
		if (ita!=additionalNamesIndex.end())
		{
			sid = ita.value();
			return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
		}
	}

	// Search by scientific name
	auto itd = sciDesignationsIndexI18n.find(name); // case sensitive!
	if (itd!=sciDesignationsIndexI18n.end())
	{
		sid = itd.value();
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}
	auto itdi = sciDesignationsIndexI18n.find(objw); // case insensitive!
	if (itdi!=sciDesignationsIndexI18n.end())
	{
		sid = itdi.value();
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	// Search by scientific name
	auto eitd = sciExtraDesignationsIndexI18n.find(name); // case sensitive!
	if (eitd!=sciExtraDesignationsIndexI18n.end())
	{
		sid = eitd.value();
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}
	auto eitdi = sciExtraDesignationsIndexI18n.find(objw); // case insensitive!
	if (eitdi!=sciExtraDesignationsIndexI18n.end())
	{
		sid = eitdi.value();
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	// Search by GCVS name
	auto it4 = varStarsIndexI18n.find(objw);
	if (it4!=varStarsIndexI18n.end())
	{
		sid = it4.value();
		return (sid <= NR_OF_HIP) ? searchHP(sid) : searchGaia(sid);
	}

	// Search by WDS name
	auto wdsIt = wdsStarsIndexI18n.find(objw);
	if (wdsIt!=wdsStarsIndexI18n.end())
	{
		sid = wdsIt.value();
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

	QString objw = objPrefix.toUpper();
	bool found;

	// Search for common names
	QMapIterator<QString, StarId> i(commonNamesIndexI18n);
	while (i.hasNext())
	{
		i.next();
		if (useStartOfWords && i.key().startsWith(objw))
			found = true;
		else if (!useStartOfWords && i.key().contains(objw))
			found = true;
		else
			found = false;

		if (found)
		{
			if (maxNbItem<=0)
				break;
			result.append(getCommonName(i.value()));
			--maxNbItem;
		}
	}

	QMapIterator<QString, StarId> j(commonNamesIndex);
	while (j.hasNext())
	{
		j.next();
		if (useStartOfWords && j.key().startsWith(objw))
			found = true;
		else if (!useStartOfWords && j.key().contains(objw))
			found = true;
		else
			found = false;

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
		QMapIterator<QString, StarId> k(additionalNamesIndexI18n);
		while (k.hasNext())
		{
			k.next();
			QStringList names = getAdditionalNames(k.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objw, Qt::CaseInsensitive))
					found = true;
				else if (!useStartOfWords && name.contains(objw, Qt::CaseInsensitive))
					found = true;
				else
					found = false;

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}

		QMapIterator<QString, StarId> l(additionalNamesIndex);
		while (l.hasNext())
		{
			l.next();
			QStringList names = getAdditionalEnglishNames(l.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objw, Qt::CaseInsensitive))
					found = true;
				else if (!useStartOfWords && name.contains(objw, Qt::CaseInsensitive))
					found = true;
				else
					found = false;

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
	}

	// Search for sci names
	// need special character escape because many stars have name starting with "*"
	QString bayerPattern = objPrefix;
	QRegularExpression bayerRegEx(QRegularExpression::escape(bayerPattern));
	QString bayerPatternCI = objw;
	QRegularExpression bayerRegExCI(QRegularExpression::escape(bayerPatternCI));

	// if the first character is a Greek letter, check if there's an index
	// after it, such as "alpha1 Cen".
	if (objPrefix.at(0).unicode() >= 0x0391 && objPrefix.at(0).unicode() <= 0x03A9)
		bayerRegEx.setPattern(bayerPattern.insert(1,"\\d?"));	
	if (objw.at(0).unicode() >= 0x0391 && objw.at(0).unicode() <= 0x03A9)
		bayerRegExCI.setPattern(bayerPatternCI.insert(1,"\\d?"));

	for (auto it = sciDesignationsIndexI18n.lowerBound(objPrefix); it != sciDesignationsIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegEx)==0 || it.key().indexOf(objPrefix)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciName(it.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else if (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive))
					found = true;
				else
					found = false;

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

	for (auto it = sciDesignationsIndexI18n.lowerBound(objw); it != sciDesignationsIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegExCI)==0 || it.key().indexOf(objw)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciName(it.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else if (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive))
					found = true;
				else
					found = false;

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
		else if (it.key().at(0) != objw.at(0))
			break;
	}

	for (auto it = sciExtraDesignationsIndexI18n.lowerBound(objPrefix); it != sciExtraDesignationsIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegEx)==0 || it.key().indexOf(objPrefix)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciExtraName(it.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else if (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive))
					found = true;
				else
					found = false;

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

	for (auto it = sciExtraDesignationsIndexI18n.lowerBound(objw); it != sciExtraDesignationsIndexI18n.end(); ++it)
	{
		if (it.key().indexOf(bayerRegExCI)==0 || it.key().indexOf(objw)==0)
		{
			if (maxNbItem<=0)
				break;
			QStringList names = getSciExtraName(it.value()).split(" - ");
			for (const auto &name : std::as_const(names))
			{
				if (useStartOfWords && name.startsWith(objPrefix, Qt::CaseInsensitive))
					found = true;
				else if (!useStartOfWords && name.contains(objPrefix, Qt::CaseInsensitive))
					found = true;
				else
					found = false;

				if (found)
				{
					if (maxNbItem<=0)
						break;
					result.append(name);
					--maxNbItem;
				}
			}
		}
		else if (it.key().at(0) != objw.at(0))
			break;
	}

	// Search for sci names for var stars
	for (auto it = varStarsIndexI18n.lowerBound(objw); it != varStarsIndexI18n.end(); ++it)
	{
		if (it.key().startsWith(objw))
		{
			if (maxNbItem<=0)
				break;
			result << getGcvsName(it.value());
			--maxNbItem;
		}
		else
			break;
	}

	StarId sid;
	// Add exact Hp catalogue numbers. The final part (A/B/...) is ignored
	static const QRegularExpression hpRx("^(HIP|HP)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch match=hpRx.match(objw);
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
	match=saoRx.match(objw);
	if (match.hasMatch())
	{
		int saoNum = match.captured(2).toInt();
		auto sao = saoStarsIndex.find(saoNum);
		if (sao!=saoStarsIndex.end())
		{
			sid = sao.value();
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
	match=hdRx.match(objw);
	if (match.hasMatch())
	{
		int hdNum = match.captured(2).toInt();
		auto hd = hdStarsIndex.find(hdNum);
		if (hd!=hdStarsIndex.end())
		{
			sid = hd.value();
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
	match=hrRx.match(objw);
	if (match.hasMatch())
	{
		int hrNum = match.captured(2).toInt();
		auto hr = hrStarsIndex.find(hrNum);
		if (hr!=hrStarsIndex.end())
		{
			sid = hr.value();
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
	if (wdsRx.match(objw).hasMatch())
	{
		for (auto wds = wdsStarsIndexI18n.lowerBound(objw); wds != wdsStarsIndexI18n.end(); ++wds)
		{
			if (wds.key().startsWith(objw))
			{
				if (maxNbItem==0)
					break;
				result << getWdsName(wds.value());
				--maxNbItem;
			}
			else
				break;
		}
	}

	// Add exact Gaia DR3 catalogue numbers.
	static const QRegularExpression gaiaRx("^\\s*(Gaia DR3)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
	match=gaiaRx.match(objw);
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
	additionalNamesMap.clear();
	additionalNamesMapI18n.clear();
	commonNamesIndexI18n.clear();
	commonNamesIndex.clear();
	additionalNamesIndex.clear();
	additionalNamesIndexI18n.clear();

	if (!skyCulture.names.isEmpty())
		loadCultureSpecificNames(skyCulture.names, commonNamesIndexToSearchWhileLoading);

	if (skyCulture.fallbackToInternationalNames)
	{
		for (auto it = commonNames.hipByName.begin(); it != commonNames.hipByName.end(); ++it)
		{
			const StarId HIP = it.value();
			const auto& englishName = commonNames.byHIP[HIP];
			const auto englishNameCap = englishName.toUpper();
			if (commonNamesMap.find(HIP) == commonNamesMap.end())
			{
				commonNamesMap[HIP] = englishName;
				commonNamesMapI18n[HIP] = englishName;
				commonNamesIndexI18n[englishNameCap] = HIP;
				commonNamesIndex[englishNameCap] = HIP;
			}
			else if (additionalNamesMap.find(HIP) == additionalNamesMap.end())
			{
				additionalNamesMap[HIP] = englishName;
				additionalNamesMapI18n[HIP] = englishName;
				additionalNamesIndex[englishNameCap] = HIP;
				additionalNamesIndexI18n[englishNameCap] = HIP;
			}
			else
			{
				const auto newName = additionalNamesMap[HIP] + (" - " + englishName);
				additionalNamesMap[HIP] = newName;
				additionalNamesMapI18n[HIP] = newName;
				additionalNamesIndex[englishNameCap] = HIP;
				additionalNamesIndexI18n[englishNameCap] = HIP;
			}
		}
	}

	// Turn on sci names/catalog names for modern cultures only
	setFlagSciNames(skyCulture.englishName.toLower().contains("modern", Qt::CaseInsensitive));
	updateI18n();
}

void StarMgr::increaseStarsMagnitudeLimit()
{
	StelCore* core = StelApp::getInstance().getCore();
	core->getSkyDrawer()->setCustomStarMagnitudeLimit(core->getSkyDrawer()->getCustomStarMagnitudeLimit() + 0.1);
}

void StarMgr::reduceStarsMagnitudeLimit()
{
	StelCore* core = StelApp::getInstance().getCore();
	core->getSkyDrawer()->setCustomStarMagnitudeLimit(core->getSkyDrawer()->getCustomStarMagnitudeLimit() - 0.1);
}

void StarMgr::populateStarsDesignations()
{
	QString fic;
	fic = StelFileMgr::findFile("stars/hip_gaia3/name.fab");
	if (fic.isEmpty())
		qWarning() << "Could not load scientific star names file: stars/hip_gaia3/name.fab";
	else
		loadSciNames(fic, false);

	fic = StelFileMgr::findFile("stars/hip_gaia3/extra_name.fab");
	if (fic.isEmpty())
		qWarning() << "Could not load scientific star extra names file: stars/hip_gaia3/extra_name.fab";
	else
		loadSciNames(fic, true);

	fic = StelFileMgr::findFile("stars/hip_gaia3/gcvs.cat");
	if (fic.isEmpty())
		qWarning() << "Could not load variable stars file: stars/hip_gaia3/gcvs.cat";
	else
		loadGcvs(fic);

	fic = StelFileMgr::findFile("stars/hip_gaia3/wds_hip_part.dat");
	if (fic.isEmpty())
		qWarning() << "Could not load double stars file: stars/hip_gaia3/wds_hip_part.dat";
	else
		loadWds(fic);

	fic = StelFileMgr::findFile("stars/hip_gaia3/cross-id.cat");
	if (fic.isEmpty())
		qWarning() << "Could not load cross-identification data file: stars/hip_gaia3/cross-id.cat";
	else
		loadCrossIdentificationData(fic);
	
	fic = StelFileMgr::findFile("stars/hip_gaia3/binary_orbitparam.dat");
	if (fic.isEmpty())
		qWarning() << "Could not load binary orbital parameters data file: stars/hip_gaia3/binary_orbitparam.dat";
	else
		loadBinaryOrbitalData(fic);
}

QStringList StarMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		QMapIterator<QString, StarId> i(commonNamesIndex);
		while (i.hasNext())
		{
			i.next();
			result << getCommonEnglishName(i.value());
		}
	}
	else
	{
		QMapIterator<QString, StarId> i(commonNamesIndexI18n);
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
	// type 1
	bool isStarT1 = false;
	QList<StelObjectP> starsT1;
	// type 2
	bool isStarT2 = false;
	QList<QMap<StelObjectP, float>> starsT2;
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
			starName = inEnglish ? star.firstKey()->getEnglishName() : star.firstKey()->getNameI18n();
			if (!starName.isEmpty())
				result << starName;
			else
				result << star.firstKey()->getID();
		}
	}

	result.removeDuplicates();
	return result;
}

QString StarMgr::getStelObjectType() const
{
	return STAR_TYPE;
}
