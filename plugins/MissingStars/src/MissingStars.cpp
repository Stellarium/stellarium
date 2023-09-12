/*
 * Copyright (C) 2023 Alexander Wolf
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
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"
#include "StarMgr.hpp"
#include "MissingStar.hpp"
#include "MissingStars.hpp"
#include "MissingStarsDialog.hpp"

#include <QKeyEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QVariantMap>
#include <QVariant>
#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QDir>
#include <QSettings>
#include <stdexcept>

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
StelModule* MissingStarsStelPluginInterface::getStelModule() const
{
	return new MissingStars();
}

StelPluginInfo MissingStarsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(MissingStars);

	StelPluginInfo info;
	info.id = "MissingStars";
	info.displayedName = N_("Missing Stars");
	info.authors = "Alexander Wolf";
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("This plugin allows you to see some missing stars.");
	info.version = MISSINGSTARS_PLUGIN_VERSION;
	info.license = MISSINGSTARS_PLUGIN_LICENSE;
	return info;
}

/*
 Constructor
*/
MissingStars::MissingStars()
{
	setObjectName("MissingStars");
	configDialog = new MissingStarsDialog();
	//conf = StelApp::getInstance().getSettings();
	setFontSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
}

/*
 Destructor
*/
MissingStars::~MissingStars()
{
	delete configDialog;
}

void MissingStars::deinit()
{
	texPointer.clear();
}

/*
 Reimplementation of the getCallOrder method
*/
double MissingStars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName);
	return 0;
}


/*
 Init our module
*/
void MissingStars::init()
{
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

	readJsonFile();

	StarMgr* smgr = GETSTELMODULE(StarMgr);
	connect(smgr, SIGNAL(starsDisplayedChanged(bool)),      this, SLOT(setFlagShowStars(bool)));
	connect(smgr, SIGNAL(starLabelsDisplayedChanged(bool)), this, SLOT(setFlagShowLabels(bool)));

	setFlagShowStars(true);

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	// key bindings and other actions
	addAction("actionShow_MissingStars_ConfigDialog", N_("Missing Stars"), N_("Missing Stars configuration window"), configDialog, "visible", ""); // Allow assign shortkey
}

/*
 Draw our module. This should print name of first missing star in the main window
*/
void MissingStars::draw(StelCore* core)
{
	if (!flagShowStars)
		return;

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);
	
	for (const auto& ms : qAsConst(missingstars))
	{
		if (ms && ms->initialized)
			ms->draw(core, &painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
}

void MissingStars::drawPointer(StelCore* core, StelPainter& painter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("MissingStar");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		painter.setColor(obj->getInfoColor());
		texPointer->bind();
		painter.setBlending(true);
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]), static_cast<float>(screenpos[1]), 13.f, static_cast<float>(StelApp::getInstance().getTotalRunTime())*40.f);
	}
}

QList<StelObjectP> MissingStars::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& ms : missingstars)
	{
		if (ms->initialized)
		{
			equPos = ms->getJ2000EquatorialPos(core);
			equPos.normalize();
			if (equPos.dot(v) >= cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(ms));
			}
		}
	}

	return result;
}

StelObjectP MissingStars::searchByName(const QString& englishName) const
{
	for (const auto& msn : missingstars)
	{
		if (msn->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(msn);
	}

	return nullptr;
}

StelObjectP MissingStars::searchByNameI18n(const QString& nameI18n) const
{
	for (const auto& msn : missingstars)
	{
		if (msn->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(msn);
	}

	return nullptr;
}

QStringList MissingStars::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		for (const auto& msn : missingstars)
		{
			result << msn->getEnglishName();
		}
	}
	else
	{
		for (const auto& msn : missingstars)
		{
			result << msn->getNameI18n();
		}
	}
	return result;
}
 
/*
  Read the JSON file and create list of missing stars.
*/
void MissingStars::readJsonFile(void)
{
	setMissingStarsMap(loadMissingStarsMap());
}

/*
  Parse JSON file and load missing stars to map
*/
QVariantMap MissingStars::loadMissingStarsMap()
{
	QVariantMap map;
	QFile jsonFile(":/MissingStars/missingstars.json");
	if (jsonFile.open(QIODevice::ReadOnly))
	{
		map = StelJsonParser::parse(jsonFile.readAll()).toMap();
		jsonFile.close();
	}
	return map;
}

/*
  Set items for list of struct from data map
*/
void MissingStars::setMissingStarsMap(const QVariantMap& map)
{
	missingstars.clear();
	designations.clear();
	int mscount = 0;
	QVariantMap msMap = map.value("catalog").toMap();
	for (auto &msEntry : msMap)
	{
		QVariantMap msData = msEntry.toMap();
		msData["designation"] = msMap.key(msEntry);

		MissingStarP ms(new MissingStar(msData));
		if (ms->initialized)
		{
			missingstars.append(ms);
			designations.append(ms->getID());
			mscount++;
		}
	}
	qInfo().noquote() << "[MissingStars] Loaded" << mscount << "extra stars (missing in main catalogs)";
}

MissingStarP MissingStars::getByID(const QString& id) const
{
	for (const auto& msn : missingstars)
	{
		if (msn->initialized && msn->designation == id)
			return msn;
	}
	return MissingStarP();
}

bool MissingStars::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

QString MissingStars::getMissingStarsList() const
{
	return designations.join(", ");
}

