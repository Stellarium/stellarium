/*
 * Stellarium
 * Copyright (C) 2016 Alexander Wolf
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPropertyMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "ConfigureDSOColorsDialog.hpp"
#include "ui_dsoColorsDialog.h"

#include <QSettings>
#include <QColorDialog>

ConfigureDSOColorsDialog::ConfigureDSOColorsDialog() : StelDialog("ConfigureDSOColorsDialog")
{
	ui = new Ui_ConfigureDSOColorsDialogForm;
}

ConfigureDSOColorsDialog::~ConfigureDSOColorsDialog()
{
	delete ui;
}

void ConfigureDSOColorsDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ConfigureDSOColorsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectColorButton(ui->colorDSOLabels,                     "NebulaMgr.labelsColor",                     "color/dso_label_color");
	connectColorButton(ui->colorDSOMarkers,                    "NebulaMgr.circlesColor",                    "color/dso_circle_color");
	connectColorButton(ui->colorDSOGalaxies,                   "NebulaMgr.galaxiesColor",                   "color/dso_galaxy_color");
	connectColorButton(ui->colorDSOActiveGalaxies,             "NebulaMgr.activeGalaxiesColor",             "color/dso_active_galaxy_color");
	connectColorButton(ui->colorDSORadioGalaxies,              "NebulaMgr.radioGalaxiesColor",              "color/dso_radio_galaxy_color");
	connectColorButton(ui->colorDSOInteractingGalaxies,        "NebulaMgr.interactingGalaxiesColor",        "color/dso_interacting_galaxy_color");
	connectColorButton(ui->colorDSOQuasars,                    "NebulaMgr.quasarsColor",                    "color/dso_quasar_color");
	connectColorButton(ui->colorDSOPossibleQuasars,            "NebulaMgr.possibleQuasarsColor",            "color/dso_possible_quasar_color");
	connectColorButton(ui->colorDSOStarClusters,               "NebulaMgr.clustersColor",                   "color/dso_cluster_color");
	connectColorButton(ui->colorDSOOpenStarClusters,           "NebulaMgr.openClustersColor",               "color/dso_open_cluster_color");
	connectColorButton(ui->colorDSOGlobularStarClusters,       "NebulaMgr.globularClustersColor",           "color/dso_globular_cluster_color");
	connectColorButton(ui->colorDSOStellarAssociations,        "NebulaMgr.stellarAssociationsColor",        "color/dso_stellar_association_color");
	connectColorButton(ui->colorDSOStarClouds,                 "NebulaMgr.starCloudsColor",                 "color/dso_star_cloud_color");
	connectColorButton(ui->colorDSOStars,                      "NebulaMgr.starsColor",                      "color/dso_star_color");
	connectColorButton(ui->colorDSOSymbioticStars,             "NebulaMgr.symbioticStarsColor",             "color/dso_symbiotic_star_color");
	connectColorButton(ui->colorDSOEmissionLineStars,          "NebulaMgr.emissionLineStarsColor",          "color/dso_emission_star_color");
	connectColorButton(ui->colorDSONebulae,                    "NebulaMgr.nebulaeColor",                    "color/dso_nebula_color");
	connectColorButton(ui->colorDSOPlanetaryNebulae,           "NebulaMgr.planetaryNebulaeColor",           "color/dso_planetary_nebula_color");
	connectColorButton(ui->colorDSODarkNebulae,                "NebulaMgr.darkNebulaeColor",                "color/dso_dark_nebula_color");
	connectColorButton(ui->colorDSOReflectionNebulae,          "NebulaMgr.reflectionNebulaeColor",          "color/dso_reflection_nebula_color");
	connectColorButton(ui->colorDSOBipolarNebulae,             "NebulaMgr.bipolarNebulaeColor",             "color/dso_bipolar_nebula_color");
	connectColorButton(ui->colorDSOEmissionNebulae,            "NebulaMgr.emissionNebulaeColor",            "color/dso_emission_nebula_color");
	connectColorButton(ui->colorDSOPossiblePlanetaryNebulae,   "NebulaMgr.possiblePlanetaryNebulaeColor",   "color/dso_possible_planetary_nebula_color");
	connectColorButton(ui->colorDSOProtoplanetaryNebulae,      "NebulaMgr.protoplanetaryNebulaeColor",      "color/dso_protoplanetary_nebula_color");
	connectColorButton(ui->colorDSOHydrogenRegions,            "NebulaMgr.hydrogenRegionsColor",            "color/dso_hydrogen_region_color");
	connectColorButton(ui->colorDSOInterstellarMatter,         "NebulaMgr.interstellarMatterColor",         "color/dso_interstellar_matter_color");
	connectColorButton(ui->colorDSOEmissionObjects,            "NebulaMgr.emissionObjectsColor",            "color/dso_emission_object_color");
	connectColorButton(ui->colorDSOMolecularClouds,            "NebulaMgr.molecularCloudsColor",            "color/dso_molecular_cloud_color");
	connectColorButton(ui->colorDSOBLLacObjects,               "NebulaMgr.blLacObjectsColor",               "color/dso_bl_lac_color");
	connectColorButton(ui->colorDSOBlazars,                    "NebulaMgr.blazarsColor",                    "color/dso_blazar_color");
	connectColorButton(ui->colorDSOYoungStellarObjects,        "NebulaMgr.youngStellarObjectsColor",        "color/dso_young_stellar_object_color");
	connectColorButton(ui->colorDSOSupernovaRemnants,          "NebulaMgr.supernovaRemnantsColor",          "color/dso_supernova_remnant_color");
	connectColorButton(ui->colorDSOSupernovaCandidates,        "NebulaMgr.supernovaCandidatesColor",        "color/dso_supernova_candidate_color");
	connectColorButton(ui->colorDSOSupernovaRemnantCandidates, "NebulaMgr.supernovaRemnantCandidatesColor", "color/dso_supernova_remnant_cand_color");
	connectColorButton(ui->colorDSOGalaxyClusters,             "NebulaMgr.galaxyClustersColor",             "color/dso_galaxy_cluster_color");
	connectColorButton(ui->colorDSORegions,                    "NebulaMgr.regionsColor",                    "color/dso_regions_color");
}
