/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti
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

#include "SceneInfo.hpp"

#include "Scenery3d.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelLocationMgr.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QFileInfo>

Q_LOGGING_CATEGORY(sceneInfo,"stel.plugin.scenery3d.sceneinfo")
Q_LOGGING_CATEGORY(storedView,"stel.plugin.scenery3d.storedview")

const QString SceneInfo::SCENES_PATH("scenery3d/");
const QString StoredView::USERVIEWS_FILE = SceneInfo::SCENES_PATH + "userviews.ini";
QSettings* StoredView::userviews = nullptr;

int SceneInfo::metaTypeId = qRegisterMetaType<SceneInfo>();

bool SceneInfo::loadByID(const QString &id,SceneInfo& info)
{
	qCDebug(sceneInfo)<<"Loading scene info for id"<<id;
	QString file = StelFileMgr::findFile(SCENES_PATH + id + "/scenery3d.ini", StelFileMgr::File);
	if(file.isEmpty())
	{
		qCCritical(sceneInfo)<<"scenery3d.ini file with id "<<id<<" does not exist!";
		return false;
	}
	//get full directory path
	QString path = QFileInfo(file).absolutePath();
	qCDebug(sceneInfo)<<"Found scene in"<<path;

	QSettings ini(file,StelIniFormat);
	if (ini.status() != QSettings::NoError)
	{
	    qCCritical(sceneInfo) << "ERROR parsing scenery3d.ini file: " << file;
	    return false;
	}

	//load QSettings file
	info.id = id;
	info.fullPath = path;

	//primary description of the scene
	ini.beginGroup("model");
	info.name = ini.value("name").toString();
	info.author = ini.value("author").toString();
	info.description = ini.value("description","No description").toString();
	info.landscapeName = ini.value("landscape").toString();
	info.modelScenery = ini.value("scenery").toString();
	info.modelGround = ini.value("ground","").toString();
	info.vertexOrder = ini.value("obj_order","XYZ").toString();

	info.vertexOrderEnum = StelOBJ::XYZ;
	if(!info.vertexOrder.compare("XYZ", Qt::CaseInsensitive)) info.vertexOrderEnum=StelOBJ::XYZ; // no change
	else if (!info.vertexOrder.compare("XZY", Qt::CaseInsensitive)) info.vertexOrderEnum=StelOBJ::XZY;
	else if (!info.vertexOrder.compare("YXZ", Qt::CaseInsensitive)) info.vertexOrderEnum=StelOBJ::YXZ;
	else if (!info.vertexOrder.compare("YZX", Qt::CaseInsensitive)) info.vertexOrderEnum=StelOBJ::YZX;
	else if (!info.vertexOrder.compare("ZXY", Qt::CaseInsensitive)) info.vertexOrderEnum=StelOBJ::ZXY;
	else if (!info.vertexOrder.compare("ZYX", Qt::CaseInsensitive)) info.vertexOrderEnum=StelOBJ::ZYX;
	else qCWarning(sceneInfo)<<"Invalid vertex order statement:"<<info.vertexOrder;

	info.camNearZ = ini.value("camNearZ",0.3f).toFloat();
	info.camFarZ = ini.value("camFarZ",10000.0f).toFloat();
	info.shadowFarZ = ini.value("shadowDistance",info.camFarZ).toFloat();
	info.shadowSplitWeight = ini.value("shadowSplitWeight",-1.0f).toFloat();

	//constrain shadow range to cam far z, makes no sense to extend it further
	if(info.shadowFarZ>info.camFarZ)
		info.shadowFarZ = info.camFarZ;

	// In case we don't have an axis-aligned OBJ model, this is the chance to correct it.
	if (ini.contains("obj2grid_trafo"))
	{
		QString str=ini.value("obj2grid_trafo").toString();
		QStringList strList=str.split(",");
		bool conversionOK[16];
		if (strList.length()==16)
		{
			info.obj2gridMatrix.set(strList.at(0).toDouble(&conversionOK[0]),
					strList.at(1).toDouble(&conversionOK[1]),
					strList.at(2).toDouble(&conversionOK[2]),
					strList.at(3).toDouble(&conversionOK[3]),
					strList.at(4).toDouble(&conversionOK[4]),
					strList.at(5).toDouble(&conversionOK[5]),
					strList.at(6).toDouble(&conversionOK[6]),
					strList.at(7).toDouble(&conversionOK[7]),
					strList.at(8).toDouble(&conversionOK[8]),
					strList.at(9).toDouble(&conversionOK[9]),
					strList.at(10).toDouble(&conversionOK[10]),
					strList.at(11).toDouble(&conversionOK[11]),
					strList.at(12).toDouble(&conversionOK[12]),
					strList.at(13).toDouble(&conversionOK[13]),
					strList.at(14).toDouble(&conversionOK[14]),
					strList.at(15).toDouble(&conversionOK[15])
					);
			for (int i=0; i<16; ++i)
			{
				if (!conversionOK[i]) qCWarning(sceneInfo) << "WARNING: scenery3d.ini: element " << i+1 << " of obj2grid_trafo invalid, set zo zero.";
			}
		}
		else qCWarning(sceneInfo) << "obj2grid_trafo invalid: not 16 comma-separated elements";
	}
	ini.endGroup();

	//some importing/rendering params
	ini.beginGroup("general");
	info.transparencyThreshold = ini.value("transparency_threshold", 0.5f).toFloat();
	info.sceneryGenerateNormals = ini.value("scenery_generate_normals", false).toBool();
	info.groundGenerateNormals = ini.value("ground_generate_normals", false).toBool();
	ini.endGroup();

	//load location data
	if(ini.childGroups().contains("location"))
	{
		ini.beginGroup("location");
		info.location.reset(new StelLocation());
		info.location->name = ini.value("name",info.name).toString();
		info.location->planetName = ini.value("planetName","Earth").toString();

		if(ini.contains("altitude"))
		{
			QVariant val = ini.value("altitude");
			if(val == "from_model")
			{
				info.altitudeFromModel = true;
				//info.location->altitude = -32766;
			}
			else
			{
				info.altitudeFromModel = false;
				info.location->altitude = val.toInt();
			}
		}

		if(ini.contains("latitude"))
			info.location->setLatitude(StelUtils::getDecAngle(ini.value("latitude").toString())*M_180_PI);
		if (ini.contains("longitude"))
			info.location->setLongitude(StelUtils::getDecAngle(ini.value("longitude").toString())*M_180_PI);
		if (ini.contains("country"))
			info.location->region = StelLocationMgr::pickRegionFromCountry(ini.value("country").toString());
		if (ini.contains("state"))
			info.location->state = ini.value("state").toString();
		info.location->ianaTimeZone = StelLocationMgr::sanitizeTimezoneStringFromLocationDB(ini.value("timezone", "LMST").toString());

		info.location->landscapeKey = info.landscapeName;
		ini.endGroup();
	}
	else
		info.location.reset();

	//load coord info
	ini.beginGroup("coord");
	info.gridName = ini.value("grid_name","Unspecified Coordinate Frame").toString();
	double orig_x = ini.value("orig_E", 0.0).toDouble();
	double orig_y = ini.value("orig_N", 0.0).toDouble();
	double orig_z = ini.value("orig_H", 0.0).toDouble();
	info.modelWorldOffset=Vec3d(orig_x, orig_y, orig_z); // RealworldGridCoords=objCoords+modelWorldOffset

	// Find a rotation around vertical axis, most likely required by meridian convergence.
	double rot_z=0.0;
	QVariant convAngle = ini.value("convergence_angle",0.0);
	if (convAngle.toString().startsWith("from_"))
	{ // compute rot_z from grid_meridian and location. Check their existence!
		if (convAngle.toString()=="from_utm")
		{
			if (!info.location.isNull())
			{
				QPair<int, QChar>utmZone=StelLocationMgr::utmZone(info.location->getLongitude(), info.location->getLatitude());
				QPair<Vec3d,Vec2d> utm= StelLocationMgr::geo2utm(info.location->getLongitude(), info.location->getLatitude());
				rot_z = utm.second[0];

				qCInfo(sceneInfo) << "With Longitude " << info.location->getLongitude()
					 << ", Latitude " << info.location->getLatitude() << " and CM="
					 << utm.first[2] << " of UTM zone" << utmZone.first << utmZone.second << ", ";
				qCInfo(sceneInfo) << "setting meridian convergence to " << rot_z*M_180_PI << "degrees";
			}
			else
			{
				qCWarning(sceneInfo) << "scenery3d.ini: Convergence angle \"from_utm\" requires location section!";
			}
		}
		else if (ini.contains("grid_meridian"))
		{
			double gridCentralMeridian=StelUtils::getDecAngle(ini.value("grid_meridian").toString())*M_180_PI;
			if (!info.location.isNull())
			{
				// Formula from: http://en.wikipedia.org/wiki/Transverse_Mercator_projection, Convergence
				//rot_z=std::atan(std::tan((lng-gridCentralMeridian)*M_PI/180.)*std::sin(lat*M_PI/180.));
				// or from http://de.wikipedia.org/wiki/Meridiankonvergenz
				rot_z=(info.location->getLongitude() - gridCentralMeridian)*M_PI_180*std::sin(info.location->getLatitude()*M_PI_180);

				qCInfo(sceneInfo) << "With Longitude " << info.location->getLongitude()
					 << ", Latitude " << info.location->getLatitude() << " and CM="
					 << gridCentralMeridian << ", ";
				qCInfo(sceneInfo) << "setting meridian convergence to " << rot_z*M_180_PI << "degrees";
			}
			else
			{
				qCWarning(sceneInfo) << "scenery3d.ini: Convergence angle \"from_grid\" requires location section!";
			}
		}
		else
		{
			qCWarning(sceneInfo) << "scenery3d.ini: Convergence angle \"from_grid\": cannot compute without grid_meridian!";
		}
	}
	else
	{
		rot_z = convAngle.toDouble() * M_PI_180;
	}
	// We must apply also a 90 degree rotation, plus convergence(rot_z)

	// Meridian Convergence is negative in north-west quadrant.
	// positive MC means True north is "left" of grid north, and model must be rotated clockwise. E.g. Sterngarten (east of UTM CM +15deg) has +0.93, we must rotate clockwise!
	// A zRotate rotates counterclockwise, so we must reverse rot_z.
	info.zRotateMatrix = Mat4d::zrotation(M_PI_2 - rot_z);

	// At last, find start points.
	if(ini.contains("start_E") && ini.contains("start_N"))
	{
		info.startPositionFromModel = false;
		info.startWorldOffset[0] = ini.value("start_E").toDouble();
		info.startWorldOffset[1] = ini.value("start_N").toDouble();
		//FS this is not really used anymore, i think
		info.startWorldOffset[2] = ini.value("start_H",0.0).toDouble();
	}
	else
	{
		info.startPositionFromModel = true;
	}
	info.eyeLevel=ini.value("start_Eye", 1.65).toDouble();

	//calc pos in model coords
	info.relativeStartPosition = info.startWorldOffset - info.modelWorldOffset;
	// I love code without comments
	info.relativeStartPosition[1]*=-1.0;
	info.relativeStartPosition = info.zRotateMatrix.inverse() * info.relativeStartPosition;
	info.relativeStartPosition[1]*=-1.0;

	if(ini.contains("zero_ground_height"))
	{
		info.groundNullHeightFromModel=false;
		info.groundNullHeight = ini.value("zero_ground_height").toDouble();
	}
	else
	{
		info.groundNullHeightFromModel=true;
		info.groundNullHeight=0.;
	}

	if (ini.contains("start_az_alt_fov") && !(GETSTELPROPERTYVALUE("Scenery3d.ignoreInitialView").toBool()))
	{
		qCDebug(sceneInfo) << "scenery3d.ini: setting initial dir/fov.";
		info.lookAt_fov=Vec3f(ini.value("start_az_alt_fov").toString());
		info.lookAt_fov[0]=180.0f-info.lookAt_fov[0]; // fix azimuth
	}
	else
	{
		info.lookAt_fov=Vec3f(0.f, 0.f, -1000.f);
		qCDebug(sceneInfo) << "scenery3d.ini: No start view direction/fov given, or ignoring initial view setting.";
	}
	ini.endGroup();

	info.isValid = true;
	return true;
}

