/*
 * Stellarium
 * Copyright (C) 2015 Alexander Wolf
 * Copyright (C) 2016 Nick Fedoseev (visualization of ephemeris)
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
#include "StelFileMgr.hpp"

#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"

#include "AstroCalcDialog.hpp"
#include "ui_astroCalcDialog.h"
#include "external/qcustomplot/qcustomplot.h"

#include <QFileDialog>
#include <QDir>

QVector<Vec3d> AstroCalcDialog::EphemerisListJ2000;
QVector<QString> AstroCalcDialog::EphemerisListDates;
QVector<float> AstroCalcDialog::EphemerisListMagnitudes;
int AstroCalcDialog::DisplayedPositionIndex = -1;
float AstroCalcDialog::brightLimit = 10.f;
float AstroCalcDialog::minY = -90.f;
float AstroCalcDialog::maxY = 90.f;

AstroCalcDialog::AstroCalcDialog(QObject *parent)
	: StelDialog("AstroCalc",parent)
	, currentTimeLine(NULL)
	, delimiter(", ")
	, acEndl("\n")
{
	ui = new Ui_astroCalcDialogForm;
	core = StelApp::getInstance().getCore();
	solarSystem = GETSTELMODULE(SolarSystem);
	dsoMgr = GETSTELMODULE(NebulaMgr);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	starMgr = GETSTELMODULE(StarMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();
	conf = StelApp::getInstance().getSettings();
	ephemerisHeader.clear();
	phenomenaHeader.clear();
	positionsHeader.clear();
}

AstroCalcDialog::~AstroCalcDialog()
{
	if (currentTimeLine)
	{
		currentTimeLine->stop();
		delete currentTimeLine;
		currentTimeLine = NULL;
	}
	delete ui;
}

void AstroCalcDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setPlanetaryPositionsHeaderNames();
		setCelestialPositionsHeaderNames();
		setEphemerisHeaderNames();
		setPhenomenaHeaderNames();
		populateCelestialBodyList();
		populateCelestialCategoryList();
		populateEphemerisTimeStepsList();
		populateMajorPlanetList();
		populateGroupCelestialBodyList();		
		currentPlanetaryPositions();
		currentCelestialPositions();
		prepareAxesAndGraph();
		drawAltVsTimeDiagram();
		populateTimeIntervalsList();
		populateWutGroups();
		//Hack to shrink the tabs to optimal size after language change
		//by causing the list items to be laid out again.
		updateTabBarListWidgetWidth();		
	}
}

void AstroCalcDialog::styleChanged()
{
	// Nothing for now
}

void AstroCalcDialog::createDialogContent()
{
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	// Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->planetaryPositionsTreeWidget;
	installKineticScrolling(addscroll);
	acEndl="\r\n";
#else
	acEndl="\n";
#endif

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	initListPlanetaryPositions();
	initListCelestialPositions();
	initListEphemeris();
	initListPhenomena();
	populateCelestialBodyList();
	populateCelestialCategoryList();
	populateEphemerisTimeStepsList();
	populateMajorPlanetList();
	populateGroupCelestialBodyList();
	// Altitude vs. Time feature
	prepareAxesAndGraph();
	drawCurrentTimeDiagram();
	// WUT
	populateTimeIntervalsList();
	populateWutGroups();

	double JD = core->getJD() + core->getUTCOffset(core->getJD())/24;
	QDateTime currentDT = StelUtils::jdToQDateTime(JD);
	ui->dateFromDateTimeEdit->setDateTime(currentDT);
	ui->dateToDateTimeEdit->setDateTime(currentDT.addMonths(1));
	ui->phenomenFromDateEdit->setDateTime(currentDT);
	ui->phenomenToDateEdit->setDateTime(currentDT.addYears(1));

	// TODO: Switch a QDateTimeEdit to StelDateTimeEdit widget to apply wide range of dates
	QDate min = QDate(100,1,1);
	ui->dateFromDateTimeEdit->setMinimumDate(min);
	ui->dateToDateTimeEdit->setMinimumDate(min);
	ui->phenomenFromDateEdit->setMinimumDate(min);
	ui->phenomenToDateEdit->setMinimumDate(min);

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(ui->planetaryPositionsTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		ui->planetaryPositionsTreeWidget, SLOT(repaint()));
	connect(ui->celestialPositionsTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		ui->celestialPositionsTreeWidget, SLOT(repaint()));

	ui->planetaryMagnitudeDoubleSpinBox->setValue(conf->value("astrocalc/positions_magnitude_limit", 12.0).toDouble());
	connect(ui->planetaryMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(savePlanetaryPositionsMagnitudeLimit(double)));

	ui->aboveHorizonCheckBox->setChecked(conf->value("astrocalc/flag_positions_above_horizon", false).toBool());
	connect(ui->aboveHorizonCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePlanetaryPositionsAboveHorizonFlag(bool)));

	connect(ui->planetaryPositionsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentPlanetaryPosition(QModelIndex)));
	connect(ui->planetaryPositionsUpdateButton, SIGNAL(clicked()), this, SLOT(currentPlanetaryPositions()));

	ui->celestialMagnitudeDoubleSpinBox->setValue(conf->value("astrocalc/celestial_magnitude_limit", 6.0).toDouble());
	connect(ui->celestialMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveCelestialPositionsMagnitudeLimit(double)));

	ui->horizontalCoordinatesCheckBox->setChecked(conf->value("astrocalc/flag_horizontal_coordinates", false).toBool());
	connect(ui->horizontalCoordinatesCheckBox, SIGNAL(toggled(bool)), this, SLOT(saveCelestialPositionsHorizontalCoordinatesFlag(bool)));

	connect(ui->celestialPositionsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentCelestialPosition(QModelIndex)));
	connect(ui->celestialPositionsUpdateButton, SIGNAL(clicked()), this, SLOT(currentCelestialPositions()));
	connect(ui->celestialCategoryComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveCelestialPositionsCategory(int)));

	connect(ui->ephemerisPushButton, SIGNAL(clicked()), this, SLOT(generateEphemeris()));
	connect(ui->ephemerisCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupEphemeris()));
	connect(ui->ephemerisSaveButton, SIGNAL(clicked()), this, SLOT(saveEphemeris()));
	connect(ui->ephemerisTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentEphemeride(QModelIndex)));
	connect(ui->ephemerisTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(onChangedEphemerisPosition(QModelIndex)));
	connect(ui->ephemerisStepComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisTimeStep(int)));
	connect(ui->celestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(saveEphemerisCelestialBody(int)));

	ui->phenomenaOppositionCheckBox->setChecked(conf->value("astrocalc/flag_phenomena_opposition", false).toBool());
	connect(ui->phenomenaOppositionCheckBox, SIGNAL(toggled(bool)), this, SLOT(savePhenomenaOppositionFlag(bool)));

	connect(ui->phenomenaPushButton, SIGNAL(clicked()), this, SLOT(calculatePhenomena()));
	connect(ui->phenomenaCleanupButton, SIGNAL(clicked()), this, SLOT(cleanupPhenomena()));
	connect(ui->phenomenaTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentPhenomen(QModelIndex)));
	connect(ui->phenomenaSaveButton, SIGNAL(clicked()), this, SLOT(savePhenomena()));
	connect(ui->object1ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(savePhenomenaCelestialBody(int)));
	connect(ui->object2ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(savePhenomenaCelestialGroup(int)));

	connect(ui->altVsTimePlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseOverLine(QMouseEvent*)));
	connect(objectMgr, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(drawAltVsTimeDiagram()));
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(drawAltVsTimeDiagram()));
	connect(core, SIGNAL(dateChanged()), this, SLOT(drawAltVsTimeDiagram()));
	drawAltVsTimeDiagram();

	connectBoolProperty(ui->ephemerisShowMarkersCheckBox, "SolarSystem.ephemerisMarkersDisplayed");
	connectBoolProperty(ui->ephemerisShowDatesCheckBox, "SolarSystem.ephemerisDatesDisplayed");
	connectBoolProperty(ui->ephemerisShowMagnitudesCheckBox, "SolarSystem.ephemerisMagnitudesDisplayed");

	ui->wutMagnitudeDoubleSpinBox->setValue(conf->value("astrocalc/wut_magnitude_limit", 10.0).toDouble());
	connect(ui->wutMagnitudeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(saveWutMagnitudeLimit(double)));
	connect(ui->wutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(calculateWutObjects()));
	connect(ui->wutCategoryListWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(calculateWutObjects()));
	connect(ui->wutMatchingObjectsListWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(selectWutObject()));	
	connect(dsoMgr, SIGNAL(catalogFiltersChanged(Nebula::CatalogGroup)), this, SLOT(calculateWutObjects()));

	currentPlanetaryPositions();
	currentCelestialPositions();

	currentTimeLine = new QTimer(this);
	connect(currentTimeLine, SIGNAL(timeout()), this, SLOT(drawCurrentTimeDiagram()));
	currentTimeLine->start(500); // Update 'now' line position every 0.5 seconds

	connect(solarSystem, SIGNAL(solarSystemDataReloaded()), this, SLOT(updateSolarSystemData()));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
}

void AstroCalcDialog::savePlanetaryPositionsAboveHorizonFlag(bool b)
{
	conf->setValue("astrocalc/flag_positions_above_horizon", b);
	// Refresh the planetary positions table
	currentPlanetaryPositions();
}

void AstroCalcDialog::savePlanetaryPositionsMagnitudeLimit(double mag)
{
	conf->setValue("astrocalc/positions_magnitude_limit", QString::number(mag, 'f', 2));
	// Refresh the planetary positions table
	currentPlanetaryPositions();
}

void AstroCalcDialog::initListPlanetaryPositions()
{
	ui->planetaryPositionsTreeWidget->clear();
	ui->planetaryPositionsTreeWidget->setColumnCount(ColumnCount);

	setPlanetaryPositionsHeaderNames();

	ui->planetaryPositionsTreeWidget->header()->setSectionsMovable(false);
}

void AstroCalcDialog::initListCelestialPositions()
{
	ui->celestialPositionsTreeWidget->clear();
	ui->celestialPositionsTreeWidget->setColumnCount(CColumnCount);

	setCelestialPositionsHeaderNames();

	ui->celestialPositionsTreeWidget->header()->setSectionsMovable(false);
}


void AstroCalcDialog::setPlanetaryPositionsHeaderNames()
{
	positionsHeader.clear();
	//TRANSLATORS: name of object
	positionsHeader << q_("Name");
	//TRANSLATORS: right ascension
	positionsHeader << q_("RA (J2000)");
	//TRANSLATORS: declination
	positionsHeader << q_("Dec (J2000)");
	//TRANSLATORS: magnitude
	positionsHeader << q_("mag");
	//TRANSLATORS: type of object
	positionsHeader << q_("Type");
	ui->planetaryPositionsTreeWidget->setHeaderLabels(positionsHeader);

	// adjust the column width
	for(int i = 0; i < ColumnCount; ++i)
	{
	    ui->planetaryPositionsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::setCelestialPositionsHeaderNames()
{
	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	int celType = category->itemData(category->currentIndex()).toInt();

	bool horizon = ui->horizontalCoordinatesCheckBox->isChecked();

	positionsHeader.clear();
	//TRANSLATORS: name of object
	positionsHeader << q_("Name");
	if (horizon)
	{
		//TRANSLATORS: azimuth
		positionsHeader << q_("Azimuth");
		//TRANSLATORS: altitude
		positionsHeader << q_("Altitude");
	}
	else
	{
		//TRANSLATORS: right ascension
		positionsHeader << q_("RA (J2000)");
		//TRANSLATORS: declination
		positionsHeader << q_("Dec (J2000)");
	}
	if (celType==12 || celType==102 || celType==111) // check for dark nebulae
	{
		//TRANSLATORS: opacity
		positionsHeader << q_("opacity");
	}
	else
	{
		//TRANSLATORS: magnitude
		positionsHeader << q_("mag");
	}
	if (celType==170)
	{
		//TRANSLATORS: separation, arcseconds
		positionsHeader << QString("%1, \"").arg(q_("Sep."));
	}
	else if (celType==171)
	{
		//TRANSLATORS: period, days
		positionsHeader << QString("%1, %2").arg(q_("Per."), qc_("d", "days"));
	}
	else
	{
		//TRANSLATORS: surface brightness
		positionsHeader << q_("S.B.");
	}
	//TRANSLATORS: type of object
	positionsHeader << q_("Type");

	ui->celestialPositionsTreeWidget->setHeaderLabels(positionsHeader);
	// adjust the column width
	for(int i = 0; i < CColumnCount; ++i)
	{
	    ui->celestialPositionsTreeWidget->resizeColumnToContents(i);
	}
}

void AstroCalcDialog::currentPlanetaryPositions()
{
	float ra, dec;
	QList<PlanetP> allPlanets = solarSystem->getAllPlanets();

	initListPlanetaryPositions();

	PlanetP sun = solarSystem->getSun();
	double mag = ui->planetaryMagnitudeDoubleSpinBox->value();
	bool horizon = ui->aboveHorizonCheckBox->isChecked();

	StelCore* core = StelApp::getInstance().getCore();	
	double JD = core->getJD();
	ui->planetaryPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));

	foreach (const PlanetP& planet, allPlanets)
	{
		if ((planet->getPlanetType()!=Planet::isUNDEFINED && planet!=sun && planet!=core->getCurrentPlanet()) && planet->getVMagnitudeWithExtinction(core)<=mag)
		{
			if (horizon && !planet->isAboveRealHorizon(core))
				continue;

			StelUtils::rectToSphe(&ra,&dec,planet->getJ2000EquatorialPos(core));
			ACPlanPosTreeWidgetItem *treeItem = new ACPlanPosTreeWidgetItem(ui->planetaryPositionsTreeWidget);
			treeItem->setText(ColumnName, planet->getNameI18n());
			treeItem->setText(ColumnRA, StelUtils::radToHmsStr(ra));
			treeItem->setTextAlignment(ColumnRA, Qt::AlignRight);
			treeItem->setText(ColumnDec, StelUtils::radToDmsStr(dec, true));
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

void AstroCalcDialog::populateCelestialCategoryList()
{
	Q_ASSERT(ui->celestialCategoryComboBox);

	QComboBox* category = ui->celestialCategoryComboBox;


	category->blockSignals(true);
	int index = category->currentIndex();
	QVariant selectedCategoryId = category->itemData(index);

	category->clear();
	// TODO: Automatic sync list with QMap<QString, QString> StelObjectMgr::objectModulesMap() data
	category->addItem(q_("Galaxies"), "0");
	category->addItem(q_("Active galaxies"), "1");
	category->addItem(q_("Radio galaxies"), "2");
	category->addItem(q_("Interacting galaxies"), "3");
	category->addItem(q_("Bright quasars"), "4");
	category->addItem(q_("Star clusters"), "5");
	category->addItem(q_("Open star clusters"), "6");
	category->addItem(q_("Globular star clusters"), "7");
	category->addItem(q_("Stellar associations"), "8");
	category->addItem(q_("Star clouds"), "9");
	category->addItem(q_("Nebulae"), "10");
	category->addItem(q_("Planetary nebulae"), "11");
	category->addItem(q_("Dark nebulae"), "12");
	category->addItem(q_("Reflection nebulae"), "13");
	category->addItem(q_("Bipolar nebulae"), "14");
	category->addItem(q_("Emission nebulae"), "15");
	category->addItem(q_("Clusters associated with nebulosity"), "16");
	category->addItem(q_("HII regions"), "17");
	category->addItem(q_("Supernova remnants"), "18");
	category->addItem(q_("Interstellar matter"), "19");
	category->addItem(q_("Emission objects"), "20");
	category->addItem(q_("BL Lac objects"), "21");
	category->addItem(q_("Blazars"), "22");
	category->addItem(q_("Molecular Clouds"), "23");
	category->addItem(q_("Young Stellar Objects"), "24");
	category->addItem(q_("Possible Quasars"), "25");
	category->addItem(q_("Possible Planetary Nebulae"), "26");
	category->addItem(q_("Protoplanetary Nebulae"), "27");
	category->addItem(q_("Messier Catalogue"), "100");
	category->addItem(q_("Caldwell Catalogue"), "101");
	category->addItem(q_("Barnard Catalogue"), "102");
	category->addItem(q_("Sharpless Catalogue"), "103");
	category->addItem(q_("Van den Bergh Catalogue"), "104");
	category->addItem(q_("The Catalogue of Rodgers, Campbell, and Whiteoak"), "105");
	category->addItem(q_("Collinder Catalogue"), "106");
	category->addItem(q_("Melotte Catalogue"), "107");
	category->addItem(q_("New General Catalogue"), "108");
	category->addItem(q_("Index Catalogue"), "109");
	category->addItem(q_("Lynds' Catalogue of Bright Nebulae"), "110");
	category->addItem(q_("Lynds' Catalogue of Dark Nebulae"), "111");
	category->addItem(q_("Principal Galaxy Catalog"), "112");
	category->addItem(q_("The Uppsala General Catalogue of Galaxies"), "113");
	category->addItem(q_("Cederblad Catalog"), "114");
	category->addItem(q_("Dwarf galaxies"), "150");
	category->addItem(q_("Herschel 400 Catalogue"), "151");
	category->addItem(q_("Bright double stars"), "170");
	category->addItem(q_("Bright variable stars"), "171");

	index = category->findData(selectedCategoryId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
	{
		// default step: Messier Catalogue
		index = category->findData(conf->value("astrocalc/celestial_category", "100").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	category->setCurrentIndex(index);
	category->model()->sort(0);
	category->blockSignals(false);
}

void AstroCalcDialog::saveCelestialPositionsMagnitudeLimit(double mag)
{
	conf->setValue("astrocalc/celestial_magnitude_limit", QString::number(mag, 'f', 2));
	// Refresh the celestial bodies positions table
	currentCelestialPositions();
}

void AstroCalcDialog::saveCelestialPositionsHorizontalCoordinatesFlag(bool b)
{
	conf->setValue("astrocalc/flag_horizontal_coordinates", b);
	// Refresh the celestial bodies positions table
	currentCelestialPositions();
}

void AstroCalcDialog::saveCelestialPositionsCategory(int index)
{
	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	conf->setValue("astrocalc/celestial_category", category->itemData(index).toInt());
	// Refresh the celestial bodies positions table
	currentCelestialPositions();
}

void AstroCalcDialog::currentCelestialPositions()
{
	float ra, dec;
	QString raStr, decStr, extra, celObjName = "", celObjId = "";

	initListCelestialPositions();

	double mag = ui->celestialMagnitudeDoubleSpinBox->value();
	bool horizon = ui->horizontalCoordinatesCheckBox->isChecked();
	bool useSouthAzimuth = StelApp::getInstance().getFlagSouthAzimuthUsage();

	StelCore* core = StelApp::getInstance().getCore();
	double JD = core->getJD();
	ui->celestialPositionsTimeLabel->setText(q_("Positions on %1").arg(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD))));

	Q_ASSERT(ui->celestialCategoryComboBox);
	QComboBox* category = ui->celestialCategoryComboBox;
	QString celType = category->itemData(category->currentIndex()).toString();
	int celTypeId = celType.toInt();

	if (celTypeId<170)
	{
		QString mu = QString("<sup>m</sup>/%1'").arg(QChar(0x2B1C));
		if (dsoMgr->getFlagSurfaceBrightnessArcsecUsage())
			mu = QString("<sup>m</sup>/%1\"").arg(QChar(0x2B1C));
		// Deep-sky objects
		QList<NebulaP> celestialObjects = dsoMgr->getDeepSkyObjectsByType(celType);
		foreach (const NebulaP& obj, celestialObjects)
		{
			if (obj->getVMagnitudeWithExtinction(core)<=mag && obj->isAboveRealHorizon(core))
			{
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction*M_PI - ra;
					if (ra > M_PI*2)
						ra -= M_PI*2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getJ2000EquatorialPos(core));
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				ACCelPosTreeWidgetItem *treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);

				celObjName = obj->getNameI18n();
				celObjId = obj->getDSODesignation();
				if (celObjName.isEmpty())
					celObjName = celObjId;

				extra = QString::number(obj->getSurfaceBrightnessWithExtinction(core), 'f', 2);
				if (extra.toFloat()>90.f)
					extra = QChar(0x2014);

				treeItem->setText(CColumnName, celObjName);
				treeItem->setToolTip(CColumnName, celObjId);
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, mu);
				treeItem->setText(CColumnType, q_(obj->getTypeString()));
			}
		}
	}
	else
	{
		// stars
		QString sType = "star";
		QList<starData> celestialObjects;
		if (celTypeId==170)
		{
			// double stars
			celestialObjects = starMgr->getHipparcosDoubleStars();
			sType = "double star";
		}
		else
		{
			// variable stars
			celestialObjects = starMgr->getHipparcosVariableStars();
			sType = "variable star";
		}

		foreach (const starData& star, celestialObjects)
		{
			StelObjectP obj = star.firstKey();
			if (obj->getVMagnitudeWithExtinction(core)<=mag && obj->isAboveRealHorizon(core))
			{
				if (horizon)
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getAltAzPosAuto(core));
					float direction = 3.; // N is zero, E is 90 degrees
					if (useSouthAzimuth)
						direction = 2.;
					ra = direction*M_PI - ra;
					if (ra > M_PI*2)
						ra -= M_PI*2;
					raStr = StelUtils::radToDmsStr(ra, true);
					decStr = StelUtils::radToDmsStr(dec, true);
				}
				else
				{
					StelUtils::rectToSphe(&ra, &dec, obj->getJ2000EquatorialPos(core));
					raStr = StelUtils::radToHmsStr(ra);
					decStr = StelUtils::radToDmsStr(dec, true);
				}

				if (celTypeId==170)
					extra = QString::number(star.value(obj), 'f', 3); // arcseconds
				else
				{
					if (star.value(obj)>0.f)
						extra = QString::number(star.value(obj), 'f', 5); // days
					else
						extra = QChar(0x2014);
				}


				ACCelPosTreeWidgetItem *treeItem = new ACCelPosTreeWidgetItem(ui->celestialPositionsTreeWidget);
				treeItem->setText(CColumnName, obj->getNameI18n());
				treeItem->setToolTip(CColumnName, "");
				treeItem->setText(CColumnRA, raStr);
				treeItem->setTextAlignment(CColumnRA, Qt::AlignRight);
				treeItem->setText(CColumnDec, decStr);
				treeItem->setTextAlignment(CColumnDec, Qt::AlignRight);
				treeItem->setText(CColumnMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
				treeItem->setTextAlignment(CColumnMagnitude, Qt::AlignRight);
				treeItem->setText(CColumnExtra, extra);
				treeItem->setTextAlignment(CColumnExtra, Qt::AlignRight);
				treeItem->setToolTip(CColumnExtra, "");
				treeItem->setText(CColumnType, q_(sType));
			}
		}
	}

	// adjust the column width
	for(int i = 0; i < CColumnCount; ++i)
	{
	    ui->celestialPositionsTreeWidget->resizeColumnToContents(i);
	}

	// sort-by-name
	ui->celestialPositionsTreeWidget->sortItems(CColumnName, Qt::AscendingOrder);
}

void AstroCalcDialog::selectCurrentCelestialPosition(const QModelIndex &modelIndex)
{
	// Find the object
	QString nameI18n = modelIndex.sibling(modelIndex.row(), CColumnName).data().toString();

	if (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n))
	{
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
			mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
			mvmgr->setFlagTracking(true);
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
	ephemerisHeader.clear();
	ephemerisHeader << q_("Date and Time");
	ephemerisHeader << q_("Julian Day");
	//TRANSLATORS: right ascension
	ephemerisHeader << q_("RA (J2000)");
	//TRANSLATORS: declination
	ephemerisHeader << q_("Dec (J2000)");
	//TRANSLATORS: magnitude
	ephemerisHeader << q_("mag");
	//TRANSLATORS: phase
	ephemerisHeader << q_("phase");
	//TRANSLATORS: distance
	ephemerisHeader << q_("dist.");
	//TRANSLATORS: elongation
	ephemerisHeader << q_("elong.");
	ui->ephemerisTreeWidget->setHeaderLabels(ephemerisHeader);

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
	Vec3d observerHelioPos;
	QString currentPlanet = ui->celestialBodyComboBox->currentData().toString();
	QString distanceInfo = q_("Planetocentric distance");
	if (core->getUseTopocentricCoordinates())
		distanceInfo = q_("Topocentric distance");
	QString distanceUM = q_("AU");

	initListEphemeris();

	switch (ui->ephemerisStepComboBox->currentData().toInt()) {
		case 1:
			currentStep = 10 * StelCore::JD_MINUTE;
			break;
		case 2:
			currentStep = 30 * StelCore::JD_MINUTE;
			break;
		case 3:
			currentStep = StelCore::JD_HOUR;
			break;
		case 4:
			currentStep = 6 * StelCore::JD_HOUR;
			break;
		case 5:
			currentStep = 12 * StelCore::JD_HOUR;
			break;
		case 6:
			currentStep = StelCore::JD_DAY;
			break;
		case 7:
			currentStep = 5 * StelCore::JD_DAY;
			break;
		case 8:
			currentStep = 10 * StelCore::JD_DAY;
			break;
		case 9:
			currentStep = 15 * StelCore::JD_DAY;
			break;
		case 10:
			currentStep = 30 * StelCore::JD_DAY;
			break;
		case 11:
			currentStep = 60 * StelCore::JD_DAY;
			break;
		default:
			currentStep = StelCore::JD_DAY;
			break;
	}

	PlanetP obj = solarSystem->searchByEnglishName(currentPlanet);
	if (obj)
	{
		double currentJD = core->getJD(); // save current JD
		double firstJD = StelUtils::qDateTimeToJd(ui->dateFromDateTimeEdit->dateTime());
		firstJD = firstJD - core->getUTCOffset(firstJD)/24;
		int elements = (int)((StelUtils::qDateTimeToJd(ui->dateToDateTimeEdit->dateTime()) - firstJD)/currentStep);
		EphemerisListJ2000.clear();
		EphemerisListJ2000.reserve(elements);
		EphemerisListDates.clear();
		EphemerisListDates.reserve(elements);
		EphemerisListMagnitudes.clear();
		EphemerisListMagnitudes.reserve(elements);
		bool withTime = false;
		if (currentStep<StelCore::JD_DAY)
			withTime = true;
		for (int i=0; i<elements; i++)
		{
			double JD = firstJD + i*currentStep;
			core->setJD(JD);
			core->update(0); // force update to get new coordinates			
			Vec3d pos = obj->getJ2000EquatorialPos(core);
			EphemerisListJ2000.append(pos);
			if (withTime)
				EphemerisListDates.append(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
			else
				EphemerisListDates.append(localeMgr->getPrintableDateLocal(JD));
			EphemerisListMagnitudes.append(obj->getVMagnitudeWithExtinction(core));
			StelUtils::rectToSphe(&ra,&dec,pos);

			observerHelioPos = core->getObserverHeliocentricEclipticPos();

			ACEphemTreeWidgetItem *treeItem = new ACEphemTreeWidgetItem(ui->ephemerisTreeWidget);
			// local date and time
			treeItem->setText(EphemerisDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(JD), localeMgr->getPrintableTimeLocal(JD)));
			treeItem->setText(EphemerisJD, QString::number(JD, 'f', 5));
			treeItem->setText(EphemerisRA, StelUtils::radToHmsStr(ra));
			treeItem->setTextAlignment(EphemerisRA, Qt::AlignRight);
			treeItem->setText(EphemerisDec, StelUtils::radToDmsStr(dec, true));
			treeItem->setTextAlignment(EphemerisDec, Qt::AlignRight);
			treeItem->setText(EphemerisMagnitude, QString::number(obj->getVMagnitudeWithExtinction(core), 'f', 2));
			treeItem->setTextAlignment(EphemerisMagnitude, Qt::AlignRight);
			treeItem->setText(EphemerisPhase, QString::number(obj->getPhase(observerHelioPos) * 100, 'f', 2) + "%");
			treeItem->setTextAlignment(EphemerisPhase, Qt::AlignRight);
			treeItem->setText(EphemerisDistance, QString::number(obj->getJ2000EquatorialPos(core).length(), 'f', 6));
			treeItem->setTextAlignment(EphemerisDistance, Qt::AlignRight);
			treeItem->setToolTip(EphemerisDistance, QString("%1, %2").arg(distanceInfo, distanceUM));
			treeItem->setText(EphemerisElongation, StelUtils::radToDmsStr(obj->getElongation(observerHelioPos), true));
			treeItem->setTextAlignment(EphemerisElongation, Qt::AlignRight);
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

void AstroCalcDialog::saveEphemeris()
{
	QString filter = q_("CSV (Comma delimited)");
	filter.append(" (*.csv)");
	QString filePath = QFileDialog::getSaveFileName(0, q_("Save calculated ephemerides as..."), QDir::homePath() + "/ephemeris.csv", filter);
	QFile ephem(filePath);
	if (!ephem.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file"
			   << QDir::toNativeSeparators(filePath);
		return;
	}

	QTextStream ephemList(&ephem);
	ephemList.setCodec("UTF-8");

	int count = ui->ephemerisTreeWidget->topLevelItemCount();

	ephemList << ephemerisHeader.join(delimiter) << acEndl;
	for (int i = 0; i < count; i++)
	{
		int columns = ephemerisHeader.size();
		for (int j=0; j<columns; j++)
		{
			ephemList << ui->ephemerisTreeWidget->topLevelItem(i)->text(j);
			if (j<columns-1)
				ephemList << delimiter;
			else
				ephemList << acEndl;
		}
	}

	ephem.close();
}

void AstroCalcDialog::cleanupEphemeris()
{
	EphemerisListJ2000.clear();
	ui->ephemerisTreeWidget->clear();
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
		if (!name.contains("Observer", Qt::CaseInsensitive) && name!="Sun" && name!=core->getCurrentPlanet()->getEnglishName())
			planets->addItem(trans.qtranslate(name), name);
	}
	//Restore the selection
	index = planets->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = planets->findData(conf->value("astrocalc/ephemeris_celestial_body", "Moon").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	planets->setCurrentIndex(index);
	planets->model()->sort(0);
	planets->blockSignals(false);
}

void AstroCalcDialog::saveEphemerisCelestialBody(int index)
{
	Q_ASSERT(ui->celestialBodyComboBox);
	QComboBox* planets = ui->celestialBodyComboBox;
	conf->setValue("astrocalc/ephemeris_celestial_body", planets->itemData(index).toString());
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
	steps->addItem(q_("30 minutes"), "2");
	steps->addItem(q_("1 hour"), "3");
	steps->addItem(q_("6 hours"), "4");
	steps->addItem(q_("12 hours"), "5");
	steps->addItem(q_("1 day"), "6");
	steps->addItem(q_("5 days"), "7");
	steps->addItem(q_("10 days"), "8");
	steps->addItem(q_("15 days"), "9");
	steps->addItem(q_("30 days"), "10");
	steps->addItem(q_("60 days"), "11");

	index = steps->findData(selectedStepId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
	{
		// default step: one day
		index = steps->findData(conf->value("astrocalc/ephemeris_time_step", "6").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	steps->setCurrentIndex(index);
	steps->blockSignals(false);
}

void AstroCalcDialog::saveEphemerisTimeStep(int index)
{
	Q_ASSERT(ui->ephemerisStepComboBox);
	QComboBox* steps = ui->ephemerisStepComboBox;
	conf->setValue("astrocalc/ephemeris_time_step", steps->itemData(index).toInt());
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
		// major planets and the Sun
		if ((planet->getPlanetType()==Planet::isPlanet || planet->getPlanetType()==Planet::isStar) && planet->getEnglishName()!=core->getCurrentPlanet()->getEnglishName())
			majorPlanet->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());

		// moons of the current planet
		if (planet->getPlanetType()==Planet::isMoon && planet->getEnglishName()!=core->getCurrentPlanet()->getEnglishName() && planet->getParent()==core->getCurrentPlanet())
			majorPlanet->addItem(trans.qtranslate(planet->getNameI18n()), planet->getEnglishName());

	}	
	//Restore the selection
	index = majorPlanet->findData(selectedPlanetId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = majorPlanet->findData(conf->value("astrocalc/phenomena_celestial_body", "Venus").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	majorPlanet->setCurrentIndex(index);
	majorPlanet->model()->sort(0);
	majorPlanet->blockSignals(false);
}

void AstroCalcDialog::savePhenomenaCelestialBody(int index)
{
	Q_ASSERT(ui->object1ComboBox);
	QComboBox* planets = ui->object1ComboBox;
	conf->setValue("astrocalc/phenomena_celestial_body", planets->itemData(index).toString());
}

void AstroCalcDialog::populateGroupCelestialBodyList()
{
	Q_ASSERT(ui->object2ComboBox);

	QComboBox* groups = ui->object2ComboBox;
	groups->blockSignals(true);
	int index = groups->currentIndex();
	QVariant selectedGroupId = groups->itemData(index);

	QString brLimit = QString::number(brightLimit, 'f', 1);
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
	groups->addItem(q_("Sednoids"), "9");
	groups->addItem(q_("Bright stars (<%1 mag)").arg(QString::number(brightLimit-5.0f, 'f', 1)), "10");
	groups->addItem(q_("Bright double stars (<%1 mag)").arg(QString::number(brightLimit-5.0f, 'f', 1)), "11");
	groups->addItem(q_("Bright variable stars (<%1 mag)").arg(QString::number(brightLimit-5.0f, 'f', 1)), "12");
	groups->addItem(q_("Bright star clusters (<%1 mag)").arg(brLimit), "13");
	groups->addItem(q_("Planetary nebulae"), "14");
	groups->addItem(q_("Bright nebulae (<%1 mag)").arg(brLimit), "15");
	groups->addItem(q_("Dark nebulae"), "16");
	groups->addItem(q_("Bright galaxies (<%1 mag)").arg(brLimit), "17");

	index = groups->findData(selectedGroupId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = groups->findData(conf->value("astrocalc/phenomena_celestial_group", "1").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	groups->setCurrentIndex(index);
	groups->model()->sort(0);
	groups->blockSignals(false);
}

void AstroCalcDialog::savePhenomenaCelestialGroup(int index)
{
	Q_ASSERT(ui->object2ComboBox);
	QComboBox* group = ui->object2ComboBox;
	conf->setValue("astrocalc/phenomena_celestial_group", group->itemData(index).toInt());
}

void AstroCalcDialog::cleanupPhenomena()
{
	ui->phenomenaTreeWidget->clear();
}

void AstroCalcDialog::savePhenomenaOppositionFlag(bool b)
{
	conf->setValue("astrocalc/flag_phenomena_opposition", b);
}

void AstroCalcDialog::drawAltVsTimeDiagram()
{
	QList<StelObjectP> selectedObjects = objectMgr->getSelectedObject();
	if (!selectedObjects.isEmpty())
	{
		// X axis - time; Y axis - altitude
		QList<double> aX, aY;

		StelObjectP selectedObject = selectedObjects[0];

		double currentJD = core->getJD();
		double noon = (int)currentJD;
		double az, alt, deg;
		bool sign;

		double shift = core->getUTCOffset(currentJD)/24.0;
		for(int i=-5;i<=485;i++) // 24 hours + 15 minutes in both directions
		{
			// A new point on the graph every 3 minutes with shift to right 12 hours
			// to get midnight at the center of diagram (i.e. accuracy is 3 minutes)
			double ltime = i*180 + 43200;
			aX.append(ltime);
			double JD = noon + ltime/86400 - shift - 0.5;
			core->setJD(JD);
			StelUtils::rectToSphe(&az, &alt, selectedObject->getAltAzPosAuto(core));
			StelUtils::radToDecDeg(alt, sign, deg);
			if (!sign)
				deg *= -1;
			aY.append(deg);
			core->update(0.0);
		}
		core->setJD(currentJD);

		double dec_sidereal, ra_sidereal, ha_sidereal;
		StelUtils::rectToSphe(&ra_sidereal, &dec_sidereal, selectedObject->getSiderealPosGeometric(core));
		ha_sidereal = (2.*M_PI-ra_sidereal)*12/M_PI;
		if (ha_sidereal>24.0)
			ha_sidereal -= 24.0;

		QVector<double> x = aX.toVector(), y = aY.toVector();

		double minYa = aY.first();
		double maxYa = aY.first();

		foreach (double temp, aY)
		{
			if(maxYa < temp) maxYa = temp;
			if(minYa > temp) minYa = temp;
		}

		minY = minYa - 2.0;
		maxY = maxYa + 2.0;

		prepareAxesAndGraph();
		drawCurrentTimeDiagram();

		QString name = selectedObject->getNameI18n();
		if (name.isEmpty() && selectedObject->getType()=="Nebula")
			name = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();

		// FIXME: Satellites have different values for equatorial coordinates and same values for horizontal coordinates - a caching?
		// NOTE: Drawing a line of transit time was added to else block to avoid troubles with satellites.
		if (selectedObject->getType()=="Satellite")
		{
			x.clear();
			y.clear();
		}
		else
			drawTransitTimeDiagram((currentJD + 0.5 - (int)currentJD + shift) * 24.0 - ha_sidereal);

		ui->altVsTimePlot->graph(0)->setData(x, y);
		ui->altVsTimePlot->graph(0)->setName(name);
		ui->altVsTimePlot->replot();
	}
}

// Added vertical line indicating "now"
void AstroCalcDialog::drawCurrentTimeDiagram()
{
	double currentJD = core->getJD();
	double now = ((currentJD + 0.5 - (int)currentJD) * 86400.0) + core->getUTCOffset(currentJD)*3600.0;
	if (now>129600)
		now -= 86400;
	if (now<43200)
		now += 86400;
	QList<double> ax, ay;
	ax.append(now);
	ax.append(now);
	ay.append(minY);
	ay.append(maxY);
	QVector<double> x = ax.toVector(), y = ay.toVector();	
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(1)->setData(x, y);
	ui->altVsTimePlot->graph(1)->setPen(QPen(Qt::yellow, 1));
	ui->altVsTimePlot->graph(1)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(1)->setName("[Now]");

	ui->altVsTimePlot->replot();
}

// Added vertical line indicating time of transit
void AstroCalcDialog::drawTransitTimeDiagram(double transitTime)
{
	double transit = transitTime * 3600.0;
	if (transit>129600)
		transit -= 86400;
	if (transit<43200)
		transit += 86400;
	QList<double> ax, ay;
	ax.append(transit);
	ax.append(transit);
	ay.append(minY);
	ay.append(maxY);
	QVector<double> x = ax.toVector(), y = ay.toVector();
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->graph(2)->setData(x, y);
	ui->altVsTimePlot->graph(2)->setPen(QPen(Qt::cyan, 1));
	ui->altVsTimePlot->graph(2)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(2)->setName("[Transit]");

	ui->altVsTimePlot->replot();
}

void AstroCalcDialog::prepareAxesAndGraph()
{
	QString xAxisStr = q_("Local Time");
	QString yAxisStr = QString("%1, %2").arg(q_("Altitude"), QChar(0x00B0));

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->altVsTimePlot->clearGraphs();
	ui->altVsTimePlot->addGraph();
	ui->altVsTimePlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->altVsTimePlot->graph(0)->setPen(QPen(Qt::red, 1));
	ui->altVsTimePlot->graph(0)->setLineStyle(QCPGraph::lsLine);
	ui->altVsTimePlot->graph(0)->rescaleAxes(true);
	ui->altVsTimePlot->xAxis->setLabel(xAxisStr);
	ui->altVsTimePlot->yAxis->setLabel(yAxisStr);

	ui->altVsTimePlot->xAxis->setRange(43200, 129600); // 24 hours since 12h00m (range in seconds)
	ui->altVsTimePlot->xAxis->setScaleType(QCPAxis::stLinear);
	ui->altVsTimePlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
	ui->altVsTimePlot->xAxis->setLabelColor(axisColor);
	ui->altVsTimePlot->xAxis->setTickLabelColor(axisColor);
	ui->altVsTimePlot->xAxis->setBasePen(axisPen);
	ui->altVsTimePlot->xAxis->setTickPen(axisPen);
	ui->altVsTimePlot->xAxis->setSubTickPen(axisPen);
	ui->altVsTimePlot->xAxis->setDateTimeFormat("H:mm");
	ui->altVsTimePlot->xAxis->setDateTimeSpec(Qt::UTC); // Qt::UTC + core->getUTCOffset() give local time
	ui->altVsTimePlot->xAxis->setAutoTickStep(false);
	ui->altVsTimePlot->xAxis->setTickStep(7200); // step is 2 hours (in seconds)
	ui->altVsTimePlot->xAxis->setAutoSubTicks(false);
	ui->altVsTimePlot->xAxis->setSubTickCount(7);

	ui->altVsTimePlot->yAxis->setRange(minY, maxY);
	ui->altVsTimePlot->yAxis->setScaleType(QCPAxis::stLinear);
	ui->altVsTimePlot->yAxis->setLabelColor(axisColor);
	ui->altVsTimePlot->yAxis->setTickLabelColor(axisColor);
	ui->altVsTimePlot->yAxis->setBasePen(axisPen);
	ui->altVsTimePlot->yAxis->setTickPen(axisPen);
	ui->altVsTimePlot->yAxis->setSubTickPen(axisPen);
}

void AstroCalcDialog::mouseOverLine(QMouseEvent *event)
{
	double x = ui->altVsTimePlot->xAxis->pixelToCoord(event->pos().x());
	double y = ui->altVsTimePlot->yAxis->pixelToCoord(event->pos().y());

	QCPAbstractPlottable *abstractGraph = ui->altVsTimePlot->plottableAt(event->pos(), false);
	QCPGraph *graph = qobject_cast<QCPGraph *>(abstractGraph);

	if (x>ui->altVsTimePlot->xAxis->range().lower && x<ui->altVsTimePlot->xAxis->range().upper && y>ui->altVsTimePlot->yAxis->range().lower && y<ui->altVsTimePlot->yAxis->range().upper)
	{
		if (graph)
		{
			double JD = x/86400.0 + (int)core->getJD() - 0.5;
			QString LT = StelUtils::jdToQDateTime(JD - core->getUTCOffset(JD)).toString("H:mm");

			QString info;
			if (graph->name()=="[Now]")
				info = q_("Now about %1").arg(LT);
			else if (graph->name()=="[Transit]")
				info = q_("Passage of meridian at approximately %1").arg(LT);
			else
			{
				if (StelApp::getInstance().getFlagShowDecimalDegrees())
					info = QString("%1<br />%2: %3<br />%4: %5%6").arg(ui->altVsTimePlot->graph(0)->name(), q_("Local Time"), LT, q_("Altitude"), QString::number(y, 'f', 2), QChar(0x00B0));
				else
					info = QString("%1<br />%2: %3<br />%4: %5").arg(ui->altVsTimePlot->graph(0)->name(), q_("Local Time"), LT, q_("Altitude"), StelUtils::decDegToDmsStr(y));
			}

			QToolTip::hideText();
			QToolTip::showText(event->globalPos(), info, ui->altVsTimePlot, ui->altVsTimePlot->rect());
		}
		else
			QToolTip::hideText();
	}

	ui->altVsTimePlot->update();
	ui->altVsTimePlot->replot();
}

void AstroCalcDialog::setPhenomenaHeaderNames()
{
	phenomenaHeader.clear();
	phenomenaHeader << q_("Phenomenon");
	phenomenaHeader << q_("Date and Time");
	phenomenaHeader << q_("Object 1");
	phenomenaHeader << q_("Object 2");
	phenomenaHeader << q_("Separation");
	ui->phenomenaTreeWidget->setHeaderLabels(phenomenaHeader);

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
	JD -= core->getUTCOffset(JD)/24.;

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
	QString currentPlanet = ui->object1ComboBox->currentData().toString();
	double separation = ui->allowedSeparationDoubleSpinBox->value();
	bool opposition = ui->phenomenaOppositionCheckBox->isChecked();

	initListPhenomena();

	QList<PlanetP> objects;
	objects.clear();
	QList<PlanetP> allObjects = solarSystem->getAllPlanets();

	QList<NebulaP> dso;
	dso.clear();
	QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();

	QList<StelObjectP> star, doubleStar, variableStar;
	star.clear();
	doubleStar.clear();
	variableStar.clear();
	QList<StelObjectP> hipStars = starMgr->getHipparcosStars();
	QList<starData> doubleHipStars = starMgr->getHipparcosDoubleStars();
	QList<starData> variableHipStars = starMgr->getHipparcosVariableStars();

	int obj2Type = ui->object2ComboBox->currentData().toInt();
	switch (obj2Type)
	{
		case 0: // Solar system
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()!=Planet::isUNDEFINED)
					objects.append(object);
			}
			break;
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
		case 9: // Sednoids
			foreach(const PlanetP& object, allObjects)
			{
				if (object->getPlanetType()==Planet::isSednoid)
					objects.append(object);
			}
			break;
		case 10: // Stars			
			foreach(const StelObjectP& object, hipStars)
			{
				if (object->getVMagnitude(core)<(brightLimit-5.0f))
					star.append(object);
			}
			break;
		case 11: // Double stars
			foreach(const starData& object, doubleHipStars)
			{
				if (object.firstKey()->getVMagnitude(core)<(brightLimit-5.0f))
					star.append(object.firstKey());
			}
			break;
		case 12: // Variable stars
			foreach(const starData& object, variableHipStars)
			{
				if (object.firstKey()->getVMagnitude(core)<(brightLimit-5.0f))
					star.append(object.firstKey());
			}
			break;
		case 13: // Star clusters
			foreach(const NebulaP& object, allDSO)
			{
				if (object->getVMagnitude(core)<brightLimit && (object->getDSOType()==Nebula::NebCl || object->getDSOType()==Nebula::NebOc || object->getDSOType()==Nebula::NebGc || object->getDSOType()==Nebula::NebSA || object->getDSOType()==Nebula::NebSC || object->getDSOType()==Nebula::NebCn))
					dso.append(object);
			}
			break;
		case 14: // Planetary nebulae
			foreach(const NebulaP& object, allDSO)
			{
				if (object->getDSOType()==Nebula::NebPn || object->getDSOType()==Nebula::NebPossPN || object->getDSOType()==Nebula::NebPPN)
					dso.append(object);
			}
			break;
		case 15: // Bright nebulae
			foreach(const NebulaP& object, allDSO)
			{
				if (object->getVMagnitude(core)<brightLimit && (object->getDSOType()==Nebula::NebN || object->getDSOType()==Nebula::NebBn || object->getDSOType()==Nebula::NebEn || object->getDSOType()==Nebula::NebRn || object->getDSOType()==Nebula::NebHII || object->getDSOType()==Nebula::NebISM || object->getDSOType()==Nebula::NebCn || object->getDSOType()==Nebula::NebSNR))
					dso.append(object);
			}
			break;
		case 16: // Dark nebulae
			foreach(const NebulaP& object, allDSO)
			{
				if (object->getDSOType()==Nebula::NebDn || object->getDSOType()==Nebula::NebMolCld || object->getDSOType()==Nebula::NebYSO)
					dso.append(object);
			}
			break;
		case 17: // Galaxies
			foreach(const NebulaP& object, allDSO)
			{
				if (object->getVMagnitude(core)<brightLimit && (object->getDSOType()==Nebula::NebGx || object->getDSOType()==Nebula::NebAGx || object->getDSOType()==Nebula::NebRGx || object->getDSOType()==Nebula::NebQSO || object->getDSOType()==Nebula::NebPossQSO || object->getDSOType()==Nebula::NebBLL || object->getDSOType()==Nebula::NebBLA || object->getDSOType()==Nebula::NebIGx))
					dso.append(object);
			}
			break;
	}

	PlanetP planet = solarSystem->searchByEnglishName(currentPlanet);
	if (planet)
	{
		double currentJD = core->getJD(); // save current JD
		double currentJDE = core->getJDE(); // save current JDE
		double startJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenFromDateEdit->date()));
		double stopJD = StelUtils::qDateTimeToJd(QDateTime(ui->phenomenToDateEdit->date().addDays(1)));
		startJD = startJD - core->getUTCOffset(startJD)/24;
		stopJD = stopJD - core->getUTCOffset(stopJD)/24;

		// Calculate the limits on coordinates for speed-up of calculations
		double coordsLimit = std::abs(core->getCurrentPlanet()->getRotObliquity(currentJDE)) + std::abs(planet->getRotObliquity(currentJDE)) + 0.026;
		coordsLimit += separation*M_PI/180;
		double ra, dec;

		if (obj2Type<10)
		{
			// Solar system objects
			foreach (PlanetP obj, objects)
			{
				// conjunction
				fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation, false), planet, obj, false);
				// opposition
				if (opposition)
					fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation, true), planet, obj, true);
			}
		}
		else if (obj2Type==10 || obj2Type==11 || obj2Type==12)
		{
			// Stars
			foreach (StelObjectP obj, star)
			{
				StelUtils::rectToSphe(&ra, &dec, obj->getEquinoxEquatorialPos(core));
				// Add limits on coordinates for speed-up calculations
				if (dec<=coordsLimit && dec>=-coordsLimit)
				{
					// conjunction
					fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation), planet, obj);
				}
			}
		}
		else
		{
			// Deep-sky objects
			foreach (NebulaP obj, dso)
			{
				StelUtils::rectToSphe(&ra, &dec, obj->getEquinoxEquatorialPos(core));
				// Add limits on coordinates for speed-up calculations
				if (dec<=coordsLimit && dec>=-coordsLimit)
				{
					// conjunction
					fillPhenomenaTable(findClosestApproach(planet, obj, startJD, stopJD, separation), planet, obj);
				}
			}
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

void AstroCalcDialog::savePhenomena()
{
	QString filter = q_("CSV (Comma delimited)");
	filter.append(" (*.csv)");
	QString filePath = QFileDialog::getSaveFileName(0, q_("Save calculated phenomena as..."), QDir::homePath() + "/phenomena.csv", filter);
	QFile phenomena(filePath);
	if (!phenomena.open(QFile::WriteOnly | QFile::Truncate))
	{
		qWarning() << "AstroCalc: Unable to open file"
			   << QDir::toNativeSeparators(filePath);
		return;
	}

	QTextStream phenomenaList(&phenomena);
	phenomenaList.setCodec("UTF-8");

	int count = ui->phenomenaTreeWidget->topLevelItemCount();

	phenomenaList << phenomenaHeader.join(delimiter) << acEndl;
	for (int i = 0; i < count; i++)
	{
		int columns = phenomenaHeader.size();
		for (int j=0; j<columns; j++)
		{
			phenomenaList << ui->phenomenaTreeWidget->topLevelItem(i)->text(j);
			if (j<columns-1)
				phenomenaList << delimiter;
			else
				phenomenaList << acEndl;
		}
	}

	phenomena.close();
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
		bool occultation = false;
		double s1 = object1->getSpheroidAngularSize(core);
		double s2 = object2->getSpheroidAngularSize(core);
		if (opposition)
		{
			phenomenType = q_("Opposition");
			separation += M_PI;
		}
		else if (separation<(s2*M_PI/180.) || separation<(s1*M_PI/180.))
		{
			double d1 = object1->getJ2000EquatorialPos(core).length();
			double d2 = object2->getJ2000EquatorialPos(core).length();
			if ((d1<d2 && s1<=s2) || (d1>d2 && s1>s2))
				phenomenType = q_("Transit");
			else
				phenomenType = q_("Occultation");

			// Added a special case - eclipse
			if (qAbs(s1-s2)<=0.05 && (object1->getEnglishName()=="Sun" || object2->getEnglishName()=="Sun")) // 5% error of difference of sizes
				phenomenType = q_("Eclipse");

			occultation = true;
		}

		ACPhenTreeWidgetItem *treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(it.key()), localeMgr->getPrintableTimeLocal(it.key())));
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		if (occultation)
			treeItem->setText(PhenomenaSeparation, QChar(0x2014));
		else
			treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP &object1, PlanetP &object2, double startJD, double stopJD, double maxSeparation, bool opposition)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD)/12.0;
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

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const NebulaP object2)
{
	QMap<double, double>::ConstIterator it;
	for (it=list.constBegin(); it!=list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		if (separation<(object2->getAngularSize(core)*M_PI/180.) || separation<(object1->getSpheroidAngularSize(core)*M_PI/180.))
		{
			phenomenType = q_("Occultation");
			occultation = true;
		}

		ACPhenTreeWidgetItem *treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate, StelUtils::jdToQDateTime(it.key() + core->getUTCOffset(it.key())/24).toString("yyyy-MM-dd hh:mm:ss"));
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		if (!object2->getNameI18n().isEmpty())
			treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		else
			treeItem->setText(PhenomenaObject2, object2->getDSODesignation());
		if (occultation)
			treeItem->setText(PhenomenaSeparation, QChar(0x2014));
		else
			treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP &object1, NebulaP &object2, double startJD, double stopJD, double maxSeparation)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD)/8.0;
	if (step0>24.8*365.25)
		step0 = 24.8*365.25;

	if (object1->getEnglishName()=="Neptune" || object1->getEnglishName()=="Uranus")
		if (step0 > 3652.5)
			step0 = 3652.5;
	if (object1->getEnglishName()=="Jupiter" || object1->getEnglishName()=="Saturn")
		if (step0 > 365.25)
			step0 = 365.f;
	if (object1->getEnglishName()=="Mars")
		if (step0 > 10.f)
			step0 = 10.f;
	if (object1->getEnglishName()=="Venus" || object1->getEnglishName()=="Mercury")
		if (step0 > 5.f)
			step0 = 5.f;
	if (object1->getEnglishName()=="Moon")
		if (step0 > 0.25)
			step0 = 0.25;

	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2);
	jd += step;
	while(jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2);
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
					dist = findDistance(jd, object1, object2);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn!=prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn))
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

bool AstroCalcDialog::findPrecise(QPair<double, double> *out, PlanetP object1, NebulaP object2, double JD, double step, int prevSign)
{
	int sgn;
	double dist, prevDist;

	if (out==NULL)
		return false;

	prevDist = findDistance(JD, object1, object2);
	step = -step/2.f;
	prevSign = -prevSign;

	while(true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2);

		if (qAbs(step)< 1.f/1440.f)
		{
			out->first = JD - step/2.0;
			out->second = findDistance(JD - step/2.0, object1, object2);
			if (out->second < findDistance(JD - 5.0, object1, object2))
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

double AstroCalcDialog::findDistance(double JD, PlanetP object1, NebulaP object2)
{
	core->setJD(JD);
	core->update(0);
	Vec3d obj1 = object1->getJ2000EquatorialPos(core);
	Vec3d obj2 = object2->getJ2000EquatorialPos(core);
	return obj1.angle(obj2);
}

void AstroCalcDialog::fillPhenomenaTable(const QMap<double, double> list, const PlanetP object1, const StelObjectP object2)
{
	QMap<double, double>::ConstIterator it;
	for (it=list.constBegin(); it!=list.constEnd(); ++it)
	{
		core->setJD(it.key());
		core->update(0);

		QString phenomenType = q_("Conjunction");
		double separation = it.value();
		bool occultation = false;
		if (separation<(object2->getAngularSize(core)*M_PI/180.) || separation<(object1->getSpheroidAngularSize(core)*M_PI/180.))
		{
			phenomenType = q_("Occultation");
			occultation = true;
		}

		ACPhenTreeWidgetItem *treeItem = new ACPhenTreeWidgetItem(ui->phenomenaTreeWidget);
		treeItem->setText(PhenomenaType, phenomenType);
		// local date and time
		treeItem->setText(PhenomenaDate, QString("%1 %2").arg(localeMgr->getPrintableDateLocal(it.key()), localeMgr->getPrintableTimeLocal(it.key())));
		treeItem->setText(PhenomenaObject1, object1->getNameI18n());
		treeItem->setText(PhenomenaObject2, object2->getNameI18n());
		if (occultation)
			treeItem->setText(PhenomenaSeparation, QChar(0x2014));
		else
			treeItem->setText(PhenomenaSeparation, StelUtils::radToDmsStr(separation));
	}
}

QMap<double, double> AstroCalcDialog::findClosestApproach(PlanetP &object1, StelObjectP &object2, double startJD, double stopJD, double maxSeparation)
{
	double dist, prevDist, step, step0;
	int sgn, prevSgn = 0;
	QMap<double, double> separations;
	QPair<double, double> extremum;

	step0 = (stopJD - startJD)/8.0;
	if (step0>24.8*365.25)
		step0 = 24.8*365.25;

	if (object1->getEnglishName()=="Neptune" || object1->getEnglishName()=="Uranus")
		if (step0 > 1811.25)
			step0 = 1811.25;
	if (object1->getEnglishName()=="Jupiter" || object1->getEnglishName()=="Saturn")
		if (step0 > 181.125)
			step0 = 181.125;
	if (object1->getEnglishName()=="Mars")
		if (step0 > 5.f)
			step0 = 5.0;
	if (object1->getEnglishName()=="Venus" || object1->getEnglishName()=="Mercury")
		if (step0 > 2.5f)
			step0 = 2.5;
	if (object1->getEnglishName()=="Moon")
		if (step0 > 0.25)
			step0 = 0.25;

	step = step0;
	double jd = startJD;
	prevDist = findDistance(jd, object1, object2);
	jd += step;
	while(jd <= stopJD)
	{
		dist = findDistance(jd, object1, object2);
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
					dist = findDistance(jd, object1, object2);
					sgn = StelUtils::sign(dist - prevDist);
					if (sgn!=prevSgn)
						break;

					prevDist = dist;
					prevSgn = sgn;
					jd += step;
				}
			}

			if (findPrecise(&extremum, object1, object2, jd, step, sgn))
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

bool AstroCalcDialog::findPrecise(QPair<double, double> *out, PlanetP object1, StelObjectP object2, double JD, double step, int prevSign)
{
	int sgn;
	double dist, prevDist;

	if (out==NULL)
		return false;

	prevDist = findDistance(JD, object1, object2);
	step = -step/2.f;
	prevSign = -prevSign;

	while(true)
	{
		JD += step;
		dist = findDistance(JD, object1, object2);

		if (qAbs(step)< 1.f/1440.f)
		{
			out->first = JD - step/2.0;
			out->second = findDistance(JD - step/2.0, object1, object2);
			if (out->second < findDistance(JD - 5.0, object1, object2))
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

double AstroCalcDialog::findDistance(double JD, PlanetP object1, StelObjectP object2)
{
	core->setJD(JD);
	core->update(0);
	Vec3d obj1 = object1->getJ2000EquatorialPos(core);
	Vec3d obj2 = object2->getJ2000EquatorialPos(core);
	return obj1.angle(obj2);
}

void AstroCalcDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	// special cases
	switch (ui->stackListWidget->row(current))
	{
		case 0: // update planetary positions
			currentPlanetaryPositions();
			break;
		case 1: // update positions of celestial bodies
			currentCelestialPositions();
			break;
	}
}

void AstroCalcDialog::updateTabBarListWidgetWidth()
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
		int textWidth = fontMetrics.width(ui->stackListWidget->item(row)->text());
		width += iconSize > textWidth ? iconSize : textWidth; // use the wider one
		width += 24; // margin - 12px left and 12px right
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void AstroCalcDialog::updateSolarSystemData()
{
	if (dialog)
	{
		populateCelestialBodyList();
		populateGroupCelestialBodyList();
		currentPlanetaryPositions();
		calculateWutObjects();
	}
}

void AstroCalcDialog::populateTimeIntervalsList()
{
	Q_ASSERT(ui->wutComboBox);

	QComboBox* wut = ui->wutComboBox;
	wut->blockSignals(true);
	int index = wut->currentIndex();
	QVariant selectedIntervalId = wut->itemData(index);

	wut->clear();
	wut->addItem(q_("In the Evening"), "0");
	wut->addItem(q_("In the Morning"), "1");
	wut->addItem(q_("Any Time Tonight"), "2");

	index = wut->findData(selectedIntervalId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (index<0)
		index = wut->findData(conf->value("astrocalc/wut_time_interval", "0").toString(), Qt::UserRole, Qt::MatchCaseSensitive);
	wut->setCurrentIndex(index);
	wut->model()->sort(0);
	wut->blockSignals(false);
}

void AstroCalcDialog::populateWutGroups()
{
	Q_ASSERT(ui->wutCategoryListWidget);

	QListWidget* category = ui->wutCategoryListWidget;
	category->blockSignals(true);

	wutCategories.clear();
	wutCategories.insert(q_("Planets"), 0);
	wutCategories.insert(q_("Bright stars"), 1);
	wutCategories.insert(q_("Bright nebulae"), 2);
	wutCategories.insert(q_("Dark nebulae"), 3);
	wutCategories.insert(q_("Galaxies"), 4);
	wutCategories.insert(q_("Star clusters"), 5);
	wutCategories.insert(q_("Asteroids"), 6);
	wutCategories.insert(q_("Comets"), 7);
	wutCategories.insert(q_("Plutinos"), 8);
	wutCategories.insert(q_("Dwarf planets"), 9);
	wutCategories.insert(q_("Cubewanos"), 10);
	wutCategories.insert(q_("Scattered disc objects"), 11);
	wutCategories.insert(q_("Oort cloud objects"), 12);
	wutCategories.insert(q_("Sednoids"), 13);
	wutCategories.insert(q_("Planetary nebulae"), 14);
	wutCategories.insert(q_("Bright double stars"), 15);
	wutCategories.insert(q_("Bright variable stars"), 16);

	category->clear();
	category->addItems(wutCategories.keys());
	category->sortItems(Qt::AscendingOrder);

	category->blockSignals(false);
}

void AstroCalcDialog::saveWutMagnitudeLimit(double mag)
{
	conf->setValue("astrocalc/wut_magnitude_limit", QString::number(mag, 'f', 2));
	calculateWutObjects();
}

void AstroCalcDialog::calculateWutObjects()
{
	ui->wutMatchingObjectsListWidget->clear();
	if(ui->wutCategoryListWidget->currentItem())
	{
		QString categoryName = ui->wutCategoryListWidget->currentItem()->text();
		int categoryId = wutCategories.value(categoryName);

		wutObjects.clear();

		QList<PlanetP> allObjects = solarSystem->getAllPlanets();
		QVector<NebulaP> allDSO = dsoMgr->getAllDeepSkyObjects();
		QList<StelObjectP> hipStars = starMgr->getHipparcosStars();
		QList<starData> dblHipStars = starMgr->getHipparcosDoubleStars();
		QList<starData> varHipStars = starMgr->getHipparcosVariableStars();

		double magLimit = ui->wutMagnitudeDoubleSpinBox->value();
		double highLimit = 6.0;
		double JD = core->getJD();
		double wutJD = (int)JD;
		double az, alt;

		// Dirty hack to calculate sunrise/sunset
		// FIXME: This block of code should be replaced in future!
		float phi = core->getCurrentLocation().latitude;
		double dec_sidereal, ra_sidereal, ha_sidereal;
		StelUtils::rectToSphe(&ra_sidereal,&dec_sidereal,GETSTELMODULE(SolarSystem)->getSun()->getSiderealPosGeometric(core));
		ha_sidereal = (2.*M_PI-ra_sidereal)*12/M_PI;
		if (ha_sidereal>24.)
			ha_sidereal -= 24.;
		double cost = (-0.01483475 - sin(phi)*sin(dec_sidereal))/(cos(phi)*cos(dec_sidereal));
		if (cost>1)
			cost -= 1.;
		if (cost<-1)
			cost += 1.;
		double h = std::abs(std::acos(cost)*180./M_PI)/360.;
		wutJD += ((JD - wutJD)*24.0 - ha_sidereal)/24.0;

		QComboBox* wut = ui->wutComboBox;
		switch (wut->itemData(wut->currentIndex()).toInt())
		{
			case 1: // Morning
				wutJD += 0.5 + h;
				break;
			case 2: // Night
				wutJD += 0.5;
				break;
			default: // Evening
				wutJD += 0.5 - h;
				break;
		}
		core->setJD(wutJD);
		core->update(0);

		switch (categoryId)
		{
			case 1: // Bright stars
				foreach(const StelObjectP& object, hipStars)
				{
					if (object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 2: // Bright nebulae
				foreach(const NebulaP& object, allDSO)
				{
					Nebula::NebulaType ntype = object->getDSOType();
					if ((ntype==Nebula::NebN || ntype==Nebula::NebBn || ntype==Nebula::NebEn || ntype==Nebula::NebRn || ntype==Nebula::NebHII || ntype==Nebula::NebISM || ntype==Nebula::NebCn || ntype==Nebula::NebSNR) && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
						{
							QString d = object->getDSODesignation();
							if (object->getNameI18n().isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(object->getNameI18n(), d);
						}
					}
				}
				break;
			case 3: // Dark nebulae
				foreach(const NebulaP& object, allDSO)
				{
					Nebula::NebulaType ntype = object->getDSOType();
					if ((ntype==Nebula::NebDn || ntype==Nebula::NebMolCld || ntype==Nebula::NebYSO) && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
						{
							QString d = object->getDSODesignation();
							if (object->getNameI18n().isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(object->getNameI18n(), d);
						}
					}
				}
				break;
			case 4: // Galaxies
				foreach(const NebulaP& object, allDSO)
				{
					Nebula::NebulaType ntype = object->getDSOType();
					if ((ntype==Nebula::NebGx || ntype==Nebula::NebAGx || ntype==Nebula::NebRGx || ntype==Nebula::NebQSO || ntype==Nebula::NebPossQSO || ntype==Nebula::NebBLL || ntype==Nebula::NebBLA || ntype==Nebula::NebIGx) && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
						{
							QString d = object->getDSODesignation();
							if (object->getNameI18n().isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(object->getNameI18n(), d);
						}
					}
				}
				break;
			case 5: // Star clusters
				foreach(const NebulaP& object, allDSO)
				{
					Nebula::NebulaType ntype = object->getDSOType();
					if ((ntype==Nebula::NebCl || ntype==Nebula::NebOc || ntype==Nebula::NebGc || ntype==Nebula::NebSA || ntype==Nebula::NebSC || ntype==Nebula::NebCn) && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
						{
							QString d = object->getDSODesignation();
							if (object->getNameI18n().isEmpty())
								wutObjects.insert(d, d);
							else
								wutObjects.insert(object->getNameI18n(), d);
						}
					}
				}
				break;
			case 6: // Asteroids
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isAsteroid && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 7: // Comets
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isComet && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 8: // Plutinos
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isPlutino && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 9: // Dwarf planets
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isDwarfPlanet && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 10: // Cubewanos
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isCubewano && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 11: // Scattered disc objects
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isSDO && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 12: // Oort cloud objects
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isOCO && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 13: // Sednoids
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isSednoid && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 14: // Planetary nebulae
				foreach(const NebulaP& object, allDSO)
				{
					Nebula::NebulaType ntype = object->getDSOType();
					if ((ntype==Nebula::NebPn || ntype==Nebula::NebPossPN || ntype==Nebula::NebPPN) && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
						{
							if (object->getNameI18n().isEmpty())
								wutObjects.insert(object->getDSODesignation(), object->getDSODesignation());
							else
								wutObjects.insert(object->getNameI18n(), object->getDSODesignation());
						}
					}
				}
				break;
			case 15: // Bright double stars
				foreach(const starData& dblStar, dblHipStars)
				{
					StelObjectP object = dblStar.firstKey();
					if (object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			case 16: // Bright variale stars
				foreach(const starData& varStar, varHipStars)
				{
					StelObjectP object = varStar.firstKey();
					if (object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
			default: // Planets
				foreach(const PlanetP& object, allObjects)
				{
					if (object->getPlanetType()==Planet::isPlanet && object->getVMagnitudeWithExtinction(core)<=magLimit)
					{
						StelUtils::rectToSphe(&az, &alt, object->getAltAzPosAuto(core));
						alt = std::fmod(alt,2.0*M_PI);
						if (alt*180./M_PI >= highLimit)
							wutObjects.insert(object->getNameI18n(), object->getEnglishName());
					}
				}
				break;
		}

		core->setJD(JD);
		ui->wutMatchingObjectsListWidget->blockSignals(true);
		ui->wutMatchingObjectsListWidget->clear();
		ui->wutMatchingObjectsListWidget->addItems(wutObjects.keys());
		ui->wutMatchingObjectsListWidget->sortItems(Qt::AscendingOrder);
		ui->wutMatchingObjectsListWidget->blockSignals(false);
	}
}

void AstroCalcDialog::selectWutObject()
{
	if(ui->wutMatchingObjectsListWidget->currentItem())
	{
		QString wutObjectEnglisName = wutObjects.value(ui->wutMatchingObjectsListWidget->currentItem()->text());
		if (objectMgr->findAndSelectI18n(wutObjectEnglisName) || objectMgr->findAndSelect(wutObjectEnglisName))
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
}
