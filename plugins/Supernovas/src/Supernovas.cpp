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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "Supernovas.hpp"
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
StelModule* SupernovasStelPluginInterface::getStelModule() const
{
	return new Supernovas();
}

StelPluginInfo SupernovasStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Supernovas);

	StelPluginInfo info;
	info.id = "Supernovas";
	info.displayedName = q_("Historical supernova");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = QString("%1: %2.").arg(q_("The plugin for visualization of some historical supernovas, brighter 10 magnitude")).arg(q_("SN 185A (7 December), SN 386A (24 April), SN 1006A (29 April), SN 1054A (3 July), SN 1181A (4 August), SN 1572A (5 November), SN 1604A (8 October), SN 1680A (15 August), SN 1885A (17 August), SN 1895B (5 July), SN 1937C (21 August), SN 1972E (8 May) and SN 1987A (24 February)"));
	return info;
}

Q_EXPORT_PLUGIN2(Supernovas, SupernovasStelPluginInterface)


/*
 Constructor
*/
Supernovas::Supernovas()
{
	setObjectName("Supernovas");
	font.setPixelSize(13);
}

/*
 Destructor
*/
Supernovas::~Supernovas()
{
	//
}

void Supernovas::deinit()
{
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Supernovas::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Supernovas::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Supernovas");

		sneJsonPath = StelFileMgr::findFile("modules/Supernovas", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/supernovas.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur2.png");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Supernovas::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(!QFileInfo(sneJsonPath).exists())
	{
		if (getJsonFileVersion() != PLUGIN_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Supernovas::init supernovas.json does not exist - copying default file to " << sneJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Supernovas::init using supernovas.json file: " << sneJsonPath;

	readJsonFile();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first SNe in the main window
*/
void Supernovas::draw(StelCore* core)
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

void Supernovas::drawPointer(StelCore* core, StelPainter& painter)
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

QList<StelObjectP> Supernovas::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	foreach(const SupernovaP& sn, snstar)
	{
		result.append(qSharedPointerCast<StelObject>(sn));
	}

	return result;
}

StelObjectP Supernovas::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(sn);
	}

	return NULL;
}

StelObjectP Supernovas::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(sn);
	}

	return NULL;
}

QStringList Supernovas::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
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
void Supernovas::restoreDefaultJsonFile(void)
{
	if (QFileInfo(sneJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/Supernovas/supernovas.json");
	if (!src.copy(sneJsonPath))
	{
		qWarning() << "Supernovas::restoreDefaultJsonFile cannot copy json resource to " + sneJsonPath;
	}
	else
	{
		qDebug() << "Supernovas::init copied default supernovas.json to " << sneJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(sneJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

/*
  Creates a backup of the sne.json file called sne.json.old
*/
bool Supernovas::backupJsonFile(bool deleteOriginal)
{
	QFile old(sneJsonPath);
	if (!old.exists())
	{
		qWarning() << "Supernovas::backupJsonFile no file to backup";
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
				qWarning() << "Supernovas::backupJsonFile WARNING - could not remove old supernovas.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Supernovas::backupJsonFile WARNING - failed to copy supernovas.json to supernovas.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of supernovaes.
*/
void Supernovas::readJsonFile(void)
{
	setSNeMap(loadSNeMap());
}

/*
  Parse JSON file and load supernovaes to map
*/
QVariantMap Supernovas::loadSNeMap(QString path)
{
	if (path.isEmpty())
	    path = sneJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "Supernovas::loadSNeMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Supernovas::setSNeMap(const QVariantMap& map)
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

const QString Supernovas::getJsonFileVersion(void)
{
	QString jsonVersion("unknown");
	QFile sneJsonFile(sneJsonPath);
	if (!sneJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Supernovas::init cannot open " << sneJsonPath;
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
	qDebug() << "Supernovas::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

SupernovaP Supernovas::getByID(const QString& id)
{
	foreach(const SupernovaP& sn, snstar)
	{
		if (sn->initialized && sn->designation == id)
			return sn;
	}
	return SupernovaP();
}
