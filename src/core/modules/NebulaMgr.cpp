/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2015 Georg Zotti
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

// class used to manage groups of Nebulas

#include "StelApp.hpp"
#include "NebulaList.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyImageTile.hpp"
#include "StelPainter.hpp"
#include "RefractionExtinction.hpp"
#include "StelActionMgr.hpp"

#include <algorithm>
#include <vector>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QDir>

// Define version of valid Stellarium DSO Catalog
// This number must be incremented each time the content or file format of the stars catalogs change
static const QString StellariumDSOCatalogVersion = "3.8";

void NebulaMgr::setLabelsColor(const Vec3f& c) {Nebula::labelColor = c; emit labelsColorChanged(c);}
const Vec3f NebulaMgr::getLabelsColor(void) const {return Nebula::labelColor;}
void NebulaMgr::setCirclesColor(const Vec3f& c) {Nebula::circleColor = c; emit circlesColorChanged(c); }
const Vec3f NebulaMgr::getCirclesColor(void) const {return Nebula::circleColor;}
void NebulaMgr::setGalaxyColor(const Vec3f& c) {Nebula::galaxyColor = c; emit galaxiesColorChanged(c); }
const Vec3f NebulaMgr::getGalaxyColor(void) const {return Nebula::galaxyColor;}
void NebulaMgr::setRadioGalaxyColor(const Vec3f& c) {Nebula::radioGalaxyColor = c; emit radioGalaxiesColorChanged(c); }
const Vec3f NebulaMgr::getRadioGalaxyColor(void) const {return Nebula::radioGalaxyColor;}
void NebulaMgr::setActiveGalaxyColor(const Vec3f& c) {Nebula::activeGalaxyColor = c; emit activeGalaxiesColorChanged(c); }
const Vec3f NebulaMgr::getActiveGalaxyColor(void) const {return Nebula::activeGalaxyColor;}
void NebulaMgr::setInteractingGalaxyColor(const Vec3f& c) {Nebula::interactingGalaxyColor = c; emit interactingGalaxiesColorChanged(c); }
const Vec3f NebulaMgr::getInteractingGalaxyColor(void) const {return Nebula::interactingGalaxyColor;}
void NebulaMgr::setQuasarColor(const Vec3f& c) {Nebula::quasarColor = c; emit quasarsColorChanged(c); }
const Vec3f NebulaMgr::getQuasarColor(void) const {return Nebula::quasarColor;}
void NebulaMgr::setNebulaColor(const Vec3f& c) {Nebula::nebulaColor = c; emit nebulaeColorChanged(c); }
const Vec3f NebulaMgr::getNebulaColor(void) const {return Nebula::nebulaColor;}
void NebulaMgr::setPlanetaryNebulaColor(const Vec3f& c) {Nebula::planetaryNebulaColor = c; emit planetaryNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getPlanetaryNebulaColor(void) const {return Nebula::planetaryNebulaColor;}
void NebulaMgr::setReflectionNebulaColor(const Vec3f& c) {Nebula::reflectionNebulaColor = c; emit reflectionNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getReflectionNebulaColor(void) const {return Nebula::reflectionNebulaColor;}
void NebulaMgr::setBipolarNebulaColor(const Vec3f& c) {Nebula::bipolarNebulaColor = c; emit bipolarNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getBipolarNebulaColor(void) const {return Nebula::bipolarNebulaColor;}
void NebulaMgr::setEmissionNebulaColor(const Vec3f& c) {Nebula::emissionNebulaColor = c; emit emissionNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getEmissionNebulaColor(void) const {return Nebula::emissionNebulaColor;}
void NebulaMgr::setDarkNebulaColor(const Vec3f& c) {Nebula::darkNebulaColor= c; emit darkNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getDarkNebulaColor(void) const {return Nebula::darkNebulaColor;}
void NebulaMgr::setHydrogenRegionColor(const Vec3f& c) {Nebula::hydrogenRegionColor= c; emit hydrogenRegionsColorChanged(c);}
const Vec3f NebulaMgr::getHydrogenRegionColor(void) const {return Nebula::hydrogenRegionColor;}
void NebulaMgr::setSupernovaRemnantColor(const Vec3f& c) {Nebula::supernovaRemnantColor = c; emit supernovaRemnantsColorChanged(c);}
const Vec3f NebulaMgr::getSupernovaRemnantColor(void) const {return Nebula::supernovaRemnantColor;}
void NebulaMgr::setSupernovaCandidateColor(const Vec3f& c) {Nebula::supernovaCandidateColor = c; emit supernovaCandidatesColorChanged(c);}
const Vec3f NebulaMgr::getSupernovaCandidateColor(void) const {return Nebula::supernovaCandidateColor;}
void NebulaMgr::setSupernovaRemnantCandidateColor(const Vec3f& c) {Nebula::supernovaRemnantCandidateColor = c; emit supernovaRemnantCandidatesColorChanged(c);}
const Vec3f NebulaMgr::getSupernovaRemnantCandidateColor(void) const {return Nebula::supernovaRemnantCandidateColor;}
void NebulaMgr::setInterstellarMatterColor(const Vec3f& c) {Nebula::interstellarMatterColor = c; emit interstellarMatterColorChanged(c);}
const Vec3f NebulaMgr::getInterstellarMatterColor(void) const {return Nebula::interstellarMatterColor;}
void NebulaMgr::setClusterWithNebulosityColor(const Vec3f& c) {Nebula::clusterWithNebulosityColor= c; emit clusterWithNebulosityColorChanged(c);}
const Vec3f NebulaMgr::getClusterWithNebulosityColor(void) const {return Nebula::clusterWithNebulosityColor;}
void NebulaMgr::setClusterColor(const Vec3f& c) {Nebula::clusterColor= c; emit clustersColorChanged(c);}
const Vec3f NebulaMgr::getClusterColor(void) const {return Nebula::clusterColor;}
void NebulaMgr::setOpenClusterColor(const Vec3f& c) {Nebula::openClusterColor = c; emit openClustersColorChanged(c);}
const Vec3f NebulaMgr::getOpenClusterColor(void) const {return Nebula::openClusterColor;}
void NebulaMgr::setGlobularClusterColor(const Vec3f& c) {Nebula::globularClusterColor = c; emit globularClustersColorChanged(c);}
const Vec3f NebulaMgr::getGlobularClusterColor(void) const {return Nebula::globularClusterColor;}
void NebulaMgr::setStellarAssociationColor(const Vec3f& c) {Nebula::stellarAssociationColor = c; emit stellarAssociationsColorChanged(c);}
const Vec3f NebulaMgr::getStellarAssociationColor(void) const {return Nebula::stellarAssociationColor;}
void NebulaMgr::setStarCloudColor(const Vec3f& c) {Nebula::starCloudColor = c; emit starCloudsColorChanged(c);}
const Vec3f NebulaMgr::getStarCloudColor(void) const {return Nebula::starCloudColor;}
void NebulaMgr::setEmissionObjectColor(const Vec3f& c) {Nebula::emissionObjectColor = c; emit emissionObjectsColorChanged(c);}
const Vec3f NebulaMgr::getEmissionObjectColor(void) const {return Nebula::emissionObjectColor;}
void NebulaMgr::setBlLacObjectColor(const Vec3f& c) {Nebula::blLacObjectColor = c; emit blLacObjectsColorChanged(c);}
const Vec3f NebulaMgr::getBlLacObjectColor(void) const {return Nebula::blLacObjectColor;}
void NebulaMgr::setBlazarColor(const Vec3f& c) {Nebula::blazarColor = c; emit blazarsColorChanged(c);}
const Vec3f NebulaMgr::getBlazarColor(void) const {return Nebula::blazarColor;}
void NebulaMgr::setMolecularCloudColor(const Vec3f& c) {Nebula::molecularCloudColor = c; emit molecularCloudsColorChanged(c);}
const Vec3f NebulaMgr::getMolecularCloudColor(void) const {return Nebula::molecularCloudColor;}
void NebulaMgr::setYoungStellarObjectColor(const Vec3f& c) {Nebula::youngStellarObjectColor = c; emit youngStellarObjectsColorChanged(c);}
const Vec3f NebulaMgr::getYoungStellarObjectColor(void) const {return Nebula::youngStellarObjectColor;}
void NebulaMgr::setPossibleQuasarColor(const Vec3f& c) {Nebula::possibleQuasarColor = c; emit possibleQuasarsColorChanged(c);}
const Vec3f NebulaMgr::getPossibleQuasarColor(void) const {return Nebula::possibleQuasarColor;}
void NebulaMgr::setPossiblePlanetaryNebulaColor(const Vec3f& c) {Nebula::possiblePlanetaryNebulaColor = c; emit possiblePlanetaryNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getPossiblePlanetaryNebulaColor(void) const {return Nebula::possiblePlanetaryNebulaColor;}
void NebulaMgr::setProtoplanetaryNebulaColor(const Vec3f& c) {Nebula::protoplanetaryNebulaColor = c; emit protoplanetaryNebulaeColorChanged(c);}
const Vec3f NebulaMgr::getProtoplanetaryNebulaColor(void) const {return Nebula::protoplanetaryNebulaColor;}
void NebulaMgr::setStarColor(const Vec3f& c) {Nebula::starColor = c; emit starsColorChanged(c);}
const Vec3f NebulaMgr::getStarColor(void) const {return Nebula::starColor;}
void NebulaMgr::setSymbioticStarColor(const Vec3f& c) {Nebula::symbioticStarColor = c; emit symbioticStarsColorChanged(c);}
const Vec3f NebulaMgr::getSymbioticStarColor(void) const {return Nebula::symbioticStarColor;}
void NebulaMgr::setEmissionLineStarColor(const Vec3f& c) {Nebula::emissionLineStarColor = c; emit emissionLineStarsColorChanged(c);}
const Vec3f NebulaMgr::getEmissionLineStarColor(void) const {return Nebula::emissionLineStarColor;}
void NebulaMgr::setGalaxyClusterColor(const Vec3f& c) {Nebula::galaxyClusterColor = c; emit galaxyClustersColorChanged(c);}
const Vec3f NebulaMgr::getGalaxyClusterColor(void) const {return Nebula::galaxyClusterColor;}
void NebulaMgr::setHintsProportional(const bool proportional) {if(Nebula::drawHintProportional!=proportional){ Nebula::drawHintProportional=proportional; emit hintsProportionalChanged(proportional);}}
bool NebulaMgr::getHintsProportional(void) const {return Nebula::drawHintProportional;}
void NebulaMgr::setDesignationUsage(const bool flag) {if(Nebula::designationUsage!=flag){ Nebula::designationUsage=flag; emit designationUsageChanged(flag);}}
bool NebulaMgr::getDesignationUsage(void) const {return Nebula::designationUsage; }
void NebulaMgr::setFlagOutlines(const bool flag) {if(Nebula::flagUseOutlines!=flag){ Nebula::flagUseOutlines=flag; emit flagOutlinesDisplayedChanged(flag);}}
bool NebulaMgr::getFlagOutlines(void) const {return Nebula::flagUseOutlines;}
void NebulaMgr::setFlagAdditionalNames(const bool flag) {if(Nebula::flagShowAdditionalNames!=flag){ Nebula::flagShowAdditionalNames=flag; emit flagAdditionalNamesDisplayedChanged(flag);}}
bool NebulaMgr::getFlagAdditionalNames(void) const {return Nebula::flagShowAdditionalNames;}

NebulaMgr::NebulaMgr(void)
	: nebGrid(200)
	, hintsAmount(0)
	, labelsAmount(0)
	, flagConverter(false)
	, flagDecimalCoordinates(true)
{
	setObjectName("NebulaMgr");
}

NebulaMgr::~NebulaMgr()
{
	Nebula::texCircle = StelTextureSP();
	Nebula::texCircleLarge = StelTextureSP();
	Nebula::texGalaxy = StelTextureSP();
	Nebula::texGalaxyLarge = StelTextureSP();
	Nebula::texOpenCluster = StelTextureSP();
	Nebula::texOpenClusterLarge = StelTextureSP();
	Nebula::texOpenClusterXLarge = StelTextureSP();
	Nebula::texGlobularCluster = StelTextureSP();
	Nebula::texGlobularClusterLarge = StelTextureSP();
	Nebula::texPlanetaryNebula = StelTextureSP();
	Nebula::texDiffuseNebula = StelTextureSP();
	Nebula::texDiffuseNebulaLarge = StelTextureSP();
	Nebula::texDiffuseNebulaXLarge = StelTextureSP();
	Nebula::texDarkNebula = StelTextureSP();
	Nebula::texDarkNebulaLarge = StelTextureSP();
	Nebula::texOpenClusterWithNebulosity = StelTextureSP();
	Nebula::texOpenClusterWithNebulosityLarge = StelTextureSP();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NebulaMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MilkyWay")->getCallOrder(actionName)+10;
	return 0;
}

// read from stream
void NebulaMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	nebulaFont.setPixelSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), SLOT(setFontSizeFromApp(int)));
	// Load circle texture
	Nebula::texCircle		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb.png");
	// Load circle texture for large DSO
	Nebula::texCircleLarge		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_lrg.png");
	// Load ellipse texture
	Nebula::texGalaxy		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_gal.png");
	// Load ellipse texture for large galaxies
	Nebula::texGalaxyLarge		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_gal_lrg.png");
	// Load open cluster marker texture
	Nebula::texOpenCluster		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocl.png");
	// Load open cluster marker texture for large objects
	Nebula::texOpenClusterLarge	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocl_lrg.png");
	// Load open cluster marker texture for extra large objects
	Nebula::texOpenClusterXLarge	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocl_xlrg.png");
	// Load globular cluster marker texture
	Nebula::texGlobularCluster	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_gcl.png");
	// Load globular cluster marker texture for large GCls
	Nebula::texGlobularClusterLarge	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_gcl_lrg.png");
	// Load planetary nebula marker texture
	Nebula::texPlanetaryNebula	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_pnb.png");
	// Load diffuse nebula marker texture
	Nebula::texDiffuseNebula	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_dif.png");
	// Load diffuse nebula marker texture for large DSO
	Nebula::texDiffuseNebulaLarge	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_dif_lrg.png");
	// Load diffuse nebula marker texture for extra large DSO
	Nebula::texDiffuseNebulaXLarge	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_dif_xlrg.png");
	// Load dark nebula marker texture
	Nebula::texDarkNebula		= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_drk.png");
	// Load dark nebula marker texture for large DSO
	Nebula::texDarkNebulaLarge	= StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_drk_lrg.png");
	// Load Ocl/Nebula marker texture
	Nebula::texOpenClusterWithNebulosity = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocln.png");
	// Load Ocl/Nebula marker texture for large objects
	Nebula::texOpenClusterWithNebulosityLarge = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/neb_ocln_lrg.png");
	// Load pointer texture
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");

	setFlagShow(conf->value("astro/flag_nebula",true).toBool());
	setFlagHints(conf->value("astro/flag_nebula_name",false).toBool());
	setHintsAmount(conf->value("astro/nebula_hints_amount", 3.0).toDouble());
	setLabelsAmount(conf->value("astro/nebula_labels_amount", 3.0).toDouble());
	setHintsProportional(conf->value("astro/flag_nebula_hints_proportional", false).toBool());
	setFlagOutlines(conf->value("astro/flag_dso_outlines_usage", false).toBool());
	setFlagAdditionalNames(conf->value("astro/flag_dso_additional_names",true).toBool());
	setDesignationUsage(conf->value("astro/flag_dso_designation_usage", false).toBool());
	setFlagSurfaceBrightnessUsage(conf->value("astro/flag_surface_brightness_usage", false).toBool());
	setFlagSurfaceBrightnessArcsecUsage(conf->value("gui/flag_surface_brightness_arcsec", false).toBool());
	setFlagSurfaceBrightnessShortNotationUsage(conf->value("gui/flag_surface_brightness_short", false).toBool());

	setFlagSizeLimitsUsage(conf->value("astro/flag_size_limits_usage", false).toBool());
	setMinSizeLimit(conf->value("astro/size_limit_min", 1.0).toDouble());
	setMaxSizeLimit(conf->value("astro/size_limit_max", 600.0).toDouble());

	// Load colors from config file
	// Upgrade config keys
	if (conf->contains("color/nebula_label_color"))
	{
		conf->setValue("color/dso_label_color", conf->value("color/nebula_label_color", "0.4,0.3,0.5").toString());
		conf->remove("color/nebula_label_color");
	}
	if (conf->contains("color/nebula_circle_color"))
	{
		conf->setValue("color/dso_circle_color", conf->value("color/nebula_circle_color", "0.8,0.8,0.1").toString());
		conf->remove("color/nebula_circle_color");
	}
	if (conf->contains("color/nebula_galaxy_color"))
	{
		conf->setValue("color/dso_galaxy_color", conf->value("color/nebula_galaxy_color", "1.0,0.2,0.2").toString());
		conf->remove("color/nebula_galaxy_color");
	}
	if (conf->contains("color/nebula_radioglx_color"))
	{
		conf->setValue("color/dso_radio_galaxy_color", conf->value("color/nebula_radioglx_color", "0.3,0.3,0.3").toString());
		conf->remove("color/nebula_radioglx_color");
	}
	if (conf->contains("color/nebula_activeglx_color"))
	{
		conf->setValue("color/dso_active_galaxy_color", conf->value("color/nebula_activeglx_color", "1.0,0.5,0.2").toString());
		conf->remove("color/nebula_activeglx_color");
	}
	if (conf->contains("color/nebula_intglx_color"))
	{
		conf->setValue("color/dso_interacting_galaxy_color", conf->value("color/nebula_intglx_color", "0.2,0.5,1.0").toString());
		conf->remove("color/nebula_intglx_color");
	}
	if (conf->contains("color/nebula_brightneb_color"))
	{
		conf->setValue("color/dso_nebula_color", conf->value("color/nebula_brightneb_color", "0.1,1.0,0.1").toString());
		conf->remove("color/nebula_brightneb_color");
	}
	if (conf->contains("color/nebula_darkneb_color"))
	{
		conf->setValue("color/dso_dark_nebula_color", conf->value("color/nebula_darkneb_color", "0.3,0.3,0.3").toString());
		conf->remove("color/nebula_darkneb_color");
	}
	if (conf->contains("color/nebula_hregion_color"))
	{
		conf->setValue("color/dso_hydrogen_region_color", conf->value("color/nebula_hregion_color", "0.1,1.0,0.1").toString());
		conf->remove("color/nebula_hregion_color");
	}
	if (conf->contains("color/nebula_snr_color"))
	{
		conf->setValue("color/dso_supernova_remnant_color", conf->value("color/nebula_snr_color", "0.1,1.0,0.1").toString());
		conf->remove("color/nebula_snr_color");
	}
	if (conf->contains("color/nebula_cluster_color"))
	{
		conf->setValue("color/dso_cluster_color", conf->value("color/nebula_cluster_color", "0.8,0.8,0.1").toString());
		conf->remove("color/nebula_cluster_color");
	}

	// Set colors for markers
	setLabelsColor(StelUtils::strToVec3f(conf->value("color/dso_label_color", "0.2,0.6,0.7").toString()));
	setCirclesColor(StelUtils::strToVec3f(conf->value("color/dso_circle_color", "1.0,0.7,0.2").toString()));

	QString defaultGalaxyColor = conf->value("color/dso_galaxy_color", "1.0,0.2,0.2").toString();
	setGalaxyColor(StelUtils::strToVec3f(defaultGalaxyColor));
	setRadioGalaxyColor(StelUtils::strToVec3f(conf->value("color/dso_radio_galaxy_color", "0.3,0.3,0.3").toString()));
	setActiveGalaxyColor(StelUtils::strToVec3f(conf->value("color/dso_active_galaxy_color", "1.0,0.5,0.2").toString()));
	setInteractingGalaxyColor(StelUtils::strToVec3f(conf->value("color/dso_interacting_galaxy_color", "0.2,0.5,1.0").toString()));
	setGalaxyClusterColor(StelUtils::strToVec3f(conf->value("color/dso_galaxy_cluster_color", "0.2,0.8,1.0").toString()));
	setQuasarColor(StelUtils::strToVec3f(conf->value("color/dso_quasar_color", defaultGalaxyColor).toString()));
	setPossibleQuasarColor(StelUtils::strToVec3f(conf->value("color/dso_possible_quasar_color", defaultGalaxyColor).toString()));
	setBlLacObjectColor(StelUtils::strToVec3f(conf->value("color/dso_bl_lac_color", defaultGalaxyColor).toString()));
	setBlazarColor(StelUtils::strToVec3f(conf->value("color/dso_blazar_color", defaultGalaxyColor).toString()));

	QString defaultNebulaColor = conf->value("color/dso_nebula_color", "0.1,1.0,0.1").toString();
	setNebulaColor(StelUtils::strToVec3f(defaultNebulaColor));
	setPlanetaryNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_planetary_nebula_color", defaultNebulaColor).toString()));
	setReflectionNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_reflection_nebula_color", defaultNebulaColor).toString()));
	setBipolarNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_bipolar_nebula_color", defaultNebulaColor).toString()));
	setEmissionNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_emission_nebula_color", defaultNebulaColor).toString()));
	setDarkNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_dark_nebula_color", "0.3,0.3,0.3").toString()));
	setHydrogenRegionColor(StelUtils::strToVec3f(conf->value("color/dso_hydrogen_region_color", defaultNebulaColor).toString()));
	setSupernovaRemnantColor(StelUtils::strToVec3f(conf->value("color/dso_supernova_remnant_color", defaultNebulaColor).toString()));
	setSupernovaCandidateColor(StelUtils::strToVec3f(conf->value("color/dso_supernova_candidate_color", defaultNebulaColor).toString()));
	setSupernovaRemnantCandidateColor(StelUtils::strToVec3f(conf->value("color/dso_supernova_remnant_cand_color", defaultNebulaColor).toString()));
	setInterstellarMatterColor(StelUtils::strToVec3f(conf->value("color/dso_interstellar_matter_color", defaultNebulaColor).toString()));
	setClusterWithNebulosityColor(StelUtils::strToVec3f(conf->value("color/dso_cluster_with_nebulosity_color", defaultNebulaColor).toString()));
	setMolecularCloudColor(StelUtils::strToVec3f(conf->value("color/dso_molecular_cloud_color", defaultNebulaColor).toString()));
	setPossiblePlanetaryNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_possible_planetary_nebula_color", defaultNebulaColor).toString()));
	setProtoplanetaryNebulaColor(StelUtils::strToVec3f(conf->value("color/dso_protoplanetary_nebula_color", defaultNebulaColor).toString()));

	QString defaultClusterColor = conf->value("color/dso_cluster_color", "1.0,1.0,0.1").toString();
	setClusterColor(StelUtils::strToVec3f(defaultClusterColor));
	setOpenClusterColor(StelUtils::strToVec3f(conf->value("color/dso_open_cluster_color", defaultClusterColor).toString()));
	setGlobularClusterColor(StelUtils::strToVec3f(conf->value("color/dso_globular_cluster_color", defaultClusterColor).toString()));
	setStellarAssociationColor(StelUtils::strToVec3f(conf->value("color/dso_stellar_association_color", defaultClusterColor).toString()));
	setStarCloudColor(StelUtils::strToVec3f(conf->value("color/dso_star_cloud_color", defaultClusterColor).toString()));

	QString defaultStellarColor = conf->value("color/dso_star_color", "1.0,0.7,0.2").toString();
	setStarColor(StelUtils::strToVec3f(defaultStellarColor));
	setSymbioticStarColor(StelUtils::strToVec3f(conf->value("color/dso_symbiotic_star_color", defaultStellarColor).toString()));
	setEmissionLineStarColor(StelUtils::strToVec3f(conf->value("color/dso_emission_star_color", defaultStellarColor).toString()));
	setEmissionObjectColor(StelUtils::strToVec3f(conf->value("color/dso_emission_object_color", defaultStellarColor).toString()));
	setYoungStellarObjectColor(StelUtils::strToVec3f(conf->value("color/dso_young_stellar_object_color", defaultStellarColor).toString()));

	// for DSO convertor (for developers!)
	flagConverter = conf->value("devel/convert_dso_catalog", false).toBool();
	flagDecimalCoordinates = conf->value("devel/convert_dso_decimal_coord", true).toBool();

	setFlagUseTypeFilters(conf->value("astro/flag_use_type_filter", false).toBool());

	Nebula::CatalogGroup catalogFilters = Nebula::CatalogGroup(Q_NULLPTR);

	conf->beginGroup("dso_catalog_filters");
	if (conf->value("flag_show_ngc", true).toBool())
		catalogFilters	|= Nebula::CatNGC;
	if (conf->value("flag_show_ic", true).toBool())
		catalogFilters	|= Nebula::CatIC;
	if (conf->value("flag_show_m", true).toBool())
		catalogFilters	|= Nebula::CatM;
	if (conf->value("flag_show_c", false).toBool())
		catalogFilters	|= Nebula::CatC;
	if (conf->value("flag_show_b", false).toBool())
		catalogFilters	|= Nebula::CatB;
	if (conf->value("flag_show_sh2", false).toBool())
		catalogFilters	|= Nebula::CatSh2;
	if (conf->value("flag_show_vdb", false).toBool())
		catalogFilters	|= Nebula::CatVdB;
	if (conf->value("flag_show_lbn", false).toBool())
		catalogFilters	|= Nebula::CatLBN;
	if (conf->value("flag_show_ldn", false).toBool())
		catalogFilters	|= Nebula::CatLDN;
	if (conf->value("flag_show_rcw", false).toBool())
		catalogFilters	|= Nebula::CatRCW;
	if (conf->value("flag_show_cr", false).toBool())
		catalogFilters	|= Nebula::CatCr;
	if (conf->value("flag_show_mel", false).toBool())
		catalogFilters	|= Nebula::CatMel;
	if (conf->value("flag_show_pgc", false).toBool())
		catalogFilters	|= Nebula::CatPGC;
	if (conf->value("flag_show_ced", false).toBool())
		catalogFilters	|= Nebula::CatCed;
	if (conf->value("flag_show_ugc", false).toBool())
		catalogFilters	|= Nebula::CatUGC;
	if (conf->value("flag_show_arp", false).toBool())
		catalogFilters	|= Nebula::CatArp;
	if (conf->value("flag_show_vv", false).toBool())
		catalogFilters	|= Nebula::CatVV;
	if (conf->value("flag_show_pk", false).toBool())
		catalogFilters	|= Nebula::CatPK;
	if (conf->value("flag_show_png", false).toBool())
		catalogFilters	|= Nebula::CatPNG;
	if (conf->value("flag_show_snrg", false).toBool())
		catalogFilters	|= Nebula::CatSNRG;
	if (conf->value("flag_show_aco", false).toBool())
		catalogFilters	|= Nebula::CatACO;
	if (conf->value("flag_show_hcg", false).toBool())
		catalogFilters	|= Nebula::CatHCG;
	if (conf->value("flag_show_abell", false).toBool())
		catalogFilters	|= Nebula::CatAbell;
	if (conf->value("flag_show_eso", false).toBool())
		catalogFilters	|= Nebula::CatESO;
	if (conf->value("flag_show_vdbh", false).toBool())
		catalogFilters	|= Nebula::CatVdBH;
	if (conf->value("flag_show_dwb", false).toBool())
		catalogFilters	|= Nebula::CatDWB;
	conf->endGroup();

	// NB: nebula set loaded inside setter of catalog filter
	setCatalogFilters(catalogFilters);

	Nebula::TypeGroup typeFilters = Nebula::TypeGroup(Q_NULLPTR);

	conf->beginGroup("dso_type_filters");
	if (conf->value("flag_show_galaxies", true).toBool())
		typeFilters	|= Nebula::TypeGalaxies;
	if (conf->value("flag_show_active_galaxies", true).toBool())
		typeFilters	|= Nebula::TypeActiveGalaxies;
	if (conf->value("flag_show_interacting_galaxies", true).toBool())
		typeFilters	|= Nebula::TypeInteractingGalaxies;
	if (conf->value("flag_show_clusters", true).toBool())
		typeFilters	|= Nebula::TypeStarClusters;
	if (conf->value("flag_show_bright_nebulae", true).toBool())
		typeFilters	|= Nebula::TypeBrightNebulae;
	if (conf->value("flag_show_dark_nebulae", true).toBool())
		typeFilters	|= Nebula::TypeDarkNebulae;
	if (conf->value("flag_show_planetary_nebulae", true).toBool())
		typeFilters	|= Nebula::TypePlanetaryNebulae;
	if (conf->value("flag_show_hydrogen_regions", true).toBool())
		typeFilters	|= Nebula::TypeHydrogenRegions;
	if (conf->value("flag_show_supernova_remnants", true).toBool())
		typeFilters	|= Nebula::TypeSupernovaRemnants;
	if (conf->value("flag_show_galaxy_clusters", true).toBool())
		typeFilters	|= Nebula::TypeGalaxyClusters;
	if (conf->value("flag_show_other", true).toBool())
		typeFilters	|= Nebula::TypeOther;
	conf->endGroup();

	setTypeFilters(typeFilters);

	// TODO: mechanism to specify which sets get loaded at start time.
	// candidate methods:
	// 1. config file option (list of sets to load at startup)
	// 2. load all
	// 3. flag in nebula_textures.fab (yuk)
	// 4. info.ini file in each set containing a "load at startup" item
	// For now (0.9.0), just load the default set
	loadNebulaSet("default");

	updateI18n();

	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(updateSkyCulture(const QString&)));
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	addAction("actionShow_Nebulas", N_("Display Options"), N_("Deep-sky objects"), "flagHintDisplayed", "D", "N");
	addAction("actionSet_Nebula_TypeFilterUsage", N_("Display Options"), N_("Toggle DSO type filter"), "flagTypeFiltersUsage");
}

struct DrawNebulaFuncObject
{
	DrawNebulaFuncObject(float amaxMagHints, float amaxMagLabels, StelPainter* p, StelCore* aCore, bool acheckMaxMagHints)
		: maxMagHints(amaxMagHints)
		, maxMagLabels(amaxMagLabels)
		, sPainter(p)
		, core(aCore)
		, checkMaxMagHints(acheckMaxMagHints)
	{
		angularSizeLimit = 5.f/sPainter->getProjector()->getPixelPerRadAtCenter()*M_180_PIf;
	}
	void operator()(StelRegionObject* obj)
	{
		if (checkMaxMagHints)
			return;

		Nebula* n = static_cast<Nebula*>(obj);
		float mag = n->vMag;
		if (mag>90.f)
			mag = n->bMag;

		StelSkyDrawer *drawer = core->getSkyDrawer();
		// filter out DSOs which are too dim to be seen (e.g. for bino observers)		
		if ((drawer->getFlagNebulaMagnitudeLimit()) && (mag > static_cast<float>(drawer->getCustomNebulaMagnitudeLimit())))
			return;

		if (!n->objectInDisplayedCatalog())
			return;

		if (!n->objectInAllowedSizeRangeLimits())
			return;

		if (n->majorAxisSize>angularSizeLimit || n->majorAxisSize==0.f || mag <= maxMagHints)
		{
			sPainter->getProjector()->project(n->XYZ,n->XY);
			n->drawLabel(*sPainter, maxMagLabels);
			n->drawHints(*sPainter, maxMagHints);
			n->drawOutlines(*sPainter, maxMagHints);
		}
	}
	float maxMagHints;
	float maxMagLabels;
	StelPainter* sPainter;
	StelCore* core;
	float angularSizeLimit;
	bool checkMaxMagHints;
};

void NebulaMgr::setCatalogFilters(Nebula::CatalogGroup cflags)
{
	if(static_cast<int>(cflags) != static_cast<int>(Nebula::catalogFilters))
	{
		Nebula::catalogFilters = cflags;
		emit catalogFiltersChanged(cflags);
	}
}

void NebulaMgr::setTypeFilters(Nebula::TypeGroup tflags)
{
	if(static_cast<int>(tflags) != static_cast<int>(Nebula::typeFilters))
	{
		Nebula::typeFilters = tflags;
		emit typeFiltersChanged(tflags);
	}
}

void NebulaMgr::setFlagSurfaceBrightnessUsage(const bool usage)
{
	if (usage!=Nebula::surfaceBrightnessUsage)
	{
		Nebula::surfaceBrightnessUsage=usage;
		emit flagSurfaceBrightnessUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSurfaceBrightnessUsage(void) const
{
	return Nebula::surfaceBrightnessUsage;
}

void NebulaMgr::setFlagSurfaceBrightnessArcsecUsage(const bool usage)
{
	if (usage!=Nebula::flagUseArcsecSurfaceBrightness)
	{
		Nebula::flagUseArcsecSurfaceBrightness=usage;
		emit flagSurfaceBrightnessArcsecUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSurfaceBrightnessArcsecUsage(void) const
{
	return Nebula::flagUseArcsecSurfaceBrightness;
}

void NebulaMgr::setFlagSurfaceBrightnessShortNotationUsage(const bool usage)
{
	if (usage!=Nebula::flagUseShortNotationSurfaceBrightness)
	{
		Nebula::flagUseShortNotationSurfaceBrightness=usage;
		emit flagSurfaceBrightnessShortNotationUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSurfaceBrightnessShortNotationUsage(void) const
{
	return Nebula::flagUseShortNotationSurfaceBrightness;
}

void NebulaMgr::setFlagSizeLimitsUsage(const bool usage)
{
	if (usage!=Nebula::flagUseSizeLimits)
	{
		Nebula::flagUseSizeLimits=usage;
		emit flagSizeLimitsUsageChanged(usage);
	}
}

bool NebulaMgr::getFlagSizeLimitsUsage(void) const
{
	return Nebula::flagUseSizeLimits;
}

void NebulaMgr::setFlagUseTypeFilters(const bool b)
{
	if (Nebula::flagUseTypeFilters!=b)
	{
		Nebula::flagUseTypeFilters=b;
		emit flagUseTypeFiltersChanged(b);
	}
}

bool NebulaMgr::getFlagUseTypeFilters(void) const
{
	return Nebula::flagUseTypeFilters;
}

void NebulaMgr::setLabelsAmount(double a)
{
	if((a-labelsAmount) != 0.)
	{
		labelsAmount=a;
		emit labelsAmountChanged(a);
	}
}

double NebulaMgr::getLabelsAmount(void) const
{
	return labelsAmount;
}

void NebulaMgr::setHintsAmount(double f)
{
	if((hintsAmount-f) != 0.)
	{
		hintsAmount = f;
		emit hintsAmountChanged(f);
	}
}

double NebulaMgr::getHintsAmount(void) const
{
	return hintsAmount;
}

void NebulaMgr::setMinSizeLimit(double s)
{
	if((Nebula::minSizeLimit-s) != 0.)
	{
		Nebula::minSizeLimit = s;
		emit minSizeLimitChanged(s);
	}
}

double NebulaMgr::getMinSizeLimit() const
{
	return Nebula::minSizeLimit;
}

void NebulaMgr::setMaxSizeLimit(double s)
{
	if((Nebula::maxSizeLimit-s) != 0.)
	{
		Nebula::maxSizeLimit = s;
		emit maxSizeLimitChanged(s);
	}
}

double NebulaMgr::getMaxSizeLimit() const
{
	return Nebula::maxSizeLimit;
}

float NebulaMgr::computeMaxMagHint(const StelSkyDrawer* skyDrawer) const
{
	return skyDrawer->getLimitMagnitude()*1.2f-2.f+static_cast<float>(hintsAmount *1.)-2.f;
}

// Draw all the Nebulae
void NebulaMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);

	StelSkyDrawer* skyDrawer = core->getSkyDrawer();

	Nebula::hintsBrightness = hintsFader.getInterstate()*flagShow.getInterstate();

	sPainter.setBlending(true, GL_ONE, GL_ONE);

	// Use a 4 degree margin (esp. for wide outlines)
	const float margin = 4.f*M_PI_180f*prj->getPixelPerRadAtCenter();
	const SphericalRegionP& p = prj->getViewportConvexPolygon(margin, margin);

	// Print all the nebulae of all the selected zones
	float maxMagHints  = computeMaxMagHint(skyDrawer);
	float maxMagLabels = skyDrawer->getLimitMagnitude()-2.f+static_cast<float>(labelsAmount*1.2)-2.f;
	sPainter.setFont(nebulaFont);
	DrawNebulaFuncObject func(maxMagHints, maxMagLabels, &sPainter, core, hintsFader.getInterstate()<=0.f);
	nebGrid.processIntersectingPointInRegions(p.data(), func);

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, sPainter);
}

void NebulaMgr::drawPointer(const StelCore* core, StelPainter& sPainter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		// Compute 2D pos and return if outside screen
		if (!prj->projectInPlace(pos)) return;
		sPainter.setColor(0.4f,0.5f,0.8f);

		texPointer->bind();

		sPainter.setBlending(true);

		// Size on screen
		float size = static_cast<float>(obj->getAngularSize(core))*M_PI_180f*prj->getPixelPerRadAtCenter();
		if (size>120.f) // avoid oversized marker
			size = 120.f;

		if (Nebula::drawHintProportional)
			size*=1.2f;
		size+=20.f + 10.f*std::sin(3.f * static_cast<float>(StelApp::getInstance().getAnimationTime()));
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])-size*0.5f, static_cast<float>(pos[1])-size*0.5f, 10, 90);
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])-size*0.5f, static_cast<float>(pos[1])+size*0.5f, 10, 0);
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])+size*0.5f, static_cast<float>(pos[1])+size*0.5f, 10, -90);
		sPainter.drawSprite2dMode(static_cast<float>(pos[0])+size*0.5f, static_cast<float>(pos[1])-size*0.5f, 10, -180);
	}
}

