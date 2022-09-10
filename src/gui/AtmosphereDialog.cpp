/*
 * Stellarium
 * Copyright (C) 2011 Georg Zotti
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelPropertyMgr.hpp"
#include "AtmosphereDialog.hpp"
#include "StelPropertyMgr.hpp"
#include "StelFileMgr.hpp"
#include "ui_atmosphereDialog.h"

#include <QFileInfo>
#include <QFileDialog>

namespace
{
constexpr char MODEL_PROPERTY[]      = "LandscapeMgr.atmosphereModel";
constexpr char MODEL_PATH_PROPERTY[] = "LandscapeMgr.atmosphereModelPath";
constexpr char DEFAULT_MODEL_PATH_PROPERTY[] = "LandscapeMgr.defaultAtmosphereModelPath";
constexpr char ERROR_PROPERTY[]       = "LandscapeMgr.atmosphereShowMySkyStoppedWithError";
constexpr char STATUS_TEXT_PROPERTY[] = "LandscapeMgr.atmosphereShowMySkyStatusText";
constexpr char ECLIPSE_SIM_QUALITY_PROPERTY[] = "LandscapeMgr.atmosphereEclipseSimulationQuality";
}

AtmosphereDialog::AtmosphereDialog()
	: StelDialog("Atmosphere")
{
	ui = new Ui_atmosphereDialogForm;
}

AtmosphereDialog::~AtmosphereDialog()
{
	delete ui;
}

void AtmosphereDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void AtmosphereDialog::createDialogContent()
{
	ui->setupUi(dialog);
	dialog->installEventFilter(this);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connectDoubleProperty(ui->pressureDoubleSpinBox,"StelSkyDrawer.atmospherePressure");
	connectDoubleProperty(ui->temperatureDoubleSpinBox,"StelSkyDrawer.atmosphereTemperature");
	connectDoubleProperty(ui->extinctionDoubleSpinBox,"StelSkyDrawer.extinctionCoefficient");
	connect(StelApp::getInstance().getStelPropertyManager()->getProperty(STATUS_TEXT_PROPERTY),
			&StelProperty::changed, this, [this](const QVariant& v){ ui->showMySky_statusLabel->setText(v.toString()); });
	connect(StelApp::getInstance().getStelPropertyManager()->getProperty(ERROR_PROPERTY),
			&StelProperty::changed, this, [this](const QVariant& v){ onErrorStateChanged(v.toBool()); });

	connect(ui->standardAtmosphereButton, SIGNAL(clicked()), this, SLOT(setStandardAtmosphere()));

	// Experimental settings, protected by Skylight.flagGuiPublic
	StelPropertyMgr* propMgr = StelApp::getInstance().getStelPropertyManager();
	if (propMgr->getProperty("Skylight.flagGuiPublic")->getValue().toBool())
	{
		connectBoolProperty(ui->checkBox_TfromK, "StelSkyDrawer.flagTfromK");

		if (ui->checkBox_TfromK->isChecked())
		{
			// prepare T display
			double T=25.*(propMgr->getProperty("StelSkyDrawer.extinctionCoefficient")->getValue().toDouble()-0.16)+1.;
			ui->doubleSpinBox_T->setValue(T);
		}
		connect(ui->checkBox_TfromK, SIGNAL(toggled(bool)), ui->doubleSpinBox_T, SLOT(setDisabled(bool)));
		connect(ui->extinctionDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setTfromK(double)));
		connectDoubleProperty(ui->doubleSpinBox_T, "StelSkyDrawer.turbidity");

		connectBoolProperty(ui->checkBox_noScatter, "LandscapeMgr.atmosphereNoScatter");
	}
	else
		ui->groupBox_experimental->hide();

#ifdef ENABLE_SHOWMYSKY
	connect(ui->atmosphereModel, &QComboBox::currentTextChanged, this, &AtmosphereDialog::onModelChoiceChanged);
	connect(ui->showMySky_pathToModelBrowseBtn, &QPushButton::clicked, this, &AtmosphereDialog::browsePathToModel);
	connect(ui->showMySky_pathToModelEdit, &QLineEdit::textChanged, this, &AtmosphereDialog::onPathToModelChanged);
	connect(ui->showMySky_pathToModelEdit, &QLineEdit::editingFinished, this, &AtmosphereDialog::onPathToModelEditingFinished);
	connect(ui->showMySky_debugOptionsEnabled, &QCheckBox::toggled, this,
			[this](const bool enabled){ ui->showMySky_debugOptionsGroup->setVisible(enabled); });
	connectBoolProperty(ui->showMySky_zeroOrderEnabled, "LandscapeMgr.flagAtmosphereZeroOrderScattering");
	connectBoolProperty(ui->showMySky_singleScatteringEnabled, "LandscapeMgr.flagAtmosphereSingleScattering");
	connectBoolProperty(ui->showMySky_multipleScatteringEnabled, "LandscapeMgr.flagAtmosphereMultipleScattering");
	connectIntProperty(ui->showMySky_eclipseSimulationQualitySpinBox, ECLIPSE_SIM_QUALITY_PROPERTY);
#else
	ui->visualModelConfigGroup->hide();
#endif

	setCurrentValues();
}

bool AtmosphereDialog::eventFilter(QObject* object, QEvent* event)
{
	if (object != dialog || event->type() != QEvent::KeyPress)
		return false;
	const auto keyEvent = static_cast<QKeyEvent*>(event);
	if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
		return true; // Prevent these keys pressing buttons when focus is not on the buttons
	return false;
}

void AtmosphereDialog::setStandardAtmosphere()
{
	// See https://en.wikipedia.org/wiki/International_Standard_Atmosphere#ICAO_Standard_Atmosphere
	ui->pressureDoubleSpinBox->setValue(1013.25);
	ui->temperatureDoubleSpinBox->setValue(15.0);
	// See http://www.icq.eps.harvard.edu/ICQExtinct.html
	ui->extinctionDoubleSpinBox->setValue(0.2);
}

void AtmosphereDialog::setTfromK(double k)
{
	if (ui->checkBox_TfromK->isChecked())
	{
		double T=25.*(k-0.16)+1.;
		ui->doubleSpinBox_T->setValue(T);
		StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();

		skyDrawer->setT(T);
	}
}

void AtmosphereDialog::clearStatus()
{
	ui->showMySky_statusLabel->setText("");
	ui->showMySky_statusLabel->setStyleSheet("");
}

void AtmosphereDialog::onModelChoiceChanged(const QString& model)
{
	updatePathToModelStyle();

	const bool isShowMySky = model.toLower()=="showmysky";
	ui->showMySky_optionsGroup->setVisible(isShowMySky);
	if(!isShowMySky || this->hasValidModelPath())
		StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue(MODEL_PROPERTY, model);
}

void AtmosphereDialog::browsePathToModel()
{
	const auto mgr = StelApp::getInstance().getStelPropertyManager();
	const auto dataDir = mgr->getProperty(DEFAULT_MODEL_PATH_PROPERTY)->getValue().toString() + "/..";
	const auto path=QFileDialog::getExistingDirectory(nullptr, q_("Open ShowMySky model"), dataDir);
	if(path.isNull()) return;

	const auto currentModel = mgr->getProperty(MODEL_PROPERTY)->getValue().toString();

	clearStatus();
	ui->showMySky_pathToModelEdit->setText(path);
	StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue(MODEL_PATH_PROPERTY, path);

	const auto selectedModel = ui->atmosphereModel->currentText();
	if(selectedModel.toLower() != currentModel.toLower())
		onModelChoiceChanged(selectedModel);
}

bool AtmosphereDialog::hasValidModelPath() const
{
	const auto text=ui->showMySky_pathToModelEdit->text();
	if(text.isEmpty()) return false;
	return QFileInfo(text+"/params.atmo").exists();
}

void AtmosphereDialog::onPathToModelEditingFinished()
{
	clearStatus();

	if(!hasValidModelPath())
		return;

	const auto mgr = StelApp::getInstance().getStelPropertyManager();
	const auto currentModel = mgr->getProperty(MODEL_PROPERTY)->getValue().toString();

	const auto path = ui->showMySky_pathToModelEdit->text();
	StelApp::getInstance().getStelPropertyManager()->setStelPropertyValue(MODEL_PATH_PROPERTY, path);

	const auto selectedModel = ui->atmosphereModel->currentText();
	if(selectedModel.toLower() != currentModel.toLower())
		onModelChoiceChanged(selectedModel);
}

void AtmosphereDialog::updatePathToModelStyle()
{
	if(hasValidModelPath())
		ui->showMySky_pathToModelEdit->setStyleSheet("");
	else
		ui->showMySky_pathToModelEdit->setStyleSheet("color:red;");
}

void AtmosphereDialog::onPathToModelChanged()
{
	clearStatus();
	updatePathToModelStyle();
}

void AtmosphereDialog::onErrorStateChanged(const bool error)
{
	if(error)
		ui->showMySky_statusLabel->setStyleSheet("margin-left: 1px; border-radius: 5px; background-color: #ff5757;");
	else
		ui->showMySky_statusLabel->setStyleSheet("");
}

void AtmosphereDialog::setCurrentValues()
{
	const auto mgr = StelApp::getInstance().getStelPropertyManager();
	const auto currentModel = mgr->getProperty(MODEL_PROPERTY)->getValue().toString();
	for(int i = 0; i < ui->atmosphereModel->count(); ++i)
	{
		if(ui->atmosphereModel->itemText(i).toLower()==currentModel.toLower())
		{
			ui->atmosphereModel->setCurrentIndex(i);
			break;
		}
	}
	onModelChoiceChanged(ui->atmosphereModel->currentText());

	const auto currentModelPath = mgr->getProperty(MODEL_PATH_PROPERTY)->getValue().toString();
	if(currentModelPath.isEmpty())
	{
		const auto modelPath = mgr->getProperty(DEFAULT_MODEL_PATH_PROPERTY)->getValue().toString();
		ui->showMySky_pathToModelEdit->setText(modelPath);
	}
	else
	{
		ui->showMySky_pathToModelEdit->setText(currentModelPath);
	}
	const auto currentStatusText = mgr->getProperty(STATUS_TEXT_PROPERTY)->getValue().toString();
	ui->showMySky_statusLabel->setText(currentStatusText);
	const bool currentErrorStatus = mgr->getProperty(ERROR_PROPERTY)->getValue().toBool();
	onErrorStateChanged(currentErrorStatus);

	const auto eclipseQuality = mgr->getProperty(ECLIPSE_SIM_QUALITY_PROPERTY)->getValue().toInt();
	ui->showMySky_eclipseSimulationQualitySpinBox->setValue(eclipseQuality);

	ui->showMySky_debugOptionsEnabled->setChecked(false);
}
