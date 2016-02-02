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
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"

#include "SolarSystem.hpp"
#include "Planet.hpp"

#include "AstroCalcDialog.hpp"
#include "ui_astroCalcDialog.h"

#include <QTimer>

QVector<Vec3d> AstroCalcDialog::EphemerisListJ2000;
int AstroCalcDialog::DisplayedPositionIndex = -1;

AstroCalcDialog::AstroCalcDialog(QObject *parent)
	: StelDialog(parent)
{
	dialogName = "AstroCalc";
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
		setPlanetaryPositionsHeaderNames();
		setEphemerisHeaderNames();
		setPhenomenaHeaderNames();
		populateCelestialBodyList();
		populateEphemerisTimeStepsList();
		populateMajorPlanetList();
		populateGroupCelestialBodyList();
		currentPlanetaryPositions();
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
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	initListPlanetaryPositions();
	initListEphemeris();
	initListPhenomena();
	populateCelestialBodyList();
	populateEphemerisTimeStepsList();
	populateMajorPlanetList();
	populateGroupCelestialBodyList();

	double JD = core->getJD() + StelUtils::getGMTShiftFromQT(core->getJD())/24;
	ui->dateFromDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD));
	ui->dateToDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(JD + 30.f));
	ui->phenomenFromDateEdit->setDateTime(StelUtils::jdToQDateTime(JD));
	ui->phenomenToDateEdit->setDateTime(StelUtils::jdToQDateTime(JD + 365.f));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(ui->planetaryPositionsTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		ui->planetaryPositionsTreeWidget, SLOT(repaint()));

	connect(ui->planetaryPositionsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentPlanetaryPosition(QModelIndex)));

	connect(ui->ephemerisPushButton, SIGNAL(clicked()), this, SLOT(generateEphemeris()));
	connect(ui->ephemerisTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentEphemeride(QModelIndex)));
	connect(ui->ephemerisTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(onChangedEphemerisPosition(QModelIndex)));

	connect(ui->phenomenaPushButton, SIGNAL(clicked()), this, SLOT(calculatePhenomena()));
	connect(ui->phenomenaTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentPhenomen(QModelIndex)));

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
			ACTreeWidgetItem *treeItem = new ACTreeWidgetItem(ui->planetaryPositionsTreeWidget);
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

