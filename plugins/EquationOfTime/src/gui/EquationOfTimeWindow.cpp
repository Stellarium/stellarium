/*
 * Equation of Time plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
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

#include "EquationOfTime.hpp"
#include "EquationOfTimeWindow.hpp"
#include "ui_equationOfTimeWindow.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

EquationOfTimeWindow::EquationOfTimeWindow()
	: StelDialog("EquationOfTime")
	, eq(Q_NULLPTR)
{
	ui = new Ui_equationOfTimeWindowForm();
}

EquationOfTimeWindow::~EquationOfTimeWindow()
{
	delete ui;
}

void EquationOfTimeWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void EquationOfTimeWindow::createDialogContent()
{
	eq = GETSTELMODULE(EquationOfTime);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	ui->checkBoxEnableAtStartup->setChecked(eq->getFlagEnableAtStartup());
	connect(ui->checkBoxEnableAtStartup, SIGNAL(clicked(bool)), eq, SLOT(setFlagEnableAtStartup(bool)));

	ui->checkBoxInvertedValue->setChecked(eq->getFlagInvertedValue());
	connect(ui->checkBoxInvertedValue, SIGNAL(clicked(bool)), eq, SLOT(setFlagInvertedValue(bool)));

	ui->checkBoxMsFormat->setChecked(eq->getFlagMsFormat());
	connect(ui->checkBoxMsFormat, SIGNAL(clicked(bool)), eq, SLOT(setFlagMsFormat(bool)));

	ui->spinBoxFontSize->setValue(eq->getFontSize());
	connect(ui->spinBoxFontSize, SIGNAL(valueChanged(int)), eq, SLOT(setFontSize(int)));

	ui->checkBoxShowButton->setChecked(eq->getFlagShowEOTButton());
	connect(ui->checkBoxShowButton, SIGNAL(clicked(bool)), eq, SLOT(setFlagShowEOTButton(bool)));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveEquationOfTimeSettings()));	
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetEquationOfTimeSettings()));

	connectColorButton(ui->textColorButton, "EquationOfTime.textColor", "EquationOfTime/text_color");

	setAboutHtml();
}

void EquationOfTimeWindow::setAboutHtml()
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Equation of Time plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + EQUATIONOFTIME_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + EQUATIONOFTIME_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin shows the solution of the equation of time.") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Equation of Time plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Equation_of_Time_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void EquationOfTimeWindow::saveEquationOfTimeSettings()
{
	eq->saveSettingsToConfig();
}

void EquationOfTimeWindow::resetEquationOfTimeSettings()
{
	if (askConfirmation())
	{
		qDebug() << "[EquationOfTime] restore defaults...";
		eq->restoreDefaults();
	}
	else
		qDebug() << "[EquationOfTime] restore defaults is canceled...";
}
