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
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelSkyDrawer.hpp"
#include "SNe.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QVariantMap>
#include <QVariant>
#include <QList>

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* SNeStelPluginInterface::getStelModule() const
{
	return new SNe();
}

StelPluginInfo SNeStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(SNe);

	StelPluginInfo info;
	info.id = "SNe";
	info.displayedName = q_("Historical supernova");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = QString("%1: %2.").arg(q_("The plugin for visualization of some historical supernovas, brighter 10 magnitude")).arg(q_("SN 185A (7 December), SN 386A (24 April), SN 1006A (29 April), SN 1054A (3 July), SN 1181A (4 August), SN 1572A (5 November), SN 1604A (8 October), SN 1680A (15 August), SN 1885A (17 August), SN 1895B (5 July), SN 1937C (21 August), SN 1972E (8 May) and SN 1978A (24 February)"));
	return info;
}

Q_EXPORT_PLUGIN2(SNe, SNeStelPluginInterface)


/*
 Constructor
*/
SNe::SNe()
{
	setObjectName("SNe");	
}

/*
 Destructor
*/
SNe::~SNe()
{
	//
}

/*
 Reimplementation of the getCallOrder method
*/
double SNe::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void SNe::init()
{
	try
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/SNe");

		sneJsonPath = StelFileMgr::findFile("modules/SNe", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/sne.json";
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "SNe::init error: " << e.what();
		return;
	}

	// If the json file does not already exist, create it from the resource in the Qt resource
	if(!QFileInfo(sneJsonPath).exists())
	{
		qDebug() << "SNe::init sne.json does not exist - copying default file to " << sneJsonPath;
		restoreDefaultJsonFile();
	}

	qDebug() << "SNe::init using sne.json file: " << sneJsonPath;

	readJsonFile();

}

/*
 Draw our module. This should print name of first SNe in the main window
*/
void SNe::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelSkyDrawer* sd = core->getSkyDrawer();
	StelPainter painter(prj);

	Vec3f color = Vec3f(1.f,1.f,1.f);
	float rcMag[2];
	Vec3f v;
	double cJD = core->getJDay();
	double mag;
	foreach(const supernova &sn, snstar)
	{
		StelUtils::spheToRect(sn.ra, sn.de, v);

		mag = computeSNeMag(sn.peakJD,sn.maxMagnitude,sn.type,cJD);

		if (mag <= sd->getLimitMagnitude())
		{
			sd->computeRCMag(mag, rcMag);
			sd->drawPointSource(&painter, v, rcMag, color, false);
			painter.setColor(color[0], color[1], color[2], 1);
			painter.drawText(Vec3d(v[0], v[1], v[2]), QString("SN %1").arg(sn.name), 0, 10, 10, false);
		}
	}
}

/*
  Replace the JSON file with the default from the compiled-in resource
*/
void SNe::restoreDefaultJsonFile(void)
{
	if (QFileInfo(sneJsonPath).exists())
		backupJsonFile(true);

	QFile src(":/SNe/sne.json");
	if (!src.copy(sneJsonPath))
	{
		qWarning() << "SNe::restoreDefaultJsonFile cannot copy json resource to " + sneJsonPath;
	}
	else
	{
		qDebug() << "SNe::init copied default sne.json to " << sneJsonPath;
		// The resource is read only, and the new file inherits this...  make sure the new file
		// is writable by the Stellarium process so that updates can be done.
		QFile dest(sneJsonPath);
		dest.setPermissions(dest.permissions() | QFile::WriteOwner);
	}
}

/*
  Creates a backup of the sne.json file called sne.json.old
*/
bool SNe::backupJsonFile(bool deleteOriginal)
{
	QFile old(sneJsonPath);
	if (!old.exists())
	{
		qWarning() << "SNe::backupJsonFile no file to backup";
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
				qWarning() << "SNe::backupJsonFile WARNING - could not remove old sne.json file";
				return false;
			}
		}
	}
	else
	{
		qWarning() << "SNe::backupJsonFile WARNING - failed to copy sne.json to sne.json.old";
		return false;
	}

	return true;
}

/*
  Read the JSON file and create list of supernovaes.
*/
void SNe::readJsonFile(void)
{
	setSNeMap(loadSNeMap());
}

/*
  Parse JSON file and load supernovaes to map
*/
QVariantMap SNe::loadSNeMap(QString path)
{
	if (path.isEmpty())
	    path = sneJsonPath;

	QVariantMap map;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly))
	    qWarning() << "SNe::loadSNeMap cannot open " << path;
	else
	    map = StelJsonParser::parse(jsonFile.readAll()).toMap();

	jsonFile.close();
	return map;
}

/*
  Set items for list of struct from data map
*/
void SNe::setSNeMap(const QVariantMap& map)
{
	QVariantMap sneMap = map.value("supernova").toMap();
	supernova sn;
	foreach(QString sneKey, sneMap.keys())
	{
		QVariantMap sneData = sneMap.value(sneKey).toMap();
		sn.name = sneKey;
		sn.type = sneData.value("type").toString();
		sn.maxMagnitude = sneData.value("maxMagnitude").toFloat();
		sn.peakJD = sneData.value("peakJD").toDouble();
		sn.ra = StelUtils::getDecAngle(sneData.value("alpha").toString());
		sn.de = StelUtils::getDecAngle(sneData.value("delta").toString());

		snstar.append(sn);
	}
}

/*
  Computation of visual magnitude as function from supernova type and time
*/
double SNe::computeSNeMag(double peakJD, float maxMag, QString sntype, double currentJD)
{
	double vmag = 30;
	if (peakJD<=currentJD)
	{
	    if (peakJD==std::floor(currentJD))
		vmag = maxMag;

	    else
		vmag = maxMag - 2.5 * (-3) * std::log10(currentJD-peakJD);
	}
	else
	{
	    if (std::abs(peakJD-currentJD)<=5)
		vmag = maxMag - 2.5 * (-1.75) * std::log10(std::abs(peakJD-currentJD));
		if (vmag<maxMag)
		    vmag = maxMag;
	}


	return vmag;
}
