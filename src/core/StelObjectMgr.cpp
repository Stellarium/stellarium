/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
 * Copyright (C) 2016 Marcos Cardinot
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

#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelObjectModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelMovementMgr.hpp"
#include "RefractionExtinction.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelActionMgr.hpp"

#include <QMouseEvent>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QSettings>

StelObjectMgr::StelObjectMgr() : objectPointerVisibility(true), searchRadiusPixel(25.), distanceWeight(1.f)
{
	setObjectName("StelObjectMgr");
}

StelObjectMgr::~StelObjectMgr()
{
}

void StelObjectMgr::init()
{
	// Register all the core actions.
	QString timeGroup = N_("Date and Time");
	QString movementGroup = N_("Movement and Selection");
	QString displayGroup = N_("Display Options");
	StelActionMgr* actionsMgr = StelApp::getInstance().getStelActionManager();
	actionsMgr->addAction("actionNext_Transit", timeGroup, N_("Next transit of the selected object"), this, "nextTransit()");
	actionsMgr->addAction("actionNext_Rising", timeGroup, N_("Next rising of the selected object"), this, "nextRising()");
	actionsMgr->addAction("actionNext_Setting", timeGroup, N_("Next setting of the selected object"), this, "nextSetting()");
	actionsMgr->addAction("actionToday_Transit", timeGroup, N_("Today's transit of the selected object"), this, "todayTransit()");
	actionsMgr->addAction("actionToday_Rising", timeGroup, N_("Today's rising of the selected object"), this, "todayRising()");
	actionsMgr->addAction("actionToday_Setting", timeGroup, N_("Today's setting of the selected object"), this, "todaySetting()");
	actionsMgr->addAction("actionPrevious_Transit", timeGroup, N_("Previous transit of the selected object"), this, "previousTransit()");
	actionsMgr->addAction("actionPrevious_Rising", timeGroup, N_("Previous rising of the selected object"), this, "previousRising()");
	actionsMgr->addAction("actionPrevious_Setting", timeGroup, N_("Previous setting of the selected object"), this, "previousSetting()");

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	setFlagSelectedObjectPointer(conf->value("viewing/flag_show_selection_marker", true).toBool());
}