QString SceneInfo::getLocalizedHTMLDescription() const
{
	if(!this->isValid)
		return QString();

	//This is taken from ViewDialog.cpp
	QString lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
	{
		lang = lang.split("_").at(0);
	}

	QString descFile = StelFileMgr::findFile( fullPath + "/description."+lang+".utf8");
	if(descFile.isEmpty())
	{
		//fall back to english
		qCWarning(sceneInfo)<<"No scene description found for language"<<lang<<", falling back to english";
		descFile = StelFileMgr::findFile( fullPath + "/description.en.utf8");
	}

	if(descFile.isEmpty())
	{
		//fall back to stored description
		qCWarning(sceneInfo)<<"No external scene description found";
		return QString();
	}

	//load the whole file and return it as string
	QFile f(descFile);
	QString htmlFile;
	if(f.open(QIODevice::ReadOnly))
	{
		htmlFile = QString::fromUtf8(f.readAll());
		f.close();
	}

	return htmlFile;
}

QString SceneInfo::getIDFromName(const QString &name)
{
	QMap<QString, QString> nameToDirMap = getNameToIDMap();

	return nameToDirMap.value(name);
}

bool SceneInfo::loadByName(const QString &name, SceneInfo &info)
{
	QString id = getIDFromName(name);
	if(!id.isEmpty())
		return loadByID(id,info);
	else
	{
	    qCWarning(sceneInfo) << "Can't find a 3D scenery with name=" << name;
	    return false;
	}
}

