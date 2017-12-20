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
#include "StelLocaleMgr.hpp"

#include "external/qcustomplot/qcustomplot.h"

ExoplanetsDialog::ExoplanetsDialog()
	: StelDialog("Exoplanets")
	, ep(Q_NULLPTR)
	, updateTimer(Q_NULLPTR)
{
        ui = new Ui_exoplanetsDialog;
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
	ui->displayShowDesignationsCheckBox->setChecked(ep->getFlagShowExoplanetsDesignations());
	connect(ui->displayShowDesignationsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDisplayShowExoplanetsDesignations(int)));
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateJSON()));
	connect(ep, SIGNAL(updateStateChanged(Exoplanets::UpdateState)), this, SLOT(updateStateReceiver(Exoplanets::UpdateState)));
	connect(ep, SIGNAL(jsonUpdateComplete(void)), this, SLOT(updateCompleteReceiver(void)));
	connect(ep, SIGNAL(jsonUpdateComplete(void)), ep, SLOT(reloadCatalog()));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	colorButton(ui->exoplanetMarkerColor,		ep->getMarkerColor(false));
	colorButton(ui->habitableExoplanetMarkerColor,	ep->getMarkerColor(true));

	connect(ui->exoplanetMarkerColor,		SIGNAL(released()), this, SLOT(askExoplanetsMarkerColor()));
	connect(ui->habitableExoplanetMarkerColor,	SIGNAL(released()), this, SLOT(askHabitableExoplanetsMarkerColor()));

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

	// About & Info tabs
	setAboutHtml();
	setInfoHtml();
	setWebsitesHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
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
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + EXOPLANETS_PLUGIN_LICENSE + "</td></tr>";
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
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
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
	html += "<h2>" + q_("Proper names") + "</h2>";
	html += "<p>" + q_("In December 2015, the International Astronomical Union (IAU) has officially approved names for several exoplanets after a public vote.") + "</p><ul>";
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
	{
		int n = (secondsToUpdate/60)+1;
		ui->nextUpdateLabel->setText(qn_("Next update: %1 minute(s)", n).arg(n));
	}
	else
	{
		int n = (secondsToUpdate/3600)+1;
		ui->nextUpdateLabel->setText(qn_("Next update: %1 hour(s)", n).arg(n));
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
	qDebug() << "[Exoplanets] Updating of catalog is complete";
        ui->nextUpdateLabel->setText(QString(q_("Exoplanets is updated")));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(ep->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void ExoplanetsDialog::restoreDefaults(void)
{
	qDebug() << "[Exoplanets] Restore defaults...";
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
	axis.append(qMakePair(q_("Angular Distance, arcsec"), 5));
	axis.append(qMakePair(q_("Effective temperature of host star, K"), 6));
	axis.append(qMakePair(q_("Year of Discovery"), 7));
	axis.append(qMakePair(q_("Metallicity of host star"), 8));
	axis.append(qMakePair(q_("V magnitude of host star, mag"), 9));
	axis.append(qMakePair(q_("RA (J2000) of star, deg"), 10));
	axis.append(qMakePair(q_("Dec (J2000) of star, deg"), 11));
	axis.append(qMakePair(q_("Distance to star, pc"), 12));
	axis.append(qMakePair(q_("Mass of host star, Msol"), 13));
	axis.append(qMakePair(q_("Radius of host star, Rsol"), 14));

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

void ExoplanetsDialog::askExoplanetsMarkerColor()
{
	Vec3f vColor = ep->getMarkerColor(false);
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->exoplanetMarkerColor->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		ep->setMarkerColor(vColor, false);
		ui->exoplanetMarkerColor->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ExoplanetsDialog::askHabitableExoplanetsMarkerColor()
{
	Vec3f vColor = ep->getMarkerColor(true);
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	QColor c = QColorDialog::getColor(color, Q_NULLPTR, q_(ui->habitableExoplanetMarkerColor->toolTip()));
	if (c.isValid())
	{
		vColor = Vec3f(c.redF(), c.greenF(), c.blueF());
		ep->setMarkerColor(vColor, true);
		ui->habitableExoplanetMarkerColor->setStyleSheet("QToolButton { background-color:" + c.name() + "; }");
	}
}

void ExoplanetsDialog::colorButton(QToolButton* toolButton, Vec3f vColor)
{
	QColor color(0,0,0);
	color.setRgbF(vColor.v[0], vColor.v[1], vColor.v[2]);
	// Use style sheet for create a nice buttons :)
	toolButton->setStyleSheet("QToolButton { background-color:" + color.name() + "; }");
	toolButton->setFixedSize(QSize(18, 18));
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
