/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
 * Copyright (C) 2012 Bogdan Marinov
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

#include "Dialog.hpp"
#include "ConfigurationDialog.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelMainWindow.hpp"
#include "ui_configurationDialog.h"
#include "StelAppGraphicsWidget.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelProjector.hpp"
#include "StelObjectMgr.hpp"

#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocation.hpp"
#include "LandscapeMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "SolarSystem.hpp"
#include "MeteorMgr.hpp"
#include "ConstellationMgr.hpp"
#include "StarMgr.hpp"
#include "NebulaMgr.hpp"
#include "GridLinesMgr.hpp"
#ifndef DISABLE_SCRIPTING
#include "StelScriptMgr.hpp"
#endif
#include "LabelMgr.hpp"
#include "ScreenImageMgr.hpp"
#include "SkyGui.hpp"
#include "StelJsonParser.hpp"
#include "StelTranslator.hpp"

#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QFileDialog>

ConfigurationDialog::ConfigurationDialog(StelGui* agui) : StelDialog(agui), starCatalogDownloadReply(NULL), currentDownloadFile(NULL), progressBar(NULL), gui(agui)
{
	ui = new Ui_configurationDialogForm;
	hasDownloadedStarCatalog = false;
	isDownloadingStarCatalog = false;
	savedProjectionType = StelApp::getInstance().getCore()->getCurrentProjectionType();	
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
	ui = 0;
}

void ConfigurationDialog::retranslate()
{
	if (dialog) {
		ui->retranslateUi(dialog);

		//Hack to shrink the tabs to optimal size after language change
		//by causing the list items to be laid out again.
		ui->stackListWidget->setWrapping(false);
		updateTabBarListWidgetWidth();
		
		//Initial FOV and direction on the "Main" page
		updateConfigLabels();
		
		//Star catalog download button and info
		updateStarCatalogControlsText();

		//Script information
		//(trigger re-displaying the description of the current item)
		#ifndef DISABLE_SCRIPTING
		scriptSelectionChanged(ui->scriptListWidget->currentItem()->text());
		#endif

		//Plug-in information
		populatePluginsList();
	}
}

void ConfigurationDialog::styleChanged()
{
	// Nothing for now
}

void ConfigurationDialog::updateIconsColor()
{
	QPixmap pixmap(50, 50);
	QStringList icons;
	icons << "main" << "info" << "navigation" << "tools" << "scripts" << "plugins";
	bool redIcon = false;
	if (StelApp::getInstance().getVisionModeNight())
		redIcon = true;

	foreach(const QString &iconName, icons)
	{
		pixmap.load(":/graphicGui/tabicon-" + iconName +".png");
		if (redIcon)
			pixmap = StelButton::makeRed(pixmap);

		ui->stackListWidget->item(icons.indexOf(iconName))->setIcon(QIcon(pixmap));
	}
}