void StelObjectMgr::nextTransit()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		core->addSolarDays(1.0);
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		core->setJD(static_cast<int>(JD) + static_cast<double>(rts[1]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::nextRising()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		core->addSolarDays(1.0);
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		if (rts[0]>-99.f && rts[0]<100.f)
			core->setJD(static_cast<int>(JD) + static_cast<double>(rts[0]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::nextSetting()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		core->addSolarDays(1.0);
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		if (rts[2]>-99.f && rts[2]<100.f)
			core->setJD(static_cast<int>(JD) + static_cast<double>(rts[2]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::previousTransit()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		core->addSolarDays(-1.0);
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		core->setJD(static_cast<int>(JD) + static_cast<double>(rts[1]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::previousRising()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		core->addSolarDays(-1.0);
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		if (rts[0]>-99.f && rts[0]<100.f)
			core->setJD(static_cast<int>(JD) + static_cast<double>(rts[0]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::previousSetting()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		core->addSolarDays(-1.0);
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		if (rts[2]>-99.f && rts[2]<100.f)
			core->setJD(static_cast<int>(JD) + static_cast<double>(rts[2]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::todayTransit()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		core->setJD(static_cast<int>(JD) + static_cast<double>(rts[1]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::todayRising()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		if (rts[0]>-99.f && rts[0]<100.f)
			core->setJD(static_cast<int>(JD) + static_cast<double>(rts[0]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

void StelObjectMgr::todaySetting()
{
	const QList<StelObjectP> selected = getSelectedObject();
	if (!selected.isEmpty() && selected[0]->getType()!="Satellite")
	{
		StelCore* core = StelApp::getInstance().getCore();
		double JD = core->getJD();
		Vec3f rts = selected[0]->getRTSTime(core);
		if (rts[2]>-99.f && rts[2]<100.f)
			core->setJD(static_cast<int>(JD) + static_cast<double>(rts[2]/24.f - core->getUTCOffset(JD) / 24.f + 0.5f));
	}
}

/*************************************************************************
 Add a new StelObject manager into the list of supported modules.
*************************************************************************/
void StelObjectMgr::registerStelObjectMgr(StelObjectModule* m)
{
	objectsModules.push_back(m);
	typeToModuleMap.insert(m->getStelObjectType(),m);

	objModulesMap.insert(m->objectName(), m->getName());

	//TODO: there should probably be a better way to specify the sub-types instead of hardcoding them here

	// Celestial objects from Solar system by type
	if (m->objectName()=="SolarSystem")
	{
		objModulesMap["SolarSystem:planet"] = "Planets";
		objModulesMap["SolarSystem:moon"] = "Moons";
		objModulesMap["SolarSystem:asteroid"] = "Asteroids";
		objModulesMap["SolarSystem:comet"] = "Comets";
		objModulesMap["SolarSystem:plutino"] = "Plutinos";
		objModulesMap["SolarSystem:cubewano"] = "Cubewanos";
		objModulesMap["SolarSystem:dwarf planet"] = "Dwarf planets";
		objModulesMap["SolarSystem:scattered disc object"] = "Scattered disc objects";
		objModulesMap["SolarSystem:Oort cloud object"] = "Oort cloud objects";
		objModulesMap["SolarSystem:sednoid"] = "Sednoids";
		objModulesMap["SolarSystem:interstellar object"] = "Interstellar objects";
		objModulesMap["SolarSystem:artificial"] = "Artificial objects";
	}
	// Deep-sky objects by type + amateur catalogues
	if (m->objectName()=="NebulaMgr")
	{
		objModulesMap["NebulaMgr:0"] = "Bright galaxies";
		objModulesMap["NebulaMgr:1"] = "Active galaxies";
		objModulesMap["NebulaMgr:2"] = "Radio galaxies";
		objModulesMap["NebulaMgr:3"] = "Interacting galaxies";
		objModulesMap["NebulaMgr:4"] = "Bright quasars";
		objModulesMap["NebulaMgr:5"] = "Star clusters";
		objModulesMap["NebulaMgr:6"] = "Open star clusters";
		objModulesMap["NebulaMgr:7"] = "Globular star clusters";
		objModulesMap["NebulaMgr:8"] = "Stellar associations";
		objModulesMap["NebulaMgr:9"] = "Star clouds";
		objModulesMap["NebulaMgr:10"] = "Nebulae";
		objModulesMap["NebulaMgr:11"] = "Planetary nebulae";
		objModulesMap["NebulaMgr:12"] = "Dark nebulae";
		objModulesMap["NebulaMgr:13"] = "Reflection nebulae";
		objModulesMap["NebulaMgr:14"] = "Bipolar nebulae";
		objModulesMap["NebulaMgr:15"] = "Emission nebulae";
		objModulesMap["NebulaMgr:16"] = "Clusters associated with nebulosity";
		objModulesMap["NebulaMgr:17"] = "HII regions";
		objModulesMap["NebulaMgr:18"] = "Supernova remnants";
		objModulesMap["NebulaMgr:19"] = "Interstellar matter";
		objModulesMap["NebulaMgr:20"] = "Emission objects";
		objModulesMap["NebulaMgr:21"] = "BL Lac objects";
		objModulesMap["NebulaMgr:22"] = "Blazars";
		objModulesMap["NebulaMgr:23"] = "Molecular Clouds";
		objModulesMap["NebulaMgr:24"] = "Young Stellar Objects";
		objModulesMap["NebulaMgr:25"] = "Possible Quasars";
		objModulesMap["NebulaMgr:26"] = "Possible Planetary Nebulae";
		objModulesMap["NebulaMgr:27"] = "Protoplanetary Nebulae";
		objModulesMap["NebulaMgr:29"] = "Symbiotic stars";
		objModulesMap["NebulaMgr:30"] = "Emission-line stars";
		objModulesMap["NebulaMgr:31"] = "Supernova candidates";
		objModulesMap["NebulaMgr:32"] = "Supernova remnant candidates";
		objModulesMap["NebulaMgr:33"] = "Clusters of galaxies";		
		objModulesMap["NebulaMgr:35"] = "Regions of the sky";
		objModulesMap["NebulaMgr:100"] = "Messier Catalogue";
		objModulesMap["NebulaMgr:101"] = "Caldwell Catalogue";
		objModulesMap["NebulaMgr:102"] = "Barnard Catalogue";
		objModulesMap["NebulaMgr:103"] = "Sharpless Catalogue";
		objModulesMap["NebulaMgr:104"] = "van den Bergh Catalogue";
		objModulesMap["NebulaMgr:105"] = "The Catalogue of Rodgers, Campbell, and Whiteoak";
		objModulesMap["NebulaMgr:106"] = "Collinder Catalogue";
		objModulesMap["NebulaMgr:107"] = "Melotte Catalogue";
		objModulesMap["NebulaMgr:108"] = "New General Catalogue";
		objModulesMap["NebulaMgr:109"] = "Index Catalogue";
		objModulesMap["NebulaMgr:110"] = "Lynds' Catalogue of Bright Nebulae";
		objModulesMap["NebulaMgr:111"] = "Lynds' Catalogue of Dark Nebulae";
		objModulesMap["NebulaMgr:112"] = "Principal Galaxy Catalog";
		objModulesMap["NebulaMgr:113"] = "The Uppsala General Catalogue of Galaxies";
		objModulesMap["NebulaMgr:114"] = "Cederblad Catalog";
		objModulesMap["NebulaMgr:115"] = "The Catalogue of Peculiar Galaxies";
		objModulesMap["NebulaMgr:116"] = "The Catalogue of Interacting Galaxies";
		objModulesMap["NebulaMgr:117"] = "The Catalogue of Galactic Planetary Nebulae";
		objModulesMap["NebulaMgr:118"] = "The Strasbourg-ESO Catalogue of Galactic Planetary Nebulae";
		objModulesMap["NebulaMgr:119"] = "A catalogue of Galactic supernova remnants";
		objModulesMap["NebulaMgr:120"] = "A Catalog of Rich Clusters of Galaxies";
		objModulesMap["NebulaMgr:121"] = "Hickson Compact Group";		
		objModulesMap["NebulaMgr:122"] = "ESO/Uppsala Survey of the ESO(B) Atlas";
		objModulesMap["NebulaMgr:123"] = "Catalogue of southern stars embedded in nebulosity";
		objModulesMap["NebulaMgr:124"] = "Catalogue and distances of optically visible H II regions";
		objModulesMap["NebulaMgr:125"] = "Trumpler Catalogue";
		objModulesMap["NebulaMgr:126"] = "Stock Catalogue";
		objModulesMap["NebulaMgr:127"] = "Ruprecht Catalogue";
		objModulesMap["NebulaMgr:128"] = "van den Bergh-Hagen Catalogue";
		objModulesMap["NebulaMgr:150"] = "Dwarf galaxies";
		objModulesMap["NebulaMgr:151"] = "Herschel 400 Catalogue";
		objModulesMap["NebulaMgr:152"] = "Jack Bennett's deep sky catalogue";
		objModulesMap["NebulaMgr:153"] = "James Dunlop's southern deep sky catalogue";
	}
	// Interesting stars
	if (m->objectName()=="StarMgr")
	{
		objModulesMap["StarMgr:0"] = "Interesting double stars";
		objModulesMap["StarMgr:1"] = "Interesting variable stars";
		objModulesMap["StarMgr:2"] = "Bright double stars";
		objModulesMap["StarMgr:3"] = "Bright variable stars";
		objModulesMap["StarMgr:4"] = "Bright stars with high proper motion";
	}
	// Nomenclature...
	if (m->objectName()=="NomenclatureMgr")
	{
		// list of types
		objModulesMap["NomenclatureMgr:1"]  = "Geological features: albedo features";
		objModulesMap["NomenclatureMgr:2"]  = "Geological features: arcūs";
		objModulesMap["NomenclatureMgr:3"]  = "Geological features: astra";
		objModulesMap["NomenclatureMgr:4"]  = "Geological features: catenae";
		objModulesMap["NomenclatureMgr:5"]  = "Geological features: cavi";
		objModulesMap["NomenclatureMgr:6"]  = "Geological features: chaoses";
		objModulesMap["NomenclatureMgr:7"]  = "Geological features: chasmata";
		objModulesMap["NomenclatureMgr:8"]  = "Geological features: colles";
		objModulesMap["NomenclatureMgr:9"]  = "Geological features: coronae";
		objModulesMap["NomenclatureMgr:10"] = "Geological features: craters";
		objModulesMap["NomenclatureMgr:11"] = "Geological features: dorsa";
		objModulesMap["NomenclatureMgr:12"] = "Geological features: eruptive centers";
		objModulesMap["NomenclatureMgr:13"] = "Geological features: faculae";
		objModulesMap["NomenclatureMgr:14"] = "Geological features: farra";
		objModulesMap["NomenclatureMgr:15"] = "Geological features: flexūs";
		objModulesMap["NomenclatureMgr:16"] = "Geological features: fluctūs";
		objModulesMap["NomenclatureMgr:17"] = "Geological features: flumina";
		objModulesMap["NomenclatureMgr:18"] = "Geological features: freta";
		objModulesMap["NomenclatureMgr:19"] = "Geological features: fossae";
		objModulesMap["NomenclatureMgr:20"] = "Geological features: insulae";
		objModulesMap["NomenclatureMgr:21"] = "Geological features: labēs";
		objModulesMap["NomenclatureMgr:22"] = "Geological features: labyrinthi";
		objModulesMap["NomenclatureMgr:23"] = "Geological features: lacunae";
		objModulesMap["NomenclatureMgr:24"] = "Geological features: lacūs";
		objModulesMap["NomenclatureMgr:25"] = "Geological features: large ringed features";
		objModulesMap["NomenclatureMgr:26"] = "Geological features: lineae";
		objModulesMap["NomenclatureMgr:27"] = "Geological features: lingulae";
		objModulesMap["NomenclatureMgr:28"] = "Geological features: maculae";
		objModulesMap["NomenclatureMgr:29"] = "Geological features: maria";
		objModulesMap["NomenclatureMgr:30"] = "Geological features: mensae";
		objModulesMap["NomenclatureMgr:31"] = "Geological features: montes";
		objModulesMap["NomenclatureMgr:32"] = "Geological features: oceani";
		objModulesMap["NomenclatureMgr:33"] = "Geological features: paludes";
		objModulesMap["NomenclatureMgr:34"] = "Geological features: paterae";
		objModulesMap["NomenclatureMgr:35"] = "Geological features: planitiae";
		objModulesMap["NomenclatureMgr:36"] = "Geological features: plana";
		objModulesMap["NomenclatureMgr:37"] = "Geological features: plumes";
		objModulesMap["NomenclatureMgr:38"] = "Geological features: promontoria";
		objModulesMap["NomenclatureMgr:39"] = "Geological features: regiones";
		objModulesMap["NomenclatureMgr:40"] = "Geological features: rimae";
		objModulesMap["NomenclatureMgr:41"] = "Geological features: rupēs";
		objModulesMap["NomenclatureMgr:42"] = "Geological features: scopuli";
		objModulesMap["NomenclatureMgr:43"] = "Geological features: serpentes";
		objModulesMap["NomenclatureMgr:44"] = "Geological features: sulci";
		objModulesMap["NomenclatureMgr:45"] = "Geological features: sinūs";
		objModulesMap["NomenclatureMgr:46"] = "Geological features: terrae";
		objModulesMap["NomenclatureMgr:47"] = "Geological features: tholi";
		objModulesMap["NomenclatureMgr:48"] = "Geological features: undae";
		objModulesMap["NomenclatureMgr:49"] = "Geological features: valles";
		objModulesMap["NomenclatureMgr:50"] = "Geological features: vastitates";
		objModulesMap["NomenclatureMgr:51"] = "Geological features: virgae";
		objModulesMap["NomenclatureMgr:52"] = "Geological features: landing sites";
		objModulesMap["NomenclatureMgr:53"] = "Geological features: lenticulae";
		objModulesMap["NomenclatureMgr:54"] = "Geological features: reticula";
		objModulesMap["NomenclatureMgr:56"] = "Geological features: tesserae";
		objModulesMap["NomenclatureMgr:57"] = "Geological features: saxa";
		// list of celestial bodies (alphabetical sorting)
		objModulesMap["NomenclatureMgr:Amalthea"]   = "Named geological features of Amalthea";
		objModulesMap["NomenclatureMgr:Ariel"]      = "Named geological features of Ariel";
		objModulesMap["NomenclatureMgr:Callisto"]   = "Named geological features of Callisto";
		objModulesMap["NomenclatureMgr:Ceres"]      = "Named geological features of Ceres";
		objModulesMap["NomenclatureMgr:Charon"]     = "Named geological features of Charon";
		objModulesMap["NomenclatureMgr:Dactyl"]     = "Named geological features of Dactyl";
		objModulesMap["NomenclatureMgr:Deimos"]     = "Named geological features of Deimos";
		objModulesMap["NomenclatureMgr:Dione"]      = "Named geological features of Dione";
		objModulesMap["NomenclatureMgr:Enceladus"]  = "Named geological features of Enceladus";
		objModulesMap["NomenclatureMgr:Epimetheus"] = "Named geological features of Epimetheus";
		objModulesMap["NomenclatureMgr:Eros"]       = "Named geological features of Eros";
		objModulesMap["NomenclatureMgr:Europa"]     = "Named geological features of Europa";
		objModulesMap["NomenclatureMgr:Ganymede"]   = "Named geological features of Ganymede";
		objModulesMap["NomenclatureMgr:Gaspra"]     = "Named geological features of Gaspra";
		objModulesMap["NomenclatureMgr:Hyperion"]   = "Named geological features of Hyperion";
		objModulesMap["NomenclatureMgr:Iapetus"]    = "Named geological features of Iapetus";
		objModulesMap["NomenclatureMgr:Ida"]        = "Named geological features of Ida";
		objModulesMap["NomenclatureMgr:Io"]         = "Named geological features of Io";
		objModulesMap["NomenclatureMgr:Itokawa"]    = "Named geological features of Itokawa";
		objModulesMap["NomenclatureMgr:Janus"]      = "Named geological features of Janus";
		objModulesMap["NomenclatureMgr:Lutetia"]    = "Named geological features of Lutetia";
		objModulesMap["NomenclatureMgr:Mars"]       = "Named geological features of Mars";
		objModulesMap["NomenclatureMgr:Mathilde"]   = "Named geological features of Mathilde";
		objModulesMap["NomenclatureMgr:Mercury"]    = "Named geological features of Mercury";
		objModulesMap["NomenclatureMgr:Mimas"]      = "Named geological features of Mimas";
		objModulesMap["NomenclatureMgr:Miranda"]    = "Named geological features of Miranda";
		objModulesMap["NomenclatureMgr:Moon"]       = "Named geological features of the Moon";
		objModulesMap["NomenclatureMgr:Oberon"]     = "Named geological features of Oberon";
		objModulesMap["NomenclatureMgr:Phobos"]     = "Named geological features of Phobos";
		objModulesMap["NomenclatureMgr:Phoebe"]     = "Named geological features of Phoebe";
		objModulesMap["NomenclatureMgr:Pluto"]      = "Named geological features of Pluto";
		objModulesMap["NomenclatureMgr:Proteus"]    = "Named geological features of Proteus";
		objModulesMap["NomenclatureMgr:Puck"]       = "Named geological features of Puck";
		objModulesMap["NomenclatureMgr:Rhea"]       = "Named geological features of Rhea";
		objModulesMap["NomenclatureMgr:Ryugu"]      = "Named geological features of Ryugu";
		objModulesMap["NomenclatureMgr:Steins"]     = "Named geological features of Steins";
		objModulesMap["NomenclatureMgr:Tethys"]     = "Named geological features of Tethys";
		objModulesMap["NomenclatureMgr:Thebe"]      = "Named geological features of Thebe";
		objModulesMap["NomenclatureMgr:Titania"]    = "Named geological features of Titania";
		objModulesMap["NomenclatureMgr:Titan"]      = "Named geological features of Titan";
		objModulesMap["NomenclatureMgr:Triton"]     = "Named geological features of Triton";
		objModulesMap["NomenclatureMgr:Umbriel"]    = "Named geological features of Umbriel";
		objModulesMap["NomenclatureMgr:Venus"]      = "Named geological features of Venus";
		objModulesMap["NomenclatureMgr:Vesta"]      = "Named geological features of Vesta";
	}
}

StelObjectP StelObjectMgr::searchByNameI18n(const QString &name) const
{
	StelObjectP rval;
	for (const auto* m : objectsModules)
	{
		rval = m->searchByNameI18n(name);
		if (rval)
			return rval;
	}
	return rval;
}

//! Find any kind of object by its standard program name
StelObjectP StelObjectMgr::searchByName(const QString &name) const
{
	StelObjectP rval;
	for (const auto* m : objectsModules)
	{
		rval = m->searchByName(name);
		if (rval)
			return rval;
	}
	return rval;
}

StelObjectP StelObjectMgr::searchByID(const QString &type, const QString &id) const
{
	auto it = typeToModuleMap.constFind(type);
	if(it!=typeToModuleMap.constEnd())
	{
		return (*it)->searchByID(id);
	}
	qWarning()<<"StelObject type"<<type<<"unknown";
	return Q_NULLPTR;
}

//! Find and select an object from its translated name
//! @param nameI18n the case sensitive object translated name
//! @return true if an object was found with the passed name
bool StelObjectMgr::findAndSelectI18n(const QString &nameI18n, StelModule::StelModuleSelectAction action)
{
	// Then look for another object
	StelObjectP obj = searchByNameI18n(nameI18n);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj, action);
}

//! Find and select an object from its standard program name
bool StelObjectMgr::findAndSelect(const QString &name, StelModule::StelModuleSelectAction action)
{
	// Then look for another object
	StelObjectP obj = searchByName(name);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj, action);
}


//! Find and select an object near given equatorial position
bool StelObjectMgr::findAndSelect(const StelCore* core, const Vec3d& pos, StelModule::StelModuleSelectAction action)
{
	StelObjectP tempselect = cleverFind(core, pos);
	return setSelectedObject(tempselect, action);
}

//! Find and select an object near given screen position
bool StelObjectMgr::findAndSelect(const StelCore* core, int x, int y, StelModule::StelModuleSelectAction action)
{
	StelObjectP tempselect = cleverFind(core, x, y);
	return setSelectedObject(tempselect, action);
}

// Find an object in a "clever" way, v in J2000 frame
StelObjectP StelObjectMgr::cleverFind(const StelCore* core, const Vec3d& v) const
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	// Field of view for a searchRadiusPixel pixel diameter circle on screen
	const double fov_around = core->getMovementMgr()->getCurrentFov()/qMin(prj->getViewportWidth(), prj->getViewportHeight()) * searchRadiusPixel;

	// Collect the objects inside the range
	QList<StelObjectP> candidates;
	for (const auto* m : objectsModules)
		candidates += m->searchAround(v, fov_around, core);

	// This should be exactly the sky's limit magnitude, else visible stars cannot be clicked, or suppressed stars can be found.
	const float limitMag = core->getSkyDrawer()->getFlagStarMagnitudeLimit() ?
				static_cast<float>(core->getSkyDrawer()->getCustomStarMagnitudeLimit()) :
				core->getSkyDrawer()->getLimitMagnitude();
	QList<StelObjectP> tmp;
	for (const auto& obj : candidates)
	{
		if (obj->getSelectPriority(core)<=limitMag)
			tmp.append(obj);
	}
	candidates = tmp;
	
	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	Vec3d winpos;
	prj->project(v, winpos);
	const double xpos = winpos[0];
	const double ypos = winpos[1];

	StelObjectP sobj;
	float best_object_value = 100000.f;
	for (const auto& obj : candidates)
	{
		prj->project(obj->getJ2000EquatorialPos(core), winpos);
		float distance = static_cast<float>(std::sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1])))*distanceWeight;
		float priority =  obj->getSelectPriority(core);
		// qDebug() << (*iter).getShortInfoString(core) << ": " << priority << " " << distance;
		if (distance + priority < best_object_value)
		{
			best_object_value = distance + priority;
			sobj = obj;
		}
	}

	return sobj;
}

/*************************************************************************
 Find in a "clever" way an object from its equatorial position
*************************************************************************/
StelObjectP StelObjectMgr::cleverFind(const StelCore* core, int x, int y) const
{
	Vec3d v;
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	if (prj->unProject(x,y,v))
	{
		// Nick Fedoseev patch: improve click match for refracted coordinates
		Vec3d win;
		prj->project(v,win);
		const double dx = x - win.v[0];
		const double dy = y - win.v[1];
		prj->unProject(x+dx, y+dy, v);

		return cleverFind(core, v);
	}
	return StelObjectP();
}

/*************************************************************************
 Notify that we want to unselect any object
*************************************************************************/
void StelObjectMgr::unSelect(void)
{
	if(!lastSelectedObjects.isEmpty())
	{
		lastSelectedObjects.clear();
		emit(selectedObjectChanged(StelModule::RemoveFromSelection));
	}
}

/*************************************************************************
 Notify that we want to select the given object
*************************************************************************/
bool StelObjectMgr::setSelectedObject(const StelObjectP obj, StelModule::StelModuleSelectAction action)
{
	if (!obj)
	{
		unSelect();
		return false;
	}

	// An object has been found
	QList<StelObjectP> objs;
	objs.push_back(obj);
	return setSelectedObject(objs, action);
}

/*************************************************************************
 Notify that we want to select the given objects
*************************************************************************/
bool StelObjectMgr::setSelectedObject(const QList<StelObjectP>& objs, StelModule::StelModuleSelectAction action)
{
	if (action==StelModule::AddToSelection)
		lastSelectedObjects.append(objs);
	else
		lastSelectedObjects = objs;
	emit(selectedObjectChanged(action));
	return true;
}

/*************************************************************************
 Return the list objects of type "type" which was recently selected by
  the user
*************************************************************************/
QList<StelObjectP> StelObjectMgr::getSelectedObject(const QString& type) const
{
	QList<StelObjectP> result;
	for (const auto& obj : lastSelectedObjects)
	{
		if (obj->getType() == type)
			result.push_back(obj);
	}
	return result;
}

/*****************************************************************************************
 Find and return the list of at most maxNbItem objects auto-completing passed object name
*******************************************************************************************/
QStringList StelObjectMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem <= 0)
		return result;

	// For all StelObjectmodules..
	for (const auto* m : objectsModules)
	{
		// Get matching object for this module
		QStringList matchingObj = m->listMatchingObjects(objPrefix, maxNbItem, useStartOfWords);
		result += matchingObj;
	}

	result.sort();
	return result;
}

QStringList StelObjectMgr::listAllModuleObjects(const QString &moduleId, bool inEnglish) const
{
	// search for module
	StelObjectModule* module = Q_NULLPTR;
	QStringList result, list;
	QString objModule, objType;
	bool subSet = false;
	if (moduleId.contains(":"))
	{
		subSet = true;
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		list = moduleId.split(":", Qt::SkipEmptyParts);
		#else
		list = moduleId.split(":", QString::SkipEmptyParts);
		#endif
		objModule = list.at(0);
		objType = list.at(1);
	}
	else
		objModule = moduleId;
	for (auto* m : objectsModules)
	{
		if (m->objectName() == objModule)
		{
			module = m;
			break;
		}
	}
	if (module == Q_NULLPTR)
	{
		qWarning() << "Can't find module with id " << objModule;
		return QStringList();
	}
	if (subSet)
		result = module->listAllObjectsByType(objType, inEnglish);
	else
		result = module->listAllObjects(inEnglish);

	return result;
}

QMap<QString, QString> StelObjectMgr::objectModulesMap() const
{
	return objModulesMap;
}

QVariantMap StelObjectMgr::getObjectInfo(const StelObjectP obj)
{
	QVariantMap map;
	if (!obj)
	{
		qDebug() << "getObjectInfo WARNING - object not found";
		map.insert("found", false);
	}
	else
	{
		map=obj->getInfoMap(StelApp::getInstance().getCore());
		map.insert("found", true);
	}
	return map;
}
