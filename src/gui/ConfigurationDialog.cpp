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
#include "CustomDeltaTEquationDialog.hpp"
#include "StelMainView.hpp"
#include "ui_configurationDialog.h"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelProjector.hpp"
#include "StelObjectMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelProgressController.hpp"

#include "StelUtils.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocation.hpp"
#include "StelSkyLayerMgr.hpp"
#include "ConstellationMgr.hpp"
#include "StarMgr.hpp"
#include "NebulaMgr.hpp"
#include "Planet.hpp"
#ifdef ENABLE_SCRIPTING
#include "StelScriptMgr.hpp"
#endif
#include "StelJsonParser.hpp"
#include "StelTranslator.hpp"
#include "EphemWrapper.hpp"
#include "ToastMgr.hpp"

#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFontDialog>
#include <QComboBox>
#include <QDir>
#include <QDesktopWidget>
#include <QImageWriter>
#include <QScreen>

//! Simple helper extension class which can guarantee int inputs in a useful range.
class MinMaxIntValidator: public QIntValidator
{
public:
	MinMaxIntValidator(int min, int max, QObject *parent=Q_NULLPTR):
		QIntValidator(min, max, parent){}

	virtual void fixup(QString &input) const
	{
		int allowed=qBound(bottom(), input.toInt(), top());
		input.setNum(allowed);
	}
};

ConfigurationDialog::ConfigurationDialog(StelGui* agui, QObject* parent)
	: StelDialog("Configuration", parent)
	, isDownloadingStarCatalog(false)
	, nextStarCatalogToDownloadIndex(0)
	, starCatalogsCount(0)
	, hasDownloadedStarCatalog(false)
	, starCatalogDownloadReply(Q_NULLPTR)
	, currentDownloadFile(Q_NULLPTR)
	, progressBar(Q_NULLPTR)
	, gui(agui)
	, customDeltaTEquationDialog(Q_NULLPTR)
	, savedProjectionType(StelApp::getInstance().getCore()->getCurrentProjectionType())
{
	ui = new Ui_configurationDialogForm;
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
	ui = Q_NULLPTR;
	delete customDeltaTEquationDialog;
	customDeltaTEquationDialog = Q_NULLPTR;
	delete currentDownloadFile;
	currentDownloadFile = Q_NULLPTR;
}

void ConfigurationDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);

		//Initial FOV and direction on the "Main" page
		updateConfigLabels();
		
		//Star catalog download button and info
		updateStarCatalogControlsText();

		//Script information
		//(trigger re-displaying the description of the current item)
		#ifdef ENABLE_SCRIPTING
		scriptSelectionChanged(ui->scriptListWidget->currentItem()->text());
		#endif

		populateDitherList();

		//Plug-in information
		populatePluginsList();

		populateDeltaTAlgorithmsList();
		populateDateFormatsList();
		populateTimeFormatsList();

		//Hack to shrink the tabs to optimal size after language change
		//by causing the list items to be laid out again.
		updateTabBarListWidgetWidth();
	}
}