QStringList SceneInfo::getAllSceneIDs()
{
	QMap<QString,QString> nameToDirMap = getNameToIDMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the values
	QMap<QString,QString>::const_iterator it=nameToDirMap.constBegin();
	while (it != nameToDirMap.constEnd())
	{
		result += it.value();
		it++;
	}
	return result;
}

QStringList SceneInfo::getAllSceneNames()
{
	QMap<QString,QString> nameToDirMap = getNameToIDMap();
	QStringList result;

	// We just look over the map of names to IDs and extract the keys
	QMap<QString,QString>::const_iterator it=nameToDirMap.constBegin();
	while (it != nameToDirMap.constEnd())
	{
		result += it.key();
		it++;
	}
	return result;
}

QMap<QString, QString> SceneInfo::getNameToIDMap()
{
	QMap<QString, QString> result;

	const QSet<QString> scenery3dDirs = StelFileMgr::listContents(SceneInfo::SCENES_PATH, StelFileMgr::Directory);

	for (const auto& dir : scenery3dDirs)
	{
		QSettings scenery3dIni(StelFileMgr::findFile(SceneInfo::SCENES_PATH + dir + "/scenery3d.ini"), StelIniFormat);
		QString k = scenery3dIni.value("model/name").toString();
		result[k] = dir;
	}
	return result;
}

