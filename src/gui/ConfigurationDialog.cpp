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
#include "NewGui.hpp"

#include <QSettings>
#include <QDebug>
#include <QFrame>
#include <QFile>

#include "StelAppGraphicsItem.hpp"

ConfigurationDialog::ConfigurationDialog()
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

void ConfigurationDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(ui->closeView, SIGNAL(clicked()), this, SLOT(close()));
	
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
	c->setCurrentItem(litem.at(0));
	connect(c, SIGNAL(currentTextChanged(const QString&)), this, SLOT(languageChanged(const QString&)));
	
	QSettings* conf = StelApp::getInstance().getSettings();
	Projector* proj = StelApp::getInstance().getCore()->getProjection();
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	MovementMgr* movement = (MovementMgr*)StelApp::getInstance().getModuleMgr().getModule("MovementMgr");

	// Startup Tab
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
	
	ui->initFovSpinBox->setValue(conf->value("navigation/init_fov",60.).toDouble());
	connect(ui->initFovSpinBox, SIGNAL(valueChanged(double)), proj, SLOT(setInitFov(double)));

	connect(ui->setInitViewDirection, SIGNAL(clicked()), nav, SLOT(setInitViewDirectionToCurrent()));

	// Planetarium tab
	connect(ui->sphericMirrorCheckbox, SIGNAL(toggled(bool)), this, SLOT(setSphericMirror(bool)));
	connect(ui->gravityLabelCheckbox, SIGNAL(toggled(bool)), proj, SLOT(setFlagGravityLabels(bool)));
	connect(ui->discViewportCheckbox, SIGNAL(toggled(bool)), this, SLOT(setDiskViewport(bool)));
	ui->autoZoomResetsDirectionCheckbox->setChecked(movement->getFlagAutoZoomOutResetsDirection());
	connect(ui->autoZoomResetsDirectionCheckbox, SIGNAL(toggled(bool)), movement, SLOT(setFlagAutoZoomOutResetsDirection(bool)));

	// Tools tab
	ui->adaptationCheckbox->setChecked(StelApp::getInstance().getCore()->getSkyDrawer()->getFlagLuminanceAdaptation());
	connect(ui->adaptationCheckbox, SIGNAL(toggled(bool)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setFlagLuminanceAdaptation(bool)));

	// Interface tab
	NewGui* newGui = (NewGui*)GETSTELMODULE("NewGui");
	assert(newGui);
	if (newGui->getInfoPanel()->getInfoTextFilters() == (StelObject::InfoStringGroup)0)
		ui->noSelectedInfoRadio->setChecked(true);
	else if (newGui->getInfoPanel()->getInfoTextFilters() == StelObject::InfoStringGroup(StelObject::BriefInfo))
		ui->briefSelectedInfoRadio->setChecked(true);
	else
		ui->allSelectedInfoRadio->setChecked(true);

	connect(ui->noSelectedInfoRadio, SIGNAL(released()), this, SLOT(setNoSelectedInfo()));
	connect(ui->allSelectedInfoRadio, SIGNAL(released()), this, SLOT(setAllSelectedInfo()));
	connect(ui->briefSelectedInfoRadio, SIGNAL(released()), this, SLOT(setBriefSelectedInfo()));

	// DEBUG tab
	// connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)), (const QObject*)StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setInputScale(double)));
	// connect(ui->doubleSpinBox_2, SIGNAL(valueChanged(double)), (const QObject*)StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setOutScale(double)));

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
		proj->setMaskType(Projector::DISK);
	else
		proj->setMaskType(Projector::NONE);
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
	NewGui* newGui = (NewGui*)GETSTELMODULE("NewGui");
	assert(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(0));
}

void ConfigurationDialog::setAllSelectedInfo(void)
{
	NewGui* newGui = (NewGui*)GETSTELMODULE("NewGui");
	assert(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
}

void ConfigurationDialog::setBriefSelectedInfo(void)
{
	NewGui* newGui = (NewGui*)GETSTELMODULE("NewGui");
	assert(newGui);
	newGui->getInfoPanel()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::BriefInfo));
}