void ConfigurationDialog::createDialogContent()
{
	const StelProjectorP proj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000);
	StelCore* core = StelApp::getInstance().getCore();

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);

	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(QString)), this, SLOT(updateIconsColor()));

	// Set the main tab activated by default
	ui->configurationStackedWidget->setCurrentIndex(0);
	updateIconsColor();
	ui->stackListWidget->setCurrentRow(0);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// Main tab
	// Fill the language list widget from the available list
	QString appLang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	QComboBox* cb = ui->programLanguageComboBox;
	cb->clear();
	cb->addItems(StelTranslator::globalTranslator.getAvailableLanguagesNamesNative(StelFileMgr::getLocaleDir()));
	cb->model()->sort(0);
	QString l2 = StelTranslator::iso639_1CodeToNativeName(appLang);
	int lt = cb->findText(l2, Qt::MatchExactly);
	if (lt == -1 && appLang.contains('_'))
	{
		l2 = appLang.left(appLang.indexOf('_'));
		l2=StelTranslator::iso639_1CodeToNativeName(l2);
		lt = cb->findText(l2, Qt::MatchExactly);
	}
	if (lt!=-1)
		cb->setCurrentIndex(lt);
	connect(cb, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectLanguage(const QString&)));

	connect(ui->getStarsButton, SIGNAL(clicked()), this, SLOT(downloadStars()));
	connect(ui->downloadCancelButton, SIGNAL(clicked()), this, SLOT(cancelDownload()));
	connect(ui->downloadRetryButton, SIGNAL(clicked()), this, SLOT(downloadStars()));
	resetStarCatalogControls();

	// Selected object info
	if (gui->getInfoTextFilters() == StelObject::InfoStringGroup(0))
	{
		ui->noSelectedInfoRadio->setChecked(true);
	}
	else if (gui->getInfoTextFilters() == StelObject::ShortInfo)
	{
		ui->briefSelectedInfoRadio->setChecked(true);	
	}
	else if (gui->getInfoTextFilters() == StelObject::AllInfo)
	{
		ui->allSelectedInfoRadio->setChecked(true);
	}
	else
	{
		ui->customSelectedInfoRadio->setChecked(true);
	}
	updateSelectedInfoCheckBoxes();
	
	connect(ui->noSelectedInfoRadio, SIGNAL(released()), this, SLOT(setNoSelectedInfo()));
	connect(ui->allSelectedInfoRadio, SIGNAL(released()), this, SLOT(setAllSelectedInfo()));
	connect(ui->briefSelectedInfoRadio, SIGNAL(released()), this, SLOT(setBriefSelectedInfo()));
	connect(ui->buttonGroupDisplayedFields, SIGNAL(buttonClicked(int)),
	        this, SLOT(setSelectedInfoFromCheckBoxes()));
	
	// Navigation tab
	// Startup time
	if (core->getStartupTimeMode()=="actual")
		ui->systemTimeRadio->setChecked(true);
	else if (core->getStartupTimeMode()=="today")
		ui->todayRadio->setChecked(true);
	else
		ui->fixedTimeRadio->setChecked(true);
	connect(ui->systemTimeRadio, SIGNAL(clicked(bool)), this, SLOT(setStartupTimeMode()));
	connect(ui->todayRadio, SIGNAL(clicked(bool)), this, SLOT(setStartupTimeMode()));
	connect(ui->fixedTimeRadio, SIGNAL(clicked(bool)), this, SLOT(setStartupTimeMode()));

	ui->todayTimeSpinBox->setTime(core->getInitTodayTime());
	connect(ui->todayTimeSpinBox, SIGNAL(timeChanged(QTime)), core, SLOT(setInitTodayTime(QTime)));
	ui->fixedDateTimeEdit->setMinimumDate(QDate(100,1,1));
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(core->getPresetSkyTime()));
	connect(ui->fixedDateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), core, SLOT(setPresetSkyTime(QDateTime)));

	ui->enableKeysNavigationCheckBox->setChecked(mvmgr->getFlagEnableMoveKeys() || mvmgr->getFlagEnableZoomKeys());
	ui->enableMouseNavigationCheckBox->setChecked(mvmgr->getFlagEnableMouseNavigation());
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableMoveKeys(bool)));
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableZoomKeys(bool)));
	connect(ui->enableMouseNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableMouseNavigation(bool)));
	connect(ui->fixedDateTimeCurrentButton, SIGNAL(clicked()), this, SLOT(setFixedDateTimeToCurrent()));
	connect(ui->editShortcutsPushButton, SIGNAL(clicked()),
	        this,
	        SLOT(showShortcutsWindow()));

	// Tools tab
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	Q_ASSERT(cmgr);
	ui->sphericMirrorCheckbox->setChecked(StelMainGraphicsView::getInstance().getStelAppGraphicsWidget()->getViewportEffect() == "sphericMirrorDistorter");
	connect(ui->sphericMirrorCheckbox, SIGNAL(toggled(bool)), this, SLOT(setSphericMirror(bool)));
	ui->gravityLabelCheckbox->setChecked(proj->getFlagGravityLabels());
	connect(ui->gravityLabelCheckbox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlagGravityLabels(bool)));
	ui->selectSingleConstellationButton->setChecked(cmgr->getFlagIsolateSelected());
	connect(ui->selectSingleConstellationButton, SIGNAL(toggled(bool)), cmgr, SLOT(setFlagIsolateSelected(bool)));
	ui->diskViewportCheckbox->setChecked(proj->getMaskType() == StelProjector::MaskDisk);
	connect(ui->diskViewportCheckbox, SIGNAL(toggled(bool)), this, SLOT(setDiskViewport(bool)));
	ui->autoZoomResetsDirectionCheckbox->setChecked(mvmgr->getFlagAutoZoomOutResetsDirection());
	connect(ui->autoZoomResetsDirectionCheckbox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagAutoZoomOutResetsDirection(bool)));
	ui->renderSolarShadowsCheckbox->setChecked(StelApp::getInstance().getRenderSolarShadows());
	connect(ui->renderSolarShadowsCheckbox, SIGNAL(toggled(bool)), &StelApp::getInstance(), SLOT(setRenderSolarShadows(bool)));

	ui->showFlipButtonsCheckbox->setChecked(gui->getFlagShowFlipButtons());
	connect(ui->showFlipButtonsCheckbox, SIGNAL(toggled(bool)), gui, SLOT(setFlagShowFlipButtons(bool)));

	ui->showNebulaBgButtonCheckbox->setChecked(gui->getFlagShowNebulaBackgroundButton());
	connect(ui->showNebulaBgButtonCheckbox, SIGNAL(toggled(bool)), gui, SLOT(setFlagShowNebulaBackgroundButton(bool)));

	ui->mouseTimeoutCheckbox->setChecked(StelMainGraphicsView::getInstance().getFlagCursorTimeout());
	ui->mouseTimeoutSpinBox->setValue(StelMainGraphicsView::getInstance().getCursorTimeout());
	connect(ui->mouseTimeoutCheckbox, SIGNAL(clicked()), this, SLOT(cursorTimeOutChanged()));
	connect(ui->mouseTimeoutCheckbox, SIGNAL(toggled(bool)), this, SLOT(cursorTimeOutChanged()));
	connect(ui->mouseTimeoutSpinBox, SIGNAL(valueChanged(double)), this, SLOT(cursorTimeOutChanged(double)));

	connect(ui->setViewingOptionAsDefaultPushButton, SIGNAL(clicked()), this, SLOT(saveCurrentViewOptions()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(setDefaultViewOptions()));

	ui->screenshotDirEdit->setText(StelFileMgr::getScreenshotDir());
	connect(ui->screenshotDirEdit, SIGNAL(textChanged(QString)), this, SLOT(selectScreenshotDir(QString)));
	connect(ui->screenshotBrowseButton, SIGNAL(clicked()), this, SLOT(browseForScreenshotDir()));

	ui->invertScreenShotColorsCheckBox->setChecked(StelMainGraphicsView::getInstance().getFlagInvertScreenShotColors());
	connect(ui->invertScreenShotColorsCheckBox, SIGNAL(toggled(bool)), &StelMainGraphicsView::getInstance(), SLOT(setFlagInvertScreenShotColors(bool)));

	// script tab controls
	#ifndef DISABLE_SCRIPTING
	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();
	connect(ui->scriptListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(scriptSelectionChanged(const QString&)));
	connect(ui->runScriptButton, SIGNAL(clicked()), this, SLOT(runScriptClicked()));
	connect(ui->stopScriptButton, SIGNAL(clicked()), this, SLOT(stopScriptClicked()));
	if (scriptMgr.scriptIsRunning())
		aScriptIsRunning();
	else
		aScriptHasStopped();
	connect(&scriptMgr, SIGNAL(scriptRunning()), this, SLOT(aScriptIsRunning()));
	connect(&scriptMgr, SIGNAL(scriptStopped()), this, SLOT(aScriptHasStopped()));
	ui->scriptListWidget->setSortingEnabled(true);
	populateScriptsList();
	connect(this, SIGNAL(visibleChanged(bool)), this, SLOT(populateScriptsList()));
	#endif

	// plugins control
	connect(ui->pluginsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(pluginsSelectionChanged(const QString&)));
	connect(ui->pluginLoadAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(loadAtStartupChanged(int)));
	connect(ui->pluginConfigureButton, SIGNAL(clicked()), this, SLOT(pluginConfigureCurrentSelection()));
	populatePluginsList();

	updateConfigLabels();
	updateTabBarListWidgetWidth();
}