void AstroCalcDialog::onChangedEphemerisPosition(const QModelIndex &modelIndex)
{
	DisplayedPositionIndex = modelIndex.row();
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
		EphemerisListJ2000.clear();
		EphemerisListJ2000.reserve(elements);
		for (int i=0; i<elements; i++)
		{
			double JD = firstJD + i*currentStep;
			core->setJD(JD);
			core->update(0); // force update to get new coordinates			
			Vec3d pos = obj->getJ2000EquatorialPos(core);
			EphemerisListJ2000.append(pos);
			StelUtils::rectToSphe(&ra,&dec,pos);
			ACTreeWidgetItem *treeItem = new ACTreeWidgetItem(ui->ephemerisTreeWidget);
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

	// sort-by-date
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

void AstroCalcDialog::populateMajorPlanetList()
{
	Q_ASSERT(ui->object1ComboBox); // object 1 is always major planet

	QComboBox* majorPlanet = ui->object1ComboBox;
	QList<PlanetP> planets = solarSystem->getAllPlanets();
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	//Save the current selection to be restored later
	majorPlanet->blockSignals(true);
	int index = majorPlanet->currentIndex();
	QVariant selectedPlanetId = majorPlanet->itemData(index);
	majorPlanet->clear();
	//For each planet, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	foreach(const PlanetP& planet, planets)
	{
		if (planet->getPlanetType()==Planet::isPlanet && planet->getEnglishName()!=core->getCurrentPlanet()->getEnglishName())
			majorPlanet->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());
	}
	//Restore the selection
	index = majorPlanet->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = majorPlanet->findData("Mercury", Qt::UserRole, Qt::MatchCaseSensitive);;
	majorPlanet->setCurrentIndex(index);
	majorPlanet->model()->sort(0);
	majorPlanet->blockSignals(false);
}

void AstroCalcDialog::populateGroupCelestialBodyList()
{
	Q_ASSERT(ui->object2ComboBox);

	QComboBox* groups = ui->object2ComboBox;
	groups->blockSignals(true);
	int index = groups->currentIndex();
	QVariant selectedGroupId = groups->itemData(index);

	groups->clear();
	groups->addItem(q_("Solar system"), "0");
	groups->addItem(q_("Planets"), "1");
	groups->addItem(q_("Asteroids"), "2");
	groups->addItem(q_("Plutinos"), "3");
	groups->addItem(q_("Comets"), "4");
	groups->addItem(q_("Dwarf planets"), "5");
	groups->addItem(q_("Cubewanos"), "6");
	groups->addItem(q_("Scattered disc objects"), "7");
	groups->addItem(q_("Oort cloud objects"), "8");

	index = groups->findData(selectedGroupId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = groups->findData("1", Qt::UserRole, Qt::MatchCaseSensitive);
	groups->setCurrentIndex(index);
	groups->model()->sort(0);
	groups->blockSignals(false);
}

void AstroCalcDialog::setPhenomenaHeaderNames()
{
	QStringList headerStrings;
	headerStrings << q_("Phenomenon");
	headerStrings << q_("Date and Time");
	headerStrings << q_("Object 1");
	headerStrings << q_("Object 2");
	headerStrings << q_("Separation");
	ui->phenomenaTreeWidget->setHeaderLabels(headerStrings);

	// adjust the column width
	for(int i = 0; i < PhenomenaCount; ++i)
	{
	    ui->phenomenaTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::initListPhenomena()
{
	ui->phenomenaTreeWidget->clear();
	ui->phenomenaTreeWidget->setColumnCount(PhenomenaCount);
	setPhenomenaHeaderNames();
	ui->phenomenaTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::selectCurrentPhenomen(const QModelIndex &modelIndex)
{
	// Find the object
	QString name = ui->object1ComboBox->currentData().toString();
	QString date = modelIndex.sibling(modelIndex.row(), PhenomenaDate).data().toString();
	bool ok;
	double JD  = StelUtils::getJulianDayFromISO8601String(date.left(10) + "T" + date.right(8), &ok);
	JD -= StelUtils::getGMTShiftFromQT(JD)/24.;

	if (objectMgr->findAndSelectI18n(name) || objectMgr->findAndSelect(name))
	{
		core->setJD(JD);
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
			mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
			mvmgr->setFlagTracking(true);
		}
	}
}

void AstroCalcDialog::calculatePhenomena()
{
	Vec3d obj1Point, obj2Point;
	QString currentPlanet = ui->object1ComboBox->currentData().toString();
	double separation = ui->allowedSeparationDoubleSpinBox->value();

	initListPhenomena();

	QList<PlanetP> objects;
	objects.clear();
	QList<PlanetP> allObjects = solarSystem->getAllPlanets();
	switch (ui->object2ComboBox->currentData().toInt()) {
		case 1: // Planets
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isPlanet && object->getEnglishName()!=core->getCurrentPlanet()->getEnglishName() && object->getEnglishName()!=currentPlanet)
					objects.append(object);
			}
			break;
		case 2: // Asteroids
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isAsteroid)
					objects.append(object);
			}
			break;
		case 3: // Plutinos
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isPlutino)
					objects.append(object);
			}
			break;
		case 4: // Comets
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isComet)
					objects.append(object);
			}
			break;
		case 5: // Dwarf planets
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isDwarfPlanet)
					objects.append(object);
			}
			break;
		case 6: // Cubewanos
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isCubewano)
					objects.append(object);
			}
			break;
		case 7: // Scattered disc objects
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isSDO)
					objects.append(object);
			}
			break;
		case 8: // Oort cloud objects
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isOCO)
					objects.append(object);
			}
			break;
		default:
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()!=Planet::isUNDEFINED)
					objects.append(object);
			}
			break;
	}

	PlanetP planet = solarSystem->searchByEnglishName(currentPlanet);
	if (planet)
	{
		double currentJD = core->getJD(); // save current JD
		double startJD = StelUtils::qDateTimeToJd(ui->phenomenFromDateEdit->dateTime());
		double stopJD = StelUtils::qDateTimeToJd(ui->phenomenToDateEdit->dateTime());
		startJD = startJD - StelUtils::getGMTShiftFromQT(startJD)/24;
		stopJD = stopJD - StelUtils::getGMTShiftFromQT(stopJD)/24;

		foreach (PlanetP obj, objects)
		{
			// conjunction
			fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation, false), planet, obj, false);
			// opposition
			fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation, true), planet, obj, true);
		}

		core->setJD(currentJD); // restore time
		core->update(0);
	}

	// adjust the column width
	for(int i = 0; i < PhenomenaCount; ++i)
	{
	    ui->phenomenaTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-date
	ui->phenomenaTreeWidget->sortItems(PhenomenaDate, Qt::AscendingOrder);
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const PlanetP object2, bool opposition)
{
	QMap<double, double>::ConstIterator it;
	for (it=list.constBegin(); it!=list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		if (opposition)
		{
			phenomenType = q_("Opposition");
			separation += M_PI;
		}

		ACTreeWidgetItem *treeItem = new ACTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate, StelUtils::jdToQDateTime(it.key() + StelUtils::getGMTShiftFromQT(it.key())/24).toString("yyyy-MM-dd hh:mm:ss"));
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP &object1, PlanetP &object2, double startJD, double stopJD, float maxSeparation, bool opposition)
{
	float dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD)/4.0;
	if (step0>24.8*365.25)
		step0 = 24.8*365.25;

	if (object1->getEnglishName()=="Neptune" || object2->getEnglishName()=="Neptune" || object1->getEnglishName()=="Uranus" || object2->getEnglishName()=="Uranus")
		if (step0 > 3652.5)
			step0 = 3652.5;
	if (object1->getEnglishName()=="Jupiter" || object2->getEnglishName()=="Jupiter" || object1->getEnglishName()=="Saturn" || object2->getEnglishName()=="Saturn")
		if (step0 > 365.25)
			step0 = 365.f;
	if (object1->getEnglishName()=="Mars" || object2->getEnglishName()=="Mars")
		if (step0 > 10.f)
			step0 = 10.f;
	if (object1->getEnglishName()=="Venus" || object2->getEnglishName()=="Venus" || object1->getEnglishName()=="Mercury" || object2->getEnglishName()=="Mercury")
		if (step0 > 5.f)
			step0 = 5.f;
	if (object1->getEnglishName()=="Moon" || object2->getEnglishName()=="Moon")
		if (step0 > 0.25)
			step0 = 0.25;

	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2, opposition);
	jd += step;
	while(jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2, opposition);
		sgn = StelUtils::sign(dist - prevDist);

		double factor = qAbs((dist - prevDist)/dist);
		if (factor>10.f)
			step = step0 * factor/10.f;
		else
			step = step0;

		if (sgn != prevSgn && prevSgn == -1)
		{
			if (step > step0)
			{
				jd -= step;
				step = step0;
				sgn = prevSgn;
				while(jd <= stopJD)
				{
					dist = findDistance(jd, object1, object2, opposition);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn!=prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn, opposition))
			{
				double sep = extremum.second*180./M_PI;
				if (sep<maxSeparation)
					separations.insert(extremum.first, extremum.second);
			}

		}

		prevDist = dist;
		prevSgn = sgn;
		jd += step;
	}

	return separations;
}

