/*
 * ArchaeoLines plug-in for Stellarium
 *
 * Copyright (C) 2014 Georg Zotti
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

#include "ArchaeoLines.hpp"
#include "ArchaeoLinesDialog.hpp"
#include "ui_archaeoLinesDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

ArchaeoLinesDialog::ArchaeoLinesDialog()
	: al(NULL)
{
	ui = new Ui_archaeoLinesDialog();
}

ArchaeoLinesDialog::~ArchaeoLinesDialog()
{
	delete ui;
}

void ArchaeoLinesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void ArchaeoLinesDialog::createDialogContent()
{
	al = GETSTELMODULE(ArchaeoLines);
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->aboutTextBrowser;
	installKineticScrolling(addscroll);
#endif

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	ui->useDmsFormatCheckBox->setChecked(al->isDmsFormat());
	connect(ui->useDmsFormatCheckBox, SIGNAL(toggled(bool)), al, SLOT(useDmsFormat(bool)));

	ui->solarZenithCheckBox->setChecked(al->isSolarZenithDisplayed());
	connect(ui->solarZenithCheckBox, SIGNAL(toggled(bool)), al, SLOT(showSolarZenith(bool)));


	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveArchaeoLinesSettings()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(resetArchaeoLinesSettings()));

	setAboutHtml();
}

void ArchaeoLinesDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("ArchaeoLines Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + ARCHAEOLINES_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Georg Zotti</td></tr>";
	//html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td> List with br separators </td></tr>";
	html += "</table>";

	html += "<p>" + q_("The ArchaeoLines plugin is a small tool which displays declination arcs most relevant to archaeoastronomical studies.") + "</p>";
	html += "<ul><li>" + q_("Declinations of the solstices") + "</li>";
	html += "<li>" + q_("Declinations of the crossquarter days (days right between solstices and equinoxes)") + "</li>";
	html += "<li>" + q_("Declinations of the Major Lunar Standstills (including horizon parallax effects)") + "</li>";
	html += "<li>" + q_("Declinations of the Minor Lunar Standstills (including horizon parallax effects)") + "</li>";
	html += "<li>" + q_("Declinations of the Solar Zenith passage (if applicable for current location)") + "</li>";
	html += "<li>" + q_("Declinations of the Solar Nadir passage (if applicable for current location)") + "</li></ul>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("ArchaeoLines plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	//// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	//html += "<li>" + q_("If you want to read full information about this plugin and its history, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/ArchaeoLines_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=NULL)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void ArchaeoLinesDialog::saveArchaeoLinesSettings()
{
	al->saveSettings();
}

void ArchaeoLinesDialog::resetArchaeoLinesSettings()
{
	al->restoreDefaultSettings();
}