void ConfigurationDialog::selectLanguage(const QString& langName)
{
	QString code = StelTranslator::nativeNameToIso639_1Code(langName);
	StelApp::getInstance().getLocaleMgr().setAppLanguage(code);
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(code);
	StelMainWindow::getInstance().initTitleI18n();
}

void ConfigurationDialog::setStartupTimeMode()
{
	if (ui->systemTimeRadio->isChecked())
		StelApp::getInstance().getCore()->setStartupTimeMode("actual");
	else if (ui->todayRadio->isChecked())
		StelApp::getInstance().getCore()->setStartupTimeMode("today");
	else
		StelApp::getInstance().getCore()->setStartupTimeMode("preset");

	StelApp::getInstance().getCore()->setInitTodayTime(ui->todayTimeSpinBox->time());
	StelApp::getInstance().getCore()->setPresetSkyTime(ui->fixedDateTimeEdit->dateTime());
}

void ConfigurationDialog::showShortcutsWindow()
{
	QAction* action = gui->getGuiAction("actionShow_Shortcuts_Window_Global");
	if (action)
	{
		if (action->isChecked())
			action->setChecked(false);
		action->setChecked(true);
	}
}

void ConfigurationDialog::setDiskViewport(bool b)
{
	if (b)
		StelApp::getInstance().getCore()->setMaskType(StelProjector::MaskDisk);
	else
		StelApp::getInstance().getCore()->setMaskType(StelProjector::MaskNone);
}

void ConfigurationDialog::setSphericMirror(bool b)
{
	StelCore* core = StelApp::getInstance().getCore();
	if (b)
	{
		savedProjectionType = core->getCurrentProjectionType();
		core->setCurrentProjectionType(StelCore::ProjectionFisheye);
		StelMainGraphicsView::getInstance().getStelAppGraphicsWidget()->setViewportEffect("sphericMirrorDistorter");
	}
	else
	{
		core->setCurrentProjectionType((StelCore::ProjectionType)savedProjectionType);
		StelMainGraphicsView::getInstance().getStelAppGraphicsWidget()->setViewportEffect("none");
	}
}

void ConfigurationDialog::setNoSelectedInfo(void)
{
	gui->setInfoTextFilters(StelObject::InfoStringGroup(0));
	updateSelectedInfoCheckBoxes();
}

void ConfigurationDialog::setAllSelectedInfo(void)
{
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
	updateSelectedInfoCheckBoxes();
}

void ConfigurationDialog::setBriefSelectedInfo(void)
{
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::ShortInfo));
	updateSelectedInfoCheckBoxes();
}

void ConfigurationDialog::setSelectedInfoFromCheckBoxes()
{
	// As this signal will be called when a checbox is toggled,
	// change the general mode to Custom.
	if (!ui->customSelectedInfoRadio->isChecked())
		ui->customSelectedInfoRadio->setChecked(true);
	
	StelObject::InfoStringGroup flags(0);
	
	if (ui->checkBoxName->isChecked())
		flags |= StelObject::Name;
	if (ui->checkBoxCatalogNumbers->isChecked())
		flags |= StelObject::CatalogNumber;
	if (ui->checkBoxVisualMag->isChecked())
		flags |= StelObject::Magnitude;
	if (ui->checkBoxAbsoluteMag->isChecked())
		flags |= StelObject::AbsoluteMagnitude;
	if (ui->checkBoxRaDecJ2000->isChecked())
		flags |= StelObject::RaDecJ2000;
	if (ui->checkBoxRaDecOfDate->isChecked())
		flags |= StelObject::RaDecOfDate;
	if (ui->checkBoxHourAngle->isChecked())
		flags |= StelObject::HourAngle;
	if (ui->checkBoxAltAz->isChecked())
		flags |= StelObject::AltAzi;
	if (ui->checkBoxDistance->isChecked())
		flags |= StelObject::Distance;
	if (ui->checkBoxSize->isChecked())
		flags |= StelObject::Size;
	if (ui->checkBoxExtra1->isChecked())
		flags |= StelObject::Extra1;
	if (ui->checkBoxExtra2->isChecked())
		flags |= StelObject::Extra2;
	if (ui->checkBoxExtra3->isChecked())
		flags |= StelObject::Extra3;	
	if (ui->checkBoxGalacticCoordJ2000->isChecked())
		flags |= StelObject::GalCoordJ2000;
	
	gui->setInfoTextFilters(flags);
}


void ConfigurationDialog::cursorTimeOutChanged()
{
	StelMainGraphicsView::getInstance().setFlagCursorTimeout(ui->mouseTimeoutCheckbox->isChecked());
	StelMainGraphicsView::getInstance().setCursorTimeout(ui->mouseTimeoutSpinBox->value());
}

void ConfigurationDialog::browseForScreenshotDir()
{
	QString oldScreenshorDir = StelFileMgr::getScreenshotDir();
	#ifdef Q_OS_MAC
		//work-around for Qt bug -  http://bugreports.qt.nokia.com/browse/QTBUG-16722
		QString newScreenshotDir = QFileDialog::getExistingDirectory(NULL, q_("Select screenshot directory"), oldScreenshorDir, QFileDialog::DontUseNativeDialog);
	#else
		QString newScreenshotDir = QFileDialog::getExistingDirectory(NULL, q_("Select screenshot directory"), oldScreenshorDir, QFileDialog::ShowDirsOnly);
	#endif

	if (!newScreenshotDir.isEmpty()) {
		// remove trailing slash
		if (newScreenshotDir.right(1) == "/")
			newScreenshotDir = newScreenshotDir.left(newScreenshotDir.length()-1);

		ui->screenshotDirEdit->setText(newScreenshotDir);
	}
}

