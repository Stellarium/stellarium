/*
 * Stellarium
 * Copyright (C) 2015 Alexander Wolf
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
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "AstroCalcDialog.hpp"
#include "ui_astroCalcDialog.h"

#include <QTimer>

AstroCalcDialog::AstroCalcDialog()
{
	ui = new Ui_astroCalcDialogForm;
	core = StelApp::getInstance().getCore();
	solarSystem = GETSTELMODULE(SolarSystem);
	objectMgr = GETSTELMODULE(StelObjectMgr);
}

AstroCalcDialog::~AstroCalcDialog()
{
	delete ui;
}

void AstroCalcDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAstroCalcDescription();
		setPlanetaryPositionsHeaderNames();
		setEphemerisHeaderNames();
		populateCelestialBodyList();
		populateEphemerisTimeStepsList();
	}
}

void AstroCalcDialog::createDialogContent()
{
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	// Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->planetaryPositionsTreeWidget;
	installKineticScrolling(addscroll);
#endif
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	setAstroCalcDescription();
	initListPlanetaryPositions();
	initListEphemeris();
	populateCelestialBodyList();
	populateEphemerisTimeStepsList();

	double JD = core->getJD();
	ui->dateFromDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD + StelUtils::getGMTShiftFromQT(JD)/24));
	ui->dateToDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD + StelUtils::getGMTShiftFromQT(JD)/24 + 30.f));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(ui->planetaryPositionsTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		ui->planetaryPositionsTreeWidget, SLOT(repaint()));

	connect(ui->planetaryPositionsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentPlanetaryPosition(QModelIndex)));

	connect(ui->ephemerisPushButton, SIGNAL(clicked()), this, SLOT(generateEphemeris()));
	connect(ui->ephemerisTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentEphemeride(QModelIndex)));

	// every 5 min, check if it's time to update
	QTimer* updateTimer = new QTimer(this);
	updateTimer->setInterval(300000);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(currentPlanetaryPositions()));
	updateTimer->start();
	currentPlanetaryPositions();
}

void AstroCalcDialog::initListPlanetaryPositions()
{
	ui->planetaryPositionsTreeWidget->clear();
	ui->planetaryPositionsTreeWidget->setColumnCount(ColumnCount);
	setPlanetaryPositionsHeaderNames();
	ui->planetaryPositionsTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::setPlanetaryPositionsHeaderNames()
{
	QStringList headerStrings;
	//TRANSLATORS: name of object
	headerStrings << q_("Name");
	//TRANSLATORS: right ascension
	headerStrings << q_("RA (J2000)");
	//TRANSLATORS: declination
	headerStrings << q_("Dec (J2000)");
	//TRANSLATORS: magnitude
	headerStrings << q_("Mag.");
	//TRANSLATORS: type of object
	headerStrings << q_("Type");
	ui->planetaryPositionsTreeWidget->setHeaderLabels(headerStrings);

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    ui->planetaryPositionsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::currentPlanetaryPositions()
{
	float ra, dec;
	QList<PlanetP> allPlanets = solarSystem->getAllPlanets();

	initListPlanetaryPositions();

	foreach (const PlanetP& planet, allPlanets)
	{
		if (planet->getPlanetType()!=Planet::isUNDEFINED && planet->getEnglishName()!="Sun" && planet->getEnglishName()!=core->getCurrentPlanet()->getEnglishName())
		{
			StelUtils::rectToSphe(&ra,&dec,planet->getJ2000EquatorialPos(core));
			TreeWidgetItem *treeItem = new TreeWidgetItem(ui->planetaryPositionsTreeWidget);
			treeItem->setText(ColumnName, planet->getNameI18n());
			treeItem->setText(ColumnRA, StelUtils::radToHmsStr(ra));
			treeItem->setTextAlignment(ColumnRA, Qt::AlignRight);
			treeItem->setText(ColumnDec, StelUtils::radToHmsStr(dec));
			treeItem->setTextAlignment(ColumnDec, Qt::AlignRight);
			treeItem->setText(ColumnMagnitude, QString::number(planet->getVMagnitudeWithExtinction(core), 'f', 2));
			treeItem->setTextAlignment(ColumnMagnitude, Qt::AlignRight);
			treeItem->setText(ColumnType, q_(planet->getPlanetTypeString()));
		}
	}

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    ui->planetaryPositionsTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-name
	ui->planetaryPositionsTreeWidget->sortItems(ColumnName, Qt::AscendingOrder);
}

void AstroCalcDialog::selectCurrentPlanetaryPosition(const QModelIndex &modelIndex)
{
	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), ColumnName).data().toString();

	if (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n))
	{
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=core->getCurrentLocation().planetName)
			{
				StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
			}
			else
			{
				GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
	}
}

void AstroCalcDialog::selectCurrentEphemeride(const QModelIndex &modelIndex)
{
	// Find the object
	QString name = ui->celestialBodyComboBox->currentData().toString();
	double JD = modelIndex.sibling(modelIndex.row(), EphemerisJD).data().toDouble();

	if (objectMgr->findAndSelectI18n(name) || objectMgr->findAndSelect(name))
	{
		core->setJD(JD);
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=core->getCurrentLocation().planetName)
			{
				StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
			}
			else
			{
				GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
	}
}

void AstroCalcDialog::setEphemerisHeaderNames()
{
	QStringList headerStrings;
	//TRANSLATORS: name of object
	headerStrings << q_("Date and Time");
	headerStrings << q_("JD");
	//TRANSLATORS: right ascension
	headerStrings << q_("RA (J2000)");
	//TRANSLATORS: declination
	headerStrings << q_("Dec (J2000)");
	//TRANSLATORS: magnitude
	headerStrings << q_("Mag.");
	ui->ephemerisTreeWidget->setHeaderLabels(headerStrings);

	// adjust the column width
	for(int i = 0; i < EphemerisCount; ++i)
	{
	    ui->ephemerisTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListEphemeris()
{
	ui->ephemerisTreeWidget->clear();
	ui->ephemerisTreeWidget->setColumnCount(EphemerisCount);
	setEphemerisHeaderNames();
	ui->ephemerisTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::generateEphemeris()
{
	float currentStep, ra, dec;
	QString currentPlanet = ui->celestialBodyComboBox->currentData().toString();

	initListEphemeris();

	switch (ui->ephemerisStepComboBox->currentData().toInt()) {
		case 1:
			currentStep = 10 * StelCore::JD_MINUTE;
			break;
		case 2:
			currentStep = StelCore::JD_HOUR;
			break;
		case 3:
			currentStep = StelCore::JD_DAY;
			break;
		case 4:
			currentStep = 10 * StelCore::JD_DAY;
			break;
		case 5:
			currentStep = 30 * StelCore::JD_DAY;
			break;
		default:
			currentStep = StelCore::JD_DAY;
			break;
	}

	StelObjectP obj = solarSystem->searchByName(currentPlanet);
	if (obj)
	{
		double currentJD = core->getJD(); // save current JD
		double firstJD = StelUtils::qDateTimeToJd(ui->dateFromDateTimeEdit->dateTime());
		firstJD = firstJD - StelUtils::getGMTShiftFromQT(firstJD)/24;
		int elements = (int)((StelUtils::qDateTimeToJd(ui->dateToDateTimeEdit->dateTime()) - firstJD)/currentStep);
		for (int i=0; i<elements; i++)
		{
			double JD = firstJD + i*currentStep;
			core->setJD(JD);
			core->update(10); // force update to get new coordinates
			StelUtils::rectToSphe(&ra,&dec,obj->getJ2000EquatorialPos(core));
			TreeWidgetItem *treeItem = new TreeWidgetItem(ui->ephemerisTreeWidget);
			// local date and time
			treeItem->setText(EphemerisDate, StelUtils::jdToQDateTime(JD + StelUtils::getGMTShiftFromQT(JD)/24).toString("yyyy-MM-dd hh:mm:ss"));
			treeItem->setText(EphemerisJD, QString::number(JD, 'f', 5));
			treeItem->setText(EphemerisRA, StelUtils::radToHmsStr(ra));
			treeItem->setTextAlignment(EphemerisRA, Qt::AlignRight);
			treeItem->setText(EphemerisDec, StelUtils::radToHmsStr(dec));
			treeItem->setTextAlignment(EphemerisDec, Qt::AlignRight);
			treeItem->setText(EphemerisMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
			treeItem->setTextAlignment(EphemerisMagnitude, Qt::AlignRight);
		}
		core->setJD(currentJD); // restore time
	}

	// adjust the column width
	for(int i = 0; i < EphemerisCount; ++i)
	{
	    ui->ephemerisTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-name
	ui->ephemerisTreeWidget->sortItems(EphemerisDate, Qt::AscendingOrder);
}

void AstroCalcDialog::populateCelestialBodyList()
{
	Q_ASSERT(ui->celestialBodyComboBox);

	QComboBox* planets = ui->celestialBodyComboBox;
	QStringList planetNames(solarSystem->getAllPlanetEnglishNames());
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	//Save the current selection to be restored later
	planets->blockSignals(true);
	int index = planets->currentIndex();
	QVariant selectedPlanetId = planets->itemData(index);
	planets->clear();
	//For each planet, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	foreach(const QString& name, planetNames)
	{
		if (name!="Solar System Observer" && name!="Sun" && name!=core->getCurrentPlanet()->getEnglishName())
			planets->addItem(trans.qtranslate(name), name);
	}
	//Restore the selection
	index = planets->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = planets->findData("Moon", Qt::UserRole, Qt::MatchCaseSensitive);;
	planets->setCurrentIndex(index);
	planets->model()->sort(0);
	planets->blockSignals(false);
}

void AstroCalcDialog::populateEphemerisTimeStepsList()
{
	Q_ASSERT(ui->ephemerisStepComboBox);

	QComboBox* steps = ui->ephemerisStepComboBox;
	steps->blockSignals(true);
	int index = steps->currentIndex();
	QVariant selectedStepId = steps->itemData(index);

	steps->clear();
	steps->addItem(q_("10 minutes"), "1");
	steps->addItem(q_("1 hour"), "2");
	steps->addItem(q_("1 day"), "3");
	steps->addItem(q_("10 days"), "4");
	steps->addItem(q_("30 days"), "5");

	index = steps->findData(selectedStepId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = 2;
	steps->setCurrentIndex(index);
	steps->blockSignals(false);
}

void AstroCalcDialog::setAstroCalcDescription()
{
	QString description;
	description  = "<p><b>" + q_("AstroCalc") + "</b> &mdash; " + q_("a tool for calculating planetary phenomena and which contains few modules.") + "</p>";
	description += "<ul>";
	description += "<li><b>" + q_("Planetary positions") + "</b> &mdash; " + q_("a tool for displaying planetary positions every 5 minutes.") + "</li>";
	description += "<li><b>" + q_("Ephemeris") + "</b> &mdash; " + q_("a tool for calculation ephemeris of celestial bodies for different time ranges and steps of the time.") + "</li>";
	description += "</ul>";

	ui->astroCalcDescription->setText(description);
}
