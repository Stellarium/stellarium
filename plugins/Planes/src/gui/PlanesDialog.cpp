/*
 * Copyright (C) 2013 Felix Zeltner
 * Copyright (C) 2026 Kamil Zaraś (astronow.pl)
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

#include "gui/PlanesDialog.hpp"
#include "ui_planesDialog.h"

#include "Planes.hpp"
#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QSpinBox>
#include <QUrl>

namespace
{
QString providerWebsiteLink(const QString& label, const QString& url)
{
	return QString("<a href=\"%1\">%2</a>").arg(url.toHtmlEscaped(), label.toHtmlEscaped());
}
}

PlanesDialog::PlanesDialog()
	: StelDialog("Planes")
	, planes(nullptr)
	, ui(new Ui_planesDialog)
{
}

PlanesDialog::~PlanesDialog()
{
	delete ui;
}

void PlanesDialog::updateComboTexts()
{
	if (!ui)
		return;

	ui->labelModeComboBox->setItemText(0, q_("Flight number"));
	ui->labelModeComboBox->setItemText(1, q_("Aircraft model"));
}

void PlanesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateComboTexts();
		setAboutHtml();
	}
}

void PlanesDialog::createDialogContent()
{
	planes = GETSTELMODULE(Planes);
	ui->setupUi(dialog);

	ui->refreshIntervalSpinBox->setMinimum(15);
	ui->refreshIntervalSpinBox->setMaximum(60);
	ui->refreshIntervalSpinBox->setSingleStep(5);
	ui->refreshIntervalSpinBox->setSuffix(QStringLiteral(" s"));
	ui->radiusSpinBox->setMinimum(25);
	ui->radiusSpinBox->setMaximum(500);
	ui->radiusSpinBox->setSingleStep(25);
	ui->radiusSpinBox->setSuffix(QStringLiteral(" NM"));
	ui->providerComboBox->clear();
	ui->providerComboBox->addItem(QStringLiteral("adsb.fi"), QStringLiteral("adsb_fi"));
	ui->providerComboBox->addItem(QStringLiteral("airplanes.live"), QStringLiteral("airplanes_live"));
	ui->labelModeComboBox->clear();
	ui->labelModeComboBox->addItem(QString(), 0);
	ui->labelModeComboBox->addItem(QString(), 1);
	updateComboTexts();
	ui->providerValueLabel->setOpenExternalLinks(false);
	ui->providerValueLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->enabledCheckBox, &QCheckBox::toggled, this, &PlanesDialog::setEnabledFlag);
	connect(ui->showLabelsCheckBox, &QCheckBox::toggled, this, &PlanesDialog::setShowLabels);
	connect(ui->showButtonCheckBox, &QCheckBox::toggled, this, &PlanesDialog::setShowButton);
	connect(ui->labelModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlanesDialog::setLabelMode);
	connect(ui->providerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlanesDialog::setProvider);
	connect(ui->providerValueLabel, &QLabel::linkActivated, this, &PlanesDialog::openExternalLink);
	connect(ui->radiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlanesDialog::setRadius);
	connect(ui->refreshIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlanesDialog::setFetchInterval);
	connect(ui->refreshButton, &QPushButton::clicked, this, &PlanesDialog::triggerRefresh);

	connect(planes, &Planes::enabledChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::showLabelsChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::showButtonChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::labelModeChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::providerChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::radiusChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::fetchIntervalChanged, this, &PlanesDialog::updateFromPlugin);
	connect(planes, &Planes::statusChanged, this, &PlanesDialog::setStatus);

	updateFromPlugin();
	setAboutHtml();
}

void PlanesDialog::updateFromPlugin()
{
	if (!planes)
		return;

	ui->enabledCheckBox->blockSignals(true);
	ui->showLabelsCheckBox->blockSignals(true);
	ui->showButtonCheckBox->blockSignals(true);
	ui->labelModeComboBox->blockSignals(true);
	ui->providerComboBox->blockSignals(true);
	ui->radiusSpinBox->blockSignals(true);
	ui->refreshIntervalSpinBox->blockSignals(true);

	ui->enabledCheckBox->setChecked(planes->isEnabled());
	ui->showLabelsCheckBox->setChecked(planes->getFlagShowLabels());
	ui->showButtonCheckBox->setChecked(planes->getFlagShowButton());
	ui->labelModeComboBox->setCurrentIndex(planes->getLabelMode());
	ui->providerComboBox->setCurrentText(planes->getProviderDisplayName());
	ui->radiusSpinBox->setMaximum(planes->getProviderMaxRadiusNm());
	ui->radiusSpinBox->setValue(planes->getRadiusNm());
	ui->refreshIntervalSpinBox->setValue(planes->getFetchIntervalSec());
	ui->providerValueLabel->setText(providerWebsiteLink(planes->getProviderDisplayName(), planes->getProviderWebsiteUrl()));
	ui->lastUpdateValueLabel->setText(planes->getLastSuccessfulUpdate());
	ui->statusValueLabel->setText(planes->getLastStatus());
	ui->statusValueLabel->setWordWrap(true);
	ui->refreshButton->setEnabled(planes->isEnabled());

	ui->enabledCheckBox->blockSignals(false);
	ui->showLabelsCheckBox->blockSignals(false);
	ui->showButtonCheckBox->blockSignals(false);
	ui->labelModeComboBox->blockSignals(false);
	ui->providerComboBox->blockSignals(false);
	ui->radiusSpinBox->blockSignals(false);
	ui->refreshIntervalSpinBox->blockSignals(false);
}

void PlanesDialog::setStatus(const QString& status)
{
	ui->statusValueLabel->setText(status);
}

void PlanesDialog::setEnabledFlag(bool enabled)
{
	if (planes)
		planes->setEnabled(enabled);
}

void PlanesDialog::setShowLabels(bool enabled)
{
	if (planes)
		planes->setFlagShowLabels(enabled);
}

void PlanesDialog::setShowButton(bool enabled)
{
	if (planes)
		planes->setFlagShowButton(enabled);
}

void PlanesDialog::setLabelMode(int index)
{
	if (!planes)
		return;

	if (index >= 0)
		planes->setLabelMode(ui->labelModeComboBox->itemData(index).toInt());
}

void PlanesDialog::setProvider(int index)
{
	if (!planes)
		return;

	if (index >= 0)
		planes->setProviderId(ui->providerComboBox->itemData(index).toString());
}

void PlanesDialog::setRadius(int radiusNm)
{
	if (planes)
		planes->setRadiusNm(radiusNm);
}

void PlanesDialog::setFetchInterval(int seconds)
{
	if (planes)
		planes->setFetchIntervalSec(seconds);
}

void PlanesDialog::triggerRefresh()
{
	if (planes)
		planes->refreshNow();
}

void PlanesDialog::openExternalLink(const QString& url)
{
	QDesktopServices::openUrl(QUrl(url));
}

void PlanesDialog::setAboutHtml()
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Planes Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>0.1.0</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>GPL v2 or later</td></tr>";
	html += "<tr><td><strong>" + q_("Authors") + ":</strong></td><td>Felix Zeltner, Georg Zotti, Kamil Zaraś (astronow.pl)</td></tr>";
	html += "</table>";
	html += "<p>" + q_("This plug-in shows live ADS-B aircraft as native Stellarium objects.") + "</p>";
	html += "<p>" + q_("It provides basic visibility, label, and refresh controls for a live aircraft feed around the current observer location.") + "</p>";
	html += "<p>" + q_("Live requests remain disabled until you enable aircraft display. When enabled, the plugin sends the current observer latitude, longitude, and search radius to the configured data source.") + "</p>";
	html += "<p>" + q_("The plugin is not loaded at Stellarium startup unless you explicitly enable it in the plug-in manager.") + "</p>";
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}