void ConfigurationDialog::selectScreenshotDir(const QString& dir)
{
	try
	{
		StelFileMgr::setScreenshotDir(dir);
	}
	catch (std::runtime_error& e)
	{
		// nop
		// this will happen when people are only half way through typing dirs
	}
}

// Save the current viewing option including landscape, location and sky culture
// This doesn't include the current viewing direction, time and FOV since those have specific controls
void ConfigurationDialog::saveCurrentViewOptions()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);
	MeteorMgr* mmgr = GETSTELMODULE(MeteorMgr);
	Q_ASSERT(mmgr);
	StelSkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(skyd);
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	Q_ASSERT(cmgr);
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	Q_ASSERT(smgr);
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	Q_ASSERT(nmgr);
	GridLinesMgr* glmgr = GETSTELMODULE(GridLinesMgr);
	Q_ASSERT(glmgr);
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	StelCore* core = StelApp::getInstance().getCore();
	const StelProjectorP proj = core->getProjection(StelCore::FrameJ2000);
	Q_ASSERT(proj);

	// view dialog / sky tab settings
	conf->setValue("stars/absolute_scale", skyd->getAbsoluteStarScale());
	conf->setValue("stars/relative_scale", skyd->getRelativeStarScale());
	conf->setValue("stars/flag_star_twinkle", skyd->getFlagTwinkle());
	conf->setValue("stars/star_twinkle_amount", skyd->getTwinkleAmount());
	conf->setValue("viewing/use_luminance_adaptation", skyd->getFlagLuminanceAdaptation());
	conf->setValue("astro/flag_planets", ssmgr->getFlagPlanets());
	conf->setValue("astro/flag_planets_hints", ssmgr->getFlagHints());
	conf->setValue("astro/flag_planets_orbits", ssmgr->getFlagOrbits());
	conf->setValue("astro/flag_light_travel_time", ssmgr->getFlagLightTravelTime());
	conf->setValue("viewing/flag_moon_scaled", ssmgr->getFlagMoonScale());
	conf->setValue("astro/meteor_rate", mmgr->getZHR());

	// view dialog / markings tab settings
	conf->setValue("viewing/flag_azimuthal_grid", glmgr->getFlagAzimuthalGrid());
	conf->setValue("viewing/flag_equatorial_grid", glmgr->getFlagEquatorGrid());
	conf->setValue("viewing/flag_equator_line", glmgr->getFlagEquatorLine());
	conf->setValue("viewing/flag_ecliptic_line", glmgr->getFlagEclipticLine());
	conf->setValue("viewing/flag_ecliptic_J2000_grid", glmgr->getFlagEclipticJ2000Grid());
	conf->setValue("viewing/flag_meridian_line", glmgr->getFlagMeridianLine());
	conf->setValue("viewing/flag_horizon_line", glmgr->getFlagHorizonLine());
	conf->setValue("viewing/flag_equatorial_J2000_grid", glmgr->getFlagEquatorJ2000Grid());
	conf->setValue("viewing/flag_galactic_grid", glmgr->getFlagGalacticGrid());
	conf->setValue("viewing/flag_galactic_plane_line", glmgr->getFlagGalacticPlaneLine());
	conf->setValue("viewing/flag_cardinal_points", lmgr->getFlagCardinalsPoints());
	conf->setValue("viewing/flag_constellation_drawing", cmgr->getFlagLines());
	conf->setValue("viewing/flag_constellation_name", cmgr->getFlagLabels());
	conf->setValue("viewing/flag_constellation_boundaries", cmgr->getFlagBoundaries());
	conf->setValue("viewing/flag_constellation_art", cmgr->getFlagArt());
	conf->setValue("viewing/flag_constellation_isolate_selected", cmgr->getFlagIsolateSelected());
	conf->setValue("viewing/constellation_art_intensity", cmgr->getArtIntensity());
	conf->setValue("viewing/flag_night", StelApp::getInstance().getVisionModeNight());
	conf->setValue("astro/flag_star_name", smgr->getFlagLabels());
	conf->setValue("stars/labels_amount", smgr->getLabelsAmount());
	conf->setValue("astro/flag_planets_labels", ssmgr->getFlagLabels());
	conf->setValue("astro/labels_amount", ssmgr->getLabelsAmount());
	conf->setValue("astro/nebula_hints_amount", nmgr->getHintsAmount());
	conf->setValue("astro/flag_nebula_name", nmgr->getFlagHints());
	conf->setValue("projection/type", core->getCurrentProjectionTypeKey());

	// view dialog / landscape tab settings
	lmgr->setDefaultLandscapeID(lmgr->getCurrentLandscapeID());
	conf->setValue("landscape/flag_landscape_sets_location", lmgr->getFlagLandscapeSetsLocation());
	conf->setValue("landscape/flag_landscape", lmgr->getFlagLandscape());
	conf->setValue("landscape/flag_atmosphere", lmgr->getFlagAtmosphere());
	conf->setValue("landscape/flag_fog", lmgr->getFlagFog());
	conf->setValue("stars/init_bortle_scale", core->getSkyDrawer()->getBortleScale());
        conf->setValue("landscape/atmospheric_extinction_coefficient", core->getSkyDrawer()->getExtinctionCoefficient());
        conf->setValue("landscape/pressure_mbar", core->getSkyDrawer()->getAtmospherePressure());
        conf->setValue("landscape/temperature_C", core->getSkyDrawer()->getAtmosphereTemperature());

	// view dialog / starlore tab
	StelApp::getInstance().getSkyCultureMgr().setDefaultSkyCultureID(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());

	// Save default location
	StelApp::getInstance().getCore()->setDefaultLocationID(core->getCurrentLocation().getID());

	// configuration dialog / main tab
	QString langName = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	conf->setValue("localization/app_locale", StelTranslator::nativeNameToIso639_1Code(langName));
	langName = StelApp::getInstance().getLocaleMgr().getSkyLanguage();
	conf->setValue("localization/sky_locale", StelTranslator::nativeNameToIso639_1Code(langName));

	// configuration dialog / selected object info tab
	const StelObject::InfoStringGroup& flags = gui->getInfoTextFilters();
	if (flags == StelObject::InfoStringGroup(0))
		conf->setValue("gui/selected_object_info", "none");
	else if (flags == StelObject::InfoStringGroup(StelObject::ShortInfo))
		conf->setValue("gui/selected_object_info", "short");
	else if (flags == StelObject::InfoStringGroup(StelObject::AllInfo))
		conf->setValue("gui/selected_object_info", "all");
	else
	{
		conf->setValue("gui/selected_object_info", "custom");
		
		conf->beginGroup("custom_selected_info");
		conf->setValue("flag_show_name", (bool) (flags & StelObject::Name));
		conf->setValue("flag_show_catalognumber",
		               (bool) (flags & StelObject::CatalogNumber));
		conf->setValue("flag_show_magnitude",
		               (bool) (flags & StelObject::Magnitude));
		conf->setValue("flag_show_absolutemagnitude",
		               (bool) (flags & StelObject::AbsoluteMagnitude));
		conf->setValue("flag_show_radecj2000",
		               (bool) (flags & StelObject::RaDecJ2000));
		conf->setValue("flag_show_radecofdate",
		               (bool) (flags & StelObject::RaDecOfDate));
		conf->setValue("flag_show_hourangle",
		               (bool) (flags & StelObject::HourAngle));
		conf->setValue("flag_show_altaz",
		               (bool) (flags &  StelObject::AltAzi));
		conf->setValue("flag_show_distance",
		               (bool) (flags & StelObject::Distance));
		conf->setValue("flag_show_size",
		               (bool) (flags & StelObject::Size));
		conf->setValue("flag_show_extra1",
		               (bool) (flags & StelObject::Extra1));
		conf->setValue("flag_show_extra2",
		               (bool) (flags & StelObject::Extra2));
		conf->setValue("flag_show_extra3",
		               (bool) (flags & StelObject::Extra3));
		conf->setValue("flag_show_galcoordj2000",
			       (bool) (flags & StelObject::GalCoordJ2000));
		conf->endGroup();
	}

	// toolbar auto-hide status
	conf->setValue("gui/auto_hide_horizontal_toolbar", gui->getAutoHideHorizontalButtonBar());
	conf->setValue("gui/auto_hide_vertical_toolbar", gui->getAutoHideVerticalButtonBar());
	conf->setValue("gui/flag_show_nebulae_background_button", gui->getFlagShowNebulaBackgroundButton());

	mvmgr->setInitFov(mvmgr->getCurrentFov());
	mvmgr->setInitViewDirectionToCurrent();

	// configuration dialog / navigation tab
	conf->setValue("navigation/flag_enable_zoom_keys", mvmgr->getFlagEnableZoomKeys());
	conf->setValue("navigation/flag_enable_mouse_navigation", mvmgr->getFlagEnableMouseNavigation());
	conf->setValue("navigation/flag_enable_move_keys", mvmgr->getFlagEnableMoveKeys());
	conf->setValue("navigation/startup_time_mode", core->getStartupTimeMode());
	conf->setValue("navigation/today_time", core->getInitTodayTime());
	conf->setValue("navigation/preset_sky_time", core->getPresetSkyTime());
	conf->setValue("navigation/init_fov", mvmgr->getInitFov());
	if (mvmgr->getMountMode() == StelMovementMgr::MountAltAzimuthal)
		conf->setValue("navigation/viewing_mode", "horizon");
	else
		conf->setValue("navigation/viewing_mode", "equator");


	// configuration dialog / tools tab
	conf->setValue("gui/flag_show_flip_buttons", gui->getFlagShowFlipButtons());
	conf->setValue("video/viewport_effect", StelMainGraphicsView::getInstance().getStelAppGraphicsWidget()->getViewportEffect());
	conf->setValue("projection/viewport", StelProjector::maskTypeToString(proj->getMaskType()));
	conf->setValue("viewing/flag_gravity_labels", proj->getFlagGravityLabels());
	conf->setValue("viewing/flag_render_solar_shadows", StelApp::getInstance().getRenderSolarShadows());
	conf->setValue("navigation/auto_zoom_out_resets_direction", mvmgr->getFlagAutoZoomOutResetsDirection());
	conf->setValue("gui/flag_mouse_cursor_timeout", StelMainGraphicsView::getInstance().getFlagCursorTimeout());
	conf->setValue("gui/mouse_cursor_timeout", StelMainGraphicsView::getInstance().getCursorTimeout());

	conf->setValue("main/screenshot_dir", StelFileMgr::getScreenshotDir());
	conf->setValue("main/invert_screenshots_colors", StelMainGraphicsView::getInstance().getFlagInvertScreenShotColors());

	// full screen and window size
	conf->setValue("video/fullscreen", StelMainWindow::getInstance().getFullScreen());
	if (!StelMainWindow::getInstance().getFullScreen())
	{
		StelMainWindow& mainWindow = StelMainWindow::getInstance();
		conf->setValue("video/screen_w", mainWindow.size().width());
		conf->setValue("video/screen_h", mainWindow.size().height());
		conf->setValue("video/screen_x", mainWindow.x());
		conf->setValue("video/screen_y", mainWindow.y());
	}

	// clear the restore defaults flag if it is set.
	conf->setValue("main/restore_defaults", false);

	updateConfigLabels();
}

