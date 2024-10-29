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
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	ui->colorDSOLabels                    ->setup("NebulaMgr.labelsColor",                     "color/dso_label_color");
	ui->colorDSOMarkers                   ->setup("NebulaMgr.circlesColor",                    "color/dso_circle_color");
	ui->colorDSOGalaxies                  ->setup("NebulaMgr.galaxiesColor",                   "color/dso_galaxy_color");
	ui->colorDSOActiveGalaxies            ->setup("NebulaMgr.activeGalaxiesColor",             "color/dso_active_galaxy_color");
	ui->colorDSORadioGalaxies             ->setup("NebulaMgr.radioGalaxiesColor",              "color/dso_radio_galaxy_color");
	ui->colorDSOInteractingGalaxies       ->setup("NebulaMgr.interactingGalaxiesColor",        "color/dso_interacting_galaxy_color");
	ui->colorDSOQuasars                   ->setup("NebulaMgr.quasarsColor",                    "color/dso_quasar_color");
	ui->colorDSOPossibleQuasars           ->setup("NebulaMgr.possibleQuasarsColor",            "color/dso_possible_quasar_color");
	ui->colorDSOStarClusters              ->setup("NebulaMgr.clustersColor",                   "color/dso_cluster_color");
	ui->colorDSOOpenStarClusters          ->setup("NebulaMgr.openClustersColor",               "color/dso_open_cluster_color");
	ui->colorDSOGlobularStarClusters      ->setup("NebulaMgr.globularClustersColor",           "color/dso_globular_cluster_color");
	ui->colorDSOStellarAssociations       ->setup("NebulaMgr.stellarAssociationsColor",        "color/dso_stellar_association_color");
	ui->colorDSOStarClouds                ->setup("NebulaMgr.starCloudsColor",                 "color/dso_star_cloud_color");
	ui->colorDSOStars                     ->setup("NebulaMgr.starsColor",                      "color/dso_star_color");
	ui->colorDSOSymbioticStars            ->setup("NebulaMgr.symbioticStarsColor",             "color/dso_symbiotic_star_color");
	ui->colorDSOEmissionLineStars         ->setup("NebulaMgr.emissionLineStarsColor",          "color/dso_emission_star_color");
	ui->colorDSONebulae                   ->setup("NebulaMgr.nebulaeColor",                    "color/dso_nebula_color");
	ui->colorDSOPlanetaryNebulae          ->setup("NebulaMgr.planetaryNebulaeColor",           "color/dso_planetary_nebula_color");
	ui->colorDSODarkNebulae               ->setup("NebulaMgr.darkNebulaeColor",                "color/dso_dark_nebula_color");
	ui->colorDSOReflectionNebulae         ->setup("NebulaMgr.reflectionNebulaeColor",          "color/dso_reflection_nebula_color");
	ui->colorDSOBipolarNebulae            ->setup("NebulaMgr.bipolarNebulaeColor",             "color/dso_bipolar_nebula_color");
	ui->colorDSOEmissionNebulae           ->setup("NebulaMgr.emissionNebulaeColor",            "color/dso_emission_nebula_color");
	ui->colorDSOPossiblePlanetaryNebulae  ->setup("NebulaMgr.possiblePlanetaryNebulaeColor",   "color/dso_possible_planetary_nebula_color");
	ui->colorDSOProtoplanetaryNebulae     ->setup("NebulaMgr.protoplanetaryNebulaeColor",      "color/dso_protoplanetary_nebula_color");
	ui->colorDSOHydrogenRegions           ->setup("NebulaMgr.hydrogenRegionsColor",            "color/dso_hydrogen_region_color");
	ui->colorDSOInterstellarMatter        ->setup("NebulaMgr.interstellarMatterColor",         "color/dso_interstellar_matter_color");
	ui->colorDSOEmissionObjects           ->setup("NebulaMgr.emissionObjectsColor",            "color/dso_emission_object_color");
	ui->colorDSOMolecularClouds           ->setup("NebulaMgr.molecularCloudsColor",            "color/dso_molecular_cloud_color");
	ui->colorDSOBLLacObjects              ->setup("NebulaMgr.blLacObjectsColor",               "color/dso_bl_lac_color");
	ui->colorDSOBlazars                   ->setup("NebulaMgr.blazarsColor",                    "color/dso_blazar_color");
	ui->colorDSOYoungStellarObjects       ->setup("NebulaMgr.youngStellarObjectsColor",        "color/dso_young_stellar_object_color");
	ui->colorDSOSupernovaRemnants         ->setup("NebulaMgr.supernovaRemnantsColor",          "color/dso_supernova_remnant_color");
	ui->colorDSOSupernovaCandidates       ->setup("NebulaMgr.supernovaCandidatesColor",        "color/dso_supernova_candidate_color");
	ui->colorDSOSupernovaRemnantCandidates->setup("NebulaMgr.supernovaRemnantCandidatesColor", "color/dso_supernova_remnant_cand_color");
	ui->colorDSOGalaxyClusters            ->setup("NebulaMgr.galaxyClustersColor",             "color/dso_galaxy_cluster_color");
	ui->colorDSORegions                   ->setup("NebulaMgr.regionsColor",                    "color/dso_regions_color");
}
