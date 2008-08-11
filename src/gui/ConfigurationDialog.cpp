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

#include <QSettings>
#include <QDebug>
#include <QFrame>
#include <QFile>

#include "StelAppGraphicsItem.hpp"

ConfigurationDialog::ConfigurationDialog(): flipVert(NULL), flipHoriz(NULL)
{
	ui = new Ui_configurationDialogForm;
}

ConfigurationDialog::~ConfigurationDialog()
{
	delete ui;
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
	QSettings* conf = StelApp::getInstance().getSettings();
	Projector* proj = StelApp::getInstance().getCore()->getProjection();
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	MovementMgr* mmgr = static_cast<MovementMgr*>(GETSTELMODULE("MovementMgr"));
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	
	ui->setupUi(dialog);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	// Main tab
	// Fill the language list widget from the available list
	QListWidget* c = ui->programLanguageListWidget;
	c->clear();
	c->addItems(Translator::globalTranslator.getAvailableLanguagesNamesNative(StelApp::getInstance().getFileMgr().getLocaleDir()));
	QString appLang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	QString l = Translator::iso639_1CodeToNativeName(appLang);
	QList<QListWidgetItem*> litem = c->findItems(l, Qt::MatchExactly);
	if (litem.empty() && appLang.contains('_'))
	{
		l = appLang.left(appLang.indexOf('_'));
		l=Translator::iso639_1CodeToNativeName(l);
		litem = c->findItems(l, Qt::MatchExactly);
	}
	if (!litem.empty())
		c->setCurrentItem(litem.at(0));
	connect(c, SIGNAL(currentTextChanged(const QString&)), this, SLOT(languageChanged(const QString&)));

	// Selected object info
	if (gui->getInfoPanel()->getInfoTextFilters() == (StelObject::InfoStringGroup)0)
		ui->noSelectedInfoRadio->setChecked(true);
	else if (gui->getInfoPanel()->getInfoTextFilters() == StelObject::InfoStringGroup(StelObject::BriefInfo))
		ui->briefSelectedInfoRadio->setChecked(true);
	else
		ui->allSelectedInfoRadio->setChecked(true);
	connect(ui->noSelectedInfoRadio, SIGNAL(released()), this, SLOT(setNoSelectedInfo()));
	connect(ui->allSelectedInfoRadio, SIGNAL(released()), this, SLOT(setAllSelectedInfo()));
	connect(ui->briefSelectedInfoRadio, SIGNAL(released()), this, SLOT(setBriefSelectedInfo()));
	
	// View mode radio buttons
	const bool vmodeNight = StelApp::getInstance().getVisionModeNight();
	ui->normalVisionRadioButton->setChecked(!vmodeNight);
	ui->nightVisionRadioButton->setChecked(vmodeNight);
	connect(ui->normalVisionRadioButton, SIGNAL(clicked()), this, SLOT(visionModeChanged()));
	connect(ui->nightVisionRadioButton, SIGNAL(clicked()), this, SLOT(visionModeChanged()));
	connect(ui->invertedColorsRadioButton, SIGNAL(clicked()), this, SLOT(visionModeChanged()));
	
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
	
	ui->fixedDateTimeEdit->setDateTime(nav->getInitDateTime());
	connect(ui->fixedDateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), nav, SLOT(setInitDateTime(QDateTime)));
	
	// Startup FOV/direction of view
	ui->initFovSpinBox->setValue(conf->value("navigation/init_fov",60.).toDouble());
	connect(ui->initFovSpinBox, SIGNAL(valueChanged(double)), proj, SLOT(setInitFov(double)));
	connect(ui->setInitViewDirection, SIGNAL(clicked()), nav, SLOT(setInitViewDirectionToCurrent()));

	ui->enableKeysNavigationCheckBox->setChecked(mmgr->getFlagEnableMoveKeys() || mmgr->getFlagEnableZoomKeys());
	ui->enableMouseNavigationCheckBox->setChecked(mmgr->getFlagEnableMouseNavigation());
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mmgr, SLOT(setFlagEnableMoveKeys(bool)));
	connect(ui->enableKeysNavigationCheckBox, SIGNAL(toggled(bool)), mmgr, SLOT(setFlagEnableZoomKeys(bool)));
	connect(ui->enableMouseNavigationCheckBox, SIGNAL(toggled(bool)), mmgr, SLOT(setFlagEnableMouseNavigation(bool)));
	
	// Tools tab
	connect(ui->sphericMirrorCheckbox, SIGNAL(toggled(bool)), this, SLOT(setSphericMirror(bool)));
	connect(ui->gravityLabelCheckbox, SIGNAL(toggled(bool)), proj, SLOT(setFlagGravityLabels(bool)));
	connect(ui->discViewportCheckbox, SIGNAL(toggled(bool)), this, SLOT(setDiskViewport(bool)));
	ui->autoZoomResetsDirectionCheckbox->setChecked(mmgr->getFlagAutoZoomOutResetsDirection());
	connect(ui->autoZoomResetsDirectionCheckbox, SIGNAL(toggled(bool)), mmgr, SLOT(setFlagAutoZoomOutResetsDirection(bool)));
	
	const bool b = conf->value("gui/flag_show_flip_buttons",false).toBool();
	ui->showFlipButtonsCheckbox->setChecked(b);
	if (b==true)
		setShowFlipButtons(b);
	connect(ui->showFlipButtonsCheckbox, SIGNAL(toggled(bool)), this, SLOT(setShowFlipButtons(bool)));
}