// Search by name
NebulaP NebulaMgr::search(const QString& name)
{
	QString uname = name.toUpper();

	for (const auto& n : dsoArray)
	{
		QString testName = n->getEnglishName().toUpper();
		if (testName==uname) return n;
	}

	// If no match found, try search by catalog reference
	static QRegExp catNumRx("^(M|NGC|IC|C|B|VDB|RCW|LDN|LBN|CR|MEL|PGC|UGC|ARP|VV|ABELL|DWB)\\s*(\\d+)$");
	if (catNumRx.exactMatch(uname))
	{
		QString cat = catNumRx.capturedTexts().at(1);
		unsigned int num = catNumRx.capturedTexts().at(2).toUInt();

		//qWarning() << "[SEARCH]" << cat << num;

		if (cat == "M") return searchM(num);
		if (cat == "NGC") return searchNGC(num);
		if (cat == "IC") return searchIC(num);
		if (cat == "C") return searchC(num);
		if (cat == "B") return searchB(num);
		if (cat == "VDB") return searchVdB(num);
		if (cat == "RCW") return searchRCW(num);
		if (cat == "LDN") return searchLDN(num);
		if (cat == "LBN") return searchLBN(num);
		if (cat == "CR") return searchCr(num);
		if (cat == "MEL") return searchMel(num);
		if (cat == "PGC") return searchPGC(num);
		if (cat == "UGC") return searchUGC(num);
		if (cat == "ARP") return searchArp(num);
		if (cat == "VV") return searchVV(num);
		if (cat == "ABELL") return searchAbell(num);
		if (cat == "DWB") return searchDWB(num);
	}
	static QRegExp dCatNumRx("^(SH)\\s*\\d-\\s*(\\d+)$");
	if (dCatNumRx.exactMatch(uname))
	{
		QString dcat = dCatNumRx.capturedTexts().at(1);
		unsigned int dnum = dCatNumRx.capturedTexts().at(2).toUInt();

		if (dcat == "SH") return searchSh2(dnum);
	}
	static QRegExp sCatNumRx("^(CED|PK|ACO|HCG|ESO|VDBH)\\s*(.+)$");
	if (sCatNumRx.exactMatch(uname))
	{
		QString cat = sCatNumRx.capturedTexts().at(1);
		QString num = sCatNumRx.capturedTexts().at(2).trimmed();

		if (cat == "CED") return searchCed(num);
		if (cat == "PK") return searchPK(num);
		if (cat == "ACO") return searchACO(num);
		if (cat == "HCG") return searchHCG(num);
		if (cat == "ESO") return searchESO(num);
		if (cat == "VDBH") return searchVdBH(num);
	}
	static QRegExp gCatNumRx("^(PN|SNR)\\s*G(.+)$");
	if (gCatNumRx.exactMatch(uname))
	{
		QString cat = gCatNumRx.capturedTexts().at(1);
		QString num = gCatNumRx.capturedTexts().at(2).trimmed();

		if (cat == "PN") return searchPNG(num);
		if (cat == "SNR") return searchSNRG(num);
	}
	return NebulaP();
}

