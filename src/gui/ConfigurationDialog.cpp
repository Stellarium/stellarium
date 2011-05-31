/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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
#include "StelScriptMgr.hpp"
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
	savedProjectionType = StelApp::getInstance().getCore()->getCurrentProjectionType();
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
}

void ConfigurationDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
		ui->stackListWidget->repaint();
	}

	//Script information
	//(trigger re-displaying the description of the current item)
	scriptSelectionChanged(ui->scriptListWidget->currentItem()->text());

	//Plug-in information
	//(the same trick)
	pluginsSelectionChanged(ui->pluginsListWidget->currentItem()->text());
}

void ConfigurationDialog::styleChanged()
{
	// Nothing for now
}

void ConfigurationDialog::createDialogContent()
{
	const StelProjectorP proj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000);
	StelCore* core = StelApp::getInstance().getCore();

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);

	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));

	// Set the main tab activated by default
	ui->configurationStackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// Main tab
	// Fill the language list widget from the available list
	QString appLang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	QComboBox* cb = ui->programLanguageComboBox;
	cb->clear();
	cb->addItems(StelTranslator::globalTranslator.getAvailableLanguagesNamesNative(StelFileMgr::getLocaleDir()));
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
	connect(cb, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(languageChanged(const QString&)));

	connect(ui->getStarsButton, SIGNAL(clicked()), this, SLOT(downloadStars()));
	connect(ui->downloadCancelButton, SIGNAL(clicked()), this, SLOT(cancelDownload()));
	connect(ui->downloadRetryButton, SIGNAL(clicked()), this, SLOT(downloadStars()));
	refreshStarCatalogButton();

	// Selected object info
	if (gui->getInfoTextFilters() == (StelObject::InfoStringGroup)0)
		ui->noSelectedInfoRadio->setChecked(true);
	else if (gui->getInfoTextFilters() == StelObject::InfoStringGroup(StelObject::ShortInfo))
		ui->briefSelectedInfoRadio->setChecked(true);
	else
		ui->allSelectedInfoRadio->setChecked(true);
	connect(ui->noSelectedInfoRadio, SIGNAL(released()), this, SLOT(setNoSelectedInfo()));
	connect(ui->allSelectedInfoRadio, SIGNAL(released()), this, SLOT(setAllSelectedInfo()));
	connect(ui->briefSelectedInfoRadio, SIGNAL(released()), this, SLOT(setBriefSelectedInfo()));

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
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(core->getPresetSkyTime()));
	connect(ui->fixedDateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), core, SLOT(setPresetSkyTime(QDateTime)));

	ui->enableKeysNavigationCheckBox->setChecked(mvmgr->getFlagEnableMoveKeys() || mvmgr->getFlagEnableZoomKeys());
	ui->enableMouseNavigationCheckBox->setChecked(mvmgr->getFlagEnableMouseNavigation());
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableMoveKeys(bool)));
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableZoomKeys(bool)));
	connect(ui->enableMouseNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableMouseNavigation(bool)));
	connect(ui->fixedDateTimeCurrentButton, SIGNAL(clicked()), this, SLOT(setFixedDateTimeToCurrent()));

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

	ui->showFlipButtonsCheckbox->setChecked(gui->getFlagShowFlipButtons());
	connect(ui->showFlipButtonsCheckbox, SIGNAL(toggled(bool)), gui, SLOT(setFlagShowFlipButtons(bool)));

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

	// plugins control
	connect(ui->pluginsListWidget, SIGNAL(currentTextChanged(const QString&)), this, SLOT(pluginsSelectionChanged(const QString&)));
	connect(ui->pluginLoadAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(loadAtStartupChanged(int)));
	connect(ui->pluginConfigureButton, SIGNAL(clicked()), this, SLOT(pluginConfigureCurrentSelection()));
	populatePluginsList();


	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	updateConfigLabels();
}