void ConfigurationDialog::updateConfigLabels()
{
	ui->startupFOVLabel->setText(q_("Startup FOV: %1%2").arg(StelApp::getInstance().getCore()->getMovementMgr()->getCurrentFov()).arg(QChar(0x00B0)));

	double az, alt;
	const Vec3d& v = GETSTELMODULE(StelMovementMgr)->getInitViewingDirection();
	StelUtils::rectToSphe(&az, &alt, v);
	az = 3.*M_PI - az;  // N is zero, E is 90 degrees
	if (az > M_PI*2)
		az -= M_PI*2;
	ui->startupDirectionOfViewlabel->setText(q_("Startup direction of view Az/Alt: %1/%2").arg(StelUtils::radToDmsStr(az), StelUtils::radToDmsStr(alt)));
}

void ConfigurationDialog::setDefaultViewOptions()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	conf->setValue("main/restore_defaults", true);
}

void ConfigurationDialog::populatePluginsList()
{
	int prevSel = ui->pluginsListWidget->currentRow();
	ui->pluginsListWidget->clear();
	const QList<StelModuleMgr::PluginDescriptor> pluginsList = StelApp::getInstance().getModuleMgr().getPluginsList();
	foreach (const StelModuleMgr::PluginDescriptor& desc, pluginsList)
	{
		QString label = q_(desc.info.displayedName);
		QListWidgetItem* item = new QListWidgetItem(label);
		item->setData(Qt::UserRole, desc.info.id);
		ui->pluginsListWidget->addItem(item);
	}
	// If we had a valid previous selection (i.e. not first time we populate), restore it
	if (prevSel >= 0 && prevSel < ui->pluginsListWidget->count())
		ui->pluginsListWidget->setCurrentRow(prevSel);
	else
		ui->pluginsListWidget->setCurrentRow(0);
}

