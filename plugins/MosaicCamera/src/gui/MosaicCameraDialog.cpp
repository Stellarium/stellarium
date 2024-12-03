/*
 * Mosaic Camera Plug-in GUI
 *
 * Copyright (C) 2024 Josh Meyers
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

// #include <QFileDialog>

#include "StelApp.hpp"
#include "ui_MosaicCameraDialog.h"
#include "MosaicCameraDialog.hpp"
#include "MosaicCamera.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"


MosaicCameraDialog::MosaicCameraDialog()
	: StelDialog("MosaicCamera")
	, mc(nullptr)
{
	ui = new Ui_mosaicCameraDialog;
}

MosaicCameraDialog::~MosaicCameraDialog()
{
	delete ui;
}

void MosaicCameraDialog::updateRA()
{
	mc->setRA(ui->RASpinBox->valueDegrees());
}

void MosaicCameraDialog::updateDec()
{
	mc->setDec(ui->DecSpinBox->valueDegrees());
}

void MosaicCameraDialog::updateRSP()
{
	mc->setRSP(ui->RSPSpinBox->valueDegrees());
}

void MosaicCameraDialog::onCameraSelectionChanged()
{
    if (ui->lsstCamRadioButton->isChecked()) {
		mc->readPolygonSetsFromJson(":/MosaicCamera/RubinMosaic.json");
    } else if (ui->decamRadioButton->isChecked()) {
		mc->readPolygonSetsFromJson(":/MosaicCamera/DECam.json");
    } else if (ui->hscRadioButton->isChecked()) {
		mc->readPolygonSetsFromJson(":/MosaicCamera/HSC.json");
    } else if (ui->megaPrimeRadioButton->isChecked()) {
		mc->readPolygonSetsFromJson(":/MosaicCamera/MegaPrime.json");
    } else if (ui->latissPrimeRadioButton->isChecked()) {
		mc->readPolygonSetsFromJson(":/MosaicCamera/Latiss.json");
    }
}

void MosaicCameraDialog::setRA(double ra)
{
	ui->RASpinBox->setDegrees(ra);
}

void MosaicCameraDialog::setDec(double dec)
{
	ui->DecSpinBox->setDegrees(dec);
}

void MosaicCameraDialog::setRSP(double rsp)
{
	ui->RSPSpinBox->setDegrees(rsp);
}

void MosaicCameraDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void MosaicCameraDialog::createDialogContent()
{
	mc = GETSTELMODULE(MosaicCamera);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	ui->lsstCamRadioButton->setChecked(true);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		this, SLOT(retranslate()));

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// About tab
	setAboutHtml();
	if(gui!=nullptr)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	// Location tab
	ui->RASpinBox->setDisplayFormat(AngleSpinBox::HMSSymbols);
	ui->RSPSpinBox->setDisplayFormat(AngleSpinBox::DecimalDeg);
	ui->DecSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->DecSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->DecSpinBox->setMinimum(-90.0, true);
	ui->DecSpinBox->setMaximum(90.0, true);
	connect(ui->RASpinBox, SIGNAL(valueChanged()), this, SLOT(updateRA()));
	connect(ui->DecSpinBox, SIGNAL(valueChanged()), this, SLOT(updateDec()));
	connect(ui->RSPSpinBox, SIGNAL(valueChanged()), this, SLOT(updateRSP()));
	connect(ui->lsstCamRadioButton, SIGNAL(clicked()), this, SLOT(onCameraSelectionChanged()));
	connect(ui->decamRadioButton, SIGNAL(clicked()), this, SLOT(onCameraSelectionChanged()));
	connect(ui->hscRadioButton, SIGNAL(clicked()), this, SLOT(onCameraSelectionChanged()));
	connect(ui->megaPrimeRadioButton, SIGNAL(clicked()), this, SLOT(onCameraSelectionChanged()));
	connect(ui->latissPrimeRadioButton, SIGNAL(clicked()), this, SLOT(onCameraSelectionChanged()));
}

void MosaicCameraDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Mosaic Camera Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + MOSAICCAMERA_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + MOSAICCAMERA_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Josh Meyers</td></tr>";
	html += "</table>";
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=nullptr)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}