void ConfigurationDialog::languageChanged(const QString& langName)
{
	QString code = StelTranslator::nativeNameToIso639_1Code(langName);
	StelApp::getInstance().getLocaleMgr().setAppLanguage(code);
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(code);
	updateConfigLabels();
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
}

void ConfigurationDialog::setAllSelectedInfo(void)
{
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
}

void ConfigurationDialog::setBriefSelectedInfo(void)
{
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::ShortInfo));
}

void ConfigurationDialog::cursorTimeOutChanged()
{
	StelMainGraphicsView::getInstance().setFlagCursorTimeout(ui->mouseTimeoutCheckbox->isChecked());
	StelMainGraphicsView::getInstance().setCursorTimeout(ui->mouseTimeoutSpinBox->value());
}

void ConfigurationDialog::browseForScreenshotDir()
{
	QString oldScreenshorDir = StelFileMgr::getScreenshotDir();
	QString newScreenshotDir = QFileDialog::getExistingDirectory(NULL, q_("Select screenshot directory"), oldScreenshorDir, QFileDialog::ShowDirsOnly);

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
	conf->setValue("viewing/flag_meridian_line", glmgr->getFlagMeridianLine());
	conf->setValue("viewing/flag_equatorial_J2000_grid", glmgr->getFlagEquatorJ2000Grid());
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

	// view dialog / starlore tab
	StelApp::getInstance().getSkyCultureMgr().setDefaultSkyCultureID(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());

	// Save default location
	StelApp::getInstance().getCore()->setDefaultLocationID(core->getCurrentLocation().getID());

	// configuration dialog / main tab
	QString langName = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	conf->setValue("localization/app_locale", StelTranslator::nativeNameToIso639_1Code(langName));
	langName = StelApp::getInstance().getLocaleMgr().getSkyLanguage();
	conf->setValue("localization/sky_locale", StelTranslator::nativeNameToIso639_1Code(langName));

	if (gui->getInfoTextFilters() == (StelObject::InfoStringGroup)0)
		conf->setValue("gui/selected_object_info", "none");
	else if (gui->getInfoTextFilters() == StelObject::InfoStringGroup(StelObject::ShortInfo))
		conf->setValue("gui/selected_object_info", "short");
	else
		conf->setValue("gui/selected_object_info", "all");

	// toolbar auto-hide status
	conf->setValue("gui/auto_hide_horizontal_toolbar", gui->getAutoHideHorizontalButtonBar());
	conf->setValue("gui/auto_hide_vertical_toolbar", gui->getAutoHideVerticalButtonBar());

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
	conf->setValue("navigation/auto_zoom_out_resets_direction", mvmgr->getFlagAutoZoomOutResetsDirection());
	conf->setValue("gui/flag_mouse_cursor_timeout", StelMainGraphicsView::getInstance().getFlagCursorTimeout());
	conf->setValue("gui/mouse_cursor_timeout", StelMainGraphicsView::getInstance().getCursorTimeout());

	conf->setValue("main/screenshot_dir", StelFileMgr::getScreenshotDir());
	conf->setValue("main/invert_screenshots_colors", StelMainGraphicsView::getInstance().getFlagInvertScreenShotColors());

	// full screen and window size
	conf->setValue("video/fullscreen", StelMainWindow::getInstance().getFullScreen());
	if (!StelMainWindow::getInstance().getFullScreen())
	{
		conf->setValue("video/screen_w", StelMainWindow::getInstance().size().width());
		conf->setValue("video/screen_h", StelMainWindow::getInstance().size().height());
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
		ui->pluginsListWidget->addItem(desc.info.displayedName);
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
		//BM: Localization of plug-in name and description should be done
		//in the plug-in, not here. This is acceptable only as a temporary
		//solution if someone puts the necessary strings in translations.h
		//so that they are included in the translation template.
		if (s==desc.info.displayedName)
		{
			QString html = "<html><head></head><body>";
			html += "<h2>" + q_(desc.info.displayedName) + "</h2>";
			html += "<h3>" + q_("Authors") + ": " + desc.info.authors + "</h3>";
			QString d = desc.info.description;
			d.replace("\n", "<br />");
			html += "<p>" + q_(d) + "</p>";
			html += "<h3>" + q_("Contact") + ": " + desc.info.contact + "</h3>";
			html += "</body></html>";
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

void ConfigurationDialog::pluginConfigureCurrentSelection(void)
{
	QString s = ui->pluginsListWidget->currentItem()->text();
	if (s.isEmpty() || s=="")
		return;

	const QList<StelModuleMgr::PluginDescriptor> pluginsList = StelApp::getInstance().getModuleMgr().getPluginsList();
	foreach (const StelModuleMgr::PluginDescriptor& desc, pluginsList)
	{
		if (s==desc.info.displayedName)
		{
			StelModule* pmod = StelApp::getInstance().getModuleMgr().getModule(desc.info.id);
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
	QString name = ui->pluginsListWidget->currentItem()->text();
	QString key;
	QList<StelModuleMgr::PluginDescriptor> pluginsList = StelApp::getInstance().getModuleMgr().getPluginsList();
	foreach (const StelModuleMgr::PluginDescriptor& desc, pluginsList)
	{
		if (desc.info.displayedName==name)
			key = desc.info.id;
	}
	if (!key.isEmpty())
		StelApp::getInstance().getModuleMgr().setPluginLoadAtStartup(key, state==Qt::Checked);
}

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
	html += "<h2>" + q_(scriptMgr.getName(s)) + "</h2>";
	html += "<h3>" + q_("Author") + ": " + scriptMgr.getAuthor(s) + "</h3>";
	html += "<h3>" + q_("License") + ": " + scriptMgr.getLicense(s) + "</h3>";
	QString d = scriptMgr.getDescription(s);
	d.replace("\n", "<br />");
	html += "<p>" + q_(d) + "</p>";
	html += "</body></html>";
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
	GETSTELMODULE(LabelMgr)->deleteAllLabels();
	GETSTELMODULE(ScreenImageMgr)->deleteAllImages();
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


void ConfigurationDialog::setFixedDateTimeToCurrent(void)
{
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(StelApp::getInstance().getCore()->getJDay()));
	ui->fixedTimeRadio->setChecked(true);
	setStartupTimeMode();
}

void ConfigurationDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->configurationStackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}


void ConfigurationDialog::refreshStarCatalogButton()
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

	if (idx == catalogConfig.size() && !hasDownloadedStarCatalog)//The size is 9; for "stars8", idx is 9
	{
		ui->getStarsButton->setVisible(false);
		ui->downloadLabel->setText(q_("Finished downloading all star catalogs!"));
		//BM: Doesn't this message duplicate the one below?
		//This one should be something like "All available star catalogs are installed."
		return;
	}

	ui->getStarsButton->setEnabled(true);
	if (!nextStarCatalogToDownload.isEmpty())
	{
		ui->getStarsButton->setText(q_("Get catalog %1 of %2").arg(idx).arg(catalogConfig.size()));
		const QVariantList& magRange = nextStarCatalogToDownload.value("magRange").toList();
		ui->downloadLabel->setText(q_("Download size: %1MB\nStar count: %2 Million\nMagnitude range: %3 - %4")
			.arg(nextStarCatalogToDownload.value("sizeMb").toString())
			.arg(nextStarCatalogToDownload.value("count").toString())
			.arg(magRange.at(0).toString())
			.arg(magRange.at(1).toString()));
		ui->getStarsButton->setVisible(true);
	}
	else
	{
		ui->downloadLabel->setText(q_("Finished downloading new star catalogs!\nRestart Stellarium to display them."));
		ui->getStarsButton->setVisible(false);
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

	ui->downloadLabel->setText(q_("Downloading %1...\n(You can close this window.)").arg(nextStarCatalogToDownload.value("id").toString()));
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

	refreshStarCatalogButton();
}