void NebulaMgr::loadNebulaSet(const QString& setName)
{
	QString srcCatalogPath		= StelFileMgr::findFile("nebulae/" + setName + "/catalog.txt");
	QString dsoCatalogPath		= StelFileMgr::findFile("nebulae/" + setName + "/catalog.dat");
	QString dsoOutlinesPath		= StelFileMgr::findFile("nebulae/" + setName + "/outlines.dat");

	dsoArray.clear();
	dsoIndex.clear();
	nebGrid.clear();

	if (flagConverter)
	{
		if (!srcCatalogPath.isEmpty())
			convertDSOCatalog(srcCatalogPath, StelFileMgr::findFile("nebulae/" + setName + "/catalog.pack", StelFileMgr::New), flagDecimalCoordinates);
		else
			qWarning() << "ERROR convert catalogue, because source data set is not exists for " << setName;
	}

	if (dsoCatalogPath.isEmpty())
	{
		qWarning() << "ERROR while loading deep-sky catalog data set " << setName;
		return;
	}

	loadDSOCatalog(dsoCatalogPath);

	if (!dsoOutlinesPath.isEmpty())
		loadDSOOutlines(dsoOutlinesPath);
}

// Look for a nebulae by XYZ coords
NebulaP NebulaMgr::search(const Vec3d& apos)
{
	Vec3d pos = apos;
	pos.normalize();
	NebulaP plusProche;
	double anglePlusProche=0.0;
	for (const auto& n : dsoArray)
	{
		if (n->XYZ*pos>anglePlusProche)
		{
			anglePlusProche=n->XYZ*pos;
			plusProche=n;
		}
	}
	if (anglePlusProche>0.999)
	{
		return plusProche;
	}
	else return NebulaP();
}


