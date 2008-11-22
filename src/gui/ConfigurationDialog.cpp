/*
 *
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
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "StelCore.hpp"
#include "MovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SkyDrawer.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "Location.hpp"
#include "LandscapeMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "SolarSystem.hpp"
#include "MeteorMgr.hpp"
#include "ConstellationMgr.hpp"
#include "StarMgr.hpp"
#include "NebulaMgr.hpp"
#include "GridLinesMgr.hpp"
#include "QtScriptMgr.hpp"

#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QFileDialog>

#include "StelAppGraphicsScene.hpp"

ConfigurationDialog::ConfigurationDialog()
{
	ui = new Ui_configurationDialogForm;
	downloader = NULL;
	updatesData = NULL;
	downloaded = 0;
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
	if(downloader != NULL)
		delete downloader;
	if(updatesData != NULL)
		delete updatesData;
}

void ConfigurationDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ConfigurationDialog::styleChanged()
{
	// Nothing for now
}

void ConfigurationDialog::createDialogContent()
{
	const ProjectorP proj = StelApp::getInstance().getCore()->getProjection(Mat4d());
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	
	ui->setupUi(dialog);
	
	// Set the main tab activated by default
	ui->configurationTabWidget->setCurrentIndex(0);
	
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	// Main tab
	// Fill the language list widget from the available list
	QString appLang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	QComboBox* cb = ui->programLanguageComboBox;
	cb->clear();
	cb->addItems(Translator::globalTranslator.getAvailableLanguagesNamesNative(StelApp::getInstance().getFileMgr().getLocaleDir()));
	QString l2 = Translator::iso639_1CodeToNativeName(appLang);
	int lt = cb->findText(l2, Qt::MatchExactly);
	if (lt == -1 && appLang.contains('_'))
	{
		l2 = appLang.left(appLang.indexOf('_'));
		l2=Translator::iso639_1CodeToNativeName(l2);
		lt = cb->findText(l2, Qt::MatchExactly);
	}
	if (lt!=-1)
		cb->setCurrentIndex(lt);
	connect(cb, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(languageChanged(const QString&)));
	
	starSettings = ((StarMgr*)GETSTELMODULE("StarMgr"))->getStarSettings();
	
	connect(ui->getStarsButton, SIGNAL(clicked(void)), this, SLOT(downloadStars(void)));
	connect(ui->downloadCancelButton, SIGNAL(clicked(void)), this, SLOT(cancelDownload(void)));
	connect(ui->downloadRetryButton, SIGNAL(clicked(void)), this, SLOT(retryDownload(void)));
	checkUpdates();
	
	// Selected object info
	if (gui->getInfoPanel()->getInfoTextFilters() == (StelObject::InfoStringGroup)0)
		ui->noSelectedInfoRadio->setChecked(true);
	else if (gui->getInfoPanel()->getInfoTextFilters() == StelObject::InfoStringGroup(StelObject::ShortInfo))
		ui->briefSelectedInfoRadio->setChecked(true);
	else
		ui->allSelectedInfoRadio->setChecked(true);
	connect(ui->noSelectedInfoRadio, SIGNAL(released()), this, SLOT(setNoSelectedInfo()));
	connect(ui->allSelectedInfoRadio, SIGNAL(released()), this, SLOT(setAllSelectedInfo()));
	connect(ui->briefSelectedInfoRadio, SIGNAL(released()), this, SLOT(setBriefSelectedInfo()));
	
	// Navigation tab
	// Startup time
	if (nav->getStartupTimeMode()=="actual")
		ui->systemTimeRadio->setChecked(true);
	else if (nav->getStartupTimeMode()=="today")
		ui->todayRadio->setChecked(true);
	else
		ui->fixedTimeRadio->setChecked(true);
	connect(ui->systemTimeRadio, SIGNAL(clicked(bool)), this, SLOT(setStartupTimeMode()));
	connect(ui->todayRadio, SIGNAL(clicked(bool)), this, SLOT(setStartupTimeMode()));
	connect(ui->fixedTimeRadio, SIGNAL(clicked(bool)), this, SLOT(setStartupTimeMode()));
	
	ui->todayTimeSpinBox->setTime(nav->getInitTodayTime());
	connect(ui->todayTimeSpinBox, SIGNAL(timeChanged(QTime)), nav, SLOT(setInitTodayTime(QTime)));
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(nav->getPresetSkyTime()));
	connect(ui->fixedDateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), nav, SLOT(setPresetSkyTime(QDateTime)));

	ui->enableKeysNavigationCheckBox->setChecked(mvmgr->getFlagEnableMoveKeys() || mvmgr->getFlagEnableZoomKeys());
	ui->enableMouseNavigationCheckBox->setChecked(mvmgr->getFlagEnableMouseNavigation());
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableMoveKeys(bool)));
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableZoomKeys(bool)));
	connect(ui->enableMouseNavigationCheckBox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagEnableMouseNavigation(bool)));
	connect(ui->fixedDateTimeCurrentButton, SIGNAL(clicked()), this, SLOT(setFixedDateTimeToCurrent()));
	
	// Tools tab
	ui->sphericMirrorCheckbox->setChecked(StelAppGraphicsScene::getInstance().getViewPortDistorterType() == "fisheye_to_spheric_mirror");
	connect(ui->sphericMirrorCheckbox, SIGNAL(toggled(bool)), this, SLOT(setSphericMirror(bool)));
	ui->gravityLabelCheckbox->setChecked(proj->getFlagGravityLabels());
	connect(ui->gravityLabelCheckbox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlagGravityLabels(bool)));
	ui->discViewportCheckbox->setChecked(proj->getMaskType() == Projector::MaskDisk);
	connect(ui->discViewportCheckbox, SIGNAL(toggled(bool)), this, SLOT(setDiskViewport(bool)));
	ui->autoZoomResetsDirectionCheckbox->setChecked(mvmgr->getFlagAutoZoomOutResetsDirection());
	connect(ui->autoZoomResetsDirectionCheckbox, SIGNAL(toggled(bool)), mvmgr, SLOT(setFlagAutoZoomOutResetsDirection(bool)));
	
	ui->showFlipButtonsCheckbox->setChecked(gui->getFlagShowFlipButtons());
	connect(ui->showFlipButtonsCheckbox, SIGNAL(toggled(bool)), gui, SLOT(setFlagShowFlipButtons(bool)));
	
	ui->mouseTimeoutCheckbox->setChecked(StelAppGraphicsScene::getInstance().getFlagCursorTimeout());
	ui->mouseTimeoutSpinBox->setValue(StelAppGraphicsScene::getInstance().getCursorTimeout());
	connect(ui->mouseTimeoutCheckbox, SIGNAL(clicked()), this, SLOT(cursorTimeOutChanged()));
	connect(ui->mouseTimeoutCheckbox, SIGNAL(toggled(bool)), this, SLOT(cursorTimeOutChanged()));
	connect(ui->mouseTimeoutSpinBox, SIGNAL(valueChanged(double)), this, SLOT(cursorTimeOutChanged(double)));
	
	connect(ui->setViewingOptionAsDefaultPushButton, SIGNAL(clicked()), this, SLOT(saveCurrentViewOptions()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(setDefaultViewOptions()));

	ui->screenshotDirEdit->setText(StelApp::getInstance().getFileMgr().getScreenshotDir());
	connect(ui->screenshotDirEdit, SIGNAL(textChanged(QString)), this, SLOT(selectScreenshotDir(QString)));
	connect(ui->screenshotBrowseButton, SIGNAL(clicked()), this, SLOT(browseForScreenshotDir()));
	
	ui->invertScreenShotColorsCheckBox->setChecked(StelMainGraphicsView::getInstance().getFlagInvertScreenShotColors());
	connect(ui->invertScreenShotColorsCheckBox, SIGNAL(toggled(bool)), &StelMainGraphicsView::getInstance(), SLOT(setFlagInvertScreenShotColors(bool)));
	
	// script tab controls
	QtScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();
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
	
	updateConfigLabels();
}

void ConfigurationDialog::languageChanged(const QString& langName)
{
	QString code = Translator::nativeNameToIso639_1Code(langName);
	StelApp::getInstance().getLocaleMgr().setAppLanguage(code);
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(code);
	updateConfigLabels();
}

void ConfigurationDialog::setStartupTimeMode(void)
{
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);

	if (ui->systemTimeRadio->isChecked())
		StelApp::getInstance().getCore()->getNavigation()->setStartupTimeMode("actual");
	else if (ui->todayRadio->isChecked())
		StelApp::getInstance().getCore()->getNavigation()->setStartupTimeMode("today");
	else
		StelApp::getInstance().getCore()->getNavigation()->setStartupTimeMode("preset");

	nav->setInitTodayTime(ui->todayTimeSpinBox->time());
	nav->setPresetSkyTime(ui->fixedDateTimeEdit->dateTime());
}

void ConfigurationDialog::setDiskViewport(bool b)
{
	if (b)
		StelApp::getInstance().getCore()->setMaskType(Projector::MaskDisk);
	else
		StelApp::getInstance().getCore()->setMaskType(Projector::MaskNone);
}

void ConfigurationDialog::setSphericMirror(bool b)
{
	if (b)
		StelAppGraphicsScene::getInstance().setViewPortDistorterType("fisheye_to_spheric_mirror");
	else
		StelAppGraphicsScene::getInstance().setViewPortDistorterType("none");
}

void ConfigurationDialog::setNoSelectedInfo(void)
{
	StelGui* newGui = (StelGui*)GETSTELMODULE("StelGui");
	Q_ASSERT(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(0));
}

void ConfigurationDialog::setAllSelectedInfo(void)
{
	StelGui* newGui = (StelGui*)GETSTELMODULE("StelGui");
	Q_ASSERT(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
}

void ConfigurationDialog::setBriefSelectedInfo(void)
{
	StelGui* newGui = (StelGui*)GETSTELMODULE("StelGui");
	Q_ASSERT(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::ShortInfo));
}

void ConfigurationDialog::cursorTimeOutChanged()
{
	StelAppGraphicsScene::getInstance().setFlagCursorTimeout(ui->mouseTimeoutCheckbox->isChecked());
	StelAppGraphicsScene::getInstance().setCursorTimeout(ui->mouseTimeoutSpinBox->value());
}

void ConfigurationDialog::browseForScreenshotDir()
{
	QString oldScreenshorDir = StelApp::getInstance().getFileMgr().getScreenshotDir();
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
		StelApp::getInstance().getFileMgr().setScreenshotDir(dir);
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

	LandscapeMgr* lmgr = (LandscapeMgr*)GETSTELMODULE("LandscapeMgr");
	Q_ASSERT(lmgr);
	SolarSystem* ssmgr = (SolarSystem*)GETSTELMODULE("SolarSystem");
	Q_ASSERT(ssmgr);
	MeteorMgr* mmgr = (MeteorMgr*)GETSTELMODULE("MeteorMgr");
	Q_ASSERT(mmgr);
	SkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(skyd);
	ConstellationMgr* cmgr = (ConstellationMgr*)GETSTELMODULE("ConstellationMgr");
	Q_ASSERT(cmgr);
	StarMgr* smgr = (StarMgr*)GETSTELMODULE("StarMgr");
	Q_ASSERT(smgr);
	NebulaMgr* nmgr = (NebulaMgr*)GETSTELMODULE("NebulaMgr");
	Q_ASSERT(nmgr);
	GridLinesMgr* glmgr = (GridLinesMgr*)GETSTELMODULE("GridLinesMgr");
	Q_ASSERT(glmgr);
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	Q_ASSERT(gui);
	MovementMgr* mvmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	Q_ASSERT(mvmgr);
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	Q_ASSERT(nav);
	const ProjectorP proj = StelApp::getInstance().getCore()->getProjection(Mat4d());
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
	conf->setValue("viewing/constellation_art_intensity", cmgr->getArtIntensity());
	conf->setValue("astro/flag_star_name", smgr->getFlagLabels());
	conf->setValue("stars/labels_amount", smgr->getLabelsAmount());
	conf->setValue("astro/flag_planets_labels", ssmgr->getFlagLabels());
	conf->setValue("astro/labels_amount", ssmgr->getLabelsAmount());
	conf->setValue("astro/nebula_hints_amount", nmgr->getHintsAmount());
	conf->setValue("astro/flag_nebula_name", nmgr->getFlagHints());
	conf->setValue("projection/type", StelApp::getInstance().getCore()->getCurrentProjectionTypeKey());

	// view dialog / landscape tab settings
	lmgr->setDefaultLandscapeID(lmgr->getCurrentLandscapeID());
	conf->setValue("landscape/flag_landscape_sets_location", lmgr->getFlagLandscapeSetsLocation());
	conf->setValue("landscape/flag_landscape", lmgr->getFlagLandscape());
	conf->setValue("landscape/flag_atmosphere", lmgr->getFlagAtmosphere());
	conf->setValue("landscape/flag_fog", lmgr->getFlagFog());
	conf->setValue("stars/init_bortle_scale", StelApp::getInstance().getCore()->getSkyDrawer()->getBortleScale());

	// view dialog / starlore tab
	StelApp::getInstance().getSkyCultureMgr().setDefaultSkyCultureID(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());
	
	// Save default location
	nav->setDefaultLocationID(nav->getCurrentLocation().getID());

	// configuration dialog / main tab
	QString langName = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	conf->setValue("localization/app_locale", Translator::nativeNameToIso639_1Code(langName));
	langName = StelApp::getInstance().getLocaleMgr().getSkyLanguage();
	conf->setValue("localization/sky_locale", Translator::nativeNameToIso639_1Code(langName));

	if (gui->getInfoPanel()->getInfoTextFilters() == (StelObject::InfoStringGroup)0)
		conf->setValue("gui/selected_object_info", "none");
	else if (gui->getInfoPanel()->getInfoTextFilters() == StelObject::InfoStringGroup(StelObject::ShortInfo))
		conf->setValue("gui/selected_object_info", "short");
	else
		conf->setValue("gui/selected_object_info", "all");

	// toolbar auto-hide status
	conf->setValue("gui/auto_hide_horizontal_toolbar", gui->getAutoHideHorizontalButtonBar());
	conf->setValue("gui/auto_hide_vertical_toolbar", gui->getAutoHideVerticalButtonBar());

	mvmgr->setInitFov(StelApp::getInstance().getCore()->getMovementMgr()->getCurrentFov());
	StelApp::getInstance().getCore()->getNavigation()->setInitViewDirectionToCurrent();
	
	// configuration dialog / navigation tab
	conf->setValue("navigation/flag_enable_zoom_keys", mvmgr->getFlagEnableZoomKeys());
	conf->setValue("navigation/flag_enable_mouse_navigation", mvmgr->getFlagEnableMouseNavigation());
	conf->setValue("navigation/flag_enable_move_keys", mvmgr->getFlagEnableMoveKeys());
	conf->setValue("navigation/startup_time_mode", nav->getStartupTimeMode());
	conf->setValue("navigation/today_time", nav->getInitTodayTime());
	conf->setValue("navigation/preset_sky_time", nav->getPresetSkyTime());
	conf->setValue("navigation/init_fov", mvmgr->getInitFov());
	if (nav->getMountMode() == Navigator::MountAltAzimuthal)
		conf->setValue("navigation/viewing_mode", "horizon");
	else
		conf->setValue("navigation/viewing_mode", "equator");
	

	// configuration dialog / tools tab
	conf->setValue("gui/flag_show_flip_buttons", gui->getFlagShowFlipButtons());
	conf->setValue("video/distorter", StelAppGraphicsScene::getInstance().getViewPortDistorterType());
	conf->setValue("projection/viewport", Projector::maskTypeToString(proj->getMaskType()));
	conf->setValue("viewing/flag_gravity_labels", proj->getFlagGravityLabels());
	conf->setValue("navigation/auto_zoom_out_resets_direction", mvmgr->getFlagAutoZoomOutResetsDirection());
	conf->setValue("gui/flag_mouse_cursor_timeout", StelAppGraphicsScene::getInstance().getFlagCursorTimeout());
	conf->setValue("gui/mouse_cursor_timeout", StelAppGraphicsScene::getInstance().getCursorTimeout());
	
	conf->setValue("main/screenshot_dir", StelApp::getInstance().getFileMgr().getScreenshotDir());

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
	const Vec3d v = StelApp::getInstance().getCore()->getNavigation()->getInitViewingDirection();
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

void ConfigurationDialog::populateScriptsList(void)
{
	int prevSel = ui->scriptListWidget->currentRow();
	QtScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();
	ui->scriptListWidget->clear();
	ui->scriptListWidget->addItems(scriptMgr.getScriptList().replaceInStrings(QRegExp("\\.ssc$"), ""));
	// If we had a valid previous selection (i.e. not first time we populate), restore it
	if (prevSel >= 0 && prevSel < ui->scriptListWidget->count())
		ui->scriptListWidget->setCurrentRow(prevSel);
	else
		ui->scriptListWidget->setCurrentRow(0);
}

void ConfigurationDialog::scriptSelectionChanged(const QString& s)
{
	if (s.isEmpty() || s=="")
		return;
	QtScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();
	//ui->scriptInfoBrowser->document()->setDefaultStyleSheet(QString(StelApp::getInstance().getCurrentStelStyle()->htmlStyleSheet));
	QString html = "<html><head></head><body>";
	html += "<h2>" + scriptMgr.getName(s+".ssc") + "</h2>";
	html += "<h3>" + q_("Author") + ": " + scriptMgr.getAuthor(s+".ssc") + "</h3>";
	html += "<h3>" + q_("License") + ": " + scriptMgr.getLicense(s+".ssc") + "</h3>";
	QString d = scriptMgr.getDescription(s+".ssc");
	d.replace("\n", "<br />");
	html += "<p>" + d + "</p>";
	html += "</body></html>";
	ui->scriptInfoBrowser->setHtml(html);
}

void ConfigurationDialog::runScriptClicked(void)
{
	if (ui->closeWindowAtScriptRunCheckbox->isChecked())
		this->close();
		
	QtScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();
	if (ui->scriptListWidget->currentItem())
	{
		ui->runScriptButton->setEnabled(false);
		ui->stopScriptButton->setEnabled(true);
		scriptMgr.runScript(ui->scriptListWidget->currentItem()->text()+".ssc");
	}
}

void ConfigurationDialog::stopScriptClicked(void)
{
	StelApp::getInstance().getScriptMgr().stopScript();
}

void ConfigurationDialog::aScriptIsRunning(void)
{
	ui->scriptStatusLabel->setText(q_("Running script: ") + StelApp::getInstance().getScriptMgr().runningScriptId());
	ui->runScriptButton->setEnabled(false);
	ui->stopScriptButton->setEnabled(true);
}

void ConfigurationDialog::aScriptHasStopped(void)
{
	ui->scriptStatusLabel->setText(q_("Running script: [none]"));
	ui->runScriptButton->setEnabled(true);
	ui->stopScriptButton->setEnabled(false);
}

void ConfigurationDialog::setUpdatesState(ConfigurationDialog::UpdatesState state)
{
	// everything invisible by default
	ui->getStarsButton->setVisible(false);
	ui->downloadLabel->setText("");
	ui->downloadCancelButton->setVisible(false);
	ui->downloadRetryButton->setVisible(false);
	
	switch(state)
	{
		case ConfigurationDialog::ShowAvailable:
			if(newCatalogs.size()-downloaded > 0)
			{
				QString cat = newCatalogs.at(downloaded);
				ui->getStarsButton->setVisible(true);
				ui->getStarsButton->setText(QString("Get catalog %1 of %2").arg(downloaded+1).arg(newCatalogs.size()));
				ui->downloadLabel->setText(QString("Download size: %1MB\nStar count: %2\nMagnitude range: %3 - %4")
					.arg(updatesData->value(cat+"/size").toString())
					.arg(updatesData->value(cat+"/count").toString())
					.arg(updatesData->value(cat+"/mag_lower").toString())
					.arg(updatesData->value(cat+"/mag_upper").toString()));
			} else
				setUpdatesState(ConfigurationDialog::Finished);
			break;
		
		case ConfigurationDialog::Checking:
			ui->downloadLabel->setText("Checking for new star catalogs...");
			break;
		
		case ConfigurationDialog::NoUpdates:
			ui->downloadLabel->setText("All star catalogs are up to date.");
			break;
		
		case ConfigurationDialog::Downloading:
			ui->downloadLabel->setText(QString("Downloading %1...\n(You can close this window.)").arg(downloadName));
			ui->downloadCancelButton->setVisible(true);
			break;
		
		case ConfigurationDialog::Finished:
			ui->downloadLabel->setText("Finished downloading new star catalogs!\nRestart Stellarium to display them.");
			break;
		
		case ConfigurationDialog::Verifying:
			ui->downloadLabel->setText("Verifying file integrity...");
			break;
		
		case ConfigurationDialog::UpdatesError:
			ui->downloadLabel->setText(QString("Error checking updates:\n%2").arg(downloader->url()).arg(downloader->errorString()));
			ui->downloadRetryButton->setVisible(true);
			break;
		
		case ConfigurationDialog::MoveError:
			ui->downloadLabel->setText(QString("Could not finalize download:\nError moving temporary file %1.tmp to %1.cat").arg(downloadName));
			break;
		
		case ConfigurationDialog::DownloadError:
			ui->downloadLabel->setText(QString("Error downloading %1:\n%2").arg(downloadName).arg(downloader->errorString()));
			ui->downloadRetryButton->setVisible(true);
			break;
		
		case ConfigurationDialog::ChecksumError:
			ui->downloadLabel->setText(QString("Error downloading %1:\nFile is corrupted.").arg(downloadName));
			ui->downloadRetryButton->setVisible(true);
			break;
	}
}

void ConfigurationDialog::updatesDownloadError(QNetworkReply::NetworkError code, QString errorString)
{
	qDebug() << "Error checking updates from" << downloader->url() << ":" << errorString;
	setUpdatesState(ConfigurationDialog::UpdatesError);
}

void ConfigurationDialog::updatesDownloadFinished(void)
{
	updatesData = new QSettings(updatesFileName, QSettings::IniFormat);
	updatesData->beginGroup("extra_stars");
	QListIterator<QString> it(updatesData->childKeys());
	
	// For some reason, updatesData->childGroups() doesn't work at
	// all in this case. The workaround is to list the groups as key-value pairs.
	while(it.hasNext())
	{
		QString catName = updatesData->value(it.next()).toString();
		bool catPathExists = !starSettings->value(catName+"/path","").toString().isEmpty();
		if(!catPathExists)
			newCatalogs.append(catName);
	}
	
	if(newCatalogs.size() > 0)
		setUpdatesState(ConfigurationDialog::ShowAvailable);
	else
		setUpdatesState(ConfigurationDialog::NoUpdates);
}

void ConfigurationDialog::checkUpdates(void)
{
	setUpdatesState(ConfigurationDialog::Checking);
	
	updatesFileName = "stars/default/updates.ini";
	downloader = new Downloader(starSettings->value("updates_url").toString(), updatesFileName);
	downloader->get(false);
	
	connect(downloader, SIGNAL(finished()), this, SLOT(updatesDownloadFinished()));
	connect(downloader, SIGNAL(error(QNetworkReply::NetworkError, QString)), this, SLOT(updatesDownloadError(QNetworkReply::NetworkError, QString)));
}

void ConfigurationDialog::cancelDownload(void)
{
	downloader->abort();
	setUpdatesState(ConfigurationDialog::ShowAvailable);
}

void ConfigurationDialog::retryDownload(void)
{
	if(updatesData == NULL)
		checkUpdates();
	else
		downloadStars();
}

void ConfigurationDialog::downloadError(QNetworkReply::NetworkError code, QString errorString)
{
	qDebug() << "Error downloading file" << downloader->url() << ":" << errorString;
	setUpdatesState(ConfigurationDialog::DownloadError);
}

void ConfigurationDialog::downloadVerifying(void)
{
	setUpdatesState(ConfigurationDialog::Verifying);
}

void ConfigurationDialog::badChecksum(void)
{
	qDebug() << "Error downloading file" << downloader->url() << ": File is corrupted.";
	setUpdatesState(ConfigurationDialog::ChecksumError);
}

void ConfigurationDialog::downloadFinished(void)
{
	downloaded++;
	QString tempFileName = "stars/default/"+downloadName+".tmp";
	QString finalFileName = "stars/default/"+downloadName+".cat";
	
	if( ( QFile::exists(finalFileName) && !QFile::remove(finalFileName) ) ||
		!QFile::copy(tempFileName, finalFileName) ||
		!QFile::remove(tempFileName))
	{
		qDebug() << "Error moving" << tempFileName << "to" << finalFileName;
		setUpdatesState(ConfigurationDialog::MoveError);
	}
	else
	{
		starSettings->setValue(downloadName+"/path", downloadName+".cat");
		setUpdatesState(ConfigurationDialog::ShowAvailable);
	}
}

void ConfigurationDialog::downloadStars(void)
{
	if(downloader != NULL)
		delete downloader;
	
	downloadName = newCatalogs.at(downloaded);
	QString url = updatesData->value(downloadName+"/url").toString();
	quint16 checksum = updatesData->value(downloadName+"/checksum").toUInt();
	downloader = new Downloader(url, "stars/default/"+downloadName+".tmp", checksum);
	downloader->get(true, QString("%p%: %1 of %2").arg(downloaded+1).arg(newCatalogs.size()));
	
	connect(downloader, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(downloader, SIGNAL(error(QNetworkReply::NetworkError, QString)), this, SLOT(downloadError(QNetworkReply::NetworkError, QString)));
	connect(downloader, SIGNAL(verifying()), this, SLOT(downloadVerifying()));
	connect(downloader, SIGNAL(badChecksum()), this, SLOT(badChecksum()));
	setUpdatesState(ConfigurationDialog::Downloading);
}

void ConfigurationDialog::setFixedDateTimeToCurrent(void)
{
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(StelApp::getInstance().getCore()->getNavigation()->getJDay()));
	ui->fixedTimeRadio->setChecked(true);
	setStartupTimeMode();
}

Downloader::~Downloader()
{
	if(reply)
		delete reply;
}

void Downloader::get(bool showBar, const QString& barFormat)
{
	target.open(QIODevice::WriteOnly | QIODevice::Truncate);
	reply = networkManager->get(QNetworkRequest(QUrl(address)));
	
	showProgressBar = showBar;
	if(showProgressBar)
	{
		progressBar->setValue(0);
		progressBar->setVisible(true);
		progressBar->setFormat(barFormat);
	} else
		progressBar->setVisible(false);
	
	connect(reply, SIGNAL(readyRead(void)), this, SLOT(readData(void)));
	connect(reply, SIGNAL(finished(void)), this, SLOT(fin(void)));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(err(QNetworkReply::NetworkError)));
	if(showProgressBar)
		connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(updateDownloadBar(qint64, qint64)));
}

void Downloader::abort(void)
{
	reply->abort();
}

void Downloader::readData(void)
{
	int size = reply->bytesAvailable();
	QByteArray data = reply->read(size);
	stream.writeRawData(data.constData(), size);
}

void Downloader::updateDownloadBar(qint64 received, qint64 total)
{
	progressBar->setMaximum(total);
	progressBar->setValue(received);
}

void Downloader::fin(void)
{
	if(reply->error())
		return;
	
	if(total-received == 0)
	{
		target.close();
		if(showProgressBar)
			progressBar->setVisible(false);
		
		if(useChecksum)
		{
			emit verifying();
			
			target.open(QIODevice::ReadOnly);
			quint16 fileChecksum = qChecksum(target.readAll().constData(), target.size());
			target.close();
			
			if(fileChecksum != checksum)
				emit badChecksum();
			else
				emit finished();
		}
		else
			emit finished();
	} else
		if(!target.remove())
			qDebug() << "Error deleting incomplete file" << path;
}

void Downloader::err(QNetworkReply::NetworkError code)
{
	if(code != QNetworkReply::NoError)
	{
		target.close();
		if(!target.remove())
			qDebug() << "Error deleting incomplete file" << path;
		if(showProgressBar)
			progressBar->setVisible(false);
	}
	
	if(code != QNetworkReply::OperationCanceledError)
		emit error(code, reply->errorString());
}


