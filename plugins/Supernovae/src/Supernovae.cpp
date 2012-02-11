/*
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
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "Supernovae.hpp"
#include "Supernova.hpp"

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
StelModule* SupernovaeStelPluginInterface::getStelModule() const
{
	return new Supernovae();
}

StelPluginInfo SupernovaeStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Supernovae);

	StelPluginInfo info;
	info.id = "Supernovae";
	info.displayedName = N_("Historical Supernovae");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("A plugin that shows some historical supernovae brighter than 10 visual magnitude: SN 185A (7 December), SN 386A (24 April), SN 1006A (29 April), SN 1054A (3 July), SN 1181A (4 August), SN 1572A (5 November), SN 1604A (8 October), SN 1680A (15 August), SN 1885A (17 August), SN 1895B (5 July), SN 1937C (21 August), SN 1972E (8 May), SN 1987A (24 February) and SN 2011FE (13 September).");
	return info;
}

Q_EXPORT_PLUGIN2(Supernovae, SupernovaeStelPluginInterface)


/*
 Constructor
*/
Supernovae::Supernovae()
{
	setObjectName("Supernovae");
	font.setPixelSize(StelApp::getInstance().getSettings()->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Supernovae::~Supernovae()
{
	//
}

void Supernovae::deinit()
{
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Supernovae::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Supernovae::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Supernovae");

		sneJsonPath = StelFileMgr::findFile("modules/Supernovae", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/supernovae.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur2.png");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Supernovas::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(sneJsonPath).exists())
	{
		if (getJsonFileVersion() != PLUGIN_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Supernovae::init supernovae.json does not exist - copying default file to " << sneJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Supernovae::init using supernovae.json file: " << sneJsonPath;

	readJsonFile();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first SNe in the main window
*/
void Supernovae::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const SupernovaP& sn, snstar)
	{
		if (sn && sn->initialized)
			sn->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void Supernovae::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Supernova");
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

QList<StelObjectP> Supernovae::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->initialized)
		{
			equPos = sn->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(sn));
			}
		}
	}

	return result;
}

StelObjectP Supernovae::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(sn);
	}

	return NULL;
}

StelObjectP Supernovae::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(sn);
	}

	return NULL;
}

QStringList Supernovae::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << sn->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Supernovae::restoreDefaultJsonFile(void)
{
	if (QFileInfo(sneJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/Supernovae/supernovae.json");
	if (!src.copy(sneJsonPath))
	{
		qWarning() << "Supernovae::restoreDefaultJsonFile cannot copy json resource to " + sneJsonPath;
	}
	else
	{
		qDebug() << "Supernovae::init copied default supernovae.json to " << sneJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(sneJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

/*
  Creates a backup of the supernovae.json file called supernovae.json.old
*/
bool Supernovae::backupJsonFile(bool deleteOriginal)
{
	QFile old(sneJsonPath);
	if (!old.exists())
	{
		qWarning() << "Supernovae::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = sneJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Supernovae::backupJsonFile WARNING - could not remove old supernovas.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Supernovae::backupJsonFile WARNING - failed to copy supernovae.json to supernovae.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of supernovaes.
*/
void Supernovae::readJsonFile(void)
{
	setSNeMap(loadSNeMap());
}

/*
  Parse JSON file and load supernovaes to map
*/
QVariantMap Supernovae::loadSNeMap(QString path)
{
	if (path.isEmpty())
	    path = sneJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "Supernovae::loadSNeMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Supernovae::setSNeMap(const QVariantMap& map)
{
	snstar.clear();
	QVariantMap sneMap = map.value("supernova").toMap();
	foreach(QString sneKey, sneMap.keys())
	{
		QVariantMap sneData = sneMap.value(sneKey).toMap();
		sneData["designation"] = QString("SN %1").arg(sneKey);

		SupernovaP sn(new Supernova(sneData));
		if (sn->initialized)
			snstar.append(sn);

	}
}

const QString Supernovae::getJsonFileVersion(void)
{
	QString jsonVersion("unknown");
	QFile sneJsonFile(sneJsonPath);
	if (!sneJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Supernovae::init cannot open " << sneJsonPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&sneJsonFile).toMap();
	if (map.contains("version"))
	{
		QString creator = map.value("version").toString();
		QRegExp vRx(".*(\\d+\\.\\d+\\.\\d+).*");
		if (vRx.exactMatch(creator))
		{
			jsonVersion = vRx.capturedTexts().at(1);
		}
	}

	sneJsonFile.close();
	qDebug() << "Supernovae::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

SupernovaP Supernovae::getByID(const QString& id)
{
	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->initialized && sn->designation == id)
			return sn;
	}
	return SupernovaP();
}
