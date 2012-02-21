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
#include "Quasars.hpp"
#include "Quasar.hpp"

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
StelModule* QuasarsStelPluginInterface::getStelModule() const
{
	return new Quasars();
}

StelPluginInfo QuasarsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Quasars);

	StelPluginInfo info;
	info.id = "Quasars";
	info.displayedName = N_("Quasars");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("A plugin that shows some quasars brighter than 16 visual magnitude. A catalogue of quasars compiled from 'Quasars and Active Galactic Nuclei' (13th Ed.) (Veron+ 2010) =2010A&A...518A..10V");
	return info;
}

Q_EXPORT_PLUGIN2(Quasars, QuasarsStelPluginInterface)


/*
 Constructor
*/
Quasars::Quasars()
{
	setObjectName("Quasars");
	font.setPixelSize(StelApp::getInstance().getSettings()->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Quasars::~Quasars()
{
	//
}

void Quasars::deinit()
{
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Quasars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Quasars::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Quasars");

		catalogJsonPath = StelFileMgr::findFile("modules/Quasars", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/catalog.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur2.png");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Quasars::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(catalogJsonPath).exists())
	{
		if (getJsonFileVersion() != QUASARS_PLUGIN_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Quasars::init catalog.json does not exist - copying default file to " << catalogJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Quasars::init using catalog.json file: " << catalogJsonPath;

	readJsonFile();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first QSO in the main window
*/
void Quasars::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const QuasarP& quasar, QSO)
	{
		if (quasar && quasar->initialized)
			quasar->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void Quasars::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Quasar");
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

QList<StelObjectP> Quasars::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->initialized)
		{
			equPos = quasar->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(quasar));
			}
		}
	}

	return result;
}

StelObjectP Quasars::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(quasar);
	}

	return NULL;
}

StelObjectP Quasars::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(quasar);
	}

	return NULL;
}

QStringList Quasars::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << quasar->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Quasars::restoreDefaultJsonFile(void)
{
	if (QFileInfo(catalogJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/Quasars/catalog.json");
	if (!src.copy(catalogJsonPath))
	{
		qWarning() << "Quasars::restoreDefaultJsonFile cannot copy json resource to " + catalogJsonPath;
	}
	else
	{
		qDebug() << "Quasars::init copied default catalog.json to " << catalogJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(catalogJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

/*
  Creates a backup of the Quasars.json file called Quasars.json.old
*/
bool Quasars::backupJsonFile(bool deleteOriginal)
{
	QFile old(catalogJsonPath);
	if (!old.exists())
	{
		qWarning() << "Quasars::backupJsonFile no file to backup";
		return false;
	}

	QString backupPath = catalogJsonPath + ".old";
	if (QFileInfo(backupPath).exists())
		QFile(backupPath).remove();

	if (old.copy(backupPath))
	{
		if (deleteOriginal)
		{
			if (!old.remove())
			{
				qWarning() << "Quasars::backupJsonFile WARNING - could not remove old catalog.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Quasars::backupJsonFile WARNING - failed to copy catalog.json to catalog.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of quasars.
*/
void Quasars::readJsonFile(void)
{
	setQSOMap(loadQSOMap());
}

/*
  Parse JSON file and load quasarss to map
*/
QVariantMap Quasars::loadQSOMap(QString path)
{
	if (path.isEmpty())
	    path = catalogJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "Quasars::loadQSOMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Quasars::setQSOMap(const QVariantMap& map)
{
	QSO.clear();
	QVariantMap qsoMap = map.value("quasars").toMap();
	foreach(QString qsoKey, qsoMap.keys())
	{
		QVariantMap qsoData = qsoMap.value(qsoKey).toMap();
		qsoData["designation"] = qsoKey;

		QuasarP quasar(new Quasar(qsoData));
		if (quasar->initialized)
			QSO.append(quasar);

	}
}

const QString Quasars::getJsonFileVersion(void)
{
	QString jsonVersion("unknown");
	QFile catalogJsonFile(catalogJsonPath);
	if (!catalogJsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Quasars::init cannot open " << catalogJsonPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&catalogJsonFile).toMap();
	if (map.contains("version"))
	{
		QString creator = map.value("version").toString();
		QRegExp vRx(".*(\\d+\\.\\d+\\.\\d+).*");
		if (vRx.exactMatch(creator))
		{
			jsonVersion = vRx.capturedTexts().at(1);
		}
	}

	catalogJsonFile.close();
	qDebug() << "Quasars::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

QuasarP Quasars::getByID(const QString& id)
{
	foreach(const QuasarP& quasar, QSO)
	{
		if (quasar->initialized && quasar->designation == id)
			return quasar;
	}
	return QuasarP();
}