void ConfigurationDialog::pluginsSelectionChanged(const QString& s)
{
	const QList<StelModuleMgr::PluginDescriptor> pluginsList = StelApp::getInstance().getModuleMgr().getPluginsList();
	foreach (const StelModuleMgr::PluginDescriptor& desc, pluginsList)
	{
		if (s==q_(desc.info.displayedName))//TODO: Use ID!
		{
			QString html = "<html><head></head><body>";
			html += "<h2>" + q_(desc.info.displayedName) + "</h2>";			
			QString d = desc.info.description;
			d.replace("\n", "<br />");
			html += "<p>" + q_(d) + "</p>";
			html += "<p><strong>" + q_("Authors") + "</strong>: " + desc.info.authors;
			html += "<br /><strong>" + q_("Contact") + "</strong>: " + desc.info.contact;
			html += "</p></body></html>";
			ui->pluginsInfoBrowser->setHtml(html);
			ui->pluginLoadAtStartupCheckBox->setChecked(desc.loadAtStartup);
			StelModule* pmod = StelApp::getInstance().getModuleMgr().getModule(desc.info.id, true);
			if (pmod != NULL)
				ui->pluginConfigureButton->setEnabled(pmod->configureGui(false));
			else
				ui->pluginConfigureButton->setEnabled(false);
			return;
		}
	}
}

void ConfigurationDialog::pluginConfigureCurrentSelection()
{
	QString id = ui->pluginsListWidget->currentItem()->data(Qt::UserRole).toString();
	if (id.isEmpty())
		return;

	StelModuleMgr& moduleMgr = StelApp::getInstance().getModuleMgr();
	const QList<StelModuleMgr::PluginDescriptor> pluginsList = moduleMgr.getPluginsList();
	foreach (const StelModuleMgr::PluginDescriptor& desc, pluginsList)
	{
		if (id == desc.info.id)
		{
			StelModule* pmod = moduleMgr.getModule(desc.info.id);
			if (pmod != NULL)
			{
				pmod->configureGui(true);
			}
			return;
		}
	}
}

void ConfigurationDialog::loadAtStartupChanged(int state)
{
	if (ui->pluginsListWidget->count() <= 0)
		return;

	QString id = ui->pluginsListWidget->currentItem()->data(Qt::UserRole).toString();
	StelModuleMgr& moduleMgr = StelApp::getInstance().getModuleMgr();
	const QList<StelModuleMgr::PluginDescriptor> pluginsList = moduleMgr.getPluginsList();
	foreach (const StelModuleMgr::PluginDescriptor& desc, pluginsList)
	{
		if (id == desc.info.id)
		{
			moduleMgr.setPluginLoadAtStartup(id, state == Qt::Checked);
			break;
		}
	}
}

#ifndef DISABLE_SCRIPTING
void ConfigurationDialog::populateScriptsList(void)
{
	int prevSel = ui->scriptListWidget->currentRow();	
	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();	
	ui->scriptListWidget->clear();	
	ui->scriptListWidget->addItems(scriptMgr.getScriptList());	
	// If we had a valid previous selection (i.e. not first time we populate), restore it
	if (prevSel >= 0 && prevSel < ui->scriptListWidget->count())
		ui->scriptListWidget->setCurrentRow(prevSel);
	else
		ui->scriptListWidget->setCurrentRow(0);
}

void ConfigurationDialog::scriptSelectionChanged(const QString& s)
{
	if (s.isEmpty())
		return;	
	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();	
	//ui->scriptInfoBrowser->document()->setDefaultStyleSheet(QString(StelApp::getInstance().getCurrentStelStyle()->htmlStyleSheet));
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_(scriptMgr.getName(s).trimmed()) + "</h2>";
	QString d = scriptMgr.getDescription(s).trimmed();
	d.replace("\n", "<br />");
	d.replace(QRegExp("\\s{2,}"), " ");
	html += "<p>" + q_(d) + "</p>";
	html += "<p>";
	if (!scriptMgr.getAuthor(s).trimmed().isEmpty())
	{
		html += "<strong>" + q_("Author") + "</strong>: " + scriptMgr.getAuthor(s) + "<br />";
	}
	if (!scriptMgr.getLicense(s).trimmed().isEmpty())
	{
		html += "<strong>" + q_("License") + "</strong>: " + scriptMgr.getLicense(s);
	}	
	html += "</p></body></html>";
	ui->scriptInfoBrowser->setHtml(html);	
}

