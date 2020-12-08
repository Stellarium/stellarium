/*
 * Angle Measure plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf, Georg Zotti
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

#include "AngleMeasure.hpp"
#include "AngleMeasureDialog.hpp"
#include "ui_angleMeasureDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

AngleMeasureDialog::AngleMeasureDialog()
	: StelDialog("AngleMeasure")
{
	ui = new Ui_angleMeasureDialog();
}

AngleMeasureDialog::~AngleMeasureDialog()
{
	delete ui;
}

void AngleMeasureDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void AngleMeasureDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectBoolProperty(ui->useDmsFormatCheckBox, "AngleMeasure.dmsFormat");
	connectBoolProperty(ui->showPositionAngleCheckBox, "AngleMeasure.flagShowEquatorialPA");
	connectBoolProperty(ui->showPositionAngleHorizontalCheckBox, "AngleMeasure.flagShowHorizontalPA");
	connectBoolProperty(ui->showEquatorial_GroupBox, "AngleMeasure.flagShowEquatorial");
	connectBoolProperty(ui->showHorizontal_GroupBox, "AngleMeasure.flagShowHorizontal");
	connectBoolProperty(ui->azAltStartOnSkyCheckBox, "AngleMeasure.flagShowHorizontalStartSkylinked");
	connectBoolProperty(ui->azAltEndOnSkyCheckBox,   "AngleMeasure.flagShowHorizontalEndSkylinked");
	connectColorButton(ui->equatorialLineColorToolButton, "AngleMeasure.equatorialLineColor", "AngleMeasure/line_color");
	connectColorButton(ui->equatorialTextColorToolButton, "AngleMeasure.equatorialTextColor", "AngleMeasure/text_color");
	connectColorButton(ui->horizontalLineColorToolButton, "AngleMeasure.horizontalLineColor", "AngleMeasure/line_color_horizontal");
	connectColorButton(ui->horizontalTextColorToolButton, "AngleMeasure.horizontalTextColor", "AngleMeasure/text_color_horizontal");

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));

	setAboutHtml();
}

void AngleMeasureDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[AngleMeasure] restore defaults...";
		GETSTELMODULE(AngleMeasure)->restoreDefaultSettings();
	}
	else
		qDebug() << "[AngleMeasure] restore defaults is canceled...";
}

void AngleMeasureDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Angle Measure Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + ANGLEMEASURE_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + ANGLEMEASURE_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Matthew Gates</td></tr>";
	html += "<tr><td rowspan=3><strong>" + q_("Contributors") + ":</strong></td><td>Bogdan Marinov</td></tr>";
	html += "<tr><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "<tr><td>Georg Zotti</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Angle Measure plugin is a small tool which is used to measure the angular distance between two points on the sky (and calculation of position angle between those two points).") + "</p>";
	html += "<p>" + q_("Start and end points in horizontal mode can be linked to the rotating sky, which may be helpful to keep relations between landscape and some celestial object or (with both linked) for Dobsonian starhopping.") + "</p>";
	html += "<p>" + q_("*goes misty eyed* I recall measuring the size of the Cassini Division when I was a student. It was not the high academic glamor one might expect... It was cloudy... It was rainy... The observatory lab had some old scopes set up at one end, pointing at a <em>photograph</em> of Saturn at the other end of the lab. We measured. We calculated. We wished we were in Hawaii.") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Angle Measure plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/AngleMeasure_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}
