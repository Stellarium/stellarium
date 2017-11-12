/*
 * Distance Calculator plug-in for Stellarium
 *
 * Copyright (C) 2017 Alexander Wolf
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DistanceCalculator.hpp"
#include "DistanceCalculatorWindow.hpp"
#include "ui_distanceCalculatorWindow.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

DistanceCalculatorWindow::DistanceCalculatorWindow()
	: StelDialog("DistanceCalculator")
	, distanceCalc(Q_NULLPTR)
{
	ui = new Ui_distanceCalculatorWindowForm();
	core = StelApp::getInstance().getCore();
}

DistanceCalculatorWindow::~DistanceCalculatorWindow()
{
	delete ui;
}

void DistanceCalculatorWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void DistanceCalculatorWindow::createDialogContent()
{
	distanceCalc = GETSTELMODULE(DistanceCalculator);
	solarSystem = GETSTELMODULE(SolarSystem);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	populateCelestialBodies();

	connect(ui->firstCelestialBodyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(computeDistance()));
	connect(ui->secondCelestialBodyCcomboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(computeDistance()));
	connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(computeDistance()));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveDistanceSettings()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetDistanceSettings()));

	setAboutHtml();
	computeDistance();
}

void DistanceCalculatorWindow::setAboutHtml()
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Distance Calculator plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + DISTANCECALCULATOR_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + DISTANCECALCULATOR_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin allows measure linear distance between Solar system bodies.") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Distance Calculator plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about this plugin, its history and format of catalog, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Distance_Calculator_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void DistanceCalculatorWindow::saveDistanceSettings()
{
	distanceCalc->saveSettingsToConfig();
}

void DistanceCalculatorWindow::resetDistanceSettings()
{
	distanceCalc->restoreDefaults();

	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyCcomboBox);

	QComboBox* fbody = ui->firstCelestialBodyComboBox;
	QComboBox* sbody = ui->secondCelestialBodyCcomboBox;

	fbody->blockSignals(true);
	sbody->blockSignals(true);

	int index = fbody->findData(distanceCalc->getFirstCelestialBody(), Qt::UserRole, Qt::MatchCaseSensitive);
	fbody->setCurrentIndex(index);
	index = sbody->findData(distanceCalc->getSecondCelestialBody(), Qt::UserRole, Qt::MatchCaseSensitive);
	sbody->setCurrentIndex(index);

	fbody->blockSignals(false);
	sbody->blockSignals(false);

	computeDistance();
}

void DistanceCalculatorWindow::computeDistance()
{
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyCcomboBox);

	QComboBox* fbody = ui->firstCelestialBodyComboBox;
	QComboBox* sbody = ui->secondCelestialBodyCcomboBox;

	QString fisrtCelestialBody = fbody->currentData(Qt::UserRole).toString();
	QString secondCelestialBody = sbody->currentData(Qt::UserRole).toString();

	PlanetP firstCBId = solarSystem->searchByEnglishName(fisrtCelestialBody);
	Vec3d posFCB = firstCBId->getJ2000EquatorialPos(core);
	PlanetP secondCBId = solarSystem->searchByEnglishName(secondCelestialBody);
	Vec3d posSCB = secondCBId->getJ2000EquatorialPos(core);

	QString distanceUM = qc_("AU", "distance, astronomical unit");
	ui->distanceLabel->setText(QString("%1 %2").arg(QString::number((posFCB-posSCB).length(), 'f', 5)).arg(distanceUM));
}

void DistanceCalculatorWindow::populateCelestialBodies()
{
	Q_ASSERT(ui->firstCelestialBodyComboBox);
	Q_ASSERT(ui->secondCelestialBodyCcomboBox);

	QComboBox* fbody = ui->firstCelestialBodyComboBox;
	QComboBox* sbody = ui->secondCelestialBodyCcomboBox;

	QList<PlanetP> ss = solarSystem->getAllPlanets();

	//Save the current selection to be restored later
	fbody->blockSignals(true);
	int indexFB = fbody->currentIndex();
	QVariant selectedFisrtBodyId = fbody->itemData(indexFB);
	fbody->clear();

	sbody->blockSignals(true);
	int indexSB = sbody->currentIndex();
	QVariant selectedSecondBodyId = sbody->itemData(indexSB);
	sbody->clear();

	//For each planet, display the localized name and store the original as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	foreach(const PlanetP& p, ss)
	{
		if (!p->getEnglishName().contains("Observer", Qt::CaseInsensitive))
		{
			fbody->addItem(p->getNameI18n(), p->getEnglishName());
			sbody->addItem(p->getNameI18n(), p->getEnglishName());
		}
	}
	//Restore the selection
	indexFB = fbody->findData(selectedFisrtBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexFB<0)
		indexFB = fbody->findData(distanceCalc->getFirstCelestialBody(), Qt::UserRole, Qt::MatchCaseSensitive);
	fbody->setCurrentIndex(indexFB);
	fbody->model()->sort(0);

	indexSB = sbody->findData(selectedSecondBodyId, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexSB<0)
		indexSB = sbody->findData(distanceCalc->getSecondCelestialBody(), Qt::UserRole, Qt::MatchCaseSensitive);
	sbody->setCurrentIndex(indexSB);
	sbody->model()->sort(0);

	fbody->blockSignals(false);
	sbody->blockSignals(false);
}