void ConfigurationDialog::runScriptClicked(void)
{
	if (ui->closeWindowAtScriptRunCheckbox->isChecked())
		this->close();	
	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();
	if (ui->scriptListWidget->currentItem())
	{
		scriptMgr.runScript(ui->scriptListWidget->currentItem()->text());
	}	
}

void ConfigurationDialog::stopScriptClicked(void)
{
	StelMainGraphicsView::getInstance().getScriptMgr().stopScript();
}

void ConfigurationDialog::aScriptIsRunning(void)
{	
	ui->scriptStatusLabel->setText(q_("Running script: ") + StelMainGraphicsView::getInstance().getScriptMgr().runningScriptId());
	ui->runScriptButton->setEnabled(false);
	ui->stopScriptButton->setEnabled(true);	
}

void ConfigurationDialog::aScriptHasStopped(void)
{
	ui->scriptStatusLabel->setText(q_("Running script: [none]"));
	ui->runScriptButton->setEnabled(true);
	ui->stopScriptButton->setEnabled(false);
}
#endif


void ConfigurationDialog::setFixedDateTimeToCurrent(void)
{
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(StelApp::getInstance().getCore()->getJDay()));
	ui->fixedTimeRadio->setChecked(true);
	setStartupTimeMode();
}


void ConfigurationDialog::resetStarCatalogControls()
{
	const QVariantList& catalogConfig = GETSTELMODULE(StarMgr)->getCatalogsDescription();
	nextStarCatalogToDownload.clear();
	int idx=0;
	foreach (const QVariant& catV, catalogConfig)
	{
		++idx;
		const QVariantMap& m = catV.toMap();
		const bool checked = m.value("checked").toBool();
		if (checked)
			continue;
		nextStarCatalogToDownload=m;
		break;
	}

	ui->downloadCancelButton->setVisible(false);
	ui->downloadRetryButton->setVisible(false);

	if (idx > catalogConfig.size() && !hasDownloadedStarCatalog)
	{
		ui->getStarsButton->setVisible(false);
		updateStarCatalogControlsText();
		return;
	}

	ui->getStarsButton->setEnabled(true);
	if (!nextStarCatalogToDownload.isEmpty())
	{
		nextStarCatalogToDownloadIndex = idx;
		starCatalogsCount = catalogConfig.size();
		updateStarCatalogControlsText();
		ui->getStarsButton->setVisible(true);
	}
	else
	{
		updateStarCatalogControlsText();
		ui->getStarsButton->setVisible(false);
	}
}

void ConfigurationDialog::updateStarCatalogControlsText()
{
	if (nextStarCatalogToDownload.isEmpty())
	{
		//There are no more catalogs left?
		if (hasDownloadedStarCatalog)
		{
			ui->downloadLabel->setText(q_("Finished downloading new star catalogs!\nRestart Stellarium to display them."));
		}
		else
		{
			ui->downloadLabel->setText(q_("All available star catalogs have been installed."));
		}
	}
	else
	{
		QString text = QString(q_("Get catalog %1 of %2"))
		               .arg(nextStarCatalogToDownloadIndex)
		               .arg(starCatalogsCount);
		ui->getStarsButton->setText(text);
		
		if (isDownloadingStarCatalog)
		{
			QString text = QString(q_("Downloading %1...\n(You can close this window.)"))
			                 .arg(nextStarCatalogToDownload.value("id").toString());
			ui->downloadLabel->setText(text);
		}
		else
		{
			const QVariantList& magRange = nextStarCatalogToDownload.value("magRange").toList();
			ui->downloadLabel->setText(q_("Download size: %1MB\nStar count: %2 Million\nMagnitude range: %3 - %4")
				.arg(nextStarCatalogToDownload.value("sizeMb").toString())
				.arg(nextStarCatalogToDownload.value("count").toString())
				.arg(magRange.first().toString())
				.arg(magRange.last().toString()));
		}
	}
}

void ConfigurationDialog::cancelDownload(void)
{
	Q_ASSERT(currentDownloadFile);
	Q_ASSERT(starCatalogDownloadReply);
	qWarning() << "Aborting download";
	starCatalogDownloadReply->abort();
}

void ConfigurationDialog::newStarCatalogData()
{
	Q_ASSERT(currentDownloadFile);
	Q_ASSERT(starCatalogDownloadReply);
	Q_ASSERT(progressBar);

	int size = starCatalogDownloadReply->bytesAvailable();
	progressBar->setValue((float)progressBar->value()+(float)size/1024);
	currentDownloadFile->write(starCatalogDownloadReply->read(size));
}

void ConfigurationDialog::downloadStars()
{
	Q_ASSERT(!nextStarCatalogToDownload.isEmpty());
	Q_ASSERT(!isDownloadingStarCatalog);
	Q_ASSERT(starCatalogDownloadReply==NULL);
	Q_ASSERT(currentDownloadFile==NULL);
	Q_ASSERT(progressBar==NULL);

	QString path = StelFileMgr::getUserDir()+QString("/stars/default/")+nextStarCatalogToDownload.value("fileName").toString();
	currentDownloadFile = new QFile(path);
	if (!currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "Can't open a writable file for storing new star catalog: " << path;
		currentDownloadFile->deleteLater();
		currentDownloadFile = NULL;
		ui->downloadLabel->setText(q_("Error downloading %1:\n%2").arg(nextStarCatalogToDownload.value("id").toString()).arg(QString("Can't open a writable file for storing new star catalog: %1").arg(path)));
		ui->downloadRetryButton->setVisible(true);
		return;
	}

	isDownloadingStarCatalog = true;
	updateStarCatalogControlsText();
	ui->downloadCancelButton->setVisible(true);
	ui->downloadRetryButton->setVisible(false);
	ui->getStarsButton->setVisible(true);
	ui->getStarsButton->setEnabled(false);

	QNetworkRequest req(nextStarCatalogToDownload.value("url").toString());
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toAscii());
	starCatalogDownloadReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	starCatalogDownloadReply->setReadBufferSize(1024*1024*2);
	connect(starCatalogDownloadReply, SIGNAL(readyRead()), this, SLOT(newStarCatalogData()));
	connect(starCatalogDownloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(starCatalogDownloadReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));

	progressBar = StelApp::getInstance().getGui()->addProgressBar();
	progressBar->setValue(0);
	progressBar->setMaximum(nextStarCatalogToDownload.value("sizeMb").toDouble()*1024);
	progressBar->setVisible(true);
	progressBar->setFormat(QString("%1: %p%").arg(nextStarCatalogToDownload.value("id").toString()));
}

