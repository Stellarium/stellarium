/*
 * Stellarium Observability Plug-in GUI
 *
 * Copyright (C) 2012 Ivan Marti-Vidal (based on code by Alexander Wolf)
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
#include <QFileDialog>

#include "StelApp.hpp"
#include "ui_ObservabilityDialog.h"
#include "ObservabilityDialog.hpp"
#include "Observability.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

ObservabilityDialog::ObservabilityDialog()
{
        ui = new Ui_ObservabilityDialog;
}

ObservabilityDialog::~ObservabilityDialog()
{
	delete ui;
}

void ObservabilityDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
		setAboutHtml();

}

// Initialize the dialog widgets and connect the signals/slots
void ObservabilityDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));

	// Settings:
	connect(ui->Today, SIGNAL(stateChanged(int)), this, SLOT(setTodayFlag(int)));
	connect(ui->Heliacal, SIGNAL(stateChanged(int)), this, SLOT(setHeliacalFlag(int)));
	connect(ui->Opposition, SIGNAL(stateChanged(int)), this, SLOT(setOppositionFlag(int)));
	connect(ui->Goods, SIGNAL(stateChanged(int)), this, SLOT(setGoodDatesFlag(int)));
	connect(ui->Red, SIGNAL(sliderMoved(int)), this, SLOT(setRed(int)));
	connect(ui->Green, SIGNAL(sliderMoved(int)), this, SLOT(setGreen(int)));
	connect(ui->Blue, SIGNAL(sliderMoved(int)), this, SLOT(setBlue(int)));
	connect(ui->fontSize, SIGNAL(sliderMoved(int)), this, SLOT(setSize(int)));

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	updateGuiFromSettings();

}

void ObservabilityDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Observability Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td>" + q_("Version:") + "</td><td> 1.0.0 </td></tr>";
	html += "<tr><td>" + q_("Author:") + "</td><td>Ivan Marti-Vidal &lt;i.martividal@gmail.com&gt;</td></tr></table>";

	html += "<p>" + q_("Plugin that analyzes the observability of the selected source (or the screen center, if no source is selected). The plugin can show rise, transit, and set times, as well as the best epoch of the year (i.e., Sun's opposition), the date range when the source is above the horizon at astronomical night (i.e., with Sun elevation below -12 degrees), and the dates of Heliacal rize/set.<br>Ephemeris of the Solar-System objects and parallax effects are taken into account.<br><br> The author thanks Alexander Wolf for his advice.<br><br>Ivan Marti-Vidal (Onsala Space Observatory)") + "</p>";
	html += "</body></html>";

	ui->aboutTextBrowser->setHtml(html);
}


void ObservabilityDialog::restoreDefaults(void)
{
	qDebug() << "Observability::restoreDefaults";
	GETSTELMODULE(Observability)->restoreDefaults();
	GETSTELMODULE(Observability)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void ObservabilityDialog::updateGuiFromSettings(void)
{
	ui->Today->setChecked(GETSTELMODULE(Observability)->getShowFlags(1));
	ui->Heliacal->setChecked(GETSTELMODULE(Observability)->getShowFlags(2));
	ui->Goods->setChecked(GETSTELMODULE(Observability)->getShowFlags(3));
	ui->Opposition->setChecked(GETSTELMODULE(Observability)->getShowFlags(4));

	Vec3f currFont = GETSTELMODULE(Observability)->getFontColor();
	int Rv = (int)(100.*currFont[0]);
	int Gv = (int)(100.*currFont[1]);
	int Bv = (int)(100.*currFont[2]);
	ui->Red->setValue(Rv);
	ui->Green->setValue(Gv);
	ui->Blue->setValue(Bv);
	ui->fontSize->setValue(GETSTELMODULE(Observability)->getFontSize());
}

void ObservabilityDialog::saveSettings(void)
{
	GETSTELMODULE(Observability)->saveSettingsToConfig();
}

void ObservabilityDialog::setTodayFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setTodayShow(b);
}

void ObservabilityDialog::setHeliacalFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setHeliacalShow(b);
}

void ObservabilityDialog::setOppositionFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setOppositionShow(b);
}

void ObservabilityDialog::setGoodDatesFlag(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Observability)->setGoodDatesShow(b);
}

void ObservabilityDialog::setRed(int Value)
{
	GETSTELMODULE(Observability)->setFontColor(0,Value);
}

void ObservabilityDialog::setGreen(int Value)
{
	GETSTELMODULE(Observability)->setFontColor(1,Value);
}

void ObservabilityDialog::setBlue(int Value)
{
	GETSTELMODULE(Observability)->setFontColor(2,Value);
}

void ObservabilityDialog::setSize(int Value)
{
	GETSTELMODULE(Observability)->setFontSize(Value);
}