StoredViewList StoredView::getGlobalViewsForScene(const SceneInfo &scene)
{
	StoredViewList ret;

	//return empty
	if(!scene.isValid)
		return ret;

	//load global viewpoints
	QFileInfo globalfile( QDir(scene.fullPath), "viewpoints.ini");

	if(!globalfile.isFile())
	{
		qCWarning(storedView)<<globalfile.absoluteFilePath()<<" is not a file";
	}
	else
	{
		QSettings ini(globalfile.absoluteFilePath(),StelIniFormat);
		if (ini.status() != QSettings::NoError)
		{
			qCWarning(storedView) << "Error reading global viewpoint file " << globalfile.absoluteFilePath();
		}
		else
		{
			int size = ini.beginReadArray("StoredViews");
			readArray(ini,ret,size,true);
			ini.endArray();
		}
	}

	return ret;
}

StoredViewList StoredView::getUserViewsForScene(const SceneInfo &scene)
{
	StoredViewList ret;

	//return empty
	if(!scene.isValid)
		return ret;

	QSettings* ini = getUserViews();
	if (ini->status() != QSettings::NoError)
	{
		qCWarning(storedView) << "Error reading user viewpoint file";
	}
	else
	{
		int size = ini->beginReadArray(scene.id);
		readArray(*ini,ret,size,false);
		ini->endArray();
	}

	return ret;
}