bool AstroCalcDialog::findPrecise(QPair<double, double> *out, PlanetP object1, PlanetP object2, double JD, double step, int prevSign, bool opposition)
{
	int sgn;
	double dist, prevDist;

	if (out==NULL)
		return false;

	prevDist = findDistance(JD, object1, object2, opposition);
	step = -step/2.f;
	prevSign = -prevSign;

	while(true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2, opposition);

		if (qAbs(step)< 1.f/1440.f)
		{
			out->first = JD - step/2.0;
			out->second = findDistance(JD - step/2.0, object1, object2, opposition);
			if (out->second < findDistance(JD - 5.0, object1, object2, opposition))
				return true;
			else
				return false;
		}
		sgn = StelUtils::sign(dist - prevDist);
		if (sgn!=prevSign)
		{
			step = -step/2.0;
			sgn = -sgn;
		}
		prevDist = dist;
		prevSign = sgn;
	}
}

double AstroCalcDialog::findDistance(double JD, PlanetP object1, PlanetP object2, bool opposition)
{
	core->setJD(JD);
	core->update(0);
	Vec3d obj1 = object1->getJ2000EquatorialPos(core);
	Vec3d obj2 = object2->getJ2000EquatorialPos(core);
	double angle = obj1.angle(obj2);
	if (opposition)
		angle = M_PI - angle;
	return angle;
}