void ConfigurationDialog::downloadError(QNetworkReply::NetworkError)
{
	Q_ASSERT(currentDownloadFile);
	Q_ASSERT(starCatalogDownloadReply);

	isDownloadingStarCatalog = false;
	qWarning() << "Error downloading file" << starCatalogDownloadReply->url() << ": " << starCatalogDownloadReply->errorString();
	ui->downloadLabel->setText(q_("Error downloading %1:\n%2").arg(nextStarCatalogToDownload.value("id").toString()).arg(starCatalogDownloadReply->errorString()));
	ui->downloadCancelButton->setVisible(false);
	ui->downloadRetryButton->setVisible(true);
	ui->getStarsButton->setVisible(false);
	ui->getStarsButton->setEnabled(true);
}

void ConfigurationDialog::downloadFinished()
{
	Q_ASSERT(currentDownloadFile);
	Q_ASSERT(starCatalogDownloadReply);
	Q_ASSERT(progressBar);

	if (starCatalogDownloadReply->error()!=QNetworkReply::NoError)
	{
		starCatalogDownloadReply->deleteLater();
		starCatalogDownloadReply = NULL;
		currentDownloadFile->close();
		currentDownloadFile->deleteLater();
		currentDownloadFile = NULL;
		progressBar->deleteLater();
		progressBar=NULL;
		return;
	}

	Q_ASSERT(starCatalogDownloadReply->bytesAvailable()==0);

	const QVariant& redirect = starCatalogDownloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (!redirect.isNull())
	{
		// We got a redirection, we need to follow
		starCatalogDownloadReply->deleteLater();
		QNetworkRequest req(redirect.toUrl());
		req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
		req.setRawHeader("User-Agent", StelUtils::getApplicationName().toAscii());
		starCatalogDownloadReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		starCatalogDownloadReply->setReadBufferSize(1024*1024*2);
		connect(starCatalogDownloadReply, SIGNAL(readyRead()), this, SLOT(newStarCatalogData()));
		connect(starCatalogDownloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		connect(starCatalogDownloadReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
		return;
	}

	isDownloadingStarCatalog = false;
	currentDownloadFile->close();
	currentDownloadFile->deleteLater();
	currentDownloadFile = NULL;
	starCatalogDownloadReply->deleteLater();
	starCatalogDownloadReply = NULL;
	progressBar->deleteLater();
	progressBar=NULL;

	ui->downloadLabel->setText(q_("Verifying file integrity..."));
	if (GETSTELMODULE(StarMgr)->checkAndLoadCatalog(nextStarCatalogToDownload)==false)
	{
		ui->getStarsButton->setVisible(false);
		ui->downloadLabel->setText(q_("Error downloading %1:\nFile is corrupted.").arg(nextStarCatalogToDownload.value("id").toString()));
		ui->downloadCancelButton->setVisible(false);
		ui->downloadRetryButton->setVisible(true);
	}
	else
	{
		hasDownloadedStarCatalog = true;
	}

	resetStarCatalogControls();
}

void ConfigurationDialog::updateSelectedInfoCheckBoxes()
{
	const StelObject::InfoStringGroup& flags = gui->getInfoTextFilters();
	
	ui->checkBoxName->setChecked(flags & StelObject::Name);
	ui->checkBoxCatalogNumbers->setChecked(flags & StelObject::CatalogNumber);
	ui->checkBoxVisualMag->setChecked(flags & StelObject::Magnitude);
	ui->checkBoxAbsoluteMag->setChecked(flags & StelObject::AbsoluteMagnitude);
	ui->checkBoxRaDecJ2000->setChecked(flags & StelObject::RaDecJ2000);
	ui->checkBoxRaDecOfDate->setChecked(flags & StelObject::RaDecOfDate);
	ui->checkBoxHourAngle->setChecked(flags & StelObject::HourAngle);
	ui->checkBoxAltAz->setChecked(flags & StelObject::AltAzi);
	ui->checkBoxDistance->setChecked(flags & StelObject::Distance);
	ui->checkBoxSize->setChecked(flags & StelObject::Size);
	ui->checkBoxExtra1->setChecked(flags & StelObject::Extra1);
	ui->checkBoxExtra2->setChecked(flags & StelObject::Extra2);
	ui->checkBoxExtra3->setChecked(flags & StelObject::Extra3);
	ui->checkBoxGalacticCoordJ2000->setChecked(flags & StelObject::GalCoordJ2000);
}

void ConfigurationDialog::updateTabBarListWidgetWidth()
{
	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
		return;
	
	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();
	
	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		QModelIndex index = model->index(row, 0);
		width += ui->stackListWidget->sizeHintForIndex(index).width();
	}
	
	// TODO: Limit the width to the width of the screen *available to the window*
	// FIXME: This works only sometimes...
	/*if (width <= ui->stackListWidget->width())
	{
		//qDebug() << width << ui->stackListWidget->width();
		return;
	}*/
	
	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
	
	// FIXME: This works only sometimes...
	/*
	dialog->adjustSize();
	dialog->update();
	// ... and allow manual resize later.
	ui->stackListWidget->setMinimumWidth(0);
	*/
}