void StoredView::saveUserViews(const SceneInfo &scene, const StoredViewList &list)
{
	//this should never happen
	Q_ASSERT(scene.isValid);

	QSettings* ini = getUserViews();

	if (ini->status() != QSettings::NoError)
	{
		qCWarning(storedView) << "Error reading user viewpoint file";
	}
	else
	{
		//clear old array
		ini->remove(scene.id);

		ini->beginWriteArray(scene.id,list.size());
		//add data
		writeArray(*ini,list);
		ini->endArray();

		//explicit flushing here, may not be necessary
		ini->sync();
	}
}

void StoredView::readArray(QSettings &ini, StoredViewList &list, int size, bool isGlobal)
{
	for(int i =0;i<size;++i)
	{
		ini.setArrayIndex(i);

		StoredView sv;
		sv.isGlobal = isGlobal;
		sv.label = ini.value("label").toString();
		sv.description = ini.value("description").toString();
		sv.position = Vec4d(ini.value("position").toString());
		sv.view_fov = Vec3f(ini.value("view_fov").toString());
		if (ini.contains("JD"))
		{
			sv.jdIsRelevant=true;
			sv.jd=ini.value("JD").toDouble();
		}
		else
			sv.jdIsRelevant=false;

		list.append(sv);
	}
}

void StoredView::writeArray(QSettings &ini, const StoredViewList &list)
{
	for(int i =0;i<list.size();++i)
	{
		const StoredView& view = list.at(i);
		ini.setArrayIndex(i);

		ini.setValue("label", view.label);
		ini.setValue("description", view.description);
		ini.setValue("position", view.position.toStr());
		ini.setValue("view_fov", view.view_fov.toStr());
		if (view.jdIsRelevant)
			ini.setValue("JD", (view.jd));
	}
}

QSettings* StoredView::getUserViews()
{
	if(userviews)
		return userviews;

	//try to find an writable location
	QString file = StelFileMgr::findFile(USERVIEWS_FILE, StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	if (file.isEmpty())
	{
		//make sure the new file goes into user dir
		file = StelFileMgr::getUserDir() + "/" + USERVIEWS_FILE;
	}

	if(!StelFileMgr::exists(file))
	{
		//create an empty file, or QSettings may complain
		//is this somehow easier to do in Qt?
		QFileInfo f(file);
		QDir().mkpath(f.absolutePath());
		QFile qfile(file);
		if (!qfile.open(QIODevice::WriteOnly))
		{
			qCWarning(storedView) << "StoredView: cannot create userviews file!";
		}
		qfile.close();
	}

	//QSettings gets deleted when plugin is shut down (also saves settings)
	//TODO StelIniFormat has bugs with saving HTML! so we use the default Qt format here, no idea if this may cause some problems.
	userviews = new QSettings(file,QSettings::IniFormat,GETSTELMODULE(Scenery3d));
	return userviews;
}
