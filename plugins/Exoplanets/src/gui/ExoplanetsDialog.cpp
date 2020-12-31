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
#include <QColorDialog>

#include "StelApp.hpp"
#include "StelCore.hpp"
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
#include "StelLocaleMgr.hpp"

#include "external/qcustomplot/qcustomplot.h"

ExoplanetsDialog::ExoplanetsDialog()
	: StelDialog("Exoplanets")
	, ep(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
{
        ui = new Ui_exoplanetsDialog;
	exoplanetsHeader.clear();
	objectMgr = GETSTELMODULE(StelObjectMgr);
	mvMgr = GETSTELMODULE(StelMovementMgr);
}

ExoplanetsDialog::~ExoplanetsDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = Q_NULLPTR;
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
		populateTemperatureScales();
		fillExoplanetsTable();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void ExoplanetsDialog::createDialogContent()
{
	ep = GETSTELMODULE(Exoplanets);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser << ui->infoTextBrowser << ui->websitesTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

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
	ui->displayShowDesignationsCheckBox->setChecked(ep->getFlagShowExoplanetsDesignations());
	connect(ui->displayShowDesignationsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDisplayShowExoplanetsDesignations(int)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(ep, SIGNAL(updateStateChanged(Exoplanets::UpdateState)), this, SLOT(updateStateReceiver(Exoplanets::UpdateState)));
	connect(ep, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));	
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	connectColorButton(ui->exoplanetMarkerColor,		"Exoplanets.markerColor",    "Exoplanets/exoplanet_marker_color");
	connectColorButton(ui->habitableExoplanetMarkerColor,	"Exoplanets.habitableColor", "Exoplanets/habitable_exoplanet_marker_color");

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));	
	connect(ui->plotDiagram, SIGNAL(clicked()), this, SLOT(drawDiagram()));

	populateTemperatureScales();
	int idx = ui->temperatureScaleComboBox->findData(ep->getCurrentTemperatureScaleKey(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use Celsius as default
		idx = ui->temperatureScaleComboBox->findData(QVariant("Celsius"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->temperatureScaleComboBox->setCurrentIndex(idx);
	connect(ui->temperatureScaleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTemperatureScale(int)));

	// Table tab
	connect(ui->exoplanetsTreeWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectCurrentExoplanet(QModelIndex)));

	// About & Info tabs
	setAboutHtml();
	setInfoHtml();
	setWebsitesHtml();
	if(gui!=Q_NULLPTR)
	{
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
		ui->infoTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
		ui->websitesTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}

	populateDiagramsList();	
	fillExoplanetsTable();
	updateGuiFromSettings();
}

void ExoplanetsDialog::setColumnNames()
{
	exoplanetsHeader.clear();
	exoplanetsHeader << q_("Exoplanet");
	exoplanetsHeader << QString("%1, M%2").arg(q_("Mass")).arg(QChar(0x2643));
	exoplanetsHeader << QString("%1, R%2").arg(q_("Radius")).arg(QChar(0x2643));
	exoplanetsHeader << QString("%1, %2").arg(q_("Period"),qc_("day","time period"));
	exoplanetsHeader << QString("a, %1").arg(qc_("AU", "distance, astronomical unit"));
	exoplanetsHeader << QString("e");
	exoplanetsHeader << QString("i, %1").arg(QChar(0x00B0));
	// TRANSLATORS: angular distance
	exoplanetsHeader << QString("%1, \"").arg(q_("Ang. dist."));
	// TRANSLATORS: magnitude
	exoplanetsHeader << q_("Mag.");
	exoplanetsHeader << QString("%1, R%2").arg(q_("Radius")).arg(QChar(0x2609));
	// TRANSLATORS: detection method
	exoplanetsHeader << q_("D. M.");
	ui->exoplanetsTreeWidget->setHeaderLabels(exoplanetsHeader);

	// adjust the column width
	for (int i = 0; i < EPSCount; ++i)
	{
		ui->exoplanetsTreeWidget->resizeColumnToContents(i);
	}
}

void ExoplanetsDialog::fillExoplanetsTable()
{
	ui->exoplanetsTreeWidget->clear();
	ui->exoplanetsTreeWidget->setColumnCount(EPSCount);
	setColumnNames();
	ui->exoplanetsTreeWidget->header()->setSectionsMovable(false);
	ui->exoplanetsTreeWidget->header()->setDefaultAlignment(Qt::AlignHCenter);

	const QString dash = QChar(0x2014);
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	for (auto epsystem : ep->getAllExoplanetarySystems())
	{
		QVariantMap map = epsystem->getMap();
		float vmag = map["Vmag"].toFloat();
		float sr = map["sradius"].toFloat();
		QString sradius = sr>0.f ? QString::number(sr, 'f', 5) : dash;
		for (auto eps : map["exoplanets"].toList())
		{
			auto epdata = eps.toMap();
			QString dm = epdata.contains("detectionMethod") ? epdata["detectionMethod"].toString().trimmed() : dash;
			EPSTreeWidgetItem* treeItem = new EPSTreeWidgetItem(ui->exoplanetsTreeWidget);
			treeItem->setText(EPSExoplanetName, QString("%1 %2").arg(trans.qtranslate(map["designation"].toString().trimmed())).arg(epdata["planetName"].toString()).trimmed());
			treeItem->setData(EPSExoplanetName, Qt::UserRole, map["designation"].toString());
			if (epdata.contains("planetProperName")) {
				treeItem->setToolTip(EPSExoplanetName, trans.qtranslate(epdata["planetProperName"].toString().trimmed()));
			}
			treeItem->setText(EPSExoplanetMass, epdata.contains("mass") ? QString::number(epdata["mass"].toFloat(), 'f', 2) : dash);
			treeItem->setToolTip(EPSExoplanetMass,  q_("Mass of exoplanet in Jovian masses"));
			treeItem->setTextAlignment(EPSExoplanetMass,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetRadius, epdata.contains("radius") ? QString::number(epdata["radius"].toFloat(), 'f', 2) : dash);
			treeItem->setToolTip(EPSExoplanetRadius,  q_("Radius of exoplanet in Jovian radii"));
			treeItem->setTextAlignment(EPSExoplanetRadius,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetPeriod, epdata.contains("period") ? QString::number(epdata["period"].toFloat(), 'f', 2) : dash);
			treeItem->setToolTip(EPSExoplanetPeriod,  q_("Orbital period of exoplanet in days"));
			treeItem->setTextAlignment(EPSExoplanetPeriod,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetSemiAxes, epdata.contains("semiAxis") ? QString::number(epdata["semiAxis"].toFloat(), 'f', 4) : dash);
			treeItem->setToolTip(EPSExoplanetSemiAxes,  q_("Semi-major axis of orbit in astronomical units"));
			treeItem->setTextAlignment(EPSExoplanetSemiAxes,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetEccentricity, epdata.contains("eccentricity") ? QString::number(epdata["eccentricity"].toFloat(), 'f', 3) : dash);
			treeItem->setToolTip(EPSExoplanetEccentricity,  q_("Eccentricity of orbit"));
			treeItem->setTextAlignment(EPSExoplanetEccentricity,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetInclination, epdata.contains("inclination") ? QString::number(epdata["inclination"].toFloat(), 'f', 1) : dash);
			treeItem->setToolTip(EPSExoplanetInclination,  q_("Inclination of orbit in degrees"));
			treeItem->setTextAlignment(EPSExoplanetInclination,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetAngleDistance, epdata.contains("angleDistance") ? QString::number(epdata["angleDistance"].toFloat(), 'f', 6) : dash);
			treeItem->setToolTip(EPSExoplanetAngleDistance,  q_("Angular distance from host star in arcseconds"));
			treeItem->setTextAlignment(EPSExoplanetAngleDistance,  Qt::AlignRight);
			treeItem->setText(EPSStarMagnitude, vmag < 98.f ? QString::number(vmag, 'f', 2) : dash);
			treeItem->setTextAlignment(EPSStarMagnitude,  Qt::AlignRight);
			treeItem->setText(EPSStarRadius, sradius);
			treeItem->setToolTip(EPSStarRadius,  q_("Radius of star in solar radii"));
			treeItem->setTextAlignment(EPSStarRadius,  Qt::AlignRight);
			treeItem->setText(EPSExoplanetDetectionMethod, q_(dm));
			treeItem->setToolTip(EPSExoplanetDetectionMethod,  q_("Detection method of exoplanet"));
		}
	}
}

void ExoplanetsDialog::selectCurrentExoplanet(const QModelIndex& modelIndex)
{
	// Enable display exoplanets if it is not
	if (!ep->getFlagShowExoplanets())
		ep->setFlagShowExoplanets(true);
	// Find the object
	QString name = modelIndex.sibling(modelIndex.row(), EPSExoplanetName).data(Qt::UserRole).toString();
	if (objectMgr->findAndSelectI18n(name) || objectMgr->findAndSelect(name))
	{
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			mvMgr->moveToObject(newSelected[0], mvMgr->getAutoMoveDuration());
			mvMgr->setFlagTracking(true);
		}
	}
}

void ExoplanetsDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Exoplanets Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + EXOPLANETS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + EXOPLANETS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr></table>";

	html += "<p>" + QString(q_("This plugin plots the position of stars with exoplanets. Exoplanets data is derived from \"%1The Extrasolar Planets Encyclopaedia%2\"")).arg("<a href=\"http://exoplanet.eu/\">").arg("</a>") + ". ";
	html += QString(q_("The list of potential habitable exoplanets and data about them were taken from \"%1The Habitable Exoplanets Catalog%3\" by %2Planetary Habitability Laboratory%3.")).arg("<a href=\"http://phl.upr.edu/projects/habitable-exoplanets-catalog\">").arg("<a href=\"http://phl.upr.edu/home\">").arg("</a>") + "</p>";

	html += "<p>" + q_("The current catalog contains info about %1 planetary systems, which altogether have %2 exoplanets (including %3 potentially habitable exoplanets).").arg(ep->getCountPlanetarySystems()).arg(ep->getCountAllExoplanets()).arg(ep->getCountHabitableExoplanets()) + "</p>";
	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Exoplanets plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Exoplanets_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
	// TRANSLATORS: duration
	ui->updateFrequencySpinBox->setSuffix(qc_(" h","time unit"));
}

void ExoplanetsDialog::setInfoHtml(void)
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Potential habitable exoplanets") + "</h2>";
	html += QString("<p>%1 %2</p>")
			.arg(q_("This plugin can display potential habitable exoplanets (orange marker) and some information about those planets."))
			.arg(q_("Extra info for the optimistic samples of potentially habitable planets mark by italic text."));
	html += QString("<p><b>%1</b> &mdash; %2</p>")
			.arg(q_("Planetary Class"))
			.arg(q_("Planet classification from host star spectral type (F, G, K, M), habitable zone (hot, warm, cold) and size (miniterran, subterran, terran, superterran, jovian, neptunian) (Earth = G-Warm Terran)."));
	html += QString("<p><b><a href='http://lasp.colorado.edu/~bagenal/3720/CLASS6/6EquilibriumTemp.html'>%1</a></b> &mdash; %2 %3</p>")
			.arg(q_("Equilibrium Temperature"))
			.arg(q_("The planetary equilibrium temperature is a theoretical temperature in (°C) that the planet would be at when considered simply as if it were a black body being heated only by its parent star (assuming a 0.3 bond albedo). As example the planetary equilibrium temperature of Earth is -18.15°C (255 K)."))
			.arg(q_("Actual surface temperatures are expected to be larger than the equilibrium temperature depending on the atmosphere of the planets, which are currently unknown (e.g. Earth mean global surface temperature is about 288 K or 15°C)."));
	html += QString("<p><b>%1</b> &mdash; %2</p>")
			.arg(q_("Flux"))
			.arg(q_("Average stellar flux of the planet in Earth fluxes (Earth = 1.0 S<sub>E</sub>)."));
	html += QString("<p><b><a href='http://phl.upr.edu/projects/earth-similarity-index-esi'>%1</a></b> &mdash; %2</p>")
			.arg(q_("Earth Similarity Index (ESI)"))
			.arg(q_("Similarity to Earth on a scale from 0 to 1, with 1 being the most Earth-like. ESI depends on the planet's radius, density, escape velocity, and surface temperature."));
	html += QString("<p><b>%1</b> &mdash; %2</p>")
			.arg(q_("Conservative Sample"))
			.arg(q_("Planets in the habitable zone with a radius less 1.5 Earth radii or a minimum mass less 5 Earth masses. These are the best candidates for planets that might be rocky and support surface liquid water. They are also known as warm terrans."));
	html += QString("<p><b>%1</b> &mdash; %2</p>")
			.arg(q_("Optimistic Sample"))
			.arg(q_("Planets in the habitable zone with a radius between 1.5 to 2.5 Earth radii or between 5 to 10 Earth masses. These are planets that are less likely to be rocky or support surface liquid water. Some might be mini-Neptunes instead. They are also known as warm superterrans."));
	html += "<h2>" + q_("Proper names") + "</h2>";
	html += "<p>" + q_("In December 2015 and in December 2019, the International Astronomical Union (IAU) has officially approved names for several exoplanets after a public vote.") + "</p><ul>";
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3<sup>1</sup></li>").arg(trans.qtranslate("Veritate"), "14 And", q_("From the latin <em>Veritas</em>, truth. The ablative form means <em>where there is truth</em>."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3</li>").arg(trans.qtranslate("Spe"), "14 And b", q_("From the latin <em>Spes</em>, hope. The ablative form means <em>where there is hope</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Musica"), "18 Del", q_("Musica is Latin for <em>music</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Arion"), "18 Del b", q_("Arion was a genius of poetry and music in ancient Greece. According to legend, his life was saved at sea by dolphins after attracting their attention by the playing of his kithara."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Fafnir"), "42 Dra", q_("Fafnir was a Norse mythological dwarf who turned into a dragon."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Orbitar"), "42 Dra b", q_("Orbitar is a contrived word paying homage to the space launch and orbital operations of NASA."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Chalawan"), "47 UMa", q_("Chalawan is a mythological crocodile king from a Thai folktale."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Taphao Thong"), "47 UMa b", q_("Taphao Thong is one of two sisters associated with the Thai folk tale of Chalawan."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Taphao Kaew"), "47 UMa c", q_("Taphao Kae is one of two sisters associated with the Thai folk tale of Chalawan."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Helvetios"), "51 Peg", q_("Helvetios is Celtic for <em>the Helvetian</em> and refers to the Celtic tribe that lived in Switzerland during antiquity."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dimidium"), "51 Peg b", q_("Dimidium is Latin for <em>half</em>, referring to the planet's mass of at least half the mass of Jupiter."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Copernicus"), "55 Cnc", q_("Nicolaus Copernicus or Mikolaj Kopernik (1473-1543) was a Polish astronomer who proposed the heliocentric model of the solar system in his book <em>De revolutionibus orbium coelestium</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Galileo"), "55 Cnc b", q_("Galileo Galilei (1564-1642) was an Italian astronomer and physicist often called the <em>father of observational astronomy</em> and the <em>father of modern physics</em>. Using a telescope, he discovered the four largest satellites of Jupiter, and the reported the first telescopic observations of the phases of Venus, among other discoveries."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Brahe"), "55 Cnc c", q_("Tycho Brahe (1546-1601) was a Danish astronomer and nobleman who recorded accurate astronomical observations of the stars and planets. These observations were critical to Kepler's formulation of his three laws of planetary motion."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3<sup>6</sup></li>").arg(trans.qtranslate("Lipperhey"), "55 Cnc d", q_("Hans Lipperhey (1570-1619) was a German-Dutch lens grinder and spectacle maker who is often attributed with the invention of the refracting telescope in 1608."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Janssen"), "55 Cnc e", q_("Jacharias Janssen (1580s-1630s) was a Dutch spectacle maker who is often attributed with invention of the microscope, and more controversially with the invention of the telescope."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Harriot"), "55 Cnc f", q_("Thomas Harriot (ca. 1560-1621) was an English astronomer, mathematician, ethnographer, and translator, who is attributed with the first drawing of the Moon through telescopic observations."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3<sup>2</sup></li>").arg(trans.qtranslate("Amateru"), "ε Tau b", q_("<em>Amateru</em> is a common Japanese appellation for shrines when they enshrine Amaterasu, the Shinto goddess of the Sun, born from the left eye of the god Izanagi."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hypatia"), "ι Dra b", q_("Hypatia was a famous Greek astronomer, mathematician, and philosopher. She was head of the Neo-Platonic school at Alexandria in the early 5th century, until murdered by a Christian mob in 415."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ran"), "ε Eri", q_("Ran is the Norse goddess of the sea, who stirs up the waves and captures sailors with her net."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3<sup>3</sup></li>").arg(trans.qtranslate("AEgir"), "ε Eri b", q_("AEgir is Ran's husband, the personified god of the ocean. <em>AEgir</em> and <em>Ran</em> both represent the <em>Jotuns</em> who reign in the outer Universe; together they had nine daughters."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tadmor"), "γ Cep b", q_("Ancient Semitic name and modern Arabic name for the city of Palmyra, a UNESCO World Heritage Site."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dagon"), "α PsA b", q_("Dagon was a Semitic deity, often represented as half-man, half-fish."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tonatiuh"), "HD 104985", q_("Tonatiuh was the Aztec god of the Sun."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Meztli"), "HD 104985 b", q_("Meztli was the Aztec goddess of the Moon."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3<sup>4</sup></li>").arg(trans.qtranslate("Ogma"), "HD 149026", q_("Ogma was a deity of eloquence, writing, and great physical strength in the Celtic mythologies of Ireland and Scotland, and may be related to the Gallo-Roman deity <em>Ogmios</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Smertrios"), "HD 149026 b", q_("Smertrios was a Gallic deity of war."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Intercrus"), "HD 81688", q_("Intercrus means <em>between the legs</em> in Latin style, referring to the star's position in the constellation Ursa Major."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Arkas"), "HD 81688 b", q_("Arkas was the son of Callisto (Ursa Major) in Greek mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Cervantes"), "μ Ara", q_("Miguel de Cervantes Saavedra (1547-1616) was a famous Spanish writer and author of <em>El Ingenioso Hidalgo Don Quixote de la Mancha</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Quijote"), "μ Ara b", q_("Lead fictional character from Cervantes's <em>El Ingenioso Hidalgo Don Quixote de la Mancha</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dulcinea"), "μ Ara c", q_("Fictional character and love interest of Don Quijote (or Quixote) in Cervantes's <em>El Ingenioso Hidalgo Don Quixote de la Mancha</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Rocinante"), "μ Ara d", q_("Fictional horse of Don Quijote in Cervantes's <em>El Ingenioso Hidalgo Don Quixote de la Mancha</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sancho"), "μ Ara e", q_("Fictional squire of Don Quijote in Cervantes's <em>El Ingenioso Hidalgo Don Quixote de la Mancha</em>."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3<sup>5</sup></li>").arg(trans.qtranslate("Thestias"), "β Gem b", q_("Thestias is the patronym of Leda and her sister Althaea, the daughters of Thestius. Leda was a Greek queen, mother of Pollux and of his twin Castor, and of Helen and Clytemnestra."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Lich"), "PSR B1257+12", q_("Lich is a fictional undead creature known for controlling other undead creatures with magic."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Draugr"), "PSR B1257+12 b", q_("Draugr refers to undead creatures in Norse mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Poltergeist"), "PSR B1257+12 c", q_("Poltergeist is a name for supernatural beings that create physical disturbances, from German for <em>noisy ghost</em>."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Phobetor"), "PSR B1257+12 d", q_("Phobetor is a Greek mythological deity of nightmares, the son of Nyx, the primordial deity of night."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Titawin"), "υ And", q_("Titawin (also known as Medina of Tetouan) is a settlement in northern Morocco and UNESCO World Heritage Site. Historically it was an important point of contact between two civilizations (Spanish and Arab) and two continents (Europe and Africa) after the 8th century."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Saffar"), "υ And b", q_("Saffar is named for Abu al-Qasim Ahmed Ibn-Abd Allah Ibn-Omar al Ghafiqi Ibn-al-Saffar, who taught arithmetic, geometry, and astronomy in 11th century Cordova in Andalusia (modern Spain), and wrote an influential treatise on the uses of the astrolabe."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Samh"), "υ And c", q_("Samh is named for Abu al-Qasim 'Asbagh ibn Muhammad ibn al-Samh al-Mahri (or <em>Ibn al-Samh</em>), a noted 11th century astronomer and mathematician in the school of al Majriti in Cordova (Andalusia, now modern Spain)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Majriti"), "υ And d", q_("Majriti is named for Abu al-Qasim al-Qurtubi al-Majriti, a notable mathematician, astronomer, scholar, and teacher in 10th century and early 11th century Andalusia (modern Spain)."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3</li>").arg(trans.qtranslate("Libertas"), "ξ Aql", q_("Libertas is Latin for <em>liberty</em>. Liberty refers to social and political freedoms, and a reminder that there are people deprived of liberty in the world even today. The constellation Aquila represents an eagle - a popular symbol of liberty."));
	html += QString("<li><strong>%1</strong><sup>*</sup> (%2) &mdash; %3</li>").arg(trans.qtranslate("Fortitudo"), "ξ Aql b", q_("Fortitudo is Latin for <em>fortitude</em>. Fortitude means emotional and mental strength in the face of adversity, as embodied by the eagle (represented by the constellation Aquila)."));
	// According to IAU PR "100 000s of People from 112 Countries Select Names for Exoplanet Systems In Celebration of IAU’s 100th Anniversary": https://iau.org/news/pressreleases/detail/iau1912/
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Illyrian"), "HD 82886", q_("Historians largely believe that the Albanians are descendants of the Illyrians, a term Albanians proudly call themselves."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Arber"), "HD 82886 b", q_("Arber is the term for the inhabitants of Albania during the middle ages."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hoggar"), "HD 28678", q_("Hoggar is the name of the main mountain range in the Sahara Desert in southern Algeria."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tassili"), "HD 28678 b", q_("Tassili is a UNESCO World Heritage Site situated in the Sahara Desert and is renowned for its prehistoric cave art and scenic geological formations."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Arcalís"), "HD 131496", q_("Arcalis is a famous peak in the north of Andorra, where the Sun passes through a hole in the mountain twice a year at fixed dates. It was used as a primitive solar calendar and reference point for shepherds and early inhabitants of Andorra."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Madriu"), "HD 131496 b", q_("Madriu (Mare del riu in Catalan, Mother of the River in English) is the name of a glacial valley and of the river that runs through it in the southeast of Andorra. It is the main part of the Madriu-Perafita-Claror UNESCO World Heritage Site."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nosaxa"), "HD 48265", q_("Nosaxa means spring in the Moqoit language. The word comes from a combination of nosahuec, which means renew, and ñaaxa, which means year."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Naqaỹa"), "HD 48265 b", q_("Naqaỹa means brother-family-relative in the Moqoit language and leads us to call all humans, indigenous or non-indigenous, brother."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Malmok"), "WASP-39", q_("Malmok is an indigenous name given to a beach in Aruba with a narrow sandy stretch that interrupts the limestone and rocky terrace along the coast. Its shallow clear Caribbean waters make it a popular snorkelling spot."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bocaprins"), "WASP-39 b", q_("Boca Prins is a secluded beach with white dunes and iconic scenery situated in Arikok National Park along the northeast coast of Aruba. It is named after Plantation Prins where coconuts are cultivated."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bubup"), "HD 38283", q_("Bubup is the Boonwurrung word for child."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Yanyan"), "HD 38283 b", q_("YanYan is the Boonwurrung word for boy."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Franz"), "HAT-P-14", q_("Franz is a character in the movie 'Sissi' embodying an emperor of Austria in the XIXth century. The role is played by the actor Karlheinz Böhm."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sissi"), "HAT-P-14 b", q_("Sissi is a character in the movie 'Sissi', who is married with Franz. The role is played by the actress Romy Schneider."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mahsati"), "HD 152581", q_("Mahsati Ganjavi (1089–1159) is one of the brightest shining stars of Azerbaijani poetry. She was said to have associated with both Omar Khayyam and Nizami and was well educated and talented and played numerous musical instruments."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ganja"), "HD 152581 b", q_("Ganja is an ancient city of Azerbaijan, and is the birth place of many prominent people such as the poets Mahsati and Nizami. It is the ancient capital of Azerbaijan, the first capital of the Azerbaijan Democratic Republic and the city with the spirit of wisdom and freedom."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Timir"), "HD 148427", q_("Timir means darkness in Bengali language, alluding to the star being far away in the darkness of space."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tondra"), "HD 148427 b", q_("Tondra means nap in Bengali language, alluding to the symbolic notion that the planet was asleep until discovered."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nervia"), "HD 49674", q_("Nervia, adapted from Nervii, was a prominent Belgian Celtic tribe."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Eburonia"), "HD 49674 b", q_("Eburonia, adapted from Eburones, was a prominent Belgian Celtic tribe."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Gakyid"), "HD 73534", q_("Gakyid means happiness. Gross National Happiness is the development philosophy conceived and followed in Bhutan and is one of Bhutan's contributions to the world."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Drukyul"), "HD 73534 b", q_("Drukyul (land of the thunder dragon) is the native name for Bhutan, the country that came up with the philosophy of Gross National Happiness."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tapecue"), "HD 63765", q_("Tapecue means eternal path in Guarani and represents the Milky Way through which the first inhabitants of the Earth arrived and could return."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Yvaga"), "HD 63765 b", q_("Yvaga means paradise for the Guarani and the Milky Way was known as the road to Yvaga or paradise."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bosona"), "HD 206610", q_("Bosona is the name given to the territory of Bosnia in the 10th century. Later, the name was transformed to Bosnia originating from the name of the Bosna river."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Naron"), "HD 206610 b", q_("Naron is one of the names given to the Neretva river in Herzegovina (and partly in Croatia) in antiquity originating with the Celts who called it Nera Etwa which means the Flowing Divinity."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tupi"), "HD 23079", q_("Tupi is the name of the most populous Indigenous People living on the eastern coast of South America, before the Portuguese arrived in the 16th century."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Guarani"), "HD 23079 b", q_("Guarani is the name of the most populous Indigenous people living in South Brazil and parts of Argentina, Paraguay and Uruguay."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Gumala"), "HD 179949", q_("Gumala is a Malay word, which means a magic bezoar stone found in snakes, dragons, etc."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mastika"), "HD 179949 b", q_("Mastika is a Malay word, which means a gem, precious stone, jewel or the prettiest, the most beautiful."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tangra"), "WASP-21", q_("Tangra is the supreme celestial god that early Bulgars worshiped."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bendida"), "WASP-21 b", q_("Bendida is the Great Mother Goddess of the Thracians. She was especially revered as a goddess of marriage and living nature."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mouhoun"), "HD 30856", q_("Mouhoun, also called Volta Noire, is the largest river in Burkina Faso and plays an important role in the lives of the people in the areas it passes through."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nakanbé"), "HD 30856 b", q_("The Nakanbé, also called Volta Blanche, is the second largest river in Burkina Faso. Its source is in the heart of the Sahara Burkinabe and ends in Ghana."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nikawiy"), "HD 136418", q_("Nikawiy is the word for mother in the Indigenous Cree language of Canada."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Awasis"), "HD 136418 b", q_("Awasis is the word for child in the Indigenous Cree language of Canada."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Pincoya"), "HD 164604", q_("Pincoya is a female water spirit from southern Chilean mythology who is said to bring drowned sailors to the Caleuche so that they can live in the afterlife."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Caleuche"), "HD 164604 b", q_("Caleuche is a large ghost ship from southern Chilean mythology which sails the seas around the island of Chiloé at night."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Lionrock"), "HD 212771", q_("Lion Rock is a lion-shaped peak overlooking Hong Kong and is a cultural symbol with deep respect from the local community."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Victoriapeak"), "HD 212771 b", q_("Victoria Peak overlooks the bustling Victoria Harbour and is regarded as an ambassadorial gateway for foreign visitors wishing to experience Hong Kong first hand."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Xihe"), "HD 173416", q_("Xihe (羲和) is the goddess of the sun in the Chinese mythology and also represents the earliest astronomers and developers of calendars in ancient China."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Wangshu"), "HD 173416 b", q_("Wangshu (望舒) is the goddess who drives for the Moon and also represents the Moon in Chinese mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Formosa"), "HD 100655", q_("Formosa is the historical name of Taiwan used in the 17th century, meaning beautiful in Latin."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sazum"), "HD 100655 b", q_("Sazum is the traditional name of Yuchi, a Township in Nantou county, in which the famous Sun-Moon Lake lies. Sazum means water in the language of the Thao people who are a tribe of Taiwanese aborigines who lived in the region for hundreds of years."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Macondo"), "HD 93083", q_("Macondo is the mythical village of the novel One Hundred Years of Solitude (Cien años de soledad) the classic novel by Gabriel García Marquez. Macondo is a fictional place where magic and reality are mixed."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Melquíades"), "HD 93083 b", q_("Melquíades is a fictional character that walks around Macondo, like a planet orbiting a star, connecting it with the external world by introducing new knowledge using his inventions as well as his stories."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Poerava"), "HD 221287", q_("Poerava is the word in the Cook Islands Maori language for a large mystical black pearl of utter beauty and perfection."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Pipitea"), "HD 221287 b", q_("Pipitea is a small, white and gold pearl found in Penrhyn lagoon in the northern group of the Cook Islands."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dìwö"), "WASP-17", q_("Dìwö in Bribri language means the sun (bigger than the sun we know) and that never turns off."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ditsö̀"), "WASP-17 b", q_("Ditsö̀ is the name that the god Sibö̀ gave to the first Bribri people."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Stribor"), "HD 75898", q_("Stribor is God of winds in Slavic mythology, as well as a literature character in the book Priče iz davnine (Croatian Tales of Long Ago) by the Croatian author Ivana Brlić-Mažuranić."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Veles"), "HD 75898 b", q_("Veles is a major Slavic god of earth, waters and the underworld."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Felixvarela"), "BD-17 63", q_("Felix Varela (1788–1853) was the first to teach science in Cuba at the San Carlos and San Ambrosio Seminary. He opened the way to education for all, and began the experimental teaching of physics in Cuba."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Finlay"), "BD-17 63 b", q_("Carlos Juan Finlay (1833–1915) was an epidemiologist recognized as a pioneer in the research of yellow fever, determining that it was transmitted through mosquitoes."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Alasia"), "HD 168746", q_("Alasia is the first historically recorded name of Cyprus, dating back to mid-fifteenth century BC."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Onasilos"), "HD 168746 b", q_("Onasilos is the oldest historically recorded doctor in Cyprus, inscribed on the fifth century BC Idalion Tablet. Also known as the Onasilou Plate, it is considered as the oldest legal contract found in the world."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Absolutno"), "XO-5", q_("Absolutno is a fictional miraculous substance in the sci-fi novel Továrna na absolutno (The Factory for the Absolute) by influential Czech writer Karel Čapek."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Makropulos"), "XO-5 b", q_("Makropulos is the name from Karel Čapek's play Věc Makropulos (The Makropulos Affair), dealing with problems of immortality and consequences of an artificial prolongation of life."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Muspelheim"), "HAT-P-29", q_("Muspelheim is the Norse mythological realm of fire. The first gods used the sparks of Muspelheim to form the sun, moon, planets, and stars."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Surt"), "HAT-P-29 b", q_("Surt is the ruler of Muspelheim and the fire giants there in Norse mythology. At Ragnarok, the end of the world, he will lead the attack on our world and destroy it in flames."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Márohu"), "WASP-6", q_("Márohu the god of drought is the protector of the Sun and is engraved at a higher position on the stalagmite than Boinayel in the El Puente cave, where the Sun makes its way down every 21 December."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Boinayel"), "WASP-6 b", q_("Boinayel the god of rain that fertilizes the soil is engraved on the stalagmite at a lower position than Márohu in the El Puente cave."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nenque"), "HD 6434", q_("Nenque means the Sun in the language spoken by the Indigenous Waorani tribes of the Amazon regions of Ecuador"));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Eyeke"), "HD 6434 b", q_("Eyeke means near in the language spoken by the Indigenous Waorani tribes of the Amazon regions of Ecuador. This word is suggested for the exoplanet owing to the proximity of the planet to the host star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Citalá"), "HD 52265", q_("Citalá means River of stars in the native Nahuat language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Cayahuanca"), "HD 52265 b", q_("Cayahuanca means The rock looking at the stars in the native Nahuat language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Koit"), "XO-4", q_("Koit is Estonian for the time when the Sun rises in the morning (dawn)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hämarik"), "XO-4 b", q_("Hämarik is Estonian for the time when the Sun goes down in the evening (twilight)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Buna"), "HD 16175", q_("Buna is the commonly used word for coffee in Ethiopia."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Abol"), "HD 16175 b", q_("Abol is the first of three rounds of coffee in the Ethiopian traditional coffee ceremony."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Horna"), "HAT-P-38", q_("Horna is hell or the underworld from Finnic mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hiisi"), "HAT-P-38 b", q_("Hiisi represents sacred localities and later evil spirits from Finnic mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bélénos"), "HD 8574", q_("Bélénos was the god of light, of the Sun, and of health in Gaulish mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bélisama"), "HD 8574 b", q_("Bélisama was the goddess of fire, notably of the hearth and of metallurgy and glasswork, in Gaulish mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Itonda"), "HD 208487", q_("Itonda, in the Myene tongue, corresponds to all that is beautiful."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mintome"), "HD 208487 b", q_("Mintome, in the Fang tongue, is a mythical land where a brotherhood of brave men live."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mago"), "HD 32518", q_("Mago is a National Park in Ethiopia noted for its giraffes. The star also happens to be in the constellation of Camelopardis (the giraffe)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Neri"), "HD 32518 b", q_("The Neri river in Ethiopia runs through parts of the Mago National park."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sika"), "HD 181720", q_("Sika means gold in the Ewe language and gold is one of Ghana's principal exports."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Toge"), "HD 181720 b", q_("Toge means earring in the Ewe language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Lerna"), "HAT-P-42", q_("Lerna was the name of a lake in the eastern Peloponnese, where the Lernaean Hydra, an immortal mythical nine-headed beast lived. The star lies in the constellation of Hydra."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Iolaus"), "HAT-P-42 b", q_("Iolaus was the nephew of Heracles from Greek mythology, moving around lake Lerna in helping Heracles to exterminate the Lernaean Hydra. Similarly this exoplanet in the constellation of Hydra moves around its parent star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tojil"), "WASP-22", q_("Tojil is the name of one of the Mayan deities related to rain, storms, and fire."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Koyopa'"), "WASP-22 b", q_("Koyopa' is the word associated with lightning in K'iche' (Quiché) Mayan language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Citadelle"), "HD 1502", q_("The Citadelle is a large mountaintop fortress in Nord, Haiti built after Haiti's independence, and was designated a UNESCO World Heritage site along with the nearby Sans-Souci Palace."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Indépendance"), "HD 1502 b", q_("Indépendance is named after the Haitian Declaration of Independence on 1 January 1804, when Haiti became the first independent black republic."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hunahpú"), "HD 98219", q_("Hunahpú was one of the twin gods who became the Sun in K'iche' (Quiché) Mayan mythology as recounted in the Popol Vuh."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ixbalanqué"), "HD 98219 b", q_("Ixbalanqué was one of the twin gods who became the Moon in K'iche' (Quiché) Mayan mythology as recounted in the Popol Vuh."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hunor"), "HAT-P-2", q_("Hunor was a legendary ancestor of the Huns and the Hungarian nation, and brother of Magor."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Magor"), "HAT-P-2 b", q_("Magor was a legendary ancestor of the Magyar people and the Hungarian nation, and brother of Hunor."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Funi"), "HD 109246", q_("Funi is an old Icelandic word meaning fire or blaze."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Fold"), "HD 109246 b", q_("Fold is an old Icelandic word meaning earth or soil."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bibhā"), "HD 86081", q_("Bibhā [/bɪbɦa/] is the Bengali pronunciation of the Sanskrit word Vibha, which means a bright beam of light."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Santamasa"), "HD 86081 b", q_("Santamasa [/səntəməs/] in Sanskrit means clouded, which alludes to the exoplanet’s atmosphere."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dofida"), "HD 117618", q_("Dofida means our star in Nias language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Noifasui"), "HD 117618 b", q_("Noifasui means revolve around in Nias language, derived from the word ifasui, meaning to revolve around, and no, indicating that the action occurred in the past and continued to the present time."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Kaveh"), "HD 175541", q_("Kaveh is one of the heroes of Shahnameh, the epic poem composed by Persian poet Ferdowsi between 977 and 1010 CE. Kaveh is a blacksmith who symbolises justice."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Kavian"), "HD 175541 b", q_("Kaveh carries a banner called Derafsh Kaviani (Derafsh: banner, Kaviani: relating to Kaveh)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Uruk"), "HD 231701", q_("Uruk was an ancient city of the Sumer and Babylonian civilizations in Mesopotamia situated along an ancient channel of the Euphrates river in modern-day Iraq."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Babylonia"), "HD 231701 b", q_("Babylonia was a key kingdom in ancient Mesopotamia from the 18th to 6th centuries BC whose name-giving capital city was built on the Euphrates river."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tuiren"), "HAT-P-36", q_("Tuiren was the aunt of the hunterwarrior Fionn mac Cumhaill of Irish legend, who was turned into a hound by the jealous fairy Uchtdealbh."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bran"), "HAT-P-36 b", q_("Tuiren's son Bran was a hound and cousin of the hunterwarrior Fionn mac Cumhaill of Irish legend."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tevel"), "HAT-P-9", q_("Tevel means Universe or everything and begins with the letter Taf, the last letter in the Hebrew alphabet."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Alef"), "HAT-P-9 b", q_("Alef is the first letter in the Hebrew alphabet and also means bull."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Flegetonte"), "HD 102195", q_("Flegetonte is the underworld river of fire from Greek Mythology in the Italian narrative poem on the afterlife 'Divina Commedia' (Divine Commedy) by Dante Alighieri, chosen as an allusion to the star's fiery nature."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Lete"), "HD 102195 b", q_("Lete is the oblivion river made of fog from Greek Mythology in the Italian narrative poem on the afterlife 'Divina Commedia' (Divine Commedy) by Dante Alighieri, chosen as an allusion to the planet's gaseous nature."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nyamien"), "WASP-15", q_("Nyamien refers to the supreme creator deity of Akan mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Asye"), "WASP-15 b", q_("Asye refers to the Earth goddess of Akan mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Kamui"), "HD 145457", q_("Kamui is a word in the Ainu language denoting a supernatural entity possessing spiritual energy."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Chura"), "HD 145457 b", q_("Chura is a word in the Ryukyuan/Okinawan language meaning natural beauty."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Petra"), "WASP-80", q_("Petra is a historical and archaeological city in southern Jordan and a UNESCO World Heritage site."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Wadirum"), "WASP-80 b", q_("Wadi Rum (Valley of the Moon) is located at the far south of Jordan, it is the largest valley in Jordan, set on the high plateau at the western edge of the Arabian Desert."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Kalausi"), "HD 83443", q_("The word Kalausi means a very strong whirling column of wind in the Dholuo language of Kenya."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Buru"), "HD 83443 b", q_("Buru means dust in the Dholuo language of Kenya and is typically associated with wind storms."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Liesma"), "HD 118203", q_("Liesma means flame, and it is the name of a character from the Latvian poem Staburags un Liesma."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Staburags"), "HD 118203 b", q_("Staburags is the name of a character from the Latvian poem Staburags un Liesma, and denotes a rock with symbolic meaning in literature and history."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Phoenicia"), "HD 192263", q_("Phoenicia was an ancient thalassocratic civilisation of the Mediterranean that originated from the area of modern-day Lebanon."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Beirut"), "HD 192263 b", q_("Beirut is one of the oldest continuously inhabited cities in the world and was a Phoenician port. Beirut is now the capital and largest city of Lebanon."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Pipoltr"), "TrES-3", q_("In the local dialect of Triesenberg, Pipoltr is a bright and visible butterfly, alluding to the properties of a star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Umbäässa"), "TrES-3 b", q_("In the local dialect of southern Liechtenstein, Umbäässa is a small and barely visible ant, alluding to the properties of a planet with respect to its star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Taika"), "HAT-P-40", q_("Taika means peace in the Lithuanian language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Vytis"), "HAT-P-40 b", q_("Vytis is the symbol of the Lithuanian coat of arms."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Lucilinburhuc"), "HD 45350", q_("The Lucilinburhuc fortress was built in 963 by the founder of Luxembourg, Count Siegfried."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Peitruss"), "HD 45350 b", q_("Peitruss is derived from the name of the Luxembourg river Pétrusse, with the river's bend around Lucilinburhuc fortress alluding to the orbit of the planet around its star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Rapeto"), "HD 153950", q_("Rapeto is a giant creature from Malagasy tales."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Trimobe"), "HD 153950 b", q_("Trimobe is a rich ogre from Malagasy tales."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Intan"), "HD 20868", q_("Intan means diamond in the Malay language (Bahasa Melayu), alluding to the shining of a star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Baiduri"), "HD 20868 b", q_("Baiduri means opal in Malay language (Bahasa Melayu), alluding to the mysterious beauty of the planet."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sansuna"), "HAT-P-34", q_("Sansuna is the name of the mythological giant from traditional Maltese folk tales that carried the stones of the Gozo megalithic temples on her head."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ġgantija"), "HAT-P-34 b", q_("Ġgantija means giantess: the megalithic temple complex on the island of Gozo, which alludes to the grandeur of this gas giant exoplanet."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Diya"), "WASP-72", q_("Diya is an oil lamp that is brought by Indian ancestors to Mauritius in the 1820's, and is used for lighting during special occasions, including the light festival of Diwali."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Cuptor"), "WASP-72 b", q_("Cuptor is a thermally insulated chamber used for baking or drying substances, that has long disappeared in Mauritius and has been replaced by more sophisticated ovens."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Axólotl"), "HD 224693", q_("Axólotl means water animal in the native Nahuatl language, which is a unique and culturally significant endemic amphibious species from the basin of Mexico."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Xólotl"), "HD 224693 b", q_("Xólotl means animal in the native Nahuatl language and was an Aztec deity associated with the evening star (Venus)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tislit"), "WASP-161", q_("Tislit is the name of a lake in the Atlas mountains of Morocco. It means the bride in the Amazigh language and it is associated with a heartbroken beautiful girl in an ancient local legend."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Isli"), "WASP-161 b", q_("Isli is the name of a lake in the Atlas mountains of Morocco. It means the groom in the Amazigh language and it is associated with a heartbroken handsome boy in an ancient local legend."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Emiw"), "HD 7199", q_("Emiw represents love in the local Makhuwa language of the northern region of Mozambique."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Hairu"), "HD 7199 b", q_("Hairu represents unity in the local Makhuwa language of the northern region of Mozambique."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ayeyarwady"), "HD 18742", q_("Ayeyarwady is the largest and most important river in Myanmar."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Bagan"), "HD 18742 b", q_("Bagan is one of Myanmar's ancient cities that lies beside the Ayeyarwardy river."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sagarmatha"), "HD 100777", q_("Sagarmatha is the Nepali name for the highest peak in the world (also known as Mount Everest) and symbol of national pride of Nepal."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Laligurans"), "HD 100777 b", q_("Laligurans are the Nepali variation of the rhododendron flower and is the national flower of Nepal."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sterrennacht"), "HAT-P-6", q_("The Sterrennacht (Starry Night) is a world-famous painting by Dutch grand master Van Gogh that was painted in France in 1889 and now belongs to the permanent collection of New York's Museum of Modern Art."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nachtwacht"), "HAT-P-6 b", q_("The Nachtwacht (Night Watch) is a world-famous painting by Dutch grand master Rembrandt that was completed in 1642 and now belongs to the collection of the Rijksmuseum in Amsterdam."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Karaka"), "HD 137388", q_("Karaka is the word in the Māori language for a plant endemic to New Zealand that produces a bright orange, fleshy fruit."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Kererū"), "HD 137388 b", q_("Kererū is the word in the Māori language for a large bush pigeon native to New Zealand."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Cocibolca"), "HD 4208", q_("Cocibolca is the Nahualt name for the largest lake in Central America in Nicaragua."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Xolotlan"), "HD 4208 b", q_("Xolotlan is the name of the second largest lake of Nicaragua and its name is from the Nahualt language of the indigenous tribe that settled in Nicaragua, which symbolises a native god and a refuge for animals."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Amadioha"), "HD 43197", q_("Amadioha is the god of thunder in Igbo mythology. As well as representing justice, Amadioha is also a god of love, peace and unity."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Equiano"), "HD 43197 b", q_("Equiano was a writer and abolitionist from Ihiala, Nigeria who fought injustice and the elimination of the slave trade."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Násti"), "HD 68988", q_("Násti means star in the Northern Sami language of Norway."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Albmi"), "HD 68988 b", q_("Albmi means sky in the Northern Sami language of Norway."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Shama"), "HAT-P-23", q_("Shama is an Urdu literary term meaning a small lamp or flame, symbolic of the light of the star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Perwana"), "HAT-P-23 b", q_("Perwana means moth in Urdu, alluding to the eternal love of an object circling the source of light (the lamp)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Moriah"), "HD 99109", q_("Moriah is an ancient name for the mountain within the Old City of Jerusalem."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Jebus"), "HD 99109 b", q_("Jebus was the ancient name of Jerusalem in 2nd millennium BC when populated by the Canaanite tribe of Jebusites."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Montuno"), "WASP-79", q_("Montuno is the traditional costume the man wears in the “El Punto”, a Panamanian dance in which a man and woman dance to the sound of drums."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Pollera"), "WASP-79 b", q_("Pollera is the traditional costume the woman wears in the El Punto, a Panamanian dance in which a man and woman dance to the sound of drums."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tupã"), "HD 108147", q_("Tupã is one of four principle gods of the Guarani Cosmogony in popular Paraguayan folklore that helped the supreme god Ñamandu to create the Universe."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tumearandu"), "HD 108147 b", q_("Tume Arandu is a son of Rupavê and Sypavê, the original man and woman of the Universe, who is known as the Father of Wisdom in popular Paraguayan folklore."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Inquill"), "HD 156411", q_("Inquil was one half of the couple involved in the tragic love story Way to the Sun by famous Peruvian writer Abraham Valdelomar."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sumajmajta"), "HD 156411 b", q_("Sumaj Majta was one half of the couple involved in a tragic love story Way to the Sun by famous Peruvian writer Abraham Valdelomar."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Amansinaya"), "WASP-34", q_("Aman Sinaya is one of the two trinity deities of the Philippine's Tagalog mythology, and is the primordial deity of the ocean and protector of fisherman."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Haik"), "WASP-34 b", q_("Haik is the successor of the primordial Aman Sinaya as the God of the Sea of the Philippine's Tagalog mythology."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Uklun"), "HD 102117", q_("Uklun means us or we in the Pitkern language of the people of Pitcairn Islands."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Leklsullun"), "HD 102117 b", q_("Lekl Sullun means child or children in the Pitkern language of the people of Pitcairn Islands."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Solaris"), "BD+14 4559", q_("Solaris is the title of a 1961 science fiction novel about an ocean-covered exoplanet by Polish writer Stanislaw Lem."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Pirx"), "BD+14 4559 b", q_("Pirx is a fictional character from books by Polish science-fiction writer Stanisław Lem."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Lusitânia"), "HD 45652", q_("Lusitânia is the ancient name of the western region of the Iberic Peninsula where the Lusitanian people lived and where most of modern-day Portugal is situated."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Viriato"), "HD 45652 b", q_("Viriato was a legendary leader of the Lusitanian people, a herdsman and hunter who led the resistance against Roman invaders during 2nd century B.C."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Koeia"), "HIP 12961", q_("Koeia was the word for star in the language of the Taíno Indigenous People of the Caribbean."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Aumatex"), "HIP 12961 b", q_("Aumatex was the God of Wind in the mythology of the Taíno Indigenous People of the Caribbean."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Moldoveanu"), "XO-1", q_("Moldoveanu is the highest peak in Romania of the Făgăraș mountain range with an altitude of 2544 metres."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Negoiu"), "XO-1 b", q_("Negoiu is the second highest peak in Romania of the Făgăraș mountain range with an altitude of 2535 metres."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dombay"), "HAT-P-3", q_("Dombay is a resort region in the North Caucasus mountains that is enclosed by mountain forests and rich wildlife, including bears (as this star lies in the constellation Ursa Major, the great bear)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Teberda"), "HAT-P-3 b", q_("Teberda is a mountain river in Dombay region with a rapid water flow, symbolising the planet's rapid motion around its host star."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Belel"), "HD 181342", q_("Belel is a rare source of water in the north of Senegal."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dopere"), "HD 181342 b", q_("Dopere is an expansive historical area in the north of Senegal where Belel was located."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Morava"), "WASP-60", q_("Morava is the longest river system in Serbia."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Vlasina"), "WASP-60 b", q_("Vlasina is one of the most significant tributaries of the South Morava river."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Parumleo"), "WASP-32", q_("Parumleo is a Latin term for little lion, symbolising Singapore's struggle for independence."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Viculus"), "WASP-32 b", q_("Viculus is a Latin term for little village, embodying the spirit of the Singaporean people."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Chasoň"), "HAT-P-5", q_("Chasoň is an ancient Slovak term for Sun."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Kráľomoc"), "HAT-P-5 b", q_("Kráľomoc is an ancient Slovak term for the planet Jupiter."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Irena"), "WASP-38", q_("Irena is a leading character in the novel Under the Free Sun: a Story of the Ancient Grandfathers by Slovene writer Fran Saleški Finžgar. Irena is a woman of the court."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Iztok"), "WASP-38 b", q_("Iztok is a leading character in the novel Under the Free Sun: a Story of the Ancient Grandfathers by Slovene writer Fran Saleški Finžgar. Iztok is a freedom fighter for the Slavic people."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Naledi"), "WASP-62", q_("Naledi means star in the Sesotho, SeTswana and SePedi languages and is typically given as a name to girls in the hope that they will bring light, joy and peace to their communities."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Krotoa"), "WASP-62 b", q_("Krotoa is considered the Mother of Africa and member of the indigenous Khoi people, who was a community builder and educator during colonial times."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Baekdu"), "8 Umi", q_("Baekdu is the highest mountain on the Korean peninsula, situated in North Korea, and symbolises the national spirit of Korea."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Halla"), "8 Umi b", q_("Halla is the highest mountain in South Korea and is regarded as a sacred place in the region."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Rosalíadecastro"), "HD 149143", q_("Rosalía de Castro was a significant figure of Galician culture and prominent Spanish writer, whose pioneering work often referenced the night and celestial objects."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Riosar"), "HD 149143 b", q_("Rio Sar is the name of a river that was present in much of the literary work of the pioneering Spanish author Rosalía de Castro."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sāmaya"), "HD 205739", q_("Sāmaya means peace in the Sinhalese language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Samagiya"), "HD 205739 b", q_("Samagiya means togetherness and unity in the Sinhalese language."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Aniara"), "HD 102956", q_("Aniara is the name of a spaceship in the epic poem Aniara by Swedish author Harry Martinson."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Isagel"), "HD 102956 b", q_("Isagel is the name of the spaceship pilot in the epic science fiction poem Aniara written by Swedish author Harry Martinson."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mönch"), "HD 130322", q_("Mönch is one of the prominent peaks of the Bernese Alps in Switzerland."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Eiger"), "HD 130322 b", q_("Eiger is one of the prominent peaks of the Bernese Alps, in the Jungfrau-Aletsch protected area."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ebla"), "HD 218566", q_("Ebla was one of the earliest kingdoms in Syria, and served as a prominent region in the 2nd and 3rd millenia B.C."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ugarit"), "HD 218566 b", q_("Ugarit was a city where its scribes devised the Ugaritic alphabet around 1400 B.C. The alphabet was made up of thirty letters and was inscribed on clay tablets."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mpingo"), "WASP-71", q_("Mpingo is a famous tree that grows in southern Tanzania and produces ebony wood used for musical instruments and curios."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tanzanite"), "WASP-71 b", q_("Tanzanite is the name of a precious stone mined in Tanzania and is treasured worldwide."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Chaophraya"), "WASP-50", q_("Chao Phraya is the great river of Thailand."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Maeping"), "WASP-50 b", q_("Mae Ping is one of the tributaries of Thailand's great river Chao Phraya."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Atakoraka"), "WASP-64", q_("Atakoraka means the chain of the Atacora: the largest mountain range in Togo."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Agouto"), "WASP-64 b", q_("Agouto (Mount Agou) is the highest mountain in Togo and a treasured region of the Atakoraka."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Dingolay"), "HD 96063", q_("Dingolay means to dance, twist and turn in elaborate movements, symbolising the culture and language of the ancestors of the people of Trinidad and Tobago."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ramajay"), "HD 96063 b", q_("Ramajay means to sing and make music in a steelpan style, representing the love of culture and languages of the ancestors of the people of Trinidad and Tobago."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Chechia"), "HD 192699", q_("Chechia is a flat-surfaced, traditional red wool hat worn by men and women, symbolising the country's rich traditions and is considered as the national headdress for in Tunisia."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Khomsa"), "HD 192699 b", q_("Khomsa is a palm-shaped amulet that is popular in Tunisia, used in jewelry and decorations. It depicts an open right hand and is often found in modern designs."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Anadolu"), "WASP-52", q_("Anadolu is the primary homeland of Turkey and refers to the motherland in Turkish culture."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Göktürk"), "WASP-52 b", q_("Göktürk refers to the historical origin of the Turkish people, as it was the first established state in Turkey in 5th century AD. It is also the name of a Turkish satellite and is the combination of two words, of which 'Gök' means sky."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Berehinya"), "HAT-P-15", q_("Berehinya was a Slavic deity of waters and riverbanks but in more recent times her status has been promoted to that of a national goddess — 'hearth mother, protectress of the earth'."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Tryzub"), "HAT-P-15 b", q_("Tryzub is the most recognised ancient symbol of Ukraine, that was minted on the coins of Prince Volodymyr the Great and today remains one of the country's state symbols (a small coat)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Sharjah"), "HIP 79431", q_("Sharjah is the cultural capital of United Arab Emirates, and considered the city of knowledge due to its many educational centers, institutes, museums, libraries and heritage centers."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Barajeel"), "HIP 79431 b", q_("A barajeel is a wind tower used to direct the flow of the wind so that air can be recirculated as a form of air conditioning."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Gloas"), "WASP-13", q_("In Manx Gaelic, Gloas means to shine (like a star)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Cruinlagh"), "WASP-13 b", q_("In Manx Gaelic, Cruinlagh means to orbit (like a planet around its star)."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Nushagak"), "HD 17156", q_("Nushagak is a regional river near Dilingham, Alaska, which is famous for its wild salmon that sustain local Indigenous communities."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Mulchatna"), "HD 17156 b", q_("The Mulchatna River is a tributary of the Nushagak River in southwestern Alaska, USA."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ceibo"), "HD 63454", q_("Ceibo is the name of the native tree of Uruguay that gives rise to the national flower."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Ibirapitá"), "HD 63454 b", q_("Ibirapitá is the name of a native tree that is characteristic of the country of Uruguay, and is also known as Artigas' tree, after the national hero."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Natasha"), "HD 85390", q_("Natasha means thank you in many languages of Zambia."));
	html += QString("<li><strong>%1</strong> (%2) &mdash; %3</li>").arg(trans.qtranslate("Madalitso"), "HD 85390 b", q_("Madalitso means blessings in the native language of Nyanja in Zambia."));
	html += QString("</ul><p>%1:").arg(q_("Notes"));
	html += QString("<br /><sup>*</sup> %1").arg(q_("These names are modified based on the original proposals, to be consistent with the IAU rules."));
	html += QString("<br /><sup>1</sup> %1").arg(q_("The original name proposed, <em>Veritas</em>, is that of an asteroid important for the study of the solar system."));
	html += QString("<br /><sup>2</sup> %1").arg(q_("The name originally proposed, <em>Amaterasu</em>, is already used for an asteroid."));
	html += QString("<br /><sup>3</sup> %1").arg(q_("Note the typographical difference between <em>AEgir</em> and <em>Aegir</em>, the Norwegian transliteration. The same name, with the spelling <em>Aegir</em>, has been attributed to one of Saturn's satellites, discovered in 2004."));
	html += QString("<br /><sup>4</sup> %1").arg(q_("<em>Ogmios</em> is a name already attributed to an asteroid."));
	html += QString("<br /><sup>5</sup> %1").arg(q_("The original proposed name <em>Leda</em> is already attributed to an asteroid and to one of Jupiter's satellites. The name <em>Althaea</em> is also attributed to an asteroid."));
	html += QString("<br /><sup>6</sup> %1</p>").arg(q_("The original spelling of <em>Lippershey</em> was corrected to <em>Lipperhey</em> on 15.01.2016. The commonly seen spelling <em>Lippershey</em> (with an <em>s</em>) results in fact from a typographical error dating back from 1831, thus should be avoided."));
	html += "<h2>" + q_("Additional info") + "</h2><ul>";
	html += QString("<li><a href='https://en.wikipedia.org/wiki/Circumstellar_habitable_zone'>%1</a> (en)</li>").arg(q_("Circumstellar habitable zone"));
	html += QString("<li><a href='https://en.wikipedia.org/wiki/Planetary_equilibrium_temperature'>%1</a> (en)</li>").arg(q_("Planetary equilibrium temperature"));
	html += QString("<li><a href='https://en.wikipedia.org/wiki/Planetary_habitability'>%1</a> (en)</li>").arg(q_("Planetary habitability"));
	html += QString("<li><a href='https://en.wikipedia.org/wiki/Earth_Similarity_Index'>%1</a> (en)</li>").arg(q_("Earth Similarity Index"));
	html += QString("<li><a href='http://www.iau.org/news/pressreleases/detail/iau1514/'>%1</a> (en)</li>").arg(q_("Final Results of NameExoWorlds Public Vote Released"));
	html += QString("<li><a href='http://nameexoworlds.iau.org/'>%1</a> (en)</li>").arg(q_("NameExoWorlds website"));
	html += "</ul></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
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
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->websitesTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->websitesTextBrowser->setHtml(html);
}


void ExoplanetsDialog::refreshUpdateValues(void)
{
	QString nextUpdate = q_("Next update");
	ui->lastUpdateDateTimeEdit->setDateTime(ep->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(ep->getUpdateFrequencyHours());
	int secondsToUpdate = ep->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(ep->getUpdatesEnabled());
	if (!ep->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (ep->getUpdateState() == Exoplanets::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(QString("%1: %2").arg(nextUpdate, q_("< 1 minute")));
	else if (secondsToUpdate < 3600)
	{
		int n = (secondsToUpdate/60)+1;
		// TRANSLATORS: minutes.
		ui->nextUpdateLabel->setText(QString("%1: %2 %3").arg(nextUpdate, QString::number(n), qc_("m", "time")));
	}
	else
	{
		int n = (secondsToUpdate/3600)+1;
		// TRANSLATORS: hours.
		ui->nextUpdateLabel->setText(QString("%1: %2 %3").arg(nextUpdate, QString::number(n), qc_("h", "time")));
	}
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

void ExoplanetsDialog::setDisplayShowExoplanetsDesignations(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ep->setFlagShowExoplanetsDesignations(b);
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
	setAboutHtml();	
	fillExoplanetsTable();
}

void ExoplanetsDialog::restoreDefaults(void)
{
	if (askConfirmation())
	{
		qDebug() << "[Exoplanets] restore defaults...";
		ep->restoreDefaults();
		ep->loadConfiguration();
		updateGuiFromSettings();
	}
	else
		qDebug() << "[Exoplanets] restore defaults is canceled...";
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

	double minX = *std::min_element(aX.begin(), aX.end());
	double minY = *std::min_element(aY.begin(), aY.end());
	double maxX = *std::max_element(aX.begin(), aX.end());
	double maxY = *std::max_element(aY.begin(), aY.end());

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

	QColor axisColor(Qt::white);
	QPen axisPen(axisColor, 1);

	ui->customPlot->setBackground(QBrush(QColor(86, 87, 90)));
	ui->customPlot->xAxis->setLabelColor(axisColor);
	ui->customPlot->xAxis->setTickLabelColor(axisColor);
	ui->customPlot->xAxis->setBasePen(axisPen);
	ui->customPlot->xAxis->setTickPen(axisPen);
	ui->customPlot->xAxis->setSubTickPen(axisPen);
	ui->customPlot->yAxis->setLabelColor(axisColor);
	ui->customPlot->yAxis->setTickLabelColor(axisColor);
	ui->customPlot->yAxis->setBasePen(axisPen);
	ui->customPlot->yAxis->setTickPen(axisPen);
	ui->customPlot->yAxis->setSubTickPen(axisPen);

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

	const QList<axisPair> axis = {
	{ q_("Orbital Eccentricity"),          0},
	{ q_("Orbit Semi-Major Axis, AU"),     1},
	{ q_("Planetary Mass, Mjup"),          2},
	{ q_("Planetary Radius, Rjup"),        3},
	{ q_("Orbital Period, days"),          4},
	{ q_("Angular Distance, arc-sec"),      5},
	{ q_("Effective temperature of host star, K"), 6},
	{ q_("Year of Discovery"),             7},
	{ q_("Metallicity of host star"),      8},
	{ q_("V magnitude of host star, mag"), 9},
	{ q_("RA (J2000) of star, deg"),      10},
	{ q_("Dec (J2000) of star, deg"),     11},
	{ q_("Distance to star, pc"),         12},
	{ q_("Mass of host star, Msol"),      13},
	{ q_("Radius of host star, Rsol"),    14}};

	for(int i=0; i<axis.size(); ++i)
	{
		axisX->addItem(axis.at(i).first, axis.at(i).second);
		axisY->addItem(axis.at(i).first, axis.at(i).second);
	}

	//Restore the selection
	indexX = axisX->findData(selectedAxisX, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexX<0)
		indexX = 1;
	indexY = axisY->findData(selectedAxisY, Qt::UserRole, Qt::MatchCaseSensitive);
	if (indexY<0)
		indexY = 0;
	axisX->setCurrentIndex(indexX);
	axisY->setCurrentIndex(indexY);
	axisX->blockSignals(false);
	axisY->blockSignals(false);
}

void ExoplanetsDialog::populateTemperatureScales()
{
	Q_ASSERT(ui->temperatureScaleComboBox);

	QComboBox* tscale = ui->temperatureScaleComboBox;

	//Save the current selection to be restored later
	tscale->blockSignals(true);
	int index = tscale->currentIndex();
	QVariant selectedTScaleId = tscale->itemData(index);
	tscale->clear();

	// TRANSLATORS: Name of temperature scale
	tscale->addItem(qc_("Kelvin", "temperature scale"), "Kelvin");
	// TRANSLATORS: Name of temperature scale
	tscale->addItem(qc_("Celsius", "temperature scale"), "Celsius");
	// TRANSLATORS: Name of temperature scale
	tscale->addItem(qc_("Fahrenheit", "temperature scale"), "Fahrenheit");

	//Restore the selection
	index = tscale->findData(selectedTScaleId, Qt::UserRole, Qt::MatchCaseSensitive);
	tscale->setCurrentIndex(index);
	tscale->blockSignals(false);
}

void ExoplanetsDialog::setTemperatureScale(int tScaleID)
{
	QString currentTScaleID = ui->temperatureScaleComboBox->itemData(tScaleID).toString();
	ep->setCurrentTemperatureScaleKey(currentTScaleID);
}
