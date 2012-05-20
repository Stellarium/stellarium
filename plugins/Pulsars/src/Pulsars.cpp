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
#include "Pulsars.hpp"
#include "Pulsar.hpp"

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
StelModule* PulsarsStelPluginInterface::getStelModule() const
{
	return new Pulsars();
}

StelPluginInfo PulsarsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Pulsars);

	StelPluginInfo info;
	info.id = "Pulsars";
	info.displayedName = N_("Pulsars");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = N_("This plugin plots the position of various pulsars, with object information about each one. Pulsar data is derived from 'The ATNF Pulsar Catalogue'  (Manchester, R. N., Hobbs, G. B., Teoh, A. & Hobbs, M., Astron. J., 129, 1993-2006 (2005) (astro-ph/0412641)).<br><br>Note: pulsar identifiers have the prefix 'PSR'");
	return info;
}

Q_EXPORT_PLUGIN2(Pulsars, PulsarsStelPluginInterface)


/*
 Constructor
*/
Pulsars::Pulsars()
{
	setObjectName("Pulsars");
	font.setPixelSize(StelApp::getInstance().getSettings()->value("gui/base_font_size", 13).toInt());
}

/*
 Destructor
*/
Pulsars::~Pulsars()
{
	//
}

void Pulsars::deinit()
{
	psr.clear();
	Pulsar::markerTexture.clear();
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double Pulsars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void Pulsars::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Pulsars");

		jsonCatalogPath = StelFileMgr::findFile("modules/Pulsars", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/catalog.json";

		texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur2.png");
		Pulsar::markerTexture = StelApp::getInstance().getTextureManager().createTexture(":/Pulsars/pulsar.png");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Pulsars::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(QFileInfo(jsonCatalogPath).exists())
	{
		if (getJsonFileVersion() != PULSARS_PLUGIN_VERSION)
		{
			restoreDefaultJsonFile();
		}
	}
	else
	{
		qDebug() << "Pulsars::init catalog.json does not exist - copying default file to " << jsonCatalogPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "Pulsars::init using catalog.json file: " << jsonCatalogPath;

	readJsonFile();

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

/*
 Draw our module. This should print name of first PSR in the main window
*/
void Pulsars::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	foreach (const PulsarP& pulsar, psr)
	{
		if (pulsar && pulsar->initialized)
			pulsar->draw(core, painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void Pulsars::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Pulsar");
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

QList<StelObjectP> Pulsars::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->initialized)
		{
			equPos = pulsar->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(pulsar));
			}
		}
	}

	return result;
}

StelObjectP Pulsars::searchByName(const QString& englishName) const
{
	QString objw = englishName.toUpper();
	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getEnglishName().toUpper() == englishName)
			return qSharedPointerCast<StelObject>(pulsar);
	}

	return NULL;
}

StelObjectP Pulsars::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getNameI18n().toUpper() == nameI18n)
			return qSharedPointerCast<StelObject>(pulsar);
	}

	return NULL;
}

QStringList Pulsars::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->getNameI18n().toUpper().left(objw.length()) == objw)
		{
				result << pulsar->getNameI18n().toUpper();
		}
	}

	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void Pulsars::restoreDefaultJsonFile(void)
{
	if (QFileInfo(jsonCatalogPath).exists())
		backupJsonFile(true);

	QFile src(":/Pulsars/catalog.json");
	if (!src.copy(jsonCatalogPath))
	{
		qWarning() << "Pulsars::restoreDefaultJsonFile cannot copy json resource to " + jsonCatalogPath;
	}
	else
	{
		qDebug() << "Pulsars::init copied default catalog.json to " << jsonCatalogPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(jsonCatalogPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

/*
  Creates a backup of the catalog.json file called catalog.json.old
*/
bool Pulsars::backupJsonFile(bool deleteOriginal)
{
	QFile old(jsonCatalogPath);
	if (!old.exists())
	{
		qWarning() << "Pulsars::backupJsonFile no file to backup";
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
				qWarning() << "Pulsars::backupJsonFile WARNING - could not remove old catalog.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "Pulsars::backupJsonFile WARNING - failed to copy catalog.json to catalog.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of pulsars.
*/
void Pulsars::readJsonFile(void)
{
	setPSRMap(loadPSRMap());
}

/*
  Parse JSON file and load pulsars to map
*/
QVariantMap Pulsars::loadPSRMap(QString path)
{
	if (path.isEmpty())
	    path = jsonCatalogPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "Pulsars::loadPSRMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void Pulsars::setPSRMap(const QVariantMap& map)
{
	psr.clear();
	QVariantMap psrMap = map.value("pulsars").toMap();
	foreach(QString psrKey, psrMap.keys())
	{
		QVariantMap psrData = psrMap.value(psrKey).toMap();
		psrData["designation"] = psrKey;

		PulsarP pulsar(new Pulsar(psrData));
		if (pulsar->initialized)
			psr.append(pulsar);

	}
}

const QString Pulsars::getJsonFileVersion(void)
{
	QString jsonVersion("unknown");
	QFile jsonPSRCatalogFile(jsonCatalogPath);
	if (!jsonPSRCatalogFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Pulsars::init cannot open " << jsonCatalogPath;
		return jsonVersion;
	}

	QVariantMap map;
	map = StelJsonParser::parse(&jsonPSRCatalogFile).toMap();
	if (map.contains("version"))
	{
		QString creator = map.value("version").toString();
		QRegExp vRx(".*(\\d+\\.\\d+\\.\\d+).*");
		if (vRx.exactMatch(creator))
		{
			jsonVersion = vRx.capturedTexts().at(1);
		}
	}

	jsonPSRCatalogFile.close();
	qDebug() << "Pulsars::getJsonFileVersion() version from file:" << jsonVersion;
	return jsonVersion;
}

PulsarP Pulsars::getByID(const QString& id)
{
	foreach(const PulsarP& pulsar, psr)
	{
		if (pulsar->initialized && pulsar->designation == id)
			return pulsar;
	}
	return PulsarP();
}