void ConfigurationDialog::languageChanged(const QString& langName)
{
	StelApp::getInstance().getLocaleMgr().setAppLanguage(Translator::nativeNameToIso639_1Code(langName));
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(Translator::nativeNameToIso639_1Code(langName));
}

void ConfigurationDialog::setStartupTimeMode(void)
{
	if (ui->systemTimeRadio->isChecked())
		StelApp::getInstance().getCore()->getNavigation()->setStartupTimeMode("actual");
	else if (ui->todayRadio->isChecked())
		StelApp::getInstance().getCore()->getNavigation()->setStartupTimeMode("today");
	else
		StelApp::getInstance().getCore()->getNavigation()->setStartupTimeMode("preset");
}

void ConfigurationDialog::setDiskViewport(bool b)
{
	Projector* proj = StelApp::getInstance().getCore()->getProjection();
	assert(proj);
	if (b)
		proj->setMaskType(Projector::Disk);
	else
		proj->setMaskType(Projector::None);
}

void ConfigurationDialog::setSphericMirror(bool b)
{
	if (b)
		StelAppGraphicsItem::getInstance().setViewPortDistorterType("fisheye_to_spheric_mirror");
	else
		StelAppGraphicsItem::getInstance().setViewPortDistorterType("none");
}

void ConfigurationDialog::setNoSelectedInfo(void)
{
	StelGui* newGui = (StelGui*)GETSTELMODULE("StelGui");
	assert(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(0));
}

void ConfigurationDialog::setAllSelectedInfo(void)
{
	StelGui* newGui = (StelGui*)GETSTELMODULE("StelGui");
	assert(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
}

void ConfigurationDialog::setBriefSelectedInfo(void)
{
	StelGui* newGui = (StelGui*)GETSTELMODULE("StelGui");
	assert(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::BriefInfo));
}

void ConfigurationDialog::setShowFlipButtons(bool b)
{
	StelGui* gui = (StelGui*)GETSTELMODULE("StelGui");
	if (b==true)
	{
		if (flipVert==NULL)
		{
			// Create the vertical flip button
			QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
			flipVert = new StelButton(NULL, QPixmap(":/graphicGui/gui/btFlipVertical-on.png"), QPixmap(":/graphicGui/gui/btFlipVertical-off.png"), pxmapGlow32x32, gui->getGuiActions("actionVertical_Flip"));
		}
		if (flipHoriz==NULL)
		{
			// Create the vertical flip button
			QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
			flipHoriz = new StelButton(NULL, QPixmap(":/graphicGui/gui/btFlipHorizontal-on.png"), QPixmap(":/graphicGui/gui/btFlipHorizontal-off.png"), pxmapGlow32x32, gui->getGuiActions("actionHorizontal_Flip"));
		}
		
		gui->getButtonBar()->addButton(flipVert, "060-othersGroup", "actionQuit");
		gui->getButtonBar()->addButton(flipHoriz, "060-othersGroup", "actionVertical_Flip");
	}
	else
	{
		Q_ASSERT(gui->getButtonBar()->hideButton("actionVertical_Flip")==flipVert);
		Q_ASSERT(gui->getButtonBar()->hideButton("actionHorizontal_Flip")==flipHoriz);
	}
}

void ConfigurationDialog::visionModeChanged()
{
	const bool b = StelApp::getInstance().getVisionModeNight();
	if (ui->normalVisionRadioButton->isChecked() && b)
	{
		StelApp::getInstance().setVisionModeNight(false);
	}
	if (ui->nightVisionRadioButton->isChecked() && !b)
	{
		StelApp::getInstance().setVisionModeNight(true);
	}
}