void ConfigurationDialog::createDialogContent()
{
	const StelProjectorP proj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000);
	StelCore* core = StelApp::getInstance().getCore();

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);

	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	// Set the main tab activated by default
	ui->configurationStackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);

	// Kinetic scrolling
	kineticScrollingList << ui->pluginsListWidget << ui->scriptListWidget;
	StelGui* appGui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (appGui)
	{
		enableKineticScrolling(appGui->getFlagUseKineticScrolling());
		connect(appGui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Main tab
	#ifdef ENABLE_NLS
	// Fill the language list widget from the available list
	QComboBox* cb = ui->programLanguageComboBox;
	cb->clear();
	cb->addItems(StelTranslator::globalTranslator->getAvailableLanguagesNamesNative(StelFileMgr::getLocaleDir()));
	cb->model()->sort(0);
	updateCurrentLanguage();
	connect(cb->lineEdit(), SIGNAL(editingFinished()), this, SLOT(updateCurrentLanguage()));
	connect(cb, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectLanguage(const QString&)));
	// Do the same for sky language:
	cb = ui->skycultureLanguageComboBox;
	cb->clear();
	cb->addItems(StelTranslator::globalTranslator->getAvailableLanguagesNamesNative(StelFileMgr::getLocaleDir(), "skycultures"));
	cb->model()->sort(0);
	updateCurrentSkyLanguage();
	connect(cb->lineEdit(), SIGNAL(editingFinished()), this, SLOT(updateCurrentSkyLanguage()));
	connect(cb, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectSkyLanguage(const QString&)));
	#else
	ui->groupBox_LanguageSettings->hide();
	#endif

	connect(ui->getStarsButton, SIGNAL(clicked()), this, SLOT(downloadStars()));
	connect(ui->downloadCancelButton, SIGNAL(clicked()), this, SLOT(cancelDownload()));
	connect(ui->downloadRetryButton, SIGNAL(clicked()), this, SLOT(downloadStars()));
	resetStarCatalogControls();

	connect(ui->de430checkBox, SIGNAL(clicked()), this, SLOT(de430ButtonClicked()));
	connect(ui->de431checkBox, SIGNAL(clicked()), this, SLOT(de431ButtonClicked()));
	resetEphemControls();

	ui->nutationCheckBox->setChecked(core->getUseNutation());
	connect(ui->nutationCheckBox, SIGNAL(toggled(bool)), core, SLOT(setUseNutation(bool)));
	ui->topocentricCheckBox->setChecked(core->getUseTopocentricCoordinates());
	connect(ui->topocentricCheckBox, SIGNAL(toggled(bool)), core, SLOT(setUseTopocentricCoordinates(bool)));

	// Selected object info
	if (gui->getInfoTextFilters() == StelObject::InfoStringGroup(Q_NULLPTR))
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

	// Additional settings for selected object info
	connectBoolProperty(ui->checkBoxUMSurfaceBrightness, "NebulaMgr.flagSurfaceBrightnessArcsecUsage");
	connectBoolProperty(ui->checkBoxUMShortNotationSurfaceBrightness, "NebulaMgr.flagSurfaceBrightnessShortNotationUsage");
	connectBoolProperty(ui->checkBoxUseFormattingOutput, "StelApp.flagUseFormattingOutput");
	connectBoolProperty(ui->checkBoxUseCCSDesignations,  "StelApp.flagUseCCSDesignation");
	connectBoolProperty(ui->overwriteTextColorCheckBox,  "StelApp.flagOverwriteInfoColor");

	connect(ui->noSelectedInfoRadio, SIGNAL(released()), this, SLOT(setNoSelectedInfo()));
	connect(ui->allSelectedInfoRadio, SIGNAL(released()), this, SLOT(setAllSelectedInfo()));
	connect(ui->briefSelectedInfoRadio, SIGNAL(released()), this, SLOT(setBriefSelectedInfo()));
	connect(ui->customSelectedInfoRadio, SIGNAL(released()), this, SLOT(setCustomSelectedInfo()));
	connect(ui->buttonGroupDisplayedFields, SIGNAL(buttonClicked(int)), this, SLOT(setSelectedInfoFromCheckBoxes()));
	
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
	ui->fixedDateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
	connect(ui->fixedDateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), core, SLOT(setPresetSkyTime(QDateTime)));

	bool state = (mvmgr->getFlagEnableMoveKeys() || mvmgr->getFlagEnableZoomKeys());
	ui->enableKeysNavigationCheckBox->setChecked(state);
	ui->editShortcutsPushButton->setEnabled(state);
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), this, SLOT(setKeyNavigationState(bool)));
	connectBoolProperty(ui->enableMouseNavigationCheckBox,  "StelMovementMgr.flagEnableMouseNavigation");

	connect(ui->fixedDateTimeCurrentButton, SIGNAL(clicked()), this, SLOT(setFixedDateTimeToCurrent()));
	connect(ui->editShortcutsPushButton, SIGNAL(clicked()), this, SLOT(showShortcutsWindow()));

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();
	// Display formats of date
	populateDateFormatsList();
	int idx = ui->dateFormatsComboBox->findData(localeManager.getDateFormatStr(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use system_default as default
		idx = ui->dateFormatsComboBox->findData(QVariant("system_default"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->dateFormatsComboBox->setCurrentIndex(idx);
	connect(ui->dateFormatsComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setDateFormat()));

	// Display formats of time
	populateTimeFormatsList();
	idx = ui->timeFormatsComboBox->findData(localeManager.getTimeFormatStr(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use system_default as default
		idx = ui->timeFormatsComboBox->findData(QVariant("system_default"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->timeFormatsComboBox->setCurrentIndex(idx);
	connect(ui->timeFormatsComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setTimeFormat()));
	if (StelApp::getInstance().getSettings()->value("gui/flag_time_jd", false).toBool())
		ui->jdRadioButton->setChecked(true);
	else
		ui->dtRadioButton->setChecked(true);
	connect(ui->jdRadioButton, SIGNAL(clicked(bool)), this, SLOT(setButtonBarDTFormat()));
	connect(ui->dtRadioButton, SIGNAL(clicked(bool)), this, SLOT(setButtonBarDTFormat()));

	// Delta-T
	ui->pushButtonCustomDeltaTEquationDialog->setFixedSize(QSize(26, 26));
	populateDeltaTAlgorithmsList();	
	idx = ui->deltaTAlgorithmComboBox->findData(core->getCurrentDeltaTAlgorithmKey(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use Espenak & Meeus (2006) as default
		idx = ui->deltaTAlgorithmComboBox->findData(QVariant("EspenakMeeus"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->deltaTAlgorithmComboBox->setCurrentIndex(idx);
	connect(ui->deltaTAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setDeltaTAlgorithm(int)));
	connect(ui->pushButtonCustomDeltaTEquationDialog, SIGNAL(clicked()), this, SLOT(showCustomDeltaTEquationDialog()));
	if (core->getCurrentDeltaTAlgorithm()==StelCore::Custom)
		ui->pushButtonCustomDeltaTEquationDialog->setEnabled(true);

	// Tools tab
	ui->sphericMirrorCheckbox->setChecked(StelApp::getInstance().getViewportEffect() == "sphericMirrorDistorter");
	connect(ui->sphericMirrorCheckbox, SIGNAL(toggled(bool)), this, SLOT(setSphericMirror(bool)));
	connectBoolProperty(ui->gravityLabelCheckbox, "StelCore.flagGravityLabels");

	connectBoolProperty(ui->selectSingleConstellationButton, "ConstellationMgr.isolateSelected");
	ui->diskViewportCheckbox->setChecked(proj->getMaskType() == StelProjector::MaskDisk);
	connect(ui->diskViewportCheckbox, SIGNAL(toggled(bool)), this, SLOT(setDiskViewport(bool)));
	connectBoolProperty(ui->autoZoomResetsDirectionCheckbox, "StelMovementMgr.flagAutoZoomOutResetsDirection");

	connectBoolProperty(ui->showQuitButtonCheckBox,				"StelGui.flagShowQuitButton");
	connectBoolProperty(ui->showFlipButtonsCheckbox,				"StelGui.flagShowFlipButtons");
	connectBoolProperty(ui->showNebulaBgButtonCheckbox,			"StelGui.flagShowNebulaBackgroundButton");
	connectBoolProperty(ui->showBookmarksButtonCheckBox,		"StelGui.flagShowBookmarksButton");
	connectBoolProperty(ui->showICRSGridButtonCheckBox,			"StelGui.flagShowICRSGridButton");
	connectBoolProperty(ui->showGalacticGridButtonCheckBox,		"StelGui.flagShowGalacticGridButton");
	connectBoolProperty(ui->showEclipticGridButtonCheckBox,		"StelGui.flagShowEclipticGridButton");
	connectBoolProperty(ui->showHipsButtonCheckBox,				"StelGui.flagShowHiPSButton");
	connectBoolProperty(ui->showDSSButtonCheckbox,				"StelGui.flagShowDSSButton");
	connectBoolProperty(ui->showGotoSelectedButtonCheckBox,		"StelGui.flagShowGotoSelectedObjectButton");
	connectBoolProperty(ui->showNightmodeButtonCheckBox,		"StelGui.flagShowNightmodeButton");
	connectBoolProperty(ui->showFullscreenButtonCheckBox,			"StelGui.flagShowFullscreenButton");
	connectBoolProperty(ui->showCardinalButtonCheckBox,			"StelGui.flagShowCardinalButton");
	connectBoolProperty(ui->showCompassButtonCheckBox,			"StelGui.flagShowCompassButton");

	connectBoolProperty(ui->showConstellationBoundariesButtonCheckBox, "StelGui.flagShowConstellationBoundariesButton");
	connectBoolProperty(ui->showAsterismLinesButtonCheckBox,		"StelGui.flagShowAsterismLinesButton");
	connectBoolProperty(ui->showAsterismLabelsButtonCheckBox,	"StelGui.flagShowAsterismLabelsButton");

	connectBoolProperty(ui->decimalDegreeCheckBox,				"StelApp.flagShowDecimalDegrees");
	connectBoolProperty(ui->azimuthFromSouthcheckBox,			"StelApp.flagUseAzimuthFromSouth");

	connectBoolProperty(ui->mouseTimeoutCheckbox,				"MainView.flagCursorTimeout");
	connectDoubleProperty(ui->mouseTimeoutSpinBox,				"MainView.cursorTimeout");
	connectBoolProperty(ui->useButtonsBackgroundCheckBox,		"StelGui.flagUseButtonsBackground");
	connectBoolProperty(ui->indicationMountModeCheckBox,			"StelMovementMgr.flagIndicationMountMode");
	connectBoolProperty(ui->kineticScrollingCheckBox,				"StelGui.flagUseKineticScrolling");
	connectBoolProperty(ui->focusOnDaySpinnerCheckBox,			"StelGui.flagEnableFocusOnDaySpinner");
	connectColorButton(ui->overwriteTextColorButton,				"StelApp.overwriteInfoColor",	"color/info_text_color");
	connectColorButton(ui->daylightTextColorButton,				"StelApp.daylightInfoColor",	"color/daylight_text_color");

	// Font selection. We use a hidden, but documented entry in config.ini to optionally show a font selection option.
	connectIntProperty(ui->screenFontSizeSpinBox, "StelApp.screenFontSize");
	connectIntProperty(ui->guiFontSizeSpinBox, "StelApp.guiFontSize");
	if (StelApp::getInstance().getSettings()->value("gui/flag_font_selection", false).toBool())
	{
		populateFontWritingSystemCombo();
		connect(ui->fontWritingSystemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleFontBoxWritingSystem(int)));

		ui->fontComboBox->setWritingSystem(QFontDatabase::Any);
		ui->fontComboBox->setFontFilters(QFontComboBox::ScalableFonts | QFontComboBox::ProportionalFonts);
		ui->fontComboBox->setCurrentFont(QGuiApplication::font());
		connect(ui->fontComboBox, SIGNAL(currentFontChanged(QFont)), &StelApp::getInstance(), SLOT(setAppFont(QFont)));
	}
	else
	{
		ui->fontWritingSystemComboBox->hide();
		ui->fontComboBox->hide();
	}

	// Dithering
	populateDitherList();
	connect(ui->ditheringComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setDitherFormat()));

	// General Option Save
	connect(ui->saveViewDirAsDefaultPushButton, SIGNAL(clicked()), this, SLOT(saveCurrentViewDirSettings()));
	connect(ui->saveSettingsAsDefaultPushButton, SIGNAL(clicked()), this, SLOT(saveAllSettings()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(setDefaultViewOptions()));

	// Screenshots
	populateScreenshotFileformatsCombo();
	connectStringProperty(ui->screenshotFileFormatComboBox, "MainView.screenShotFormat");
	ui->screenshotDirEdit->setText(StelFileMgr::getScreenshotDir());
	connect(ui->screenshotDirEdit, SIGNAL(editingFinished()), this, SLOT(selectScreenshotDir()));
	connect(ui->screenshotBrowseButton, SIGNAL(clicked()), this, SLOT(browseForScreenshotDir()));
	connectBoolProperty(ui->invertScreenShotColorsCheckBox, "MainView.flagInvertScreenShotColors");
#ifdef USE_OLD_QGLWIDGET
	ui->useCustomScreenshotSizeCheckBox->setEnabled(false);
	ui->useCustomScreenshotSizeCheckBox->hide();
	ui->customScreenshotWidthLineEdit->hide();
	ui->label_times->hide();
	ui->customScreenshotHeightLineEdit->hide();
#else
	connectBoolProperty(ui->useCustomScreenshotSizeCheckBox, "MainView.flagUseCustomScreenshotSize");
	ui->customScreenshotWidthLineEdit->setValidator(new MinMaxIntValidator(128, 16384, this));
	ui->customScreenshotHeightLineEdit->setValidator(new MinMaxIntValidator(128, 16384, this));
	connectIntProperty(ui->customScreenshotWidthLineEdit, "MainView.customScreenshotWidth");
	connectIntProperty(ui->customScreenshotHeightLineEdit, "MainView.customScreenshotHeight");
#endif
	connectBoolProperty(ui->autoEnableEnvironmentCheckBox, "LandscapeMgr.flagEnvironmentAutoEnabling");
	connectBoolProperty(ui->autoChangeLandscapesCheckBox, "LandscapeMgr.flagLandscapeAutoSelection");

	// script tab controls
	#ifdef ENABLE_SCRIPTING
	StelScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();
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
	#else
	ui->configurationStackedWidget->removeWidget(ui->page_Scripts);
	delete ui->stackListWidget->takeItem(5);
	#endif

	// plugins control
	connect(ui->pluginsListWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(pluginsSelectionChanged(QListWidgetItem*, QListWidgetItem*)));
	connect(ui->pluginLoadAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(loadAtStartupChanged(int)));
	connect(ui->pluginsListWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(pluginConfigureCurrentSelection()));
	connect(ui->pluginConfigureButton, SIGNAL(clicked()), this, SLOT(pluginConfigureCurrentSelection()));
	populatePluginsList();

	updateConfigLabels();
	updateTabBarListWidgetWidth();
}

void ConfigurationDialog::setKeyNavigationState(bool state)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	mvmgr->setFlagEnableMoveKeys(state);
	mvmgr->setFlagEnableZoomKeys(state);
	ui->editShortcutsPushButton->setEnabled(state);
}

void ConfigurationDialog::updateCurrentLanguage()
{
	QComboBox* cb = ui->programLanguageComboBox;
	QString appLang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	QString l2 = StelTranslator::iso639_1CodeToNativeName(appLang);

	if (cb->currentText() == l2)
		return;

	int lt = cb->findText(l2, Qt::MatchExactly);
	if (lt == -1 && appLang.contains('_'))
	{
		l2 = appLang.left(appLang.indexOf('_'));
		l2=StelTranslator::iso639_1CodeToNativeName(l2);
		lt = cb->findText(l2, Qt::MatchExactly);
	}
	if (lt!=-1)
		cb->setCurrentIndex(lt);
}

void ConfigurationDialog::updateCurrentSkyLanguage()
{
	QComboBox* cb = ui->skycultureLanguageComboBox;
	QString skyLang = StelApp::getInstance().getLocaleMgr().getSkyLanguage();
	QString l2 = StelTranslator::iso639_1CodeToNativeName(skyLang);

	if (cb->currentText() == l2)
		return;

	int lt = cb->findText(l2, Qt::MatchExactly);
	if (lt == -1 && skyLang.contains('_'))
	{
		l2 = skyLang.left(skyLang.indexOf('_'));
		l2=StelTranslator::iso639_1CodeToNativeName(l2);
		lt = cb->findText(l2, Qt::MatchExactly);
	}
	if (lt!=-1)
		cb->setCurrentIndex(lt);
}

void ConfigurationDialog::selectLanguage(const QString& langName)
{
	QString code = StelTranslator::nativeNameToIso639_1Code(langName);
	StelApp::getInstance().getLocaleMgr().setAppLanguage(code);
	StelMainView::getInstance().initTitleI18n();
}

void ConfigurationDialog::selectSkyLanguage(const QString& langName)
{
	QString code = StelTranslator::nativeNameToIso639_1Code(langName);
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(code);
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

void ConfigurationDialog::setButtonBarDTFormat()
{
	if (ui->jdRadioButton->isChecked())
		gui->getButtonBar()->setFlagTimeJd(true);
	else
		gui->getButtonBar()->setFlagTimeJd(false);
}

void ConfigurationDialog::showShortcutsWindow()
{
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction("actionShow_Shortcuts_Window_Global");
	if (action)
		action->setChecked(true);
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
		StelApp::getInstance().setViewportEffect("sphericMirrorDistorter");
	}
	else
	{
		core->setCurrentProjectionType(static_cast<StelCore::ProjectionType>(savedProjectionType));
		StelApp::getInstance().setViewportEffect("none");
	}
}

void ConfigurationDialog::setNoSelectedInfo(void)
{
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::None));
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
	// As this signal will be called when a checkbox is toggled,
	// change the general mode to Custom.
	if (!ui->customSelectedInfoRadio->isChecked())
		ui->customSelectedInfoRadio->setChecked(true);
	
	StelObject::InfoStringGroup flags(StelObject::None);

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
	if (ui->checkBoxVelocity->isChecked())
		flags |= StelObject::Velocity;
	if (ui->checkBoxProperMotion->isChecked())
		flags |= StelObject::ProperMotion;
	if (ui->checkBoxSize->isChecked())
		flags |= StelObject::Size;
	if (ui->checkBoxExtra->isChecked())
		flags |= StelObject::Extra;
	if (ui->checkBoxGalacticCoordinates->isChecked())
		flags |= StelObject::GalacticCoord;
	if (ui->checkBoxSupergalacticCoordinates->isChecked())
		flags |= StelObject::SupergalacticCoord;
	if (ui->checkBoxOtherCoords->isChecked())
		flags |= StelObject::OtherCoord;
	if (ui->checkBoxElongation->isChecked())
		flags |= StelObject::Elongation;
	if (ui->checkBoxType->isChecked())
		flags |= StelObject::ObjectType;
	if (ui->checkBoxEclipticCoordsJ2000->isChecked())
		flags |= StelObject::EclipticCoordJ2000;
	if (ui->checkBoxEclipticCoordsOfDate->isChecked())
		flags |= StelObject::EclipticCoordOfDate;
	if (ui->checkBoxConstellation->isChecked())
		flags |= StelObject::IAUConstellation;
	if (ui->checkBoxSiderealTime->isChecked())
		flags |= StelObject::SiderealTime;
	if (ui->checkBoxRTSTime->isChecked())
		flags |= StelObject::RTSTime;

	gui->setInfoTextFilters(flags);
	// overwrite custom selected info settings
	saveCustomSelectedInfo();
}

void ConfigurationDialog::setCustomSelectedInfo()
{
	StelObject::InfoStringGroup flags(StelObject::None);
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	if (conf->value("custom_selected_info/flag_show_name", false).toBool())
		flags |= StelObject::Name;
	if (conf->value("custom_selected_info/flag_show_catalognumber", false).toBool())
		flags |= StelObject::CatalogNumber;
	if (conf->value("custom_selected_info/flag_show_magnitude", false).toBool())
		flags |= StelObject::Magnitude;
	if (conf->value("custom_selected_info/flag_show_absolutemagnitude", false).toBool())
		flags |= StelObject::AbsoluteMagnitude;
	if (conf->value("custom_selected_info/flag_show_radecj2000", false).toBool())
		flags |= StelObject::RaDecJ2000;
	if (conf->value("custom_selected_info/flag_show_radecofdate", false).toBool())
		flags |= StelObject::RaDecOfDate;
	if (conf->value("custom_selected_info/flag_show_hourangle", false).toBool())
		flags |= StelObject::HourAngle;
	if (conf->value("custom_selected_info/flag_show_altaz", false).toBool())
		flags |= StelObject::AltAzi;
	if (conf->value("custom_selected_info/flag_show_elongation", false).toBool())
		flags |= StelObject::Elongation;
	if (conf->value("custom_selected_info/flag_show_distance", false).toBool())
		flags |= StelObject::Distance;
	if (conf->value("custom_selected_info/flag_show_velocity", false).toBool())
		flags |= StelObject::Velocity;
	if (conf->value("custom_selected_info/flag_show_propermotion", false).toBool())
		flags |= StelObject::ProperMotion;
	if (conf->value("custom_selected_info/flag_show_size", false).toBool())
		flags |= StelObject::Size;
	if (conf->value("custom_selected_info/flag_show_extra", false).toBool())
		flags |= StelObject::Extra;
	if (conf->value("custom_selected_info/flag_show_galcoord", false).toBool())
		flags |= StelObject::GalacticCoord;
	if (conf->value("custom_selected_info/flag_show_supergalcoord", false).toBool())
		flags |= StelObject::SupergalacticCoord;
	if (conf->value("custom_selected_info/flag_show_othercoord", false).toBool())
		flags |= StelObject::OtherCoord;
	if (conf->value("custom_selected_info/flag_show_type", false).toBool())
		flags |= StelObject::ObjectType;
	if (conf->value("custom_selected_info/flag_show_eclcoordofdate", false).toBool())
		flags |= StelObject::EclipticCoordOfDate;
	if (conf->value("custom_selected_info/flag_show_eclcoordj2000", false).toBool())
		flags |= StelObject::EclipticCoordJ2000;
	if (conf->value("custom_selected_info/flag_show_constellation", false).toBool())
		flags |= StelObject::IAUConstellation;
	if (conf->value("custom_selected_info/flag_show_sidereal_time", false).toBool())
		flags |= StelObject::SiderealTime;
	if (conf->value("custom_selected_info/flag_show_rts_time", false).toBool())
		flags |= StelObject::RTSTime;

	gui->setInfoTextFilters(flags);
	updateSelectedInfoCheckBoxes();
}

void ConfigurationDialog::saveCustomSelectedInfo()
{
	// configuration dialog / selected object info tab
	const StelObject::InfoStringGroup& flags = gui->getInfoTextFilters();
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	conf->beginGroup("custom_selected_info");
	conf->setValue("flag_show_name",				static_cast<bool>(flags & StelObject::Name));
	conf->setValue("flag_show_catalognumber",		static_cast<bool>(flags & StelObject::CatalogNumber));
	conf->setValue("flag_show_magnitude",			static_cast<bool>(flags & StelObject::Magnitude));
	conf->setValue("flag_show_absolutemagnitude",	static_cast<bool>(flags & StelObject::AbsoluteMagnitude));
	conf->setValue("flag_show_radecj2000",		static_cast<bool>(flags & StelObject::RaDecJ2000));
	conf->setValue("flag_show_radecofdate",		static_cast<bool>(flags & StelObject::RaDecOfDate));
	conf->setValue("flag_show_hourangle",			static_cast<bool>(flags & StelObject::HourAngle));
	conf->setValue("flag_show_altaz",				static_cast<bool>(flags & StelObject::AltAzi));
	conf->setValue("flag_show_elongation",			static_cast<bool>(flags & StelObject::Elongation));
	conf->setValue("flag_show_distance",			static_cast<bool>(flags & StelObject::Distance));
	conf->setValue("flag_show_velocity",			static_cast<bool>(flags & StelObject::Velocity));
	conf->setValue("flag_show_propermotion",		static_cast<bool>(flags & StelObject::ProperMotion));
	conf->setValue("flag_show_size",				static_cast<bool>(flags & StelObject::Size));
	conf->setValue("flag_show_extra",				static_cast<bool>(flags & StelObject::Extra));
	conf->setValue("flag_show_galcoord",			static_cast<bool>(flags & StelObject::GalacticCoord));
	conf->setValue("flag_show_supergalcoord",		static_cast<bool>(flags & StelObject::SupergalacticCoord));
	conf->setValue("flag_show_othercoord",			static_cast<bool>(flags & StelObject::OtherCoord));
	conf->setValue("flag_show_type",				static_cast<bool>(flags & StelObject::ObjectType));
	conf->setValue("flag_show_eclcoordofdate",		static_cast<bool>(flags & StelObject::EclipticCoordOfDate));
	conf->setValue("flag_show_eclcoordj2000",		static_cast<bool>(flags & StelObject::EclipticCoordJ2000));
	conf->setValue("flag_show_constellation",		static_cast<bool>(flags & StelObject::IAUConstellation));
	conf->setValue("flag_show_sidereal_time",		static_cast<bool>(flags & StelObject::SiderealTime));
	conf->setValue("flag_show_rts_time",			static_cast<bool>(flags & StelObject::RTSTime));
	conf->endGroup();
}

void ConfigurationDialog::browseForScreenshotDir()
{
	QString oldScreenshorDir = StelFileMgr::getScreenshotDir();
	QString newScreenshotDir = QFileDialog::getExistingDirectory(Q_NULLPTR, q_("Select screenshot directory"), oldScreenshorDir, QFileDialog::ShowDirsOnly);

	if (!newScreenshotDir.isEmpty()) {
		// remove trailing slash
		if (newScreenshotDir.right(1) == "/")
			newScreenshotDir = newScreenshotDir.left(newScreenshotDir.length()-1);

		ui->screenshotDirEdit->setText(newScreenshotDir);
		selectScreenshotDir();
	}
}

void ConfigurationDialog::selectScreenshotDir()
{
	QString dir = ui->screenshotDirEdit->text();
	try
	{
		StelFileMgr::setScreenshotDir(dir);
	}
	catch (std::runtime_error& e)
	{
		Q_UNUSED(e);
		// nop
		// this will happen when people are only half way through typing dirs
	}
}

// Store FOV and viewing dir.
void ConfigurationDialog::saveCurrentViewDirSettings()
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	mvmgr->setInitFov(mvmgr->getCurrentFov());
	mvmgr->setInitViewDirectionToCurrent();
}


// Save the current viewing options including sky culture
// This doesn't include the current viewing direction, landscape, time and FOV since those have specific controls
void ConfigurationDialog::saveAllSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// TBD: store more properties directly, avoid to query all modules.
	StelPropertyMgr* propMgr=StelApp::getInstance().getStelPropertyManager();
	Q_ASSERT(propMgr);

	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	Q_ASSERT(nmgr);
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	StelCore* core = StelApp::getInstance().getCore();
	const StelProjectorP proj = core->getProjection(StelCore::FrameJ2000);
	Q_ASSERT(proj);

	conf->setValue("gui/screen_font_size",					propMgr->getStelPropertyValue("StelApp.screenFontSize").toInt());
	conf->setValue("gui/flag_enable_kinetic_scrolling",		propMgr->getStelPropertyValue("StelGui.flagUseKineticScrolling").toBool());

	// view dialog / sky tab settings
	conf->setValue("stars/absolute_scale",					QString::number(propMgr->getStelPropertyValue("StelSkyDrawer.absoluteStarScale").toDouble(), 'f', 2));
	conf->setValue("stars/relative_scale",					QString::number(propMgr->getStelPropertyValue("StelSkyDrawer.relativeStarScale").toDouble(), 'f', 2));
	conf->setValue("stars/flag_star_twinkle",					propMgr->getStelPropertyValue("StelSkyDrawer.flagStarTwinkle").toBool());
	conf->setValue("stars/star_twinkle_amount",				QString::number(propMgr->getStelPropertyValue("StelSkyDrawer.twinkleAmount").toDouble(), 'f', 2));
	conf->setValue("stars/flag_star_spiky",					propMgr->getStelPropertyValue("StelSkyDrawer.flagStarSpiky").toBool());
	conf->setValue("astro/flag_star_magnitude_limit",		propMgr->getStelPropertyValue("StelSkyDrawer.flagStarMagnitudeLimit").toBool());
	conf->setValue("astro/star_magnitude_limit",			QString::number(propMgr->getStelPropertyValue("StelSkyDrawer.customStarMagLimit").toDouble(), 'f', 2));
	conf->setValue("astro/flag_planet_magnitude_limit",		propMgr->getStelPropertyValue("StelSkyDrawer.flagPlanetMagnitudeLimit").toBool());
	conf->setValue("astro/planet_magnitude_limit",			QString::number(propMgr->getStelPropertyValue("StelSkyDrawer.customPlanetMagLimit").toDouble(), 'f', 2));
	conf->setValue("astro/flag_nebula_magnitude_limit",		propMgr->getStelPropertyValue("StelSkyDrawer.flagNebulaMagnitudeLimit").toBool());
	conf->setValue("astro/nebula_magnitude_limit",			QString::number(propMgr->getStelPropertyValue("StelSkyDrawer.customNebulaMagLimit").toDouble(), 'f', 2));
	conf->setValue("viewing/use_luminance_adaptation",		propMgr->getStelPropertyValue("StelSkyDrawer.flagLuminanceAdaptation").toBool());
	conf->setValue("astro/flag_planets",					propMgr->getStelPropertyValue("SolarSystem.planetsDisplayed").toBool());
	conf->setValue("astro/flag_planets_hints",				propMgr->getStelPropertyValue("SolarSystem.flagHints").toBool());
	conf->setValue("astro/flag_planets_orbits",				propMgr->getStelPropertyValue("SolarSystem.flagOrbits").toBool());
	conf->setValue("astro/flag_permanent_orbits",			propMgr->getStelPropertyValue("SolarSystem.flagPermanentOrbits").toBool());
	conf->setValue("astro/object_orbits_thickness",			propMgr->getStelPropertyValue("SolarSystem.orbitsThickness").toInt());
	conf->setValue("astro/object_trails_thickness",			propMgr->getStelPropertyValue("SolarSystem.trailsThickness").toInt());
	conf->setValue("viewing/flag_isolated_trails",			propMgr->getStelPropertyValue("SolarSystem.flagIsolatedTrails").toBool());
	conf->setValue("viewing/number_isolated_trails",			propMgr->getStelPropertyValue("SolarSystem.numberIsolatedTrails").toInt());
	conf->setValue("viewing/max_trail_points",				propMgr->getStelPropertyValue("SolarSystem.maxTrailPoints").toInt());
	conf->setValue("viewing/flag_isolated_orbits",			propMgr->getStelPropertyValue("SolarSystem.flagIsolatedOrbits").toBool());
	conf->setValue("viewing/flag_planets_orbits_only",		propMgr->getStelPropertyValue("SolarSystem.flagPlanetsOrbitsOnly").toBool());
	conf->setValue("astro/flag_light_travel_time",			propMgr->getStelPropertyValue("SolarSystem.flagLightTravelTime").toBool());
	conf->setValue("viewing/flag_draw_moon_halo",			propMgr->getStelPropertyValue("SolarSystem.flagDrawMoonHalo").toBool());
	conf->setValue("viewing/flag_draw_sun_halo",			propMgr->getStelPropertyValue("SolarSystem.flagDrawSunHalo").toBool());
	conf->setValue("viewing/flag_moon_scaled",				propMgr->getStelPropertyValue("SolarSystem.flagMoonScale").toBool());
	conf->setValue("viewing/moon_scale",					QString::number(propMgr->getStelPropertyValue("SolarSystem.moonScale").toDouble(), 'f', 2));
	conf->setValue("viewing/flag_minorbodies_scaled",		propMgr->getStelPropertyValue("SolarSystem.flagMinorBodyScale").toBool());
	conf->setValue("viewing/minorbodies_scale",			QString::number(propMgr->getStelPropertyValue("SolarSystem.minorBodyScale").toDouble(), 'f', 2));
	conf->setValue("viewing/flag_planets_scaled",				propMgr->getStelPropertyValue("SolarSystem.flagPlanetScale").toBool());
	conf->setValue("viewing/planets_scale",					QString::number(propMgr->getStelPropertyValue("SolarSystem.planetScale").toDouble(), 'f', 2));
	conf->setValue("viewing/flag_sun_scaled",				propMgr->getStelPropertyValue("SolarSystem.flagSunScale").toBool());
	conf->setValue("viewing/sun_scale",					QString::number(propMgr->getStelPropertyValue("SolarSystem.sunScale").toDouble(), 'f', 2));
	conf->setValue("astro/meteor_zhr",						propMgr->getStelPropertyValue("SporadicMeteorMgr.zhr").toInt());
	conf->setValue("astro/flag_milky_way",					propMgr->getStelPropertyValue("MilkyWay.flagMilkyWayDisplayed").toBool());
	conf->setValue("astro/milky_way_intensity",				QString::number(propMgr->getStelPropertyValue("MilkyWay.intensity").toDouble(), 'f', 2));
	conf->setValue("astro/milky_way_saturation",			QString::number(propMgr->getStelPropertyValue("MilkyWay.saturation").toDouble(), 'f', 2));
	conf->setValue("astro/flag_zodiacal_light",				propMgr->getStelPropertyValue("ZodiacalLight.flagZodiacalLightDisplayed").toBool());
	conf->setValue("astro/zodiacal_light_intensity",			QString::number(propMgr->getStelPropertyValue("ZodiacalLight.intensity").toDouble(), 'f', 2));
	conf->setValue("astro/flag_grs_custom",					propMgr->getStelPropertyValue("SolarSystem.flagCustomGrsSettings").toBool());
	conf->setValue("astro/grs_longitude",					propMgr->getStelPropertyValue("SolarSystem.customGrsLongitude").toInt());
	conf->setValue("astro/grs_drift",						propMgr->getStelPropertyValue("SolarSystem.customGrsDrift").toDouble());
	conf->setValue("astro/grs_jd",							propMgr->getStelPropertyValue("SolarSystem.customGrsJD").toDouble());
	conf->setValue("astro/shadow_enlargement_danjon",			propMgr->getStelPropertyValue("SolarSystem.earthShadowEnlargementDanjon").toBool());
	conf->setValue("astro/flag_planets_labels",				propMgr->getStelPropertyValue("SolarSystem.labelsDisplayed").toBool());
	conf->setValue("astro/labels_amount",					propMgr->getStelPropertyValue("SolarSystem.labelsAmount").toDouble());
	conf->setValue("viewing/flag_planets_native_names",		propMgr->getStelPropertyValue("SolarSystem.flagNativePlanetNames").toBool());
	conf->setValue("astro/flag_use_obj_models",				propMgr->getStelPropertyValue("SolarSystem.flagUseObjModels").toBool());
	conf->setValue("astro/flag_show_obj_self_shadows",		propMgr->getStelPropertyValue("SolarSystem.flagShowObjSelfShadows").toBool());
	conf->setValue("astro/apparent_magnitude_algorithm",	Planet::getApparentMagnitudeAlgorithmString());
	conf->setValue("astro/flag_planets_nomenclature",		propMgr->getStelPropertyValue("NomenclatureMgr.nomenclatureDisplayed").toBool());
	conf->setValue("astro/flag_hide_local_nomenclature",	propMgr->getStelPropertyValue("NomenclatureMgr.localNomenclatureHided").toBool());

	// view dialog / markings tab settings
	conf->setValue("viewing/flag_gridlines",					propMgr->getStelPropertyValue("GridLinesMgr.gridlinesDisplayed").toBool());
	conf->setValue("viewing/flag_azimuthal_grid",			propMgr->getStelPropertyValue("GridLinesMgr.azimuthalGridDisplayed").toBool());
	conf->setValue("viewing/flag_equatorial_grid",			propMgr->getStelPropertyValue("GridLinesMgr.equatorGridDisplayed").toBool());
	conf->setValue("viewing/flag_equatorial_J2000_grid",		propMgr->getStelPropertyValue("GridLinesMgr.equatorJ2000GridDisplayed").toBool());
	conf->setValue("viewing/flag_equator_line",				propMgr->getStelPropertyValue("GridLinesMgr.equatorLineDisplayed").toBool());
	conf->setValue("viewing/flag_equator_parts",			propMgr->getStelPropertyValue("GridLinesMgr.equatorPartsDisplayed").toBool());
	conf->setValue("viewing/flag_equator_labels",			propMgr->getStelPropertyValue("GridLinesMgr.equatorPartsLabeled").toBool());
	conf->setValue("viewing/flag_equator_J2000_line",		propMgr->getStelPropertyValue("GridLinesMgr.equatorJ2000LineDisplayed").toBool());
	conf->setValue("viewing/flag_equator_J2000_parts",		propMgr->getStelPropertyValue("GridLinesMgr.equatorJ2000PartsDisplayed").toBool());
	conf->setValue("viewing/flag_equator_J2000_labels",		propMgr->getStelPropertyValue("GridLinesMgr.equatorJ2000PartsLabeled").toBool());
	conf->setValue("viewing/flag_ecliptic_line",				propMgr->getStelPropertyValue("GridLinesMgr.eclipticLineDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_parts",				propMgr->getStelPropertyValue("GridLinesMgr.eclipticPartsDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_labels",			propMgr->getStelPropertyValue("GridLinesMgr.eclipticPartsLabeled").toBool());
	conf->setValue("viewing/flag_ecliptic_J2000_line",		propMgr->getStelPropertyValue("GridLinesMgr.eclipticJ2000LineDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_J2000_parts",		propMgr->getStelPropertyValue("GridLinesMgr.eclipticJ2000PartsDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_J2000_labels",		propMgr->getStelPropertyValue("GridLinesMgr.eclipticJ2000PartsLabeled").toBool());
	conf->setValue("viewing/flag_invariable_plane_line",		propMgr->getStelPropertyValue("GridLinesMgr.invariablePlaneLineDisplayed").toBool());
	conf->setValue("viewing/flag_solar_equator_line",		propMgr->getStelPropertyValue("GridLinesMgr.solarEquatorLineDisplayed").toBool());
	conf->setValue("viewing/flag_solar_equator_parts",		propMgr->getStelPropertyValue("GridLinesMgr.solarEquatorPartsDisplayed").toBool());
	conf->setValue("viewing/flag_solar_equator_labels",		propMgr->getStelPropertyValue("GridLinesMgr.solarEquatorPartsLabeled").toBool());
	conf->setValue("viewing/flag_ecliptic_grid",				propMgr->getStelPropertyValue("GridLinesMgr.eclipticGridDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_J2000_grid",		propMgr->getStelPropertyValue("GridLinesMgr.eclipticJ2000GridDisplayed").toBool());
	conf->setValue("viewing/flag_meridian_line",			propMgr->getStelPropertyValue("GridLinesMgr.meridianLineDisplayed").toBool());
	conf->setValue("viewing/flag_meridian_parts",			propMgr->getStelPropertyValue("GridLinesMgr.meridianPartsDisplayed").toBool());
	conf->setValue("viewing/flag_meridian_labels",			propMgr->getStelPropertyValue("GridLinesMgr.meridianPartsLabeled").toBool());
	conf->setValue("viewing/flag_longitude_line",			propMgr->getStelPropertyValue("GridLinesMgr.longitudeLineDisplayed").toBool());
	conf->setValue("viewing/flag_longitude_parts",			propMgr->getStelPropertyValue("GridLinesMgr.longitudePartsDisplayed").toBool());
	conf->setValue("viewing/flag_longitude_labels",			propMgr->getStelPropertyValue("GridLinesMgr.longitudePartsLabeled").toBool());
	conf->setValue("viewing/flag_horizon_line",				propMgr->getStelPropertyValue("GridLinesMgr.horizonLineDisplayed").toBool());
	conf->setValue("viewing/flag_horizon_parts",				propMgr->getStelPropertyValue("GridLinesMgr.horizonPartsDisplayed").toBool());
	conf->setValue("viewing/flag_horizon_labels",			propMgr->getStelPropertyValue("GridLinesMgr.horizonPartsLabeled").toBool());
	conf->setValue("viewing/flag_galactic_grid",				propMgr->getStelPropertyValue("GridLinesMgr.galacticGridDisplayed").toBool());
	conf->setValue("viewing/flag_galactic_equator_line",		propMgr->getStelPropertyValue("GridLinesMgr.galacticEquatorLineDisplayed").toBool());
	conf->setValue("viewing/flag_galactic_equator_parts",	propMgr->getStelPropertyValue("GridLinesMgr.galacticEquatorPartsDisplayed").toBool());
	conf->setValue("viewing/flag_galactic_equator_labels",	propMgr->getStelPropertyValue("GridLinesMgr.galacticEquatorPartsLabeled").toBool());
	conf->setValue("viewing/flag_cardinal_points",			propMgr->getStelPropertyValue("LandscapeMgr.cardinalsPointsDisplayed").toBool());
	conf->setValue("viewing/flag_compass_marks",			propMgr->getStelPropertyValue("SpecialMarkersMgr.compassMarksDisplayed").toBool());
	conf->setValue("viewing/flag_prime_vertical_line",		propMgr->getStelPropertyValue("GridLinesMgr.primeVerticalLineDisplayed").toBool());
	conf->setValue("viewing/flag_prime_vertical_parts",		propMgr->getStelPropertyValue("GridLinesMgr.primeVerticalPartsDisplayed").toBool());
	conf->setValue("viewing/flag_prime_vertical_labels",		propMgr->getStelPropertyValue("GridLinesMgr.primeVerticalPartsLabeled").toBool());
	conf->setValue("viewing/flag_current_vertical_line",		propMgr->getStelPropertyValue("GridLinesMgr.currentVerticalLineDisplayed").toBool());
	conf->setValue("viewing/flag_current_vertical_parts",		propMgr->getStelPropertyValue("GridLinesMgr.currentVerticalPartsDisplayed").toBool());
	conf->setValue("viewing/flag_current_vertical_labels",	propMgr->getStelPropertyValue("GridLinesMgr.currentVerticalPartsLabeled").toBool());
	conf->setValue("viewing/flag_colure_lines",				propMgr->getStelPropertyValue("GridLinesMgr.colureLinesDisplayed").toBool());
	conf->setValue("viewing/flag_colure_parts",				propMgr->getStelPropertyValue("GridLinesMgr.colurePartsDisplayed").toBool());
	conf->setValue("viewing/flag_colure_labels",				propMgr->getStelPropertyValue("GridLinesMgr.colurePartsLabeled").toBool());
	conf->setValue("viewing/flag_precession_circles",		propMgr->getStelPropertyValue("GridLinesMgr.precessionCirclesDisplayed").toBool());
	conf->setValue("viewing/flag_precession_parts",			propMgr->getStelPropertyValue("GridLinesMgr.precessionPartsDisplayed").toBool());
	conf->setValue("viewing/flag_precession_labels",			propMgr->getStelPropertyValue("GridLinesMgr.precessionPartsLabeled").toBool());
	conf->setValue("viewing/flag_circumpolar_circles",		propMgr->getStelPropertyValue("GridLinesMgr.circumpolarCirclesDisplayed").toBool());
	conf->setValue("viewing/flag_umbra_circle",			propMgr->getStelPropertyValue("GridLinesMgr.umbraCircleDisplayed").toBool());
	conf->setValue("viewing/flag_penumbra_circle",			propMgr->getStelPropertyValue("GridLinesMgr.penumbraCircleDisplayed").toBool());
	conf->setValue("viewing/flag_supergalactic_grid",		propMgr->getStelPropertyValue("GridLinesMgr.supergalacticGridDisplayed").toBool());
	conf->setValue("viewing/flag_supergalactic_equator_line",	propMgr->getStelPropertyValue("GridLinesMgr.supergalacticEquatorLineDisplayed").toBool());
	conf->setValue("viewing/flag_supergalactic_equator_parts",	propMgr->getStelPropertyValue("GridLinesMgr.supergalacticEquatorPartsDisplayed").toBool());
	conf->setValue("viewing/flag_supergalactic_equator_labels",	propMgr->getStelPropertyValue("GridLinesMgr.supergalacticEquatorPartsLabeled").toBool());
	conf->setValue("viewing/flag_celestial_J2000_poles",		propMgr->getStelPropertyValue("GridLinesMgr.celestialJ2000PolesDisplayed").toBool());
	conf->setValue("viewing/flag_celestial_poles",			propMgr->getStelPropertyValue("GridLinesMgr.celestialPolesDisplayed").toBool());
	conf->setValue("viewing/flag_zenith_nadir",				propMgr->getStelPropertyValue("GridLinesMgr.zenithNadirDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_J2000_poles",		propMgr->getStelPropertyValue("GridLinesMgr.eclipticJ2000PolesDisplayed").toBool());
	conf->setValue("viewing/flag_ecliptic_poles",				propMgr->getStelPropertyValue("GridLinesMgr.eclipticPolesDisplayed").toBool());
	conf->setValue("viewing/flag_galactic_poles",			propMgr->getStelPropertyValue("GridLinesMgr.galacticPolesDisplayed").toBool());
	conf->setValue("viewing/flag_galactic_center",			propMgr->getStelPropertyValue("GridLinesMgr.galacticCenterDisplayed").toBool());
	conf->setValue("viewing/flag_supergalactic_poles",		propMgr->getStelPropertyValue("GridLinesMgr.supergalacticPolesDisplayed").toBool());
	conf->setValue("viewing/flag_equinox_J2000_points",		propMgr->getStelPropertyValue("GridLinesMgr.equinoxJ2000PointsDisplayed").toBool());
	conf->setValue("viewing/flag_equinox_points",			propMgr->getStelPropertyValue("GridLinesMgr.equinoxPointsDisplayed").toBool());
	conf->setValue("viewing/flag_solstice_J2000_points",		propMgr->getStelPropertyValue("GridLinesMgr.solsticeJ2000PointsDisplayed").toBool());
	conf->setValue("viewing/flag_solstice_points",			propMgr->getStelPropertyValue("GridLinesMgr.solsticePointsDisplayed").toBool());
	conf->setValue("viewing/flag_antisolar_point",			propMgr->getStelPropertyValue("GridLinesMgr.antisolarPointDisplayed").toBool());
	conf->setValue("viewing/flag_apex_points",				propMgr->getStelPropertyValue("GridLinesMgr.apexPointsDisplayed").toBool());
	conf->setValue("viewing/flag_fov_center_marker",		propMgr->getStelPropertyValue("SpecialMarkersMgr.fovCenterMarkerDisplayed").toBool());
	conf->setValue("viewing/flag_fov_circular_marker",		propMgr->getStelPropertyValue("SpecialMarkersMgr.fovCircularMarkerDisplayed").toBool());
	conf->setValue("viewing/size_fov_circular_marker",		QString::number(propMgr->getStelPropertyValue("SpecialMarkersMgr.fovCircularMarkerSize").toDouble(), 'f', 2));
	conf->setValue("viewing/flag_fov_rectangular_marker",	propMgr->getStelPropertyValue("SpecialMarkersMgr.fovRectangularMarkerDisplayed").toBool());
	conf->setValue("viewing/width_fov_rectangular_marker",	QString::number(propMgr->getStelPropertyValue("SpecialMarkersMgr.fovRectangularMarkerWidth").toDouble(), 'f', 2));
	conf->setValue("viewing/height_fov_rectangular_marker",	QString::number(propMgr->getStelPropertyValue("SpecialMarkersMgr.fovRectangularMarkerHeight").toDouble(), 'f', 2));
	conf->setValue("viewing/rot_fov_rectangular_marker",	QString::number(propMgr->getStelPropertyValue("SpecialMarkersMgr.fovRectangularMarkerRotationAngle").toDouble(), 'f', 2));
	conf->setValue("viewing/line_thickness",				propMgr->getStelPropertyValue("GridLinesMgr.lineThickness").toInt());
	conf->setValue("viewing/part_thickness",				propMgr->getStelPropertyValue("GridLinesMgr.partThickness").toInt());

	conf->setValue("viewing/constellation_font_size",			propMgr->getStelPropertyValue("ConstellationMgr.fontSize").toInt());
	conf->setValue("viewing/flag_constellation_drawing",		propMgr->getStelPropertyValue("ConstellationMgr.linesDisplayed").toBool());
	conf->setValue("viewing/flag_constellation_name",		propMgr->getStelPropertyValue("ConstellationMgr.namesDisplayed").toBool());
	conf->setValue("viewing/flag_constellation_boundaries",	propMgr->getStelPropertyValue("ConstellationMgr.boundariesDisplayed").toBool());
	conf->setValue("viewing/flag_constellation_art",			propMgr->getStelPropertyValue("ConstellationMgr.artDisplayed").toBool());
	conf->setValue("viewing/flag_constellation_isolate_selected",	propMgr->getStelPropertyValue("ConstellationMgr.isolateSelected").toBool());
	conf->setValue("viewing/flag_landscape_autoselection",	propMgr->getStelPropertyValue("LandscapeMgr.flagLandscapeAutoSelection").toBool());
	conf->setValue("viewing/flag_light_pollution_database",	propMgr->getStelPropertyValue("LandscapeMgr.flagUseLightPollutionFromDatabase").toBool());
	conf->setValue("viewing/flag_environment_auto_enable",	propMgr->getStelPropertyValue("LandscapeMgr.flagEnvironmentAutoEnabling").toBool());
	conf->setValue("viewing/constellation_art_intensity",		propMgr->getStelPropertyValue("ConstellationMgr.artIntensity").toFloat());
	conf->setValue("viewing/constellation_name_style",		ConstellationMgr::getConstellationDisplayStyleString(static_cast<ConstellationMgr::ConstellationDisplayStyle> (propMgr->getStelPropertyValue("ConstellationMgr.constellationDisplayStyle").toInt())  ));
	conf->setValue("viewing/constellation_line_thickness",	propMgr->getStelPropertyValue("ConstellationMgr.constellationLineThickness").toInt());
	conf->setValue("viewing/constellation_boundaries_thickness",	propMgr->getStelPropertyValue("ConstellationMgr.constellationBoundariesThickness").toInt());

	conf->setValue("viewing/asterism_font_size",				propMgr->getStelPropertyValue("AsterismMgr.fontSize").toInt());
	conf->setValue("viewing/flag_asterism_drawing",			propMgr->getStelPropertyValue("AsterismMgr.linesDisplayed").toBool());
	conf->setValue("viewing/flag_asterism_name",			propMgr->getStelPropertyValue("AsterismMgr.namesDisplayed").toBool());
	conf->setValue("viewing/asterism_line_thickness",		propMgr->getStelPropertyValue("AsterismMgr.asterismLineThickness").toInt());
	conf->setValue("viewing/flag_rayhelper_drawing",		propMgr->getStelPropertyValue("AsterismMgr.rayHelpersDisplayed").toBool());
	conf->setValue("viewing/rayhelper_line_thickness",		propMgr->getStelPropertyValue("AsterismMgr.rayHelperThickness").toInt());
	conf->setValue("viewing/sky_brightness_label_threshold",	propMgr->getStelPropertyValue("StelSkyDrawer.daylightLabelThreshold").toFloat());
	conf->setValue("viewing/flag_night",					StelApp::getInstance().getVisionModeNight());
	conf->setValue("astro/flag_stars",						propMgr->getStelPropertyValue("StarMgr.flagStarsDisplayed").toBool());
	conf->setValue("astro/flag_star_name",					propMgr->getStelPropertyValue("StarMgr.flagLabelsDisplayed").toBool());
	conf->setValue("astro/flag_star_additional_names",		propMgr->getStelPropertyValue("StarMgr.flagAdditionalNamesDisplayed").toBool());
	conf->setValue("astro/flag_star_designation_usage",		propMgr->getStelPropertyValue("StarMgr.flagDesignationLabels").toBool());
	conf->setValue("stars/labels_amount",					propMgr->getStelPropertyValue("StarMgr.labelsAmount").toDouble());
	conf->setValue("astro/nebula_hints_amount",			propMgr->getStelPropertyValue("NebulaMgr.hintsAmount").toDouble());
	conf->setValue("astro/nebula_labels_amount",			propMgr->getStelPropertyValue("NebulaMgr.labelsAmount").toDouble());
	conf->setValue("astro/flag_nebula_hints_proportional",	propMgr->getStelPropertyValue("NebulaMgr.hintsProportional").toBool());
	conf->setValue("astro/flag_surface_brightness_usage",	propMgr->getStelPropertyValue("NebulaMgr.flagSurfaceBrightnessUsage").toBool());
	conf->setValue("gui/flag_surface_brightness_arcsec",		propMgr->getStelPropertyValue("NebulaMgr.flagSurfaceBrightnessArcsecUsage").toBool());
	conf->setValue("gui/flag_surface_brightness_short",		propMgr->getStelPropertyValue("NebulaMgr.flagSurfaceBrightnessShortNotationUsage").toBool());
	conf->setValue("astro/flag_dso_designation_usage",		propMgr->getStelPropertyValue("NebulaMgr.flagDesignationLabels").toBool());
	conf->setValue("astro/flag_dso_outlines_usage",			propMgr->getStelPropertyValue("NebulaMgr.flagOutlinesDisplayed").toBool());
	conf->setValue("astro/flag_dso_additional_names",		propMgr->getStelPropertyValue("NebulaMgr.flagAdditionalNamesDisplayed").toBool());
	conf->setValue("astro/flag_nebula_name",				propMgr->getStelPropertyValue("NebulaMgr.flagHintDisplayed").toBool());
	conf->setValue("astro/flag_use_type_filter",				propMgr->getStelPropertyValue("NebulaMgr.flagTypeFiltersUsage").toBool());
	conf->setValue("astro/flag_nebula_display_no_texture",	!propMgr->getStelPropertyValue("StelSkyLayerMgr.flagShow").toBool() );

	conf->setValue("astro/flag_size_limits_usage",			propMgr->getStelPropertyValue("NebulaMgr.flagUseSizeLimits").toBool());
	conf->setValue("astro/size_limit_min",				QString::number(propMgr->getStelPropertyValue("NebulaMgr.minSizeLimit").toDouble(), 'f', 0));
	conf->setValue("astro/size_limit_max",				QString::number(propMgr->getStelPropertyValue("NebulaMgr.maxSizeLimit").toDouble(), 'f', 0));

	conf->setValue("projection/type",				core->getCurrentProjectionTypeKey());
	conf->setValue("astro/flag_nutation",				core->getUseNutation());
	conf->setValue("astro/flag_topocentric_coordinates",		core->getUseTopocentricCoordinates());

	// view dialog / DSO tag settings
	const Nebula::CatalogGroup& cflags = nmgr->getCatalogFilters();

	conf->beginGroup("dso_catalog_filters");
	conf->setValue("flag_show_ngc",	static_cast<bool>(cflags & Nebula::CatNGC));
	conf->setValue("flag_show_ic",		static_cast<bool>(cflags & Nebula::CatIC));
	conf->setValue("flag_show_m",		static_cast<bool>(cflags & Nebula::CatM));
	conf->setValue("flag_show_c",		static_cast<bool>(cflags & Nebula::CatC));
	conf->setValue("flag_show_b",		static_cast<bool>(cflags & Nebula::CatB));
	conf->setValue("flag_show_vdb",	static_cast<bool>(cflags & Nebula::CatVdB));
	conf->setValue("flag_show_sh2",	static_cast<bool>(cflags & Nebula::CatSh2));
	conf->setValue("flag_show_rcw",	static_cast<bool>(cflags & Nebula::CatRCW));
	conf->setValue("flag_show_lbn",	static_cast<bool>(cflags & Nebula::CatLBN));
	conf->setValue("flag_show_ldn",	static_cast<bool>(cflags & Nebula::CatLDN));
	conf->setValue("flag_show_cr",		static_cast<bool>(cflags & Nebula::CatCr));
	conf->setValue("flag_show_mel",	static_cast<bool>(cflags & Nebula::CatMel));
	conf->setValue("flag_show_ced",	static_cast<bool>(cflags & Nebula::CatCed));
	conf->setValue("flag_show_pgc",	static_cast<bool>(cflags & Nebula::CatPGC));
	conf->setValue("flag_show_ugc",	static_cast<bool>(cflags & Nebula::CatUGC));
	conf->setValue("flag_show_arp",	static_cast<bool>(cflags & Nebula::CatArp));
	conf->setValue("flag_show_vv",	static_cast<bool>(cflags & Nebula::CatVV));
	conf->setValue("flag_show_pk",	static_cast<bool>(cflags & Nebula::CatPK));
	conf->setValue("flag_show_png",	static_cast<bool>(cflags & Nebula::CatPNG));
	conf->setValue("flag_show_snrg",	static_cast<bool>(cflags & Nebula::CatSNRG));
	conf->setValue("flag_show_aco",	static_cast<bool>(cflags & Nebula::CatACO));
	conf->setValue("flag_show_hcg",	static_cast<bool>(cflags & Nebula::CatHCG));
	conf->setValue("flag_show_eso",	static_cast<bool>(cflags & Nebula::CatESO));
	conf->setValue("flag_show_vdbh",	static_cast<bool>(cflags & Nebula::CatVdBH));
	conf->setValue("flag_show_dwb",	static_cast<bool>(cflags & Nebula::CatDWB));
	conf->setValue("flag_show_tr",		static_cast<bool>(cflags & Nebula::CatTr));
	conf->setValue("flag_show_st",		static_cast<bool>(cflags & Nebula::CatSt));
	conf->setValue("flag_show_ru",		static_cast<bool>(cflags & Nebula::CatRu));
	conf->setValue("flag_show_vdbha",	static_cast<bool>(cflags & Nebula::CatVdBHa));
	conf->setValue("flag_show_other",	static_cast<bool>(cflags & Nebula::CatOther));
	conf->endGroup();

	const Nebula::TypeGroup& tflags = nmgr->getTypeFilters();
	conf->beginGroup("dso_type_filters");
	conf->setValue("flag_show_galaxies",			static_cast<bool>(tflags & Nebula::TypeGalaxies));
	conf->setValue("flag_show_active_galaxies",		static_cast<bool>(tflags & Nebula::TypeActiveGalaxies));
	conf->setValue("flag_show_interacting_galaxies",	static_cast<bool>(tflags & Nebula::TypeInteractingGalaxies));
	conf->setValue("flag_show_open_clusters",		static_cast<bool>(tflags & Nebula::TypeOpenStarClusters));
	conf->setValue("flag_show_globular_clusters",		static_cast<bool>(tflags & Nebula::TypeGlobularStarClusters));
	conf->setValue("flag_show_bright_nebulae",		static_cast<bool>(tflags & Nebula::TypeBrightNebulae));
	conf->setValue("flag_show_dark_nebulae",		static_cast<bool>(tflags & Nebula::TypeDarkNebulae));
	conf->setValue("flag_show_planetary_nebulae",	static_cast<bool>(tflags & Nebula::TypePlanetaryNebulae));
	conf->setValue("flag_show_hydrogen_regions",	static_cast<bool>(tflags & Nebula::TypeHydrogenRegions));
	conf->setValue("flag_show_supernova_remnants",	static_cast<bool>(tflags & Nebula::TypeSupernovaRemnants));
	conf->setValue("flag_show_galaxy_clusters",		static_cast<bool>(tflags & Nebula::TypeGalaxyClusters));
	conf->setValue("flag_show_other",				static_cast<bool>(tflags & Nebula::TypeOther));
	conf->endGroup();

	// view dialog / landscape tab settings
	// DO NOT SAVE CURRENT LANDSCAPE ID! There is a dedicated button in the landscape tab of the View dialog.
	//conf->setValue("init_location/landscape_name",                     propMgr->getStelPropertyValue("LandscapeMgr.currentLandscapeID").toString());
	conf->setValue("landscape/flag_landscape_sets_location",	propMgr->getStelPropertyValue("LandscapeMgr.flagLandscapeSetsLocation").toBool());
	conf->setValue("landscape/flag_landscape",				propMgr->getStelPropertyValue("LandscapeMgr.landscapeDisplayed").toBool());
	conf->setValue("landscape/flag_atmosphere",				propMgr->getStelPropertyValue("LandscapeMgr.atmosphereDisplayed").toBool());
	conf->setValue("landscape/flag_fog",					propMgr->getStelPropertyValue("LandscapeMgr.fogDisplayed").toBool());
	conf->setValue("landscape/flag_enable_illumination_layer",	propMgr->getStelPropertyValue("LandscapeMgr.illuminationDisplayed").toBool());
	conf->setValue("landscape/flag_enable_labels",			propMgr->getStelPropertyValue("LandscapeMgr.labelsDisplayed").toBool());
	conf->setValue("landscape/flag_minimal_brightness",		propMgr->getStelPropertyValue("LandscapeMgr.flagLandscapeUseMinimalBrightness").toBool());
	conf->setValue("landscape/flag_landscape_sets_minimal_brightness", propMgr->getStelPropertyValue("LandscapeMgr.flagLandscapeSetsMinimalBrightness").toBool());
	conf->setValue("landscape/minimal_brightness",			propMgr->getStelPropertyValue("LandscapeMgr.defaultMinimalBrightness").toFloat());
	conf->setValue("stars/init_bortle_scale",					propMgr->getStelPropertyValue("StelSkyDrawer.bortleScaleIndex").toInt());
	conf->setValue("landscape/atmospheric_extinction_coefficient",	propMgr->getStelPropertyValue("StelSkyDrawer.extinctionCoefficient").toFloat());
	conf->setValue("landscape/pressure_mbar",				propMgr->getStelPropertyValue("StelSkyDrawer.atmospherePressure").toFloat());
	conf->setValue("landscape/temperature_C",				propMgr->getStelPropertyValue("StelSkyDrawer.atmosphereTemperature").toFloat());

	// view dialog / starlore tab
	QObject* scmgr = reinterpret_cast<QObject*>(&StelApp::getInstance().getSkyCultureMgr());
	scmgr->setProperty("defaultSkyCultureID", scmgr->property("currentSkyCultureID"));

	// Save default location
	StelApp::getInstance().getCore()->setDefaultLocationID(core->getCurrentLocation().getID());

	// configuration dialog / main tab
	QString langName = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	conf->setValue("localization/app_locale", StelTranslator::nativeNameToIso639_1Code(langName));
	langName = StelApp::getInstance().getLocaleMgr().getSkyLanguage();
	conf->setValue("localization/sky_locale", StelTranslator::nativeNameToIso639_1Code(langName));

	// configuration dialog / selected object info tab
	const StelObject::InfoStringGroup& flags = gui->getInfoTextFilters();
	if (flags == StelObject::InfoStringGroup(Q_NULLPTR))
		conf->setValue("gui/selected_object_info", "none");
	else if (flags == StelObject::InfoStringGroup(StelObject::ShortInfo))
		conf->setValue("gui/selected_object_info", "short");
	else if (flags == StelObject::InfoStringGroup(StelObject::AllInfo))
		conf->setValue("gui/selected_object_info", "all");
	else
	{
		conf->setValue("gui/selected_object_info", "custom");
		saveCustomSelectedInfo();
	}

	// toolbar auto-hide status
	conf->setValue("gui/auto_hide_horizontal_toolbar",		propMgr->getStelPropertyValue("StelGui.autoHideHorizontalButtonBar").toBool());
	conf->setValue("gui/auto_hide_vertical_toolbar",			propMgr->getStelPropertyValue("StelGui.autoHideVerticalButtonBar").toBool());
	conf->setValue("gui/flag_show_quit_button",				propMgr->getStelPropertyValue("StelGui.flagShowQuitButton").toBool());
	conf->setValue("gui/flag_show_nebulae_background_button",propMgr->getStelPropertyValue("StelGui.flagShowNebulaBackgroundButton").toBool());
	conf->setValue("gui/flag_show_dss_button",				propMgr->getStelPropertyValue("StelGui.flagShowDSSButton").toBool());
	conf->setValue("gui/flag_show_hips_button",				propMgr->getStelPropertyValue("StelGui.flagShowHiPSButton").toBool());
	conf->setValue("gui/flag_show_goto_selected_button",	propMgr->getStelPropertyValue("StelGui.flagShowGotoSelectedObjectButton").toBool());
	conf->setValue("gui/flag_show_nightmode_button",		propMgr->getStelPropertyValue("StelGui.flagShowNightmodeButton").toBool());
	conf->setValue("gui/flag_show_fullscreen_button",		propMgr->getStelPropertyValue("StelGui.flagShowFullscreenButton").toBool());
	conf->setValue("gui/flag_show_bookmarks_button",		propMgr->getStelPropertyValue("StelGui.flagShowBookmarksButton").toBool());
	conf->setValue("gui/flag_show_icrs_grid_button",			propMgr->getStelPropertyValue("StelGui.flagShowICRSGridButton").toBool());
	conf->setValue("gui/flag_show_galactic_grid_button",		propMgr->getStelPropertyValue("StelGui.flagShowGalacticGridButton").toBool());
	conf->setValue("gui/flag_show_ecliptic_grid_button",		propMgr->getStelPropertyValue("StelGui.flagShowEclipticGridButton").toBool());
	conf->setValue("gui/flag_show_boundaries_button",		propMgr->getStelPropertyValue("StelGui.flagShowConstellationBoundariesButton").toBool());
	conf->setValue("gui/flag_show_asterism_lines_button",	propMgr->getStelPropertyValue("StelGui.flagShowAsterismLinesButton").toBool());
	conf->setValue("gui/flag_show_asterism_labels_button",	propMgr->getStelPropertyValue("StelGui.flagShowAsterismLabelsButton").toBool());
	conf->setValue("gui/flag_show_decimal_degrees",		propMgr->getStelPropertyValue("StelApp.flagShowDecimalDegrees").toBool());
	conf->setValue("gui/flag_use_azimuth_from_south",		propMgr->getStelPropertyValue("StelApp.flagUseAzimuthFromSouth").toBool());
	conf->setValue("gui/flag_use_formatting_output",			propMgr->getStelPropertyValue("StelApp.flagUseFormattingOutput").toBool());
	conf->setValue("gui/flag_use_ccs_designations",			propMgr->getStelPropertyValue("StelApp.flagUseCCSDesignation").toBool());
	conf->setValue("gui/flag_overwrite_info_color",			propMgr->getStelPropertyValue("StelApp.flagOverwriteInfoColor").toBool());
	conf->setValue("gui/flag_time_jd",						gui->getButtonBar()->getFlagTimeJd());
	conf->setValue("gui/flag_show_buttons_background",		propMgr->getStelPropertyValue("StelGui.flagUseButtonsBackground").toBool());
	conf->setValue("gui/flag_indication_mount_mode",		mvmgr->getFlagIndicationMountMode());

	// configuration dialog / navigation tab
	conf->setValue("navigation/flag_enable_zoom_keys",		mvmgr->getFlagEnableZoomKeys());
	conf->setValue("navigation/flag_enable_mouse_navigation",	mvmgr->getFlagEnableMouseNavigation());
	conf->setValue("navigation/flag_enable_move_keys",		mvmgr->getFlagEnableMoveKeys());
	conf->setValue("navigation/startup_time_mode",			core->getStartupTimeMode());
	conf->setValue("navigation/today_time",				core->getInitTodayTime());
	conf->setValue("navigation/preset_sky_time",			core->getPresetSkyTime());
	conf->setValue("navigation/time_correction_algorithm",	core->getCurrentDeltaTAlgorithmKey());
	if (mvmgr->getMountMode() == StelMovementMgr::MountAltAzimuthal)
		conf->setValue("navigation/viewing_mode", "horizon");
	else
		conf->setValue("navigation/viewing_mode", "equator");

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();
	conf->setValue("localization/time_display_format",	localeManager.getTimeFormatStr());
	conf->setValue("localization/date_display_format",	localeManager.getDateFormatStr());

	// configuration dialog / tools tab
	conf->setValue("gui/flag_show_flip_buttons",			propMgr->getStelPropertyValue("StelGui.flagShowFlipButtons").toBool());
	conf->setValue("video/viewport_effect",				StelApp::getInstance().getViewportEffect());
	conf->setValue("projection/viewport",				StelProjector::maskTypeToString(proj->getMaskType()));
	conf->setValue("projection/viewport_center_offset_x",	core->getCurrentStelProjectorParams().viewportCenterOffset[0]);
	conf->setValue("projection/viewport_center_offset_y",	core->getCurrentStelProjectorParams().viewportCenterOffset[1]);
	conf->setValue("projection/flip_horz",				core->getCurrentStelProjectorParams().flipHorz);
	conf->setValue("projection/flip_vert",				core->getCurrentStelProjectorParams().flipVert);
	conf->setValue("viewing/flag_gravity_labels",		proj->getFlagGravityLabels());
	conf->setValue("navigation/auto_zoom_out_resets_direction",	mvmgr->getFlagAutoZoomOutResetsDirection());
	conf->setValue("gui/flag_mouse_cursor_timeout",	propMgr->getStelPropertyValue("MainView.flagCursorTimeout").toBool());
	conf->setValue("gui/mouse_cursor_timeout",			propMgr->getStelPropertyValue("MainView.cursorTimeout").toFloat());
	conf->setValue("gui/base_font_name",				QGuiApplication::font().family());
	conf->setValue("gui/screen_font_size",				propMgr->getStelPropertyValue("StelApp.screenFontSize").toInt());
	conf->setValue("gui/gui_font_size",					propMgr->getStelPropertyValue("StelApp.guiFontSize").toInt());


	conf->setValue("main/screenshot_dir",				StelFileMgr::getScreenshotDir());
	conf->setValue("main/invert_screenshots_colors",	propMgr->getStelPropertyValue("MainView.flagInvertScreenShotColors").toBool());
	conf->setValue("main/screenshot_custom_size",		propMgr->getStelPropertyValue("MainView.flagUseCustomScreenshotSize").toBool());
	conf->setValue("main/screenshot_custom_width",	propMgr->getStelPropertyValue("MainView.customScreenshotWidth").toInt());
	conf->setValue("main/screenshot_custom_height",	propMgr->getStelPropertyValue("MainView.customScreenshotHeight").toInt());

	int screenNum = qApp->desktop()->screenNumber(&StelMainView::getInstance());
	conf->setValue("video/screen_number", screenNum);

	// full screen and window size
	conf->setValue("video/fullscreen", StelMainView::getInstance().isFullScreen());
	if (!StelMainView::getInstance().isFullScreen())
	{
		QRect screenGeom = QGuiApplication::screens().at(screenNum)->geometry();

		QWidget& mainWindow = StelMainView::getInstance();
		conf->setValue("video/screen_w", mainWindow.size().width());
		conf->setValue("video/screen_h", mainWindow.size().height());
		conf->setValue("video/screen_x", mainWindow.x() - screenGeom.x());
		conf->setValue("video/screen_y", mainWindow.y() - screenGeom.y());
	}

	// clear the restore defaults flag if it is set.
	conf->setValue("main/restore_defaults", false);

	updateConfigLabels();

	emit core->configurationDataSaved();
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
	if (askConfirmation())
	{
		qDebug() << "Restore defaults...";
		QSettings* conf = StelApp::getInstance().getSettings();
		Q_ASSERT(conf);

		conf->setValue("main/restore_defaults", true);
		// reset all stored panel locations
		conf->beginGroup("DialogPositions");
		conf->remove("");
		conf->endGroup();
	}
	else
		qDebug() << "Restore defaults is canceled...";
}

void ConfigurationDialog::populatePluginsList()
{
	QListWidget *plugins = ui->pluginsListWidget;
	plugins->blockSignals(true);
	int currentRow = plugins->currentRow();
	QString selectedPluginId = "";
	if (currentRow>0)
		 selectedPluginId = plugins->currentItem()->data(Qt::UserRole).toString();

	plugins->clear();
	QString selectedPluginName = "";
	const QList<StelModuleMgr::PluginDescriptor> pluginsList = StelApp::getInstance().getModuleMgr().getPluginsList();	
	for (const auto& desc : pluginsList)
	{
		QString label = q_(desc.info.displayedName);
		QListWidgetItem* item = new QListWidgetItem(label);
		item->setData(Qt::UserRole, desc.info.id);
		plugins->addItem(item);
		if (currentRow>0 && item->data(Qt::UserRole).toString()==selectedPluginId)
			selectedPluginName = label;
	}
	plugins->sortItems(Qt::AscendingOrder);
	plugins->blockSignals(false);
	// If we had a valid previous selection (i.e. not first time we populate), restore it

	if (!selectedPluginName.isEmpty())
		plugins->setCurrentItem(plugins->findItems(selectedPluginName, Qt::MatchExactly).at(0));
	else
		plugins->setCurrentRow(0);
}

void ConfigurationDialog::pluginsSelectionChanged(QListWidgetItem* item, QListWidgetItem* previousItem)
{
	Q_UNUSED(previousItem);
	const QList<StelModuleMgr::PluginDescriptor> pluginsList = StelApp::getInstance().getModuleMgr().getPluginsList();
	for (const auto& desc : pluginsList)
	{
		if (item->data(Qt::UserRole).toString()==desc.info.id)
		{
			QString html = "<html><head></head><body>";
			html += "<h2>" + q_(desc.info.displayedName) + "</h2>";			
			QString d = desc.info.description;
			d.replace("\n", "<br />");
			html += "<p>" + q_(d) + "</p>";
			html += "<p>";
			QString thanks = desc.info.acknowledgements;
			if (!thanks.isEmpty())
			{
				html += "<strong>" + q_("Acknowledgments") + "</strong>: " + q_(thanks) + "<br/>";
			}
			html += "<strong>" + q_("Authors") + "</strong>: " + desc.info.authors;
			html += "<br /><strong>" + q_("Contact") + "</strong>: " + desc.info.contact;
			if (!desc.info.version.isEmpty())
				html += "<br /><strong>" + q_("Version") + "</strong>: " + desc.info.version;
			html += "<br /><strong>" + q_("License") + "</strong>: ";
			if (!desc.info.license.isEmpty())
				html += desc.info.license;
			else
				html += qc_("unknown", "license");
			html += "</p></body></html>";
			ui->pluginsInfoBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
			ui->pluginsInfoBrowser->setHtml(html);
			ui->pluginLoadAtStartupCheckBox->setChecked(desc.loadAtStartup);
			StelModule* pmod = StelApp::getInstance().getModuleMgr().getModule(desc.info.id, true);
			if (pmod != Q_NULLPTR)
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
	for (const auto& desc : pluginsList)
	{
		if (id == desc.info.id)
		{
			StelModule* pmod = moduleMgr.getModule(desc.info.id, QObject::sender()->objectName()=="pluginsListWidget");
			if (pmod != Q_NULLPTR)
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
	for (const auto& desc : pluginsList)
	{
		if (id == desc.info.id)
		{
			moduleMgr.setPluginLoadAtStartup(id, state == Qt::Checked);
			break;
		}
	}
}

#ifdef ENABLE_SCRIPTING
void ConfigurationDialog::populateScriptsList(void)
{
	QListWidget *scripts = ui->scriptListWidget;
	scripts->blockSignals(true);
	int currentRow = scripts->currentRow();
	QString selectedScriptId = "";
	if (currentRow>0)
		selectedScriptId = scripts->currentItem()->data(Qt::DisplayRole).toString();

	scripts->clear();
	for (const auto& ssc : StelApp::getInstance().getScriptMgr().getScriptList())
	{
		QListWidgetItem* item = new QListWidgetItem(ssc);
		scripts->addItem(item);
	}
	scripts->sortItems(Qt::AscendingOrder);
	scripts->blockSignals(false);
	// If we had a valid previous selection (i.e. not first time we populate), restore it
	if (!selectedScriptId.isEmpty())
		scripts->setCurrentItem(scripts->findItems(selectedScriptId, Qt::MatchExactly).at(0));
	else
		scripts->setCurrentRow(0);
}

void ConfigurationDialog::scriptSelectionChanged(const QString& s)
{
	if (s.isEmpty())
		return;	
	StelScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();	
	//ui->scriptInfoBrowser->document()->setDefaultStyleSheet(QString(StelApp::getInstance().getCurrentStelStyle()->htmlStyleSheet));
	QString html = scriptMgr.getHtmlDescription(s);
	ui->scriptInfoBrowser->setHtml(html);	
}

void ConfigurationDialog::runScriptClicked(void)
{
	if (ui->closeWindowAtScriptRunCheckbox->isChecked())
		this->close();
	StelScriptMgr& scriptMgr = StelApp::getInstance().getScriptMgr();
	if (ui->scriptListWidget->currentItem())
	{
		scriptMgr.runScript(ui->scriptListWidget->currentItem()->text());
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
#endif


void ConfigurationDialog::setFixedDateTimeToCurrent(void)
{
	StelCore* core = StelApp::getInstance().getCore();
	double JD = core->getJD();
	ui->fixedDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD+core->getUTCOffset(JD)/24));
	ui->fixedTimeRadio->setChecked(true);
	setStartupTimeMode();
}


void ConfigurationDialog::resetStarCatalogControls()
{
	const QVariantList& catalogConfig = GETSTELMODULE(StarMgr)->getCatalogsDescription();
	nextStarCatalogToDownload.clear();
	int idx=0;
	for (const auto& catV : catalogConfig)
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
				.arg(QString::number(nextStarCatalogToDownload.value("count").toDouble(), 'f', 1))
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

	// Ignore data from redirection.  (Not needed after Qt 5.6)
	if (!starCatalogDownloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute).isNull())
		return;
	qint64 size = starCatalogDownloadReply->bytesAvailable();
	progressBar->setValue(progressBar->getValue()+static_cast<int>(size/1024));
	currentDownloadFile->write(starCatalogDownloadReply->read(size));
}

void ConfigurationDialog::downloadStars()
{
	Q_ASSERT(!nextStarCatalogToDownload.isEmpty());
	Q_ASSERT(!isDownloadingStarCatalog);
	Q_ASSERT(starCatalogDownloadReply==Q_NULLPTR);
	Q_ASSERT(currentDownloadFile==Q_NULLPTR);
	Q_ASSERT(progressBar==Q_NULLPTR);

	QString path = StelFileMgr::getUserDir()+QString("/stars/default/")+nextStarCatalogToDownload.value("fileName").toString();
	currentDownloadFile = new QFile(path);
	if (!currentDownloadFile->open(QIODevice::WriteOnly))
	{
		qWarning() << "Can't open a writable file for storing new star catalog: " << QDir::toNativeSeparators(path);
		currentDownloadFile->deleteLater();
		currentDownloadFile = Q_NULLPTR;
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
	req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
	starCatalogDownloadReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	starCatalogDownloadReply->setReadBufferSize(1024*1024*2);	
	connect(starCatalogDownloadReply, SIGNAL(readyRead()), this, SLOT(newStarCatalogData()));
	connect(starCatalogDownloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(starCatalogDownloadReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));

	progressBar = StelApp::getInstance().addProgressBar();
	progressBar->setValue(0);
	progressBar->setRange(0, static_cast<int>(nextStarCatalogToDownload.value("sizeMb").toDouble()*1024));
	progressBar->setFormat(QString("%1: %p%").arg(nextStarCatalogToDownload.value("id").toString()));

	qDebug() << "Downloading file" << nextStarCatalogToDownload.value("url").toString();
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
		starCatalogDownloadReply = Q_NULLPTR;
		currentDownloadFile->close();
		currentDownloadFile->deleteLater();
		currentDownloadFile = Q_NULLPTR;
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar=Q_NULLPTR;
		return;
	}

	const QVariant& redirect = starCatalogDownloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (!redirect.isNull())
	{
		// We got a redirection, we need to follow
		starCatalogDownloadReply->deleteLater();
		QNetworkRequest req(redirect.toUrl());
		req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
		req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
		req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
		starCatalogDownloadReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		starCatalogDownloadReply->setReadBufferSize(1024*1024*2);
		connect(starCatalogDownloadReply, SIGNAL(readyRead()), this, SLOT(newStarCatalogData()));
		connect(starCatalogDownloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		connect(starCatalogDownloadReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
		return;
	}

	Q_ASSERT(starCatalogDownloadReply->bytesAvailable()==0);

	isDownloadingStarCatalog = false;
	currentDownloadFile->close();
	currentDownloadFile->deleteLater();
	currentDownloadFile = Q_NULLPTR;
	starCatalogDownloadReply->deleteLater();
	starCatalogDownloadReply = Q_NULLPTR;
	StelApp::getInstance().removeProgressBar(progressBar);
	progressBar=Q_NULLPTR;

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
		ui->getStarsButton->setVisible(true);
		ui->downloadCancelButton->setVisible(false);
		ui->downloadRetryButton->setVisible(false);
	}

	resetStarCatalogControls();
}

void ConfigurationDialog::de430ButtonClicked()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	StelApp::getInstance().getCore()->setDe430Active(!StelApp::getInstance().getCore()->de430IsActive());
	conf->setValue("astro/flag_use_de430", StelApp::getInstance().getCore()->de430IsActive());

	resetEphemControls(); //refresh labels
}

void ConfigurationDialog::de431ButtonClicked()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	StelApp::getInstance().getCore()->setDe431Active(!StelApp::getInstance().getCore()->de431IsActive());
	conf->setValue("astro/flag_use_de431", StelApp::getInstance().getCore()->de431IsActive());

	resetEphemControls(); //refresh labels
}

void ConfigurationDialog::resetEphemControls()
{
	ui->de430checkBox->setEnabled(StelApp::getInstance().getCore()->de430IsAvailable());
	ui->de431checkBox->setEnabled(StelApp::getInstance().getCore()->de431IsAvailable());
	ui->de430checkBox->setChecked(StelApp::getInstance().getCore()->de430IsActive());
	ui->de431checkBox->setChecked(StelApp::getInstance().getCore()->de431IsActive());

	if(StelApp::getInstance().getCore()->de430IsActive())
		ui->de430label->setText("1550..2650");
	else
	{
		if (StelApp::getInstance().getCore()->de430IsAvailable())
			ui->de430label->setText(q_("Available"));
		else
			ui->de430label->setText(q_("Not Available"));
	}
	if(StelApp::getInstance().getCore()->de431IsActive())
		ui->de431label->setText("-13000..17000");
	else
	{
		if (StelApp::getInstance().getCore()->de431IsAvailable())
			ui->de431label->setText(q_("Available"));
		else
			ui->de431label->setText(q_("Not Available"));
	}
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
	ui->checkBoxVelocity->setChecked(flags & StelObject::Velocity);
	ui->checkBoxProperMotion->setChecked(flags & StelObject::ProperMotion);
	ui->checkBoxSize->setChecked(flags & StelObject::Size);
	ui->checkBoxExtra->setChecked(flags & StelObject::Extra);
	ui->checkBoxGalacticCoordinates->setChecked(flags & StelObject::GalacticCoord);
	ui->checkBoxSupergalacticCoordinates->setChecked(flags & StelObject::SupergalacticCoord);
	ui->checkBoxOtherCoords->setChecked(flags & StelObject::OtherCoord);
	ui->checkBoxElongation->setChecked(flags & StelObject::Elongation);
	ui->checkBoxType->setChecked(flags & StelObject::ObjectType);
	ui->checkBoxEclipticCoordsJ2000->setChecked(flags & StelObject::EclipticCoordJ2000);
	ui->checkBoxEclipticCoordsOfDate->setChecked(flags & StelObject::EclipticCoordOfDate);
	ui->checkBoxConstellation->setChecked(flags & StelObject::IAUConstellation);
	ui->checkBoxSiderealTime->setChecked(flags & StelObject::SiderealTime);
	ui->checkBoxRTSTime->setChecked(flags & StelObject::RTSTime);
}

void ConfigurationDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);

	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();

	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
	{
		return;
	}

	// stackListWidget->font() does not work properly!
	// It has a incorrect fontSize in the first loading, which produces the bug#995107.
	QFont font;
	font.setPixelSize(14);
	font.setWeight(75);
	QFontMetrics fontMetrics(font);

	int iconSize = ui->stackListWidget->iconSize().width();

	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		int textWidth = fontMetrics.boundingRect(ui->stackListWidget->item(row)->text()).width();
		width += iconSize > textWidth ? iconSize : textWidth; // use the wider one
		width += 24; // margin - 12px left and 12px right
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void ConfigurationDialog::populateDeltaTAlgorithmsList()
{
	Q_ASSERT(ui->deltaTAlgorithmComboBox);

	// TRANSLATORS: Full phrase is "Algorithm of DeltaT"
	ui->deltaTLabel->setText(QString("%1 %2T:").arg(q_("Algorithm of")).arg(QChar(0x0394)));

	ui->pushButtonCustomDeltaTEquationDialog->setFixedHeight(ui->deltaTAlgorithmComboBox->height());

	QComboBox* algorithms = ui->deltaTAlgorithmComboBox;

	//Save the current selection to be restored later
	algorithms->blockSignals(true);
	int index = algorithms->currentIndex();
	QVariant selectedAlgorithmId = algorithms->itemData(index);
	algorithms->clear();
	//For each algorithm, display the localized name and store the key as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	algorithms->addItem(q_("Without correction"), "WithoutCorrection");
	algorithms->addItem(q_("Schoch (1931)"), "Schoch");
	algorithms->addItem(q_("Clemence (1948)"), "Clemence");
	algorithms->addItem(q_("IAU (1952)"), "IAU");
	algorithms->addItem(q_("Astronomical Ephemeris (1960)"), "AstronomicalEphemeris");
	algorithms->addItem(q_("Tuckerman (1962, 1964) & Goldstine (1973)"), "TuckermanGoldstine");
	algorithms->addItem(q_("Muller & Stephenson (1975)"), "MullerStephenson");
	algorithms->addItem(q_("Stephenson (1978)"), "Stephenson1978");
	algorithms->addItem(q_("Schmadel & Zech (1979)"), "SchmadelZech1979");
	algorithms->addItem(q_("Schmadel & Zech (1988)"), "SchmadelZech1988");
	algorithms->addItem(q_("Morrison & Stephenson (1982)"), "MorrisonStephenson1982");
	algorithms->addItem(q_("Stephenson & Morrison (1984)"), "StephensonMorrison1984");
	algorithms->addItem(q_("Stephenson & Houlden (1986)"), "StephensonHoulden");
	algorithms->addItem(q_("Espenak (1987, 1989)"), "Espenak");
	algorithms->addItem(q_("Borkowski (1988)"), "Borkowski");
	algorithms->addItem(q_("Chapront-Touze & Chapront (1991)"), "ChaprontTouze");	
	algorithms->addItem(q_("Stephenson & Morrison (1995)"), "StephensonMorrison1995");
	algorithms->addItem(q_("Stephenson (1997)"), "Stephenson1997");
	// The dropdown label is too long for the string, and Meeus 1998 is very popular, this should be in the beginning of the tag.
	algorithms->addItem(q_("Meeus (1998) (with Chapront, Chapront-Touze & Francou (1997))"), "ChaprontMeeus");
	algorithms->addItem(q_("JPL Horizons"), "JPLHorizons");	
	algorithms->addItem(q_("Meeus & Simons (2000)"), "MeeusSimons");
	algorithms->addItem(q_("Morrison & Stephenson (2004, 2005)"), "MorrisonStephenson2004");
	algorithms->addItem(q_("Stephenson, Morrison & Hohenkerk (2016)"), "StephensonMorrisonHohenkerk2016");
	// Espenak & Meeus (2006) used by default
	algorithms->addItem(q_("Espenak & Meeus (2006)").append(" *"), "EspenakMeeus");
	// GZ: I want to try out some things. Something is still wrong with eclipses, see lp:1275092.
	#ifndef NDEBUG
	algorithms->addItem(q_("Espenak & Meeus (2006) no extra moon acceleration"), "EspenakMeeusZeroMoonAccel");
	#endif
	algorithms->addItem(q_("Reijs (2006)"), "Reijs");
	algorithms->addItem(q_("Banjevic (2006)"), "Banjevic");
	algorithms->addItem(q_("Montenbruck & Pfleger (2000)"), "MontenbruckPfleger");
	algorithms->addItem(q_("Reingold & Dershowitz (2002, 2007, 2018)"), "ReingoldDershowitz");
	algorithms->addItem(q_("Islam, Sadiq & Qureshi (2008, 2013)"), "IslamSadiqQureshi");
	algorithms->addItem(q_("Khalid, Sultana & Zaidi (2014)"), "KhalidSultanaZaidi");
	algorithms->addItem(q_("Henriksson (2017)"), "Henriksson2017");
	algorithms->addItem(q_("Custom equation of %1T").arg(QChar(0x0394)), "Custom");

	//Restore the selection
	index = algorithms->findData(selectedAlgorithmId, Qt::UserRole, Qt::MatchCaseSensitive);
	algorithms->setCurrentIndex(index);
	//algorithms->model()->sort(0);
	algorithms->blockSignals(false);
	setDeltaTAlgorithmDescription();
}

void ConfigurationDialog::setDeltaTAlgorithm(int algorithmID)
{
	StelCore* core = StelApp::getInstance().getCore();
	QString currentAlgorithm = ui->deltaTAlgorithmComboBox->itemData(algorithmID).toString();
	core->setCurrentDeltaTAlgorithmKey(currentAlgorithm);
	setDeltaTAlgorithmDescription();
	if (currentAlgorithm.contains("Custom"))
		ui->pushButtonCustomDeltaTEquationDialog->setEnabled(true);
	else
		ui->pushButtonCustomDeltaTEquationDialog->setEnabled(false);
}

void ConfigurationDialog::setDeltaTAlgorithmDescription()
{
	ui->deltaTAlgorithmDescription->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->deltaTAlgorithmDescription->setHtml(StelApp::getInstance().getCore()->getCurrentDeltaTAlgorithmDescription());
}

void ConfigurationDialog::showCustomDeltaTEquationDialog()
{
	if (customDeltaTEquationDialog == Q_NULLPTR)
		customDeltaTEquationDialog = new CustomDeltaTEquationDialog();

	customDeltaTEquationDialog->setVisible(true);
}

void ConfigurationDialog::populateDateFormatsList()
{
	Q_ASSERT(ui->dateFormatsComboBox);

	QComboBox* dfmts = ui->dateFormatsComboBox;

	//Save the current selection to be restored later
	dfmts->blockSignals(true);
	int index = dfmts->currentIndex();
	QVariant selectedDateFormat = dfmts->itemData(index);
	dfmts->clear();
	//For each format, display the localized name and store the key as user data.
	dfmts->addItem(q_("System default"), "system_default");
	dfmts->addItem(q_("yyyy-mm-dd (ISO 8601)"), "yyyymmdd");
	dfmts->addItem(q_("dd-mm-yyyy"), "ddmmyyyy");
	dfmts->addItem(q_("mm-dd-yyyy"), "mmddyyyy");
	dfmts->addItem(q_("ww, yyyy-mm-dd"), "wwyyyymmdd");
	dfmts->addItem(q_("ww, dd-mm-yyyy"), "wwddmmyyyy");
	dfmts->addItem(q_("ww, mm-dd-yyyy"), "wwmmddyyyy");
	//Restore the selection
	index = dfmts->findData(selectedDateFormat, Qt::UserRole, Qt::MatchCaseSensitive);
	dfmts->setCurrentIndex(index);
	dfmts->blockSignals(false);
}

void ConfigurationDialog::setDateFormat()
{
	QString selectedFormat = ui->dateFormatsComboBox->itemData(ui->dateFormatsComboBox->currentIndex()).toString();

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();
	if (selectedFormat == localeManager.getDateFormatStr())
		return;

	localeManager.setDateFormatStr(selectedFormat);	
}

void ConfigurationDialog::populateTimeFormatsList()
{
	Q_ASSERT(ui->timeFormatsComboBox);

	QComboBox* tfmts = ui->timeFormatsComboBox;

	//Save the current selection to be restored later
	tfmts->blockSignals(true);
	int index = tfmts->currentIndex();
	QVariant selectedTimeFormat = tfmts->itemData(index);
	tfmts->clear();
	//For each format, display the localized name and store the key as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	tfmts->addItem(q_("System default"), "system_default");
	tfmts->addItem(q_("12-hour format"), "12h");
	tfmts->addItem(q_("24-hour format"), "24h");

	//Restore the selection
	index = tfmts->findData(selectedTimeFormat, Qt::UserRole, Qt::MatchCaseSensitive);
	tfmts->setCurrentIndex(index);
	tfmts->blockSignals(false);
}

void ConfigurationDialog::setTimeFormat()
{
	QString selectedFormat = ui->timeFormatsComboBox->itemData(ui->timeFormatsComboBox->currentIndex()).toString();

	StelLocaleMgr & localeManager = StelApp::getInstance().getLocaleMgr();
	if (selectedFormat == localeManager.getTimeFormatStr())
		return;

	localeManager.setTimeFormatStr(selectedFormat);	
}

void ConfigurationDialog::populateDitherList()
{
	Q_ASSERT(ui->ditheringComboBox);
	QComboBox* ditherCombo = ui->ditheringComboBox;

	ditherCombo->blockSignals(true);
	ditherCombo->clear();
	ditherCombo->addItem(qc_("None","disabled"), "disabled");
	ditherCombo->addItem(q_("5/6/5 bits"), "color565");
	ditherCombo->addItem(q_("6/6/6 bits"), "color666");
	ditherCombo->addItem(q_("8/8/8 bits"), "color888");
	ditherCombo->addItem(q_("10/10/10 bits"), "color101010");

	// show current setting
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	QVariant selectedDitherFormat = conf->value("video/dithering_mode", "disabled");

	int index = ditherCombo->findData(selectedDitherFormat, Qt::UserRole, Qt::MatchCaseSensitive);
	ditherCombo->setCurrentIndex(index);
	ditherCombo->blockSignals(false);
}

void ConfigurationDialog::setDitherFormat()
{
	QString selectedFormat = ui->ditheringComboBox->itemData(ui->ditheringComboBox->currentIndex()).toString();
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("video/dithering_mode", selectedFormat);
	conf->sync();
}

void ConfigurationDialog::populateFontWritingSystemCombo()
{
	QComboBox *combo=ui->fontWritingSystemComboBox;
	QFontDatabase fontDatabase;
	const QList<QFontDatabase::WritingSystem> writingSystems=fontDatabase.writingSystems();
		for (const auto& system : writingSystems)
		{
			combo->addItem(QFontDatabase::writingSystemName(system) + "  " + QFontDatabase::writingSystemSample(system), system);
		}
}

void ConfigurationDialog::handleFontBoxWritingSystem(int index)
{
	Q_UNUSED(index)
	QComboBox *sender=dynamic_cast<QComboBox *>(QObject::sender());
	ui->fontComboBox->setWritingSystem(static_cast<QFontDatabase::WritingSystem>(sender->currentData().toInt()));
}

void ConfigurationDialog::populateScreenshotFileformatsCombo()
{
	QComboBox *combo=ui->screenshotFileFormatComboBox;
	// To avoid platform differences, just ask what's available.
	// However, wbmp seems broken, disable it and a few unnecessary formats
	const QList<QByteArray> formats = QImageWriter::supportedImageFormats();
	for (const auto& format : formats)
	{
		if ((format != "icns") && (format != "cur") && (format != "wbmp"))
			combo->addItem(QString(format));
	}
	combo->setCurrentText(StelApp::getInstance().getStelPropertyManager()->getStelPropertyValue("MainView.screenShotFormat").toString()); // maybe not required.
}
