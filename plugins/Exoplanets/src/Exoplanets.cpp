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
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "Exoplanets.hpp"
#include "Exoplanet.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QVariantMap>
#include <QVariant>
#include <QList>
#include <QSharedPointer>
#include <QStringList>

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* ExoplanetsStelPluginInterface::getStelModule() const
{
	return new Exoplanets();
}

StelPluginInfo ExoplanetsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Exoplanets);

	StelPluginInfo info;
	info.id = "Exoplanets";
	info.displayedName = N_("Exoplanets");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("This plugin plots the position of stars with exoplanets. Exoplanets data is derived from The 'Extrasolar Planets Encyclopaedia' at exoplanet.eu");
	return info;
}

Q_EXPORT_PLUGIN2(Exoplanets, ExoplanetsStelPluginInterface)


/*
 Constructor
*/
Exoplanets::Exoplanets()
{
	setObjectName("Exoplanets");
	font.setPixelSize(StelApp::getInstance().getSettings()->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Exoplanets::~Exoplanets()
{
	//
}

void Exoplanets::deinit()
{
	ep.clear();
	Exoplanet::markerTexture.clear();
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Exoplanets::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Exoplanets::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Exoplanets");

		jsonCatalogPath = StelFileMgr::findFile("modules/Exoplanets", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/catalog.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur2.png");
		Exoplanet::markerTexture = StelApp::getInstance().getTextureManager().createTexture(":/Exoplanets/exoplanet.png");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Exoplanets::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(jsonCatalogPath).exists())
	{
		if (getJsonFileVersion() != EXOPLANETS_PLUGIN_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Exoplanets::init catalog.json does not exist - copying default file to " << jsonCatalogPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Exoplanets::init using catalog.json file: " << jsonCatalogPath;

	readJsonFile();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first PSR in the main window
*/
void Exoplanets::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const ExoplanetP& eps, ep)
	{
		if (eps && eps->initialized)
			eps->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void Exoplanets::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Exoplanet");
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

QList<StelObjectP> Exoplanets::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->initialized)
		{
			equPos = eps->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(eps));
			}
		}
	}

	return result;
}

StelObjectP Exoplanets::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(eps);
	}

	return NULL;
}

StelObjectP Exoplanets::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(eps);
	}

	return NULL;
}

QStringList Exoplanets::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << eps->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Exoplanets::restoreDefaultJsonFile(void)
{
	if (QFileInfo(jsonCatalogPath).exists())
		backupJsonFile(true);

	QFile src(":/Exoplanets/catalog.json");
	if (!src.copy(jsonCatalogPath))
	{
		qWarning() << "Exoplanets::restoreDefaultJsonFile cannot copy json resource to " + jsonCatalogPath;
	}
	else
	{
		qDebug() << "Exoplanets::init copied default catalog.json to " << jsonCatalogPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(jsonCatalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

/*
  Creates a backup of the catalog.json file called catalog.json.old
*/
bool Exoplanets::backupJsonFile(bool deleteOriginal)
{
	QFile old(jsonCatalogPath);
	if (!old.exists())
	{
		qWarning() << "Exoplanets::backupJsonFile no file to backup";
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
				qWarning() << "Exoplanets::backupJsonFile WARNING - could not remove old catalog.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Exoplanets::backupJsonFile WARNING - failed to copy catalog.json to catalog.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of exoplanets.
*/
void Exoplanets::readJsonFile(void)
{
	setEPMap(loadEPMap());
}

/*
  Parse JSON file and load exoplanets to map
*/
QVariantMap Exoplanets::loadEPMap(QString path)
{
	if (path.isEmpty())
	    path = jsonCatalogPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "Exoplanets::loadEPMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Exoplanets::setEPMap(const QVariantMap& map)
{
	ep.clear();
	QVariantMap epsMap = map.value("exoplanets").toMap();
	foreach(QString epsKey, epsMap.keys())
	{
		QVariantMap epsData = epsMap.value(epsKey).toMap();
		epsData["designation"] = epsKey;

		ExoplanetP eps(new Exoplanet(epsData));
		if (eps->initialized)
			ep.append(eps);

	}
}

const QString Exoplanets::getJsonFileVersion(void)
{
	QString jsonVersion("unknown");
	QFile jsonEPCatalogFile(jsonCatalogPath);
	if (!jsonEPCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Exoplanets::init cannot open " << jsonCatalogPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&jsonEPCatalogFile).toMap();
	if (map.contains("version"))
	{
		QString creator = map.value("version").toString();
		QRegExp vRx(".*(\\d+\\.\\d+\\.\\d+).*");
		if (vRx.exactMatch(creator))
		{
			jsonVersion = vRx.capturedTexts().at(1);
		}
	}

	jsonEPCatalogFile.close();
	qDebug() << "Exoplanets::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

ExoplanetP Exoplanets::getByID(const QString& id)
{
	foreach(const ExoplanetP& eps, ep)
	{
		if (eps->initialized && eps->designation == id)
			return eps;
	}
	return ExoplanetP();
}
