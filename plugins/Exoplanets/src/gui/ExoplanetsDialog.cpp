/*
 * Stellarium Exoplanets Plug-in GUI
 *
 * Copyright (C) 2012 Alexander Wolf
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


#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QUrl>
#include <QFileDialog>

#include "StelApp.hpp"
#include "ui_exoplanetsDialog.h"
#include "ExoplanetsDialog.hpp"
#include "Exoplanets.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

#include "qcustomplot.h"

ExoplanetsDialog::ExoplanetsDialog()
	: ep(NULL)
	, updateTimer(NULL)
{
        ui = new Ui_exoplanetsDialog;
	dialogName = "Exoplanets";
}

ExoplanetsDialog::~ExoplanetsDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
	}
	delete ui;
}

void ExoplanetsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		refreshUpdateValues();
		setAboutHtml();
		setInfoHtml();
		setWebsitesHtml();
		populateDiagramsList();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void ExoplanetsDialog::createDialogContent()
{
	ep = GETSTELMODULE(Exoplanets);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		this, SLOT(retranslate()));

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->aboutTextBrowser << ui->infoTextBrowser << ui->websitesTextBrowser;
	installKineticScrolling(addscroll);
#endif

	// Settings tab / updates group
	ui->displayAtStartupCheckBox->setChecked(ep->getEnableAtStartup());
	connect(ui->displayAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDisplayAtStartupEnabled(int)));
	ui->displayModeCheckBox->setChecked(ep->getDisplayMode());
	connect(ui->displayModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDistributionEnabled(int)));
	ui->displayShowExoplanetsButton->setChecked(ep->getFlagShowExoplanetsButton());
	connect(ui->displayShowExoplanetsButton, SIGNAL(stateChanged(int)), this, SLOT(setDisplayShowExoplanetsButton(int)));
	ui->timelineModeCheckBox->setChecked(ep->getTimelineMode());
	connect(ui->timelineModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setTimelineEnabled(int)));
	ui->habitableModeCheckBox->setChecked(ep->getHabitableMode());
	connect(ui->habitableModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setHabitableEnabled(int)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(ep, SIGNAL(updateStateChanged(Exoplanets::UpdateState)), this, SLOT(updateStateReceiver(Exoplanets::UpdateState)));
	connect(ep, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	connect(ui->plotDiagram, SIGNAL(clicked()), this, SLOT(drawDiagram()));

	// About & Info tabs
	setAboutHtml();
	setInfoHtml();
	setWebsitesHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=NULL)
	{
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
		ui->infoTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
		ui->websitesTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}

	populateDiagramsList();
	updateGuiFromSettings();
}

void ExoplanetsDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Exoplanets Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + EXOPLANETS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr></table>";

	html += "<p>" + QString(q_("This plugin plots the position of stars with exoplanets. Exoplanets data is derived from \"%1The Extrasolar Planets Encyclopaedia%2\"")).arg("<a href=\"http://exoplanet.eu/\">").arg("</a>") + ". ";
	html += QString(q_("The list of potential habitable exoplanets and data about them were taken from \"%1The Habitable Exoplanets Catalog%3\" by %2Planetary Habitability Laboratory%3.")).arg("<a href=\"http://phl.upr.edu/projects/habitable-exoplanets-catalog\">").arg("<a href=\"http://phl.upr.edu/home\">").arg("</a>") + "</p>";

	html += "<p>" + q_("The current catalog contains info about %1 planetary systems, which altogether have %2 exoplanets (including %3 potentially habitable exoplanets).").arg(ep->getCountPlanetarySystems()).arg(ep->getCountAllExoplanets()).arg(ep->getCountHabitableExoplanets()) + "</p>";
	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Exoplanets plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about the plugin, its history and format of the catalog you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Exoplanets_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=NULL)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}

void ExoplanetsDialog::setInfoHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Potential habitable exoplanets") + "</h2>";
	html += QString("<p>%1</p>").arg(q_("This plugin can display potential habitable exoplanets (orange marker) and some information about those planets."));
	html += QString("<p><b>%1</b> &mdash; %2</p>").arg(q_("Planetary Class")).arg(q_("Planet classification from host star spectral type (F, G, K, M), habitable zone (hot, warm, cold) and size (miniterran, subterran, terran, superterran, jovian, neptunian) (Earth = G-Warm Terran)."));
	html += QString("<p><b><a href='http://lasp.colorado.edu/~bagenal/3720/CLASS6/6EquilibriumTemp.html'>%1</a></b> &mdash; %2</p>").arg(q_("Equilibrium Temperature")).arg(q_("The planetary equilibrium temperature is a theoretical temperature in (°C) that the planet would be at when considered simply as if it were a black body being heated only by its parent star (assuming a 0.3 bond albedo). As example the planetary equilibrium temperature of Earth is -18.15°C (255 K)."));
	html += QString("<p><b><a href='http://phl.upr.edu/projects/earth-similarity-index-esi'>%1</a></b> &mdash; %2</p>").arg(q_("Earth Similarity Index (ESI)")).arg(q_("Similarity to Earth on a scale from 0 to 1, with 1 being the most Earth-like. ESI depends on the planet's radius, density, escape velocity, and surface temperature."));
	html += "<h3>" + q_("Additional info") + "</h3><ul>";
	html += QString("<li><a href='http://en.wikipedia.org/wiki/Circumstellar_habitable_zone'>%1</a> (en)</li>").arg(q_("Circumstellar habitable zone"));
	html += QString("<li><a href='http://en.wikipedia.org/wiki/Planetary_equilibrium_temperature'>%1</a> (en)</li>").arg(q_("Planetary equilibrium temperature"));
	html += QString("<li><a href='http://en.wikipedia.org/wiki/Planetary_habitability'>%1</a> (en)</li>").arg(q_("Planetary habitability"));
	html += QString("<li><a href='http://en.wikipedia.org/wiki/Earth_Similarity_Index'>%1</a> (en)</li>").arg(q_("Earth Similarity Index"));
	html += "</ul></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=NULL)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->infoTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->infoTextBrowser->setHtml(html);
}

void ExoplanetsDialog::setWebsitesHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("General professional Web sites relevant to extrasolar planets") + "</h2><ul>";
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://codementum.org/exoplanets/").arg(q_("Exoplanets: an interactive version of XKCD 1071"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.cfa.harvard.edu/HEK/").arg(q_("HEK (The Hunt for Exomoons with Kepler)"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.univie.ac.at/adg/schwarz/multiple.html").arg(q_("Exoplanets in binaries and multiple systems (Richard Schwarz)"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.iau.org/public/naming/#exoplanets").arg(q_("Naming exoplanets (IAU)"));
	html += QString("<li><a href='%1'>%2</a> (<em>%3</em>)</li>").arg("http://voparis-exoplanet.obspm.fr/people.html").arg(q_("Some Astronomers and Groups active in extrasolar planets studies")).arg(q_("update: 16 April 2012"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://exoplanets.org/").arg(q_("The Exoplanet Data Explorer"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.phys.unsw.edu.au/~cgt/planet/AAPS_Home.html").arg(q_("The Anglo-Australian Planet Search"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.exoplanets.ch/").arg(q_("Geneva Extrasolar Planet Search Programmes"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://olbin.jpl.nasa.gov/").arg(q_("OLBIN (Optical Long-Baseline Interferometry News)"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://exep.jpl.nasa.gov/").arg(q_("NASA's Exoplanet Exploration Program"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.astro.psu.edu/users/alex/pulsar_planets.htm").arg(q_("Pulsar planets"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://exoplanetarchive.ipac.caltech.edu/").arg(q_("The NASA Exoplanet Archive"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.dtm.ciw.edu/boss/c53index.html").arg(q_("IAU Commission 53: Extrasolar Planets"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.exomol.com/").arg(q_("ExoMol"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.hzgallery.org/").arg(q_("The Habitable Zone Gallery"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://planetquest.jpl.nasa.gov/").arg(q_("PlanetQuest - The Search for Another Earth"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://www.openexoplanetcatalogue.com/").arg(q_("Open Exoplanet Catalogue"));
	html += QString("<li><a href='%1'>%2</a></li>").arg("http://phl.upr.edu/projects/habitable-exoplanets-catalog").arg(q_("The Habitable Exoplanets Catalog"));
	html += "</ul></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=NULL)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->websitesTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->websitesTextBrowser->setHtml(html);
}


void ExoplanetsDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(ep->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(ep->getUpdateFrequencyHours());
	int secondsToUpdate = ep->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(ep->getUpdatesEnabled());
	if (!ep->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (ep->getUpdateState() == Exoplanets::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	else if (secondsToUpdate < 3600)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	else
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
}

void ExoplanetsDialog::setUpdateValues(int hours)
{
	ep->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void ExoplanetsDialog::setDistributionEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setDisplayMode(b);
}

void ExoplanetsDialog::setTimelineEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setTimelineMode(b);
}

void ExoplanetsDialog::setHabitableEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setHabitableMode(b);
}

void ExoplanetsDialog::setDisplayAtStartupEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setEnableAtStartup(b);
}

void ExoplanetsDialog::setDisplayShowExoplanetsButton(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setFlagShowExoplanetsButton(b);
}

void ExoplanetsDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void ExoplanetsDialog::updateStateReceiver(Exoplanets::UpdateState state)
{
	//qDebug() << "ExoplanetsDialog::updateStateReceiver got a signal";
	if (state==Exoplanets::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Exoplanets::DownloadError || state==Exoplanets::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void ExoplanetsDialog::updateCompleteReceiver(void)
{
        ui->nextUpdateLabel->setText(QString(q_("Exoplanets is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(ep->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void ExoplanetsDialog::restoreDefaults(void)
{
	qDebug() << "Exoplanets::restoreDefaults";
	ep->restoreDefaults();
	ep->loadConfiguration();
	updateGuiFromSettings();
}

void ExoplanetsDialog::updateGuiFromSettings(void)
{
	ui->internetUpdatesCheckbox->setChecked(ep->getUpdatesEnabled());
	refreshUpdateValues();
}

void ExoplanetsDialog::saveSettings(void)
{
	ep->saveConfiguration();
}

void ExoplanetsDialog::updateJSON(void)
{
	if(ep->getUpdatesEnabled())
	{
		ep->updateJSON();
	}
}

void ExoplanetsDialog::drawDiagram()
{
	int currentAxisX = ui->comboAxisX->currentData(Qt::UserRole).toInt();
	QString currentAxisXString = ui->comboAxisX->currentText();
	int currentAxisY = ui->comboAxisY->currentData(Qt::UserRole).toInt();
	QString currentAxisYString = ui->comboAxisY->currentText();

	QList<double> aX = ep->getExoplanetsData(currentAxisX), aY = ep->getExoplanetsData(currentAxisY);
	QVector<double> x = aX.toVector(), y = aY.toVector();

	double minX, minY, maxX, maxY;
	minX = maxX = aX.first();
	minY = maxY = aY.first();

	foreach (double temp, aX)
	{
		if(maxX < temp) maxX = temp;
		if(minX > temp) minX = temp;
	}
	foreach (double temp, aY)
	{
		if(maxY < temp) maxY = temp;
		if(minY > temp) minY = temp;
	}

	if (!ui->minX->text().isEmpty())
		minX = ui->minX->text().toDouble();

	if (!ui->maxX->text().isEmpty())
		maxX = ui->maxX->text().toDouble();

	if (!ui->minY->text().isEmpty())
		minY = ui->minY->text().toDouble();

	if (!ui->maxY->text().isEmpty())
		maxY = ui->maxY->text().toDouble();

	ui->customPlot->addGraph();
	ui->customPlot->graph(0)->setData(x, y);
	ui->customPlot->graph(0)->setPen(QPen(Qt::blue));
	ui->customPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
	ui->customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
	ui->customPlot->graph(0)->rescaleAxes(true);
	ui->customPlot->xAxis->setLabel(currentAxisXString);
	ui->customPlot->yAxis->setLabel(currentAxisYString);

	ui->customPlot->xAxis->setRange(minX, maxX);
	if (ui->checkBoxLogX->isChecked())
	{
		ui->customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
		ui->customPlot->xAxis->setScaleLogBase(10);
	}
	else
		ui->customPlot->xAxis->setScaleType(QCPAxis::stLinear);

	ui->customPlot->yAxis->setRange(minY, maxY);
	if (ui->checkBoxLogY->isChecked())
	{
		ui->customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
		ui->customPlot->yAxis->setScaleLogBase(10);
	}
	else
		ui->customPlot->yAxis->setScaleType(QCPAxis::stLinear);

	ui->customPlot->replot();
}

void ExoplanetsDialog::populateDiagramsList()
{
	Q_ASSERT(ui->comboAxisX);
	Q_ASSERT(ui->comboAxisY);

	QComboBox* axisX = ui->comboAxisX;
	QComboBox* axisY = ui->comboAxisY;

	//Save the current selection to be restored later
	axisX->blockSignals(true);
	axisY->blockSignals(true);
	int indexX = axisX->currentIndex();
	int indexY = axisY->currentIndex();
	QVariant selectedAxisX = axisX->itemData(indexX);
	QVariant selectedAxisY = axisY->itemData(indexY);
	axisX->clear();
	axisY->clear();

	QList<axisPair> axis;
	axis.append(qMakePair(q_("Orbital Eccentricity"), 0));
	axis.append(qMakePair(q_("Orbit Semi-Major Axis, AU"), 1));
	axis.append(qMakePair(q_("Planetary Mass, Mjup"), 2));
	axis.append(qMakePair(q_("Planetary Radius, Rjup"), 3));
	axis.append(qMakePair(q_("Orbital Period, days"), 4));
	axis.append(qMakePair(q_("Angular Distance, arcsec."), 5));

	for(int i=0; i<axis.size(); ++i)
	{
		axisX->addItem(axis.at(i).first, axis.at(i).second);
		axisY->addItem(axis.at(i).first, axis.at(i).second);
	}

	//Restore the selection
	indexX = axisX->findData(selectedAxisX, Qt::UserRole, Qt::MatchCaseSensitive);
	indexY = axisY->findData(selectedAxisY, Qt::UserRole, Qt::MatchCaseSensitive);
	axisX->setCurrentIndex(indexX);
	axisY->setCurrentIndex(indexY);
	axisX->blockSignals(false);
	axisY->blockSignals(false);
}
