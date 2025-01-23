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

#include "MosaicCamera.hpp"
#include "MosaicCameraDialog.hpp"
#include "ui_mosaicCameraDialog.h"

#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

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

QString MosaicCameraDialog::getCurrentCameraName() const
{
	return currentCameraName;
}

void MosaicCameraDialog::updateRA()
{
	mc->setRA(currentCameraName, ui->RASpinBox->valueDegrees());
}

void MosaicCameraDialog::updateDec()
{
	mc->setDec(currentCameraName, ui->DecSpinBox->valueDegrees());
}

void MosaicCameraDialog::updateRotation()
{
	mc->setRotation(currentCameraName, ui->RotationSpinBox->valueDegrees());
}

void MosaicCameraDialog::updateVisibility(bool visible)
{
    mc->setVisibility(currentCameraName, visible);
}

void MosaicCameraDialog::updateDialogFields()
{
    // Temporarily disconnect the signals to avoid triggering updates while setting values
    ui->RASpinBox->blockSignals(true);
    ui->DecSpinBox->blockSignals(true);
    ui->RotationSpinBox->blockSignals(true);
    ui->visibleCheckBox->blockSignals(true);

    ui->RASpinBox->setDegrees(mc->getRA(currentCameraName));
    ui->DecSpinBox->setDegrees(mc->getDec(currentCameraName));
    ui->RotationSpinBox->setDegrees(mc->getRotation(currentCameraName));
    ui->visibleCheckBox->setChecked(mc->getVisibility(currentCameraName));

    // Reconnect the signals
    ui->RASpinBox->blockSignals(false);
    ui->DecSpinBox->blockSignals(false);
    ui->RotationSpinBox->blockSignals(false);
    ui->visibleCheckBox->blockSignals(false);
}

void MosaicCameraDialog::setRA(double ra)
{
	ui->RASpinBox->setDegrees(ra);
}

void MosaicCameraDialog::setDec(double dec)
{
	ui->DecSpinBox->setDegrees(dec);
}

void MosaicCameraDialog::setRotation(double rot)
{
	ui->RotationSpinBox->setDegrees(rot);
}

void MosaicCameraDialog::setVisibility(bool visible)
{
	ui->visibleCheckBox->setChecked(visible);
}

void MosaicCameraDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void MosaicCameraDialog::onCameraSelectionChanged(const QString& cameraName)
{
    currentCameraName = cameraName;
    updateDialogFields();
}

// Initialize the dialog widgets and connect the signals/slots
void MosaicCameraDialog::createDialogContent()
{
	mc = GETSTELMODULE(MosaicCamera);
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);

	QStringList cameraNames = mc->getCameraNames();
    ui->cameraListWidget->addItems(cameraNames);
    if (!cameraNames.isEmpty())
    {
        ui->cameraListWidget->setCurrentRow(0);
		currentCameraName = cameraNames[0];
        updateDialogFields();
    }

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
	ui->RotationSpinBox->setDisplayFormat(AngleSpinBox::DecimalDeg);
	ui->DecSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->DecSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
	ui->DecSpinBox->setMinimum(-90.0, true);
	ui->DecSpinBox->setMaximum(90.0, true);
    connect(ui->cameraListWidget, SIGNAL(currentTextChanged(const QString&)),
            this, SLOT(onCameraSelectionChanged(const QString&)));
	connect(ui->RASpinBox, SIGNAL(valueChanged()), this, SLOT(updateRA()));
	connect(ui->DecSpinBox, SIGNAL(valueChanged()), this, SLOT(updateDec()));
	connect(ui->RotationSpinBox, SIGNAL(valueChanged()), this, SLOT(updateRotation()));
	connect(ui->visibleCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateVisibility(bool)));
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