QList<StelObjectP> NebulaMgr::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;
	if (!getFlagShow())
		return result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;
	for (const auto& n : dsoArray)
	{
		equPos = n->XYZ;
		equPos.normalize();
		if (equPos*v>=cosLimFov)
		{
			result.push_back(qSharedPointerCast<StelObject>(n));
		}
	}
	return result;
}

NebulaP NebulaMgr::searchDSO(unsigned int DSO) const
{
	if (dsoIndex.contains(DSO))
		return dsoIndex[DSO];
	return NebulaP();
}


NebulaP NebulaMgr::searchM(unsigned int M) const
{
	for (const auto& n : dsoArray)
		if (n->M_nb == M)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchNGC(unsigned int NGC) const
{
	for (const auto& n : dsoArray)
		if (n->NGC_nb == NGC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchIC(unsigned int IC) const
{
	for (const auto& n : dsoArray)
		if (n->IC_nb == IC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchC(unsigned int C) const
{
	for (const auto& n : dsoArray)
		if (n->C_nb == C)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchB(unsigned int B) const
{
	for (const auto& n : dsoArray)
		if (n->B_nb == B)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchSh2(unsigned int Sh2) const
{
	for (const auto& n : dsoArray)
		if (n->Sh2_nb == Sh2)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVdB(unsigned int VdB) const
{
	for (const auto& n : dsoArray)
		if (n->VdB_nb == VdB)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchRCW(unsigned int RCW) const
{
	for (const auto& n : dsoArray)
		if (n->RCW_nb == RCW)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchLDN(unsigned int LDN) const
{
	for (const auto& n : dsoArray)
		if (n->LDN_nb == LDN)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchLBN(unsigned int LBN) const
{
	for (const auto& n : dsoArray)
		if (n->LBN_nb == LBN)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchCr(unsigned int Cr) const
{
	for (const auto& n : dsoArray)
		if (n->Cr_nb == Cr)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchMel(unsigned int Mel) const
{
	for (const auto& n : dsoArray)
		if (n->Mel_nb == Mel)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchPGC(unsigned int PGC) const
{
	for (const auto& n : dsoArray)
		if (n->PGC_nb == PGC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchUGC(unsigned int UGC) const
{
	for (const auto& n : dsoArray)
		if (n->UGC_nb == UGC)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchCed(QString Ced) const
{
	for (const auto& n : dsoArray)
		if (n->Ced_nb.trimmed().toUpper() == Ced.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchArp(unsigned int Arp) const
{
	for (const auto& n : dsoArray)
		if (n->Arp_nb == Arp)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVV(unsigned int VV) const
{
	for (const auto& n : dsoArray)
		if (n->VV_nb == VV)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchPK(QString PK) const
{
	for (const auto& n : dsoArray)
		if (n->PK_nb.trimmed().toUpper() == PK.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchPNG(QString PNG) const
{
	for (const auto& n : dsoArray)
		if (n->PNG_nb.trimmed().toUpper() == PNG.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchSNRG(QString SNRG) const
{
	for (const auto& n : dsoArray)
		if (n->SNRG_nb.trimmed().toUpper() == SNRG.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchACO(QString ACO) const
{
	for (const auto& n : dsoArray)
		if (n->ACO_nb.trimmed().toUpper() == ACO.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchHCG(QString HCG) const
{
	for (const auto& n : dsoArray)
		if (n->HCG_nb.trimmed().toUpper() == HCG.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchAbell(unsigned int Abell) const
{
	for (const auto& n : dsoArray)
		if (n->Abell_nb == Abell)
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchESO(QString ESO) const
{
	for (const auto& n : dsoArray)
		if (n->ESO_nb.trimmed().toUpper() == ESO.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchVdBH(QString VdBH) const
{
	for (const auto& n : dsoArray)
		if (n->VdBH_nb.trimmed().toUpper() == VdBH.trimmed().toUpper())
			return n;
	return NebulaP();
}

NebulaP NebulaMgr::searchDWB(unsigned int DWB) const
{
	for (const auto& n : dsoArray)
		if (n->DWB_nb == DWB)
			return n;
	return NebulaP();
}

QString NebulaMgr::getLatestSelectedDSODesignation() const
{
	QString result = "";

	const QList<StelObjectP> selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Nebula");
	if (!selected.empty())
	{
		for (const auto& n : dsoArray)
			if (n==selected[0])
				result = n->getDSODesignation(); // Get designation for latest selected DSO
	}

	return result;
}

void NebulaMgr::convertDSOCatalog(const QString &in, const QString &out, bool decimal=false)
{
	QFile dsoIn(in);
	if (!dsoIn.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QFile dsoOut(out);
	if (!dsoOut.open(QIODevice::WriteOnly))
	{
		qDebug() << "Error converting DSO data! Cannot open file" << QDir::toNativeSeparators(out);
		return;
	}

	int totalRecords=0;
	QString record;
	while (!dsoIn.atEnd())
	{
		dsoIn.readLine();
		++totalRecords;
	}

	// rewind the file to the start
	dsoIn.seek(0);

	QDataStream dsoOutStream(&dsoOut);
	dsoOutStream.setVersion(QDataStream::Qt_5_2);

	int	id, orientationAngle, NGC, IC, M, C, B, Sh2, VdB, RCW, LDN, LBN, Cr, Mel, PGC, UGC, Arp, VV, Abell, DWB;
	float	raRad, decRad, bMag, vMag, majorAxisSize, minorAxisSize, dist, distErr, z, zErr, plx, plxErr;
	QString oType, mType, Ced, PK, PNG, SNRG, ACO, HCG, ESO, VdBH, ra, dec;

	//unsigned int nType;
	Nebula::NebulaType nType;

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;				// how many records weree rad without problems
	while (!dsoIn.atEnd())
	{
		record = QString::fromUtf8(dsoIn.readLine());
		++currentLineNumber;

		QRegExp version("ersion\\s+([\\d\\.]+)\\s+(\\w+)");
		int vp = version.indexIn(record);
		if (vp!=-1) // Version of catalog, a first line!
			dsoOutStream << version.capturedTexts().at(1).trimmed() << version.capturedTexts().at(2).trimmed();

		// skip comments
		if (record.startsWith("//") || record.startsWith("#"))
			continue;
		++currentRecordNumber;

		if (!record.isEmpty())
		{
			QStringList list=record.split("\t", QString::KeepEmptyParts);

			id				= list.at(0).toInt();		// ID (inner identification number)
			ra				= list.at(1).trimmed();
			dec				= list.at(2).trimmed();
			bMag			= list.at(3).toFloat();		// B magnitude
			vMag			= list.at(4).toFloat();		// V magnitude
			oType			= list.at(5).trimmed();		// Object type
			mType			= list.at(6).trimmed();		// Morphological type of object
			majorAxisSize		= list.at(7).toFloat();		// major axis size (arcmin)
			minorAxisSize		= list.at(8).toFloat();		// minor axis size (arcmin)
			orientationAngle	= list.at(9).toInt();		// orientation angle (degrees)
			z				= list.at(10).toFloat();	// redshift
			zErr				= list.at(11).toFloat();	// error of redshift
			plx				= list.at(12).toFloat();	// parallax (mas)
			plxErr			= list.at(13).toFloat();	// error of parallax (mas)
			dist				= list.at(14).toFloat();	// distance (Mpc for galaxies, kpc for other objects)
			distErr			= list.at(15).toFloat();	// distance error (Mpc for galaxies, kpc for other objects)
			// -----------------------------------------------
			// cross-identification data
			// -----------------------------------------------
			NGC				= list.at(16).toInt();		// NGC number
			IC				= list.at(17).toInt();		// IC number
			M				= list.at(18).toInt();		// M number
			C				= list.at(19).toInt();		// C number
			B				= list.at(20).toInt();		// B number
			Sh2				= list.at(21).toInt();		// Sh2 number
			VdB				= list.at(22).toInt();		// VdB number
			RCW				= list.at(23).toInt();		// RCW number
			LDN				= list.at(24).toInt();		// LDN number
			LBN				= list.at(25).toInt();		// LBN number
			Cr				= list.at(26).toInt();		// Cr number (alias: Col)
			Mel				= list.at(27).toInt();		// Mel number
			PGC				= list.at(28).toInt();		// PGC number (subset)
			UGC				= list.at(29).toInt();		// UGC number (subset)
			Ced				= list.at(30).trimmed();	// Ced number
			Arp				= list.at(31).toInt();		// Arp number
			VV				= list.at(32).toInt();		// VV number
			PK				= list.at(33).trimmed();	// PK number
			PNG				= list.at(34).trimmed();	// PN G number
			SNRG			= list.at(35).trimmed();	// SNR G number
			ACO				= list.at(36).trimmed();	// ACO number
			HCG				= list.at(37).trimmed();	// HCG number
			Abell				= list.at(38).toInt();		// Abell number
			ESO				= list.at(39).trimmed();	// ESO number
			VdBH			= list.at(40).trimmed();	// VdBH number
			DWB				= list.at(41).toInt();		// DWB number

			if (decimal)
			{
				// Convert from deg to rad
				raRad	= ra.toFloat() *M_PI_180f;
				decRad	= dec.toFloat()*M_PI_180f;
			}
			else
			{
				QStringList raLst;
				if (ra.contains(":"))
					raLst	= ra.split(":");
				else
					raLst	= ra.split(" ");

				QStringList decLst;
				if (dec.contains(":"))
					decLst = dec.split(":");
				else
					decLst = dec.split(" ");

				raRad	= raLst.at(0).toFloat() + raLst.at(1).toFloat()/60.f + raLst.at(2).toFloat()/3600.f;
				decRad	= qAbs(decLst.at(0).toFloat()) + decLst.at(1).toFloat()/60.f + decLst.at(2).toFloat()/3600.f;
				if (dec.startsWith("-")) decRad *= -1.f;

				raRad  *= M_PIf/12.f;	// Convert from hours to rad
				decRad *= M_PIf/180.f;    // Convert from deg to rad
			}

			majorAxisSize /= 60.f;	// Convert from arcmin to degrees
			minorAxisSize /= 60.f;	// Convert from arcmin to degrees

			// Warning: Hyades and LMC has visual magnitude less than 1.0 (0.5^m and 0.9^m)
			if (bMag <= 0.f) bMag = 99.f;
			if (vMag <= 0.f) vMag = 99.f;

			QStringList oTypes;
			oTypes << "G"	<< "GX"	 << "GC"  << "OC"  << "NB"  << "PN"  << "DN"  << "RN"  << "C+N"  << "RNE"
			       << "HII" << "SNR" << "BN"  << "EN"  << "SA"  << "SC"  << "CL"  << "IG"  << "RG"   << "AGX"
			       << "QSO" << "ISM" << "EMO" << "GNE" << "RAD" << "LIN" << "BLL" << "BLA" << "MOC"  << "YSO"
			       << "Q?"	<< "PN?" << "*"	  << "SFR" << "IR"  << "**"  << "MUL" << "PPN" << "GIG"  << "OPC"
			       << "MGR" << "IG2" << "IG3" << "SY*" << "PA*" << "CV*" << "Y*?" << "CGB" << "SNRG" << "Y*O"
			       << "SR*" << "EM*" << "AB*" << "MI*" << "MI?" << "TT*" << "WR*" << "C*"  << "WD*"  << "EL*"
			       << "NL*" << "NO*" << "HS*" << "LP*" << "OH*" << "S?R" << "IR*" << "POC" << "PNB"  << "GXCL"
			       << "AL*" << "PR*" << "RS*" << "S*B" << "SN?" << "SR?" << "DNE" << "RG*" << "PSR"  << "HH"
			       << "V*"  << "PA?" << "BUB" << "CLG" << "CGG";

			switch (oTypes.indexOf(oType.toUpper()))
			{
				case 0:
				case 1:
					nType = Nebula::NebGx;
					break;
				case 2:
					nType = Nebula::NebGc;
					break;
				case 3:
				case 39:
					nType = Nebula::NebOc;
					break;
				case 4:
					nType = Nebula::NebN;
					break;
				case 5:
				case 68:
					nType = Nebula::NebPn;
					break;
				case 6:
				case 34:
				case 76:
					nType = Nebula::NebDn;
					break;
				case 7:
				case 9:
					nType = Nebula::NebRn;
					break;
				case 8:
					nType = Nebula::NebCn;
					break;
				case 10:
				case 23:
					nType = Nebula::NebHII;
					break;
				case 11:
				case 48:
				case 78:
					nType = Nebula::NebSNR;
					break;
				case 12:
					nType = Nebula::NebBn;
					break;
				case 13:
					nType = Nebula::NebEn;
					break;
				case 14:
				case 40:
					nType = Nebula::NebSA;
					break;
				case 15:
					nType = Nebula::NebSC;
					break;
				case 16:
					nType = Nebula::NebCl;
					break;
				case 17:
				case 38:
				case 41:
				case 42:
					nType = Nebula::NebIGx;
					break;
				case 18:
					nType = Nebula::NebRGx;
					break;
				case 19:
				case 25: // LINER-type active galaxies
					nType = Nebula::NebAGx;
					break;
				case 20:
					nType = Nebula::NebQSO;
					break;
				case 21:
				case 24:
				case 47:
				case 79:
				case 82:
					nType = Nebula::NebISM;
					break;
				case 22:
					nType = Nebula::NebEMO;
					break;
				case 26:
					nType = Nebula::NebBLL;
					break;
				case 27:
					nType = Nebula::NebBLA;
					break;
				case 28:
				case 33:
				case 67:
					nType = Nebula::NebMolCld;
					break;
				case 29:
				case 46:
				case 49:
					nType = Nebula::NebYSO;
					break;
				case 30:
					nType = Nebula::NebPossQSO;
					break;
				case 31:
					nType = Nebula::NebPossPN;
					break;
				case 32:
				case 35:
				case 36:
				case 45:
				case 50:
				case 52:
				case 53:
				case 54:
				case 55:
				case 56:
				case 58:
				case 59:
				case 60:
				case 61:
				case 62:
				case 63:
				case 64:
				case 65:
				case 66:
				case 70:
				case 71:
				case 72:
				case 73:
				case 77:
				case 80:
					nType = Nebula::NebStar;
					break;
				case 37:
				case 44:
				case 81:
					nType = Nebula::NebPPN;
					break;
				case 43:
					nType = Nebula::NebSymbioticStar;
					break;
				case 51:
				case 57:
					nType = Nebula::NebEmissionLineStar;
					break;
				case 74:
					nType = Nebula::NebSNC;
					break;
				case 75:
					nType = Nebula::NebSNRC;
					break;
				case 69:
				case 83:
				case 84:
					nType = Nebula::NebGxCl;
					break;
				default:
				{
					nType = Nebula::NebUnknown;
					qDebug() << "Record with ID" << id <<"has unknown type of object:" << oType;
					break;
				}
			}

			++readOk;

			dsoOutStream << id << raRad << decRad << bMag << vMag << static_cast<unsigned int>(nType) << mType << majorAxisSize << minorAxisSize
				     << orientationAngle << z << zErr << plx << plxErr << dist  << distErr << NGC << IC << M << C
				     << B << Sh2 << VdB << RCW  << LDN << LBN << Cr << Mel << PGC << UGC << Ced << Arp << VV << PK
				     << PNG << SNRG << ACO << HCG << Abell << ESO << VdBH << DWB;
		}
	}
	dsoIn.close();
	dsoOut.flush();
	dsoOut.close();
	qDebug() << "Converted" << readOk << "/" << totalRecords << "DSO records";
	qDebug() << "[...] Please use 'gzip -nc catalog.pack > catalog.dat' to pack the catalog.";
}

bool NebulaMgr::loadDSOCatalog(const QString &filename)
{
	QFile in(filename);
	if (!in.open(QIODevice::ReadOnly))
		return false;

	qDebug() << "Loading DSO data ...";

	// Let's begin use gzipped data
	QDataStream ins(StelUtils::uncompress(in.readAll()));
	ins.setVersion(QDataStream::Qt_5_2);

	QString version = "", edition= "";
	int totalRecords=0;
	while (!ins.atEnd())
	{
		if (totalRecords==0) // Read the version of catalog
		{
			ins >> version >> edition;
			if (version.isEmpty())
				version = "3.1"; // The first version of extended edition of the catalog
			if (edition.isEmpty())
				edition = "unknown";
			qDebug() << "[...]" << QString("Stellarium DSO Catalog, version %1 (%2 edition)").arg(version).arg(edition);
			if (StelUtils::compareVersions(version, StellariumDSOCatalogVersion)!=0)
			{
				qDebug() << "WARNING: Mismatch the version of catalog! The expected version of catalog is" << StellariumDSOCatalogVersion;
				++totalRecords;
				break;
			}
		}
		else
		{
			// Create a new Nebula record
			NebulaP e = NebulaP(new Nebula);
			e->readDSO(ins);

			dsoArray.append(e);
			nebGrid.insert(qSharedPointerCast<StelRegionObject>(e));
			if (e->DSO_nb!=0)
				dsoIndex.insert(e->DSO_nb, e);
		}
		++totalRecords;
	}
	in.close();
	qDebug() << "Loaded" << --totalRecords << "DSO records";
	return true;
}

bool NebulaMgr::loadDSONames(const QString &filename)
{
	qDebug() << "Loading DSO name data ...";
	QFile dsoNameFile(filename);
	if (!dsoNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "DSO name data file" << QDir::toNativeSeparators(filename) << "not found.";
		return false;
	}

	// Read the names of the deep-sky objects
	QString name, record, ref, cdes;
	QStringList nodata;
	nodata.clear();
	int totalRecords=0;
	int lineNumber=0;
	int readOk=0;
	unsigned int nb;
	NebulaP e;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	QRegExp transRx("_[(]\"(.*)\"[)](\\s*#.*)?"); // optional comments after name.
	while (!dsoNameFile.atEnd())
	{
		record = QString::fromUtf8(dsoNameFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;

		// bytes 1 - 5, designator for catalogue (prefix)
		ref  = record.left(5).trimmed();
		// bytes 6 -20, identificator for object in the catalog
		cdes = record.mid(5, 15).trimmed().toUpper();
		// bytes 21-80, proper name of the object (translatable)
		name = record.mid(21).trimmed(); // Let gets the name with trimmed whitespaces

		nb = cdes.toUInt();

		QStringList catalogs;
		catalogs << "IC" << "M" << "C" << "CR" << "MEL" << "B" << "SH2" << "VDB" << "RCW" << "LDN" << "LBN"
			 << "NGC" << "PGC" << "UGC" << "CED" << "ARP" << "VV" << "PK" << "PNG" << "SNRG" << "ACO"
			 << "HCG" << "A66" << "ESO" << "VDBH" << "DWB";

		switch (catalogs.indexOf(ref.toUpper()))
		{
			case 0:
				e = searchIC(nb);
				break;
			case 1:
				e = searchM(nb);
				break;
			case 2:
				e = searchC(nb);
				break;
			case 3:
				e = searchCr(nb);
				break;
			case 4:
				e = searchMel(nb);
				break;
			case 5:
				e = searchB(nb);
				break;
			case 6:
				e = searchSh2(nb);
				break;
			case 7:
				e = searchVdB(nb);
				break;
			case 8:
				e = searchRCW(nb);
				break;
			case 9:
				e = searchLDN(nb);
				break;
			case 10:
				e = searchLBN(nb);
				break;
			case 11:
				e = searchNGC(nb);
				break;
			case 12:
				e = searchPGC(nb);
				break;
			case 13:
				e = searchUGC(nb);
				break;
			case 14:
				e = searchCed(cdes);
				break;
			case 15:
				e = searchArp(nb);
				break;
			case 16:
				e = searchVV(nb);
				break;
			case 17:
				e = searchPK(cdes);
				break;
			case 18:
				e = searchPNG(cdes);
				break;
			case 19:
				e = searchSNRG(cdes);
				break;
			case 20:
				e = searchACO(cdes);
				break;
			case 21:
				e = searchHCG(cdes);
				break;
			case 22: // A66
				e = searchAbell(nb);
				break;
			case 23:
				e = searchESO(cdes);
				break;
			case 24:
				e = searchVdBH(cdes);
				break;
			case 25:
				e = searchDWB(nb);
				break;
			default:
				e = searchDSO(nb);
				break;
		}

		if (!e.isNull())
		{
			if (transRx.exactMatch(name))
			{
				QString propName = transRx.capturedTexts().at(1).trimmed();
				QString currName = e->getEnglishName();
				if (currName.isEmpty())
					e->setProperName(propName);
				else if (currName!=propName)
					e->addNameAlias(propName);
			}
			readOk++;
		}
		else
			nodata.append(QString("%1 %2").arg(ref.trimmed(), cdes.trimmed()));
	}
	dsoNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "DSO name records successfully";

	int err = nodata.size();
	if (err>0)
		qDebug() << "WARNING - No position data for" << err << "objects:" << nodata.join(", ");

	return true;
}

bool NebulaMgr::loadDSOOutlines(const QString &filename)
{
	qDebug() << "Loading DSO outline data ...";
	QFile dsoOutlineFile(filename);
	if (!dsoOutlineFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "DSO outline data file" << QDir::toNativeSeparators(filename) << "not found.";
		return false;
	}

	double RA, DE;
	int i, readOk = 0;
	Vec3d XYZ;
	std::vector<Vec3d> *points = Q_NULLPTR;
	typedef QPair<double, double> coords;
	coords point, fpoint;
	QList<coords> outline;
	QString record, command, dso;
	NebulaP e;
	// Read the outlines data of the DSO
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!dsoOutlineFile.atEnd())
	{
		record = QString::fromUtf8(dsoOutlineFile.readLine());
		if (commentRx.exactMatch(record))
			continue;

		// bytes 1 - 8, RA
		RA = record.left(8).toDouble();
		// bytes 9 -18, DE
		DE = record.mid(9, 10).toDouble();
		// bytes 19-25, command
		command = record.mid(19, 7).trimmed();
		// bytes 26, designation of DSO
		dso = record.mid(26).trimmed();

		RA*=M_PI/12.;     // Convert from hours to rad
		DE*=M_PI/180.;    // Convert from deg to rad

		if (command.contains("start", Qt::CaseInsensitive))
		{
			outline.clear();
			e = search(dso);

			point.first  = RA;
			point.second = DE;
			outline.append(point);
			fpoint = point;
		}

		if (command.contains("vertex", Qt::CaseInsensitive))
		{
			point.first  = RA;
			point.second = DE;
			outline.append(point);
		}

		if (command.contains("end", Qt::CaseInsensitive))
		{
			point.first  = RA;
			point.second = DE;
			outline.append(point);
			outline.append(fpoint);

			if (!e.isNull())
			{
				points = new std::vector<Vec3d>;
				for (i = 0; i < outline.size(); i++)
				{
					// Calc the Cartesian coord with RA and DE
					point = outline.at(i);
					StelUtils::spheToRect(point.first, point.second, XYZ);
					points->push_back(XYZ);
				}

				e->outlineSegments.push_back(points);
			}
			readOk++;
		}
	}
	dsoOutlineFile.close();
	qDebug() << "Loaded" << readOk << "DSO outline records successfully";	
	return true;
}

void NebulaMgr::updateSkyCulture(const QString& skyCultureDir)
{
	QString namesFile = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/dso_names.fab");

	for (const auto& n : dsoArray)
		n->removeAllNames();

	if (namesFile.isEmpty())
	{
		QString setName = "default";
		QString dsoNamesPath = StelFileMgr::findFile("nebulae/" + setName + "/names.dat");
		if (dsoNamesPath.isEmpty())
		{
			qWarning() << "ERROR while loading deep-sky names data set " << setName;
			return;
		}
		loadDSONames(dsoNamesPath);
	}
	else
	{
		// Open file
		QFile dsoNamesFile(namesFile);
		if (!dsoNamesFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qDebug() << "Cannot open file" << QDir::toNativeSeparators(namesFile);
			return;
		}

		// Now parse the file
		// lines to ignore which start with a # or are empty
		QRegExp commentRx("^(\\s*#.*|\\s*)$");

		// lines which look like records - we use the RE to extract the fields
		// which will be available in recRx.capturedTexts()
		QRegExp recRx("^\\s*([\\w\\s]+)\\s*\\|_[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)\\n");

		QString record, dsoId, nativeName;
		int totalRecords=0;
		int readOk=0;
		int lineNumber=0;
		while (!dsoNamesFile.atEnd())
		{
			record = QString::fromUtf8(dsoNamesFile.readLine());
			lineNumber++;

			// Skip comments
			if (commentRx.exactMatch(record))
				continue;

			totalRecords++;

			if (!recRx.exactMatch(record))
			{
				qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in native deep-sky object names file" << QDir::toNativeSeparators(namesFile);
			}
			else
			{
				dsoId = recRx.capturedTexts().at(1).trimmed();
				nativeName = recRx.capturedTexts().at(2).trimmed(); // Use translatable text
				NebulaP e = search(dsoId);
				QString currentName = e->getEnglishName();
				if (currentName.isEmpty()) // Set native name of DSO
					e->setProperName(nativeName);
				else if (currentName!=nativeName) // Add traditional (well-known?) name of DSO as alias
					e->addNameAlias(nativeName);
				readOk++;
			}
		}
		dsoNamesFile.close();
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "native names of deep-sky objects";
	}

	updateI18n();
}

void NebulaMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	for (const auto& n : dsoArray)
		n->translateName(trans);
}


//! Return the matching Nebula object's pointer if exists or an "empty" StelObjectP
StelObjectP NebulaMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	// Search by common names
	for (const auto& n : dsoArray)
	{
		QString objwcap = n->nameI18.toUpper();
		if (objwcap==objw)
			return qSharedPointerCast<StelObject>(n);
	}

	// Search by aliases of common names
	for (const auto& n : dsoArray)
	{
		for (auto objwcapa : n->nameI18Aliases)
		{
			if (objwcapa.toUpper()==objw)
				return qSharedPointerCast<StelObject>(n);
		}
	}

	// Search by designation
	return searchByDesignation(objw);
}


//! Return the matching Nebula object's pointer if exists or an "empty" StelObjectP
StelObjectP NebulaMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();

	// Search by common names
	for (const auto& n : dsoArray)
	{
		QString objwcap = n->englishName.toUpper();
		if (objwcap==objw)
			return qSharedPointerCast<StelObject>(n);
	}

	if (getFlagAdditionalNames())
	{
		// Search by aliases of common names
		for (const auto& n : dsoArray)
		{
			for (auto objwcapa : n->englishAliases)
			{
				if (objwcapa.toUpper()==objw)
					return qSharedPointerCast<StelObject>(n);
			}
		}
	}

	// Search by designation
	return searchByDesignation(objw);
}

//! Return the matching Nebula object's pointer if exists or Q_NULLPTR
//! TODO Decide whether empty StelObjectP or Q_NULLPTR is the better return type and select the same for both.
StelObjectP NebulaMgr::searchByDesignation(const QString &designation) const
{
	NebulaP n;
	QString uname = designation.toUpper();
	// If no match found, try search by catalog reference
	static QRegExp catNumRx("^(M|NGC|IC|C|B|VDB|RCW|LDN|LBN|CR|MEL|PGC|UGC|ARP|VV|ABELL|DWB)\\s*(\\d+)$");
	if (catNumRx.exactMatch(uname))
	{
		QString cat = catNumRx.capturedTexts().at(1);
		unsigned int num = catNumRx.capturedTexts().at(2).toUInt();
		if (cat == "M") n = searchM(num);
		if (cat == "NGC") n = searchNGC(num);
		if (cat == "IC") n = searchIC(num);
		if (cat == "C") n = searchC(num);
		if (cat == "B") n = searchB(num);
		if (cat == "VDB") n = searchVdB(num);
		if (cat == "RCW") n = searchRCW(num);
		if (cat == "LDN") n = searchLDN(num);
		if (cat == "LBN") n = searchLBN(num);
		if (cat == "CR") n = searchCr(num);
		if (cat == "MEL") n = searchMel(num);
		if (cat == "PGC") n = searchPGC(num);
		if (cat == "UGC") n = searchUGC(num);
		if (cat == "ARP") n = searchArp(num);
		if (cat == "VV") n = searchVV(num);
		if (cat == "ABELL") n = searchAbell(num);
		if (cat == "DWB") n = searchDWB(num);
	}
	static QRegExp dCatNumRx("^(SH)\\s*\\d-\\s*(\\d+)$");
	if (dCatNumRx.exactMatch(uname))
	{
		QString dcat = dCatNumRx.capturedTexts().at(1);
		unsigned int dnum = dCatNumRx.capturedTexts().at(2).toUInt();

		if (dcat == "SH") n = searchSh2(dnum);
	}
	static QRegExp sCatNumRx("^(CED|PK|ACO|HCG|ESO|VDBH)\\s*(.+)$");
	if (sCatNumRx.exactMatch(uname))
	{
		QString cat = sCatNumRx.capturedTexts().at(1);
		QString num = sCatNumRx.capturedTexts().at(2).trimmed();

		if (cat == "CED") n = searchCed(num);
		if (cat == "PK") n = searchPK(num);
		if (cat == "ACO") n = searchACO(num);
		if (cat == "HCG") n = searchHCG(num);
		if (cat == "ESO") n = searchESO(num);
		if (cat == "VDBH") n = searchVdBH(num);
	}
	static QRegExp gCatNumRx("^(PN|SNR)\\s*G(.+)$");
	if (gCatNumRx.exactMatch(uname))
	{
		QString cat = gCatNumRx.capturedTexts().at(1);
		QString num = gCatNumRx.capturedTexts().at(2).trimmed();

		if (cat == "PN") n = searchPNG(num);
		if (cat == "SNR") n = searchSNRG(num);
	}

	if (n.isNull())
		return StelObjectP();
	else
		return qSharedPointerCast<StelObject>(n);
}

//! Find and return the list of at most maxNbItem objects auto-completing the passed object name
QStringList NebulaMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords, bool inEnglish) const
{
	QStringList result;
	if (maxNbItem <= 0)
	{
		return result;
	}

	QString objw = objPrefix.toUpper();

	// Search by Messier objects number (possible formats are "M31" or "M 31")
	if (objw.size()>=1 && objw.left(1)=="M" && objw.left(3)!="MEL")
	{
		for (const auto& n : dsoArray)
		{
			if (n->M_nb==0) continue;
			QString constw = QString("M%1").arg(n->M_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("M %1").arg(n->M_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Melotte objects number (possible formats are "Mel31" or "Mel 31")
	if (objw.size()>=1 && objw.left(3)=="MEL")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Mel_nb==0) continue;
			QString constw = QString("MEL%1").arg(n->Mel_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("MEL %1").arg(n->Mel_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by IC objects number (possible formats are "IC466" or "IC 466")
	if (objw.size()>=1 && objw.left(2)=="IC")
	{
		for (const auto& n : dsoArray)
		{
			if (n->IC_nb==0) continue;
			QString constw = QString("IC%1").arg(n->IC_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("IC %1").arg(n->IC_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	for (const auto& n : dsoArray)
	{
		if (n->NGC_nb==0) continue;
		QString constw = QString("NGC%1").arg(n->NGC_nb);
		QString constws = constw.mid(0, objw.size());
		if (constws==objw)
		{
			result << constws;
			continue;
		}
		constw = QString("NGC %1").arg(n->NGC_nb);
		constws = constw.mid(0, objw.size());
		if (constws==objw)
			result << constw;
	}

	// Search by PGC object numbers (possible formats are "PGC31" or "PGC 31")
	if (objw.size()>=1 && objw.left(3)=="PGC")
	{
		for (const auto& n : dsoArray)
		{
			if (n->PGC_nb==0) continue;
			QString constw = QString("PGC%1").arg(n->PGC_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;	// Prevent adding both forms for name
				continue;
			}
			constw = QString("PGC %1").arg(n->PGC_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by UGC object numbers (possible formats are "UGC31" or "UGC 31")
	if (objw.size()>=1 && objw.left(3)=="UGC")
	{
		for (const auto& n : dsoArray)
		{
			if (n->UGC_nb==0) continue;
			QString constw = QString("UGC%1").arg(n->UGC_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("UGC %1").arg(n->UGC_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Caldwell objects number (possible formats are "C31" or "C 31")
	if (objw.size()>=1 && objw.left(1)=="C" && objw.left(2)!="CR" && objw.left(2)!="CE")
	{
		for (const auto& n : dsoArray)
		{
			if (n->C_nb==0) continue;
			QString constw = QString("C%1").arg(n->C_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("C %1").arg(n->C_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Collinder objects number (possible formats are "Cr31" or "Cr 31")
	if (objw.size()>=1 && objw.left(2)=="CR")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Cr_nb==0) continue;
			QString constw = QString("CR%1").arg(n->Cr_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("CR %1").arg(n->Cr_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Ced objects number (possible formats are "Ced31" or "Ced 31")
	if (objw.size()>=1 && objw.left(3)=="CED")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Ced_nb.isEmpty()) continue;
			QString constw = QString("Ced%1").arg(n->Ced_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Ced %1").arg(n->Ced_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by Barnard objects number (possible formats are "B31" or "B 31")
	if (objw.size()>=1 && objw.left(1)=="B")
	{
		for (const auto& n : dsoArray)
		{
			if (n->B_nb==0) continue;
			QString constw = QString("B%1").arg(n->B_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("B %1").arg(n->B_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Sharpless objects number (possible formats are "Sh2-31" or "Sh 2-31")
	if (objw.size()>=1 && objw.left(2)=="SH")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Sh2_nb==0) continue;
			QString constw = QString("SH2-%1").arg(n->Sh2_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("SH 2-%1").arg(n->Sh2_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Van den Bergh objects number (possible formats are "VdB31" or "VdB 31")
	if (objw.size()>=1 && objw.left(3)=="VDB")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VdB_nb==0) continue;
			QString constw = QString("VDB%1").arg(n->VdB_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("VDB %1").arg(n->VdB_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by RCW objects number (possible formats are "RCW31" or "RCW 31")
	if (objw.size()>=1 && objw.left(3)=="RCW")
	{
		for (const auto& n : dsoArray)
		{
			if (n->RCW_nb==0) continue;
			QString constw = QString("RCW%1").arg(n->RCW_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("RCW %1").arg(n->RCW_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by LDN objects number (possible formats are "LDN31" or "LDN 31")
	if (objw.size()>=1 && objw.left(3)=="LDN")
	{
		for (const auto& n : dsoArray)
		{
			if (n->LDN_nb==0) continue;
			QString constw = QString("LDN%1").arg(n->LDN_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LDN %1").arg(n->LDN_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by LBN objects number (possible formats are "LBN31" or "LBN 31")
	if (objw.size()>=1 && objw.left(3)=="LBN")
	{
		for (const auto& n : dsoArray)
		{
			if (n->LBN_nb==0) continue;
			QString constw = QString("LBN%1").arg(n->LBN_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("LBN %1").arg(n->LBN_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by Arp objects number
	if (objw.size()>=1 && objw.left(3)=="ARP")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Arp_nb==0) continue;
			QString constw = QString("ARP%1").arg(n->Arp_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("ARP %1").arg(n->Arp_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by VV objects number
	if (objw.size()>=1 && objw.left(2)=="VV")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VV_nb==0) continue;
			QString constw = QString("VV%1").arg(n->VV_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("VV %1").arg(n->VV_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
				result << constw;
		}
	}

	// Search by PK objects number
	if (objw.size()>=1 && objw.left(2)=="PK")
	{
		for (const auto& n : dsoArray)
		{
			if (n->PK_nb.isEmpty()) continue;
			QString constw = QString("PK%1").arg(n->PK_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("PK %1").arg(n->PK_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by PN G objects number
	if (objw.size()>=1 && objw.left(2)=="PN")
	{
		for (const auto& n : dsoArray)
		{
			if (n->PNG_nb.isEmpty()) continue;
			QString constw = QString("PNG%1").arg(n->PNG_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("PN G%1").arg(n->PNG_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by SNR G objects number
	if (objw.size()>=1 && objw.left(3)=="SNR")
	{
		for (const auto& n : dsoArray)
		{
			if (n->SNRG_nb.isEmpty()) continue;
			QString constw = QString("SNRG%1").arg(n->SNRG_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("SNR G%1").arg(n->SNRG_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by ACO objects number
	if (objw.size()>=1 && objw.left(3)=="ACO")
	{
		for (const auto& n : dsoArray)
		{
			if (n->ACO_nb.isEmpty()) continue;
			QString constw = QString("ACO%1").arg(n->ACO_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("ACO %1").arg(n->ACO_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by Abell objects number
	if (objw.size()>=1 && objw.left(5)=="ABELL")
	{
		for (const auto& n : dsoArray)
		{
			if (n->Abell_nb==0) continue;
			QString constw = QString("Abell%1").arg(n->Abell_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("Abell %1").arg(n->Abell_nb);
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by HCG objects number
	if (objw.size()>=1 && objw.left(3)=="HCG")
	{
		for (const auto& n : dsoArray)
		{
			if (n->HCG_nb.isEmpty()) continue;
			QString constw = QString("HCG%1").arg(n->HCG_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("HCG %1").arg(n->HCG_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by ESO objects number
	if (objw.size()>=1 && objw.left(3)=="ESO")
	{
		for (const auto& n : dsoArray)
		{
			if (n->ESO_nb.isEmpty()) continue;
			QString constw = QString("ESO%1").arg(n->ESO_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("ESO %1").arg(n->ESO_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by VdBH objects number
	if (objw.size()>=1 && objw.left(4)=="VDBH")
	{
		for (const auto& n : dsoArray)
		{
			if (n->VdBH_nb.isEmpty()) continue;
			QString constw = QString("VdBH%1").arg(n->VdBH_nb.trimmed());
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("VdBH %1").arg(n->VdBH_nb.trimmed());
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by DWB objects number
	if (objw.size()>=1 && objw.left(3)=="DWB")
	{
		for (const auto& n : dsoArray)
		{
			if (n->DWB_nb==0) continue;
			QString constw = QString("DWB%1").arg(n->DWB_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
			{
				result << constws;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("DWB %1").arg(n->DWB_nb);
			constws = constw.mid(0, objw.size());
			if (constws.toUpper()==objw.toUpper())
				result << constw;
		}
	}

	// Search by common names
	for (const auto& n : dsoArray)
	{
		QString name = inEnglish ? n->englishName : n->nameI18;
		if (matchObjectName(name, objPrefix, useStartOfWords))
		{
			result.append(name);
		}
	}

	if (getFlagAdditionalNames())
	{
		// Search by aliases of common names
		for (const auto& n : dsoArray)
		{
			QStringList nameList = inEnglish ? n->englishAliases : n->nameI18Aliases;
			for (auto name : nameList)
			{
				if (matchObjectName(name, objPrefix, useStartOfWords))
					result.append(name);
			}
		}
	}

	result.sort();
	if (result.size() > maxNbItem)
	{
		result.erase(result.begin() + maxNbItem, result.end());
	}

	return result;
}

QStringList NebulaMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	for (const auto& n : dsoArray)
	{
		if (!n->getEnglishName().isEmpty())
		{
			if (inEnglish)
				result << n->getEnglishName();
			else
				result << n->getNameI18n();
		}
	}
	return result;
}

QStringList NebulaMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;
	int type = objType.toInt();
	switch (type)
	{
		case 0: // Bright galaxies?
			for (const auto& n : dsoArray)
			{
				if (n->nType==type && qMin(n->vMag, n->bMag)<=10.f)
				{
					if (!n->getEnglishName().isEmpty())
					{
						if (inEnglish)
							result << n->getEnglishName();
						else
							result << n->getNameI18n();
					}
					else if (n->NGC_nb>0)
						result << QString("NGC %1").arg(n->NGC_nb);
					else if (n->IC_nb>0)
						result << QString("IC %1").arg(n->IC_nb);
					else if (n->M_nb>0)
						result << QString("M %1").arg(n->M_nb);
					else if (n->C_nb>0)
						result << QString("C %1").arg(n->C_nb);
				}
			}
			break;
		case 100: // Messier Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("M%1").arg(n->M_nb);
			break;
		case 101: // Caldwell Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("C%1").arg(n->C_nb);
			break;
		case 102: // Barnard Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("B %1").arg(n->B_nb);
			break;
		case 103: // Sharpless Catalogue?
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("SH 2-%1").arg(n->Sh2_nb);
			break;
		case 104: // Van den Bergh Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("VdB %1").arg(n->VdB_nb);
			break;
		case 105: // RCW Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("RCW %1").arg(n->RCW_nb);
			break;
		case 106: // Collinder Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Cr %1").arg(n->Cr_nb);
			break;
		case 107: // Melotte Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Mel %1").arg(n->Mel_nb);
			break;
		case 108: // New General Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("NGC %1").arg(n->NGC_nb);
			break;
		case 109: // Index Catalogue
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("IC %1").arg(n->IC_nb);
			break;
		case 110: // Lynds' Catalogue of Bright Nebulae
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("LBN %1").arg(n->LBN_nb);
			break;
		case 111: // Lynds' Catalogue of Dark Nebulae
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("LDN %1").arg(n->LDN_nb);
			break;
		case 114: // Cederblad Catalog
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Ced %1").arg(n->Ced_nb);
			break;
		case 115: // Atlas of Peculiar Galaxies (Arp)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Arp %1").arg(n->Arp_nb);
			break;
		case 116: // The Catalogue of Interacting Galaxies by Vorontsov-Velyaminov (VV)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("VV %1").arg(n->VV_nb);
			break;
		case 117: // Catalogue of Galactic Planetary Nebulae (PK)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("PK %1").arg(n->PK_nb);
			break;
		case 118: // Strasbourg-ESO Catalogue of Galactic Planetary Nebulae by Acker et. al. (PN G)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("PN G%1").arg(n->PNG_nb);
			break;
		case 119: // A catalogue of Galactic supernova remnants by Green (SNR G)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("SNR G%1").arg(n->SNRG_nb);
			break;
		case 120: // A Catalog of Rich Clusters of Galaxies by Abell et. al. (ACO)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("ACO %1").arg(n->ACO_nb);
			break;
		case 121: // Hickson Compact Group by Hickson et. al. (HCG)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("HCG %1").arg(n->HCG_nb);
			break;
		case 122: // Abell Catalog of Planetary Nebulae (Abell)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("Abell %1").arg(n->Abell_nb);
			break;
		case 123: // ESO/Uppsala Survey of the ESO(B) Atlas (ESO)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("ESO %1").arg(n->ESO_nb);
			break;
		case 124: // Catalogue of southern stars embedded in nebulosity (VdBH)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("VdBH %1").arg(n->VdBH_nb);
			break;
		case 125: // Catalogue and distances of optically visible H II regions (DWB)
			for (const auto& n : getDeepSkyObjectsByType(objType))
				result << QString("DWB %1").arg(n->DWB_nb);
			break;
		case 150: // Dwarf galaxies [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(DWARF_GALAXIES) / sizeof(DWARF_GALAXIES[0]); i++)
				result << QString("PGC %1").arg(DWARF_GALAXIES[i]);
			break;
		}
		case 151: // Herschel 400 Catalogue [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(H400_LIST) / sizeof(H400_LIST[0]); i++)
				result << QString("NGC %1").arg(H400_LIST[i]);
			break;
		}
		case 152: // Jack Bennett's deep sky catalogue [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(BENNETT_LIST) / sizeof(BENNETT_LIST[0]); i++)
				result << QString("NGC %1").arg(BENNETT_LIST[i]);
			result << "Mel 105" << "IC 1459";
			break;
		}
		case 153: // James Dunlop's deep sky catalogue [see NebulaList.hpp]
		{
			for (unsigned int i = 0; i < sizeof(DUNLOP_LIST) / sizeof(DUNLOP_LIST[0]); i++)
				result << QString("NGC %1").arg(DUNLOP_LIST[i]);
			break;
		}
		default:
		{
			for (const auto& n : dsoArray)
			{
				if (n->nType==type)
				{
					if (!n->getEnglishName().isEmpty())
					{
						if (inEnglish)
							result << n->getEnglishName();
						else
							result << n->getNameI18n();
					}
					else if (n->NGC_nb>0)
						result << QString("NGC %1").arg(n->NGC_nb);
					else if (n->IC_nb>0)
						result << QString("IC %1").arg(n->IC_nb);
					else if (n->M_nb>0)
						result << QString("M %1").arg(n->M_nb);
					else if (n->C_nb>0)
						result << QString("C %1").arg(n->C_nb);
					else if (n->B_nb>0)
						result << QString("B %1").arg(n->B_nb);
					else if (n->Sh2_nb>0)
						result << QString("SH 2-%1").arg(n->Sh2_nb);
					else if (n->VdB_nb>0)
						result << QString("VdB %1").arg(n->VdB_nb);
					else if (n->RCW_nb>0)
						result << QString("RCW %1").arg(n->RCW_nb);
					else if (n->LBN_nb>0)
						result << QString("LBN %1").arg(n->LBN_nb);
					else if (n->LDN_nb>0)
						result << QString("LDN %1").arg(n->LDN_nb);
					else if (n->Cr_nb>0)
						result << QString("Cr %1").arg(n->Cr_nb);
					else if (n->Mel_nb>0)
						result << QString("Mel %1").arg(n->Mel_nb);
					else if (!n->Ced_nb.isEmpty())
						result << QString("Ced %1").arg(n->Ced_nb);
					else if (n->Arp_nb>0)
						result << QString("Arp %1").arg(n->Arp_nb);
					else if (n->VV_nb>0)
						result << QString("VV %1").arg(n->VV_nb);
					else if (!n->PK_nb.isEmpty())
						result << QString("PK %1").arg(n->PK_nb);
					else if (!n->PNG_nb.isEmpty())
						result << QString("PN G%1").arg(n->PNG_nb);
					else if (!n->SNRG_nb.isEmpty())
						result << QString("SNR G%1").arg(n->SNRG_nb);
					else if (n->PGC_nb>0)
						result << QString("PGC %1").arg(n->PGC_nb);
					else if (n->UGC_nb > 0)
						result << QString("UGC %1").arg(n->UGC_nb);
					else if (!n->ACO_nb.isEmpty())
						result << QString("ACO %1").arg(n->ACO_nb);
					else if (!n->HCG_nb.isEmpty())
						result << QString("HCG %1").arg(n->HCG_nb);
					else if (n->Abell_nb>0)
						result << QString("Abell %1").arg(n->Abell_nb);
					else if (!n->ESO_nb.isEmpty())
						result << QString("ESO %1").arg(n->ESO_nb);
					else if (!n->VdBH_nb.isEmpty())
						result << QString("VdBH %1").arg(n->VdBH_nb);
					else if (n->DWB_nb>0)
						result << QString("DWB %1").arg(n->DWB_nb);
				}
			}
			break;
		}
	}

	result.removeDuplicates();
	return result;
}

QList<NebulaP> NebulaMgr::getDeepSkyObjectsByType(const QString &objType) const
{
	QList<NebulaP> dso;
	int type = objType.toInt();
	switch (type)
	{
		case 100: // Messier Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->M_nb>0)
					dso.append(n);
			}
			break;
		case 101: // Caldwell Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->C_nb>0)
					dso.append(n);
			}
			break;
		case 102: // Barnard Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->B_nb>0)
					dso.append(n);
			}
			break;
		case 103: // Sharpless Catalogue?
			for (const auto& n : dsoArray)
			{
				if (n->Sh2_nb>0)
					dso.append(n);
			}
			break;
		case 104: // Van den Bergh Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->VdB_nb>0)
					dso.append(n);
			}
			break;
		case 105: // RCW Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->RCW_nb>0)
					dso.append(n);
			}
			break;
		case 106: // Collinder Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->Cr_nb>0)
					dso.append(n);
			}
			break;
		case 107: // Melotte Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->Mel_nb>0)
					dso.append(n);
			}
			break;
		case 108: // New General Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->NGC_nb>0)
					dso.append(n);
			}
			break;
		case 109: // Index Catalogue
			for (const auto& n : dsoArray)
			{
				if (n->IC_nb>0)
					dso.append(n);
			}
			break;
		case 110: // Lynds' Catalogue of Bright Nebulae
			for (const auto& n : dsoArray)
			{
				if (n->LBN_nb>0)
					dso.append(n);
			}
			break;
		case 111: // Lynds' Catalogue of Dark Nebulae
			for (const auto& n : dsoArray)
			{
				if (n->LDN_nb>0)
					dso.append(n);
			}
			break;
		case 112: // Principal Galaxy Catalog
			for (const auto& n : dsoArray)
			{
				if (n->PGC_nb>0)
					dso.append(n);
			}
			break;
		case 113: // The Uppsala General Catalogue of Galaxies
			for (const auto& n : dsoArray)
			{
				if (n->UGC_nb>0)
					dso.append(n);
			}
			break;
		case 114: // Cederblad Catalog
			for (const auto& n : dsoArray)
			{
				if (!n->Ced_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 115: // Atlas of Peculiar Galaxies (Arp)
			for (const auto& n : dsoArray)
			{
				if (n->Arp_nb>0)
					dso.append(n);
			}
			break;
		case 116: // The Catalogue of Interacting Galaxies by Vorontsov-Velyaminov (VV)
			for (const auto& n : dsoArray)
			{
				if (n->VV_nb>0)
					dso.append(n);
			}
			break;
		case 117: // Catalogue of Galactic Planetary Nebulae (PK)
			for (const auto& n : dsoArray)
			{
				if (!n->PK_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 118: // Strasbourg-ESO Catalogue of Galactic Planetary Nebulae by Acker et. al. (PN G)
			for (const auto& n : dsoArray)
			{
				if (!n->PNG_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 119: // A catalogue of Galactic supernova remnants by Green (SNR G)
			for (const auto& n : dsoArray)
			{
				if (!n->SNRG_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 120: // A Catalog of Rich Clusters of Galaxies by Abell et. al. (ACO)
			for (const auto& n : dsoArray)
			{
				if (!n->ACO_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 121: // Hickson Compact Group by Hickson et. al. (HCG)
			for (const auto& n : dsoArray)
			{
				if (!n->HCG_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 122: // Abell Catalog of Planetary Nebulae (Abell)
			for (const auto& n : dsoArray)
			{
				if (n->Abell_nb>0)
					dso.append(n);
			}
			break;
		case 123: // ESO/Uppsala Survey of the ESO(B) Atlas (ESO)
			for (const auto& n : dsoArray)
			{
				if (!n->ESO_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 124: // Catalogue of southern stars embedded in nebulosity (VdBH)
			for (const auto& n : dsoArray)
			{
				if (!n->VdBH_nb.isEmpty())
					dso.append(n);
			}
			break;
		case 125: // Catalogue and distances of optically visible H II regions (DWB)
			for (const auto& n : dsoArray)
			{
				if (n->DWB_nb > 0)
					dso.append(n);
			}
			break;
		case 150: // Dwarf galaxies [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(DWARF_GALAXIES) / sizeof(DWARF_GALAXIES[0]); i++)
			{
				ds = searchPGC(DWARF_GALAXIES[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			break;
		}
		case 151: // Herschel 400 Catalogue [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(H400_LIST) / sizeof(H400_LIST[0]); i++)
			{
				ds = searchNGC(H400_LIST[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			break;
		}
		case 152: // Jack Bennett's deep sky catalogue [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(BENNETT_LIST) / sizeof(BENNETT_LIST[0]); i++)
			{
				ds = searchNGC(BENNETT_LIST[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			ds = searchMel(105);
			if (!ds.isNull())
				dso.append(ds);
			ds = searchIC(1459);
			if (!ds.isNull())
				dso.append(ds);
			break;
		}
		case 153: // James Dunlop's deep sky catalogue [see NebulaList.hpp]
		{
			NebulaP ds;
			for (unsigned int i = 0; i < sizeof(DUNLOP_LIST) / sizeof(DUNLOP_LIST[0]); i++)
			{
				ds = searchNGC(DUNLOP_LIST[i]);
				if (!ds.isNull())
					dso.append(ds);
			}
			break;
		}
		default:
		{
			for (const auto& n : dsoArray)
			{
				if (n->nType==type)
					dso.append(n);
			}
			break;
		}
	}

	return dso;
}
