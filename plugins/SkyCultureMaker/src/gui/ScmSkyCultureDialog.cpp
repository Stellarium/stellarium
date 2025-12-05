/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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

#include "ScmSkyCultureDialog.hpp"
#include "ScmPolygonInfoTreeItem.hpp"
#include "types/Region.hpp"
#include "ui_scmSkyCultureDialog.h"
#include <cassert>
#include <qtooltip.h>
#include <QDebug>

ScmSkyCultureDialog::ScmSkyCultureDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmSkyCultureDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmSkyCultureDialog;
}

ScmSkyCultureDialog::~ScmSkyCultureDialog()
{
	delete ui;

	qDebug() << "SkyCultureMaker: Unloaded the ScmSkyCultureDialog";
}

void ScmSkyCultureDialog::setConstellations(std::vector<std::unique_ptr<scm::ScmConstellation>> *constellations)
{
	ScmSkyCultureDialog::constellations = constellations;
	if (ui && dialog && constellations != nullptr)
	{
		ui->constellationsList->clear();
		for (const auto &constellation : *constellations)
		{
			// Add the constellation to the list widget
			ui->constellationsList->addItem(getDisplayNameFromConstellation(*constellation));
		}
	}
}

void ScmSkyCultureDialog::resetConstellations()
{
	if (ui && dialog)
	{
		ui->constellationsList->clear();
		constellations = nullptr; // Reset the constellations pointer
	}
}

void ScmSkyCultureDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmSkyCultureDialog::close()
{
	maker->setDialogVisibility(scm::DialogID::HideOrAbortMakerDialog, true);
}

bool ScmSkyCultureDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == dialog && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Escape)
		{
			// escape should not close the dialog directly
			maker->setDialogVisibility(scm::DialogID::HideOrAbortMakerDialog, true);
			return true;
		}
	}
	return StelDialogSeparate::eventFilter(obj, event);
}

void ScmSkyCultureDialog::createDialogContent()
{
	ui->setupUi(dialog);
	dialog->installEventFilter(this);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmSkyCultureDialog::close);

	// Overview Tab
	connect(ui->skyCultureNameLE, &QLineEdit::textChanged, this,
	        [this]()
	        {
			name = ui->skyCultureNameLE->text();
			if (name.isEmpty())
			{
				ui->ExportSkyCultureBtn->setEnabled(false);
			}
			else
			{
				ui->ExportSkyCultureBtn->setEnabled(true);
			}
			setIdFromName(name);
		});

	ui->ExportSkyCultureBtn->setEnabled(false);
	ui->RemoveConstellationBtn->setEnabled(false);
	ui->EditConstellationBtn->setEnabled(false);

	connect(ui->ExportSkyCultureBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::saveSkyCulture);
	connect(ui->AddConstellationBtn, &QPushButton::clicked, this, [this]() { openConstellationDialog(false); });
	connect(ui->AddDarkConstellationBtn, &QPushButton::clicked, this, [this]() { openConstellationDialog(true); });

	connect(ui->EditConstellationBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::editSelectedConstellation);
	connect(ui->constellationsList, &QListWidget::itemSelectionChanged, this,
	        &ScmSkyCultureDialog::updateEditConstellationButton);

	connect(ui->RemoveConstellationBtn, &QPushButton::clicked, this,
	        &ScmSkyCultureDialog::removeSelectedConstellation);
	connect(ui->constellationsList, &QListWidget::itemSelectionChanged, this,
	        &ScmSkyCultureDialog::updateRemoveConstellationButton);

	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &ScmSkyCultureDialog::handleFontChanged);
	connect(&StelApp::getInstance(), &StelApp::guiFontSizeChanged, this, &ScmSkyCultureDialog::handleFontChanged);
	handleFontChanged();

	// Description Tab

	// add all licenses to the combo box
	for (const auto &license : scm::LICENSES)
	{
		// add name, license type
		ui->licenseCB->addItem(license.second.name, QVariant::fromValue(license.first));
		// set the license description as tooltip
		int index = ui->licenseCB->count() - 1;
		ui->licenseCB->setItemData(index, license.second.description, Qt::ToolTipRole);
		// set NONE as the default license
		if (license.first == scm::LicenseType::NONE)
		{
			ui->licenseCB->setCurrentIndex(index);
		}
	}

	// add all classifications to the combo box
	for (const auto &classification : scm::CLASSIFICATIONS)
	{
		// add name, classification type
                ui->classificationCB->addItem(qc_(classification.second.name, "sky culture classification"), QVariant::fromValue(classification.first));
		// set the classification description as tooltip
		int index = ui->classificationCB->count() - 1;
		ui->classificationCB->setItemData(index, classification.second.description, Qt::ToolTipRole);
		// set NONE as the default classification
		if (classification.first == scm::ClassificationType::NONE)
		{
			ui->classificationCB->setCurrentIndex(index);
		}
	}

	// add all regions to the combo box
	for (const auto &region : scm::REGIONS)
	{
		ui->regionComboBox->addItem(region.second.name, QVariant::fromValue(region.first), region.second.description);
	}
	ui->regionComboBox->setDefaultText("None");
	connect(ui->regionComboBox, &ScmMultiselectionComboBox::checkedItemsChanged, this, &ScmSkyCultureDialog::checkMutExRegions);

	// Geographical Location Tab

	initSkyCultureTime();

	connect(ui->skyCultureTimeSlider, &QSlider::valueChanged, this, &ScmSkyCultureDialog::updateSkyCultureTimeValue);
	connect(ui->skyCultureCurrentTimeSpinBox, &QSpinBox::valueChanged, this, &ScmSkyCultureDialog::updateSkyCultureTimeValue);
	connect(ui->scmGeoLocGraphicsView, &ScmGeoLocGraphicsView::timeValueChanged, this, &ScmSkyCultureDialog::updateSkyCultureTimeValue);

	connect(ui->scmGeoLocGraphicsView, &ScmGeoLocGraphicsView::showAddPolyDialog, this, &ScmSkyCultureDialog::showAddPolygon);
	connect(ui->scmGeoLocGraphicsView, &ScmGeoLocGraphicsView::addPolygonToCulture, this, &ScmSkyCultureDialog::addLocation);

	// polygon info options

	connect(ui->polygonInfoTreeWidget, &QTreeWidget::itemClicked, this, &ScmSkyCultureDialog::selectLocation);
	connect(ui->removePolygonButton, &QPushButton::clicked, this, &ScmSkyCultureDialog::removeLocation);
	connect(ui->polygonInfoTreeWidget, &QTreeWidget::itemSelectionChanged, this, &ScmSkyCultureDialog::updateRemovePolygonButton);
	ui->removePolygonButton->setEnabled(false);

	ui->polygonInfoTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
	ui->polygonInfoTreeWidget->header()->setSectionsMovable(false);
	ui->polygonInfoTreeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);

	// label / tooltip for controls
	ui->helpLabel->setText(mapToolTip);
	ui->helpLabel->raise();
	ui->helpLabel->hide();

	connect(ui->mapHelpToolButton, &QToolButton::clicked, this, [this]() {
		if (ui->helpLabel->isVisible())
		{
			ui->helpLabel->hide();
		}
		else
		{
			ui->helpLabel->show();
		}
	});
	connect(ui->helpLabel, &QPushButton::clicked, this, [this]() { ui->helpLabel->hide(); });

	// dialog popup for time limits

	// prevent startTimeSpinBox value to be greater than endTimeSpinBox and vice versa
	connect(ui->startTimeSpinBox, &QSpinBox::valueChanged, ui->endTimeSpinBox, &ScmAdvancedSpinBox::setMinimum);
	connect(ui->endTimeSpinBox, &QSpinBox::valueChanged, ui->startTimeSpinBox, &ScmAdvancedSpinBox::setMaximum);


	connect(ui->addPolygonDialogButtonBox, &QDialogButtonBox::accepted, this, &ScmSkyCultureDialog::confirmAddPolygon);
	connect(ui->addPolygonDialogButtonBox, &QDialogButtonBox::rejected, this, &ScmSkyCultureDialog::cancelAddPolygon);

	hideAddPolygon();
}

void ScmSkyCultureDialog::handleFontChanged()
{
	QFont headlineFont = QApplication::font();
	headlineFont.setPixelSize(headlineFont.pixelSize() + 2);
	ui->currentSCNameLbl->setFont(headlineFont);
	ui->constellationsLbl->setFont(headlineFont);
	ui->selectLicenseLbl->setFont(headlineFont);

	QFont descriptionTabLblFont = QApplication::font();
	descriptionTabLblFont.setPixelSize(descriptionTabLblFont.pixelSize() + 2);
	descriptionTabLblFont.setBold(true);
	ui->descriptionTabLbl->setFont(descriptionTabLblFont);
}

void ScmSkyCultureDialog::saveSkyCulture()
{
	scm::Description desc = getDescriptionFromTextEdit();

	// check if license is set
	if (desc.license == scm::LicenseType::NONE)
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(), q_("Please select a license for the sky culture."));
		return;
	}
	// check if description is complete
	if (!desc.isComplete())
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(), q_("The sky culture description is not complete. Please fill in all required fields correctly."));
		return;
	}

	// If valid, set the sky culture description
	maker->setSkyCultureDescription(desc);

	// check wether at least 1 polygon was digitized
	if (ui->polygonInfoTreeWidget->topLevelItemCount() < 1)
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(), q_("The sky culture territory is not complete. Please digitize at least one polygon in geographical location tab."));
		return;
	}

	// open export dialog
	maker->setDialogVisibility(scm::DialogID::SkyCultureExportDialog, true);
}

void ScmSkyCultureDialog::editSelectedConstellation()
{
	auto selectedItems = ui->constellationsList->selectedItems();
	if (!selectedItems.isEmpty() && constellations != nullptr)
	{
		QListWidgetItem *item     = selectedItems.first();
		QString constellationName = item->text();

		// Get Id by comparing to the display name
		// This will always work, even when the constellation id
		// or name contains special characters
		QString selectedConstellationId = "";
		for (const auto &constellation : *constellations)
		{
			if (constellationName == (getDisplayNameFromConstellation(*constellation)))
			{
				selectedConstellationId = constellation->getId();
				break;
			}
		}

		openConstellationDialog(selectedConstellationId);
	}
}

void ScmSkyCultureDialog::removeSelectedConstellation()
{
	auto selectedItems = ui->constellationsList->selectedItems();
	if (!selectedItems.isEmpty() && constellations != nullptr)
	{
		QListWidgetItem *item     = selectedItems.first();
		QString constellationName = item->text();

		// Get Id by comparing to the display name
		// This will always work, even when the constellation id
		// or name contains special characters
		QString selectedConstellationId = "";
		for (const auto &constellation : *constellations)
		{
			if (constellationName == (getDisplayNameFromConstellation(*constellation)))
			{
				selectedConstellationId = constellation->getId();
				break;
			}
		}
		// Remove the constellation from the SC
		maker->getCurrentSkyCulture()->removeConstellation(selectedConstellationId);
		// Disable removal button
		ui->RemoveConstellationBtn->setEnabled(false);
		// The reason for not just removing the constellation in the UI here is that
		// in case the constellation could not be removed from the SC, the UI
		// and the SC would be out of sync
		maker->updateSkyCultureDialog();
	}
}

void ScmSkyCultureDialog::openConstellationDialog(bool isDarkConstellation)
{
	maker->setDialogVisibility(scm::DialogID::ConstellationDialog, true);
	maker->setConstellationDialogIsDarkConstellation(isDarkConstellation);
	maker->setIsLineDrawEnabled(true);
	updateAddConstellationButtons(false);
}

void ScmSkyCultureDialog::openConstellationDialog(const QString &constellationId)
{
	scm::ScmSkyCulture *skyCulture = maker->getCurrentSkyCulture();
	if (skyCulture == nullptr)
	{
		qDebug() << "SkyCultureMaker: Current Sky Culture is not initialized.";
		return;
	}

	scm::ScmConstellation *constellation = skyCulture->getConstellation(constellationId);
	if (constellation != nullptr)
	{
		maker->loadDialogFromConstellation(constellation);
		maker->setDialogVisibility(scm::DialogID::ConstellationDialog, true);
		maker->setIsLineDrawEnabled(true);
		updateAddConstellationButtons(false);
	}
	else
	{
		qWarning() << "SkyCultureMaker: Constellation with ID" << constellationId << "not found.";
	}
}

void ScmSkyCultureDialog::setIdFromName(QString &name)
{
	QString id = name.toLower().replace(" ", "_");
	maker->getCurrentSkyCulture()->setId(id);
}

void ScmSkyCultureDialog::updateAddConstellationButtons(bool enabled)
{
	if(ui && dialog)
	{
		ui->AddConstellationBtn->setEnabled(enabled);
		ui->AddDarkConstellationBtn->setEnabled(enabled);
	}
}

void ScmSkyCultureDialog::updateEditConstellationButton()
{
	if (!ui->constellationsList->selectedItems().isEmpty())
	{
		ui->EditConstellationBtn->setEnabled(true);
	}
	else
	{
		ui->EditConstellationBtn->setEnabled(false);
	}
}

void ScmSkyCultureDialog::updateRemoveConstellationButton()
{
	if (!ui->constellationsList->selectedItems().isEmpty())
	{
		ui->RemoveConstellationBtn->setEnabled(true);
	}
	else
	{
		ui->RemoveConstellationBtn->setEnabled(false);
	}
}

void ScmSkyCultureDialog::updateRemovePolygonButton()
{
	if (!ui->polygonInfoTreeWidget->selectedItems().isEmpty())
	{
		ui->removePolygonButton->setEnabled(true);
	}
	else
	{
		ui->removePolygonButton->setEnabled(false);
	}
}

QString ScmSkyCultureDialog::getDisplayNameFromConstellation(const scm::ScmConstellation &constellation) const
{
	return constellation.getEnglishName() + " (" + constellation.getId() + ")";
}

QString ScmSkyCultureDialog::makeConstellationsSection() const
{
	if (!constellations) return {};

	QString text;
	for (const auto& constellation : *constellations)
	{
		const auto descr = constellation->getDescription().trimmed();
		if (descr.isEmpty()) continue;
		if (!text.isEmpty())
			text += "\n\n";
		text += "##### ";
		text += constellation->getEnglishName();
		text += "\n\n";
		text += descr;
	}
	return text.trimmed();
}

scm::Description ScmSkyCultureDialog::getDescriptionFromTextEdit() const
{
	scm::Description desc;

	desc.name               = ui->skyCultureNameLE->text();
	desc.introduction       = ui->introTE->toPlainText();
	desc.cultureDescription = ui->cultureDescriptionTE->toPlainText();
	desc.sky          = ui->skyTE->toPlainText();
	desc.moonAndSun   = ui->moonSunTE->toPlainText();
	desc.planets      = ui->planetsTE->toPlainText();
	desc.zodiac       = ui->zodiacTE->toPlainText();
	desc.milkyWay     = ui->milkyWayTE->toPlainText();
	desc.otherObjects = ui->otherObjectsTE->toPlainText();

	desc.constellations = makeConstellationsSection();

	desc.references       = ui->referencesTE->toPlainText();

	desc.authors            = ui->authorsTE->toPlainText();
	desc.about              = ui->aboutTE->toPlainText();
	desc.acknowledgements = ui->acknowledgementsTE->toPlainText();

	desc.license            = ui->licenseCB->currentData().value<scm::LicenseType>();
	desc.classification   = ui->classificationCB->currentData().value<scm::ClassificationType>();

	desc.region.clear();
	for (const auto &itemData : ui->regionComboBox->checkedItemsData())
	{
		desc.region.push_back(itemData.value<scm::RegionType>());
	}

	return desc;
}

void ScmSkyCultureDialog::resetDialog()
{
	if (ui && dialog)
	{
		ui->skyCultureNameLE->clear();
		ui->authorsTE->clear();
		ui->introTE->clear();
		ui->cultureDescriptionTE->clear();
		ui->aboutTE->clear();
		ui->skyTE->clear();
		ui->moonSunTE->clear();
		ui->planetsTE->clear();
		ui->zodiacTE->clear();
		ui->milkyWayTE->clear();
		ui->otherObjectsTE->clear();
		ui->referencesTE->clear();
		ui->acknowledgementsTE->clear();

		ui->licenseCB->setCurrentIndex(0);
		ui->classificationCB->setCurrentIndex(0);
		ui->regionComboBox->reset();

		name.clear();
		setIdFromName(name);
		resetConstellations();
		maker->setSkyCultureDescription(getDescriptionFromTextEdit());

		updateAddConstellationButtons(true);
		updateEditConstellationButton();
		updateRemoveConstellationButton();

		ui->polygonCountValueLabel->setText(QString::number(0));
		ui->cultureStartTimeValueLabel->setText("");
		ui->cultureEndTimeValueLabel->setText("");
		initSkyCultureTime(); // reuse init function to set skyCultureTimeSlider / skyCultureCurrentTimeSpinBox
		ui->polygonInfoTreeWidget->clear(); // reset the location list
		ui->scmGeoLocGraphicsView->reset(); // reset the map used for digitizing
		updateRemovePolygonButton();
	}
}

void ScmSkyCultureDialog::showAddPolygon()
{
	ui->scmGeoLocGraphicsView->clearFocus();

	// show 'popup' dialog
	ui->mapForegroundWidget->setVisible(true);
	ui->addPolygonFrame->setVisible(true);

	// set start / end time SpinBoxes to the respective values of currentTimeSpinBox
	ui->startTimeSpinBox->setMinimum(ui->skyCultureCurrentTimeSpinBox->minimum());
	ui->startTimeSpinBox->setUseCustomMaximum(true);
	ui->startTimeSpinBox->setCustomMaximum(ui->skyCultureCurrentTimeSpinBox->maximum());
	ui->endTimeSpinBox->setMaximum(ui->skyCultureCurrentTimeSpinBox->maximum() + 1); // + 1 for still existing cultures

	// connection in createDialogContent correctly sets the min / max when value is changed
	ui->endTimeSpinBox->setValue(ui->skyCultureCurrentTimeSpinBox->value());
	ui->startTimeSpinBox->setValue(ui->skyCultureCurrentTimeSpinBox->value());

	// display a fitting char for cultures that still exist
	ui->endTimeSpinBox->setDisplayCustomStringForValue(true);
	ui->endTimeSpinBox->setCustomStringForMax("∞");
}

void ScmSkyCultureDialog::hideAddPolygon()
{
	ui->addPolygonFrame->setVisible(false);
	ui->mapForegroundWidget->setVisible(false);

	ui->scmGeoLocGraphicsView->setFocus();
}

void ScmSkyCultureDialog::initSkyCultureTime()
{
	int minYear = -6500; // should be small enough so that new cultures can always be added (who knows what will be discovered in the future)
	int maxYear = QDateTime::currentDateTime().date().year();
	int currentYear = maxYear;

	// set properties of involved components
	ui->skyCultureTimeSlider->setMinimum(minYear);
	ui->skyCultureTimeSlider->setMaximum(maxYear);

	ui->skyCultureCurrentTimeSpinBox->setMinimum(minYear);
	ui->skyCultureCurrentTimeSpinBox->setMaximum(maxYear);

	// reuse function to set Value of timeSlider, currentTimeSpinBox and MapGraphicsView
	updateSkyCultureTimeValue(currentYear);
}

void ScmSkyCultureDialog::updateSkyCultureTimeValue(int year)
{
	// set timeSlider, currentTimeSpinBox and GeoLocGraphicsView Value to year (block Signals to prevent unwanted functions calls through previous connection)
	ui->skyCultureTimeSlider->blockSignals(true);
	ui->skyCultureCurrentTimeSpinBox->blockSignals(true);

	ui->skyCultureTimeSlider->setValue(year);
	ui->skyCultureCurrentTimeSpinBox->setValue(year);

	ui->skyCultureTimeSlider->blockSignals(false);
	ui->skyCultureCurrentTimeSpinBox->blockSignals(false);

	ui->scmGeoLocGraphicsView->updateTime(year);
}

void ScmSkyCultureDialog::addLocation(scm::CulturePolygon culturePoly)
{
	if (culturePoly.endTime.toInt() > ui->skyCultureCurrentTimeSpinBox->maximum())
	{
		culturePoly.endTime = "∞";
	}

	// add polygon to list (polygonInfoTreeWidget)
	ui->polygonInfoTreeWidget->addTopLevelItem(new ScmPolygonInfoTreeItem(culturePoly.id, culturePoly.startTime, culturePoly.endTime, culturePoly.polygon.size()));

	// send poly to maker
	maker->addSkyCultureLocation(culturePoly);
}

void ScmSkyCultureDialog::removeLocation()
{
	// in theory, there should always only be 1 selected item at a time in polygonInfoTreeWidget
	// (maybe this changes in the future, therefore the for-loop usage is reasonable)
	for (const auto &item : ui->polygonInfoTreeWidget->selectedItems())
	{
		ScmPolygonInfoTreeItem *polygonInfoItem = static_cast<ScmPolygonInfoTreeItem *>(item);

		// remove polygon from map (scmGeoLocGraphicsView)
		ui->scmGeoLocGraphicsView->removePolygon(polygonInfoItem->getId());

		// remove polygon from skyCulture in maker
		maker->removeSkyCultureLocation(polygonInfoItem->getId());

		// remove polygon from list (polygonInfoTreeWidget)
		delete item;

		// update the polygon count info label
		ui->polygonCountValueLabel->setText(QString::number(ui->polygonCountValueLabel->text().toInt() - 1));
	}

	if (ui->polygonInfoTreeWidget->topLevelItemCount() > 0)
	{
		// update the start / end time info labels
		int startTime = ui->polygonInfoTreeWidget->topLevelItem(0)->text(0).toInt();
		QString endTime = ui->polygonInfoTreeWidget->topLevelItem(0)->text(1);

		bool endTimeEvaluated = false; // can't use break when endTime is done because of startTime

		for (int index = 1; index < ui->polygonInfoTreeWidget->topLevelItemCount(); index++)
		{
			if (ui->polygonInfoTreeWidget->topLevelItem(index)->text(0).toInt() < startTime)
			{
				startTime = ui->polygonInfoTreeWidget->topLevelItem(index)->text(0).toInt();
			}

			if (!endTimeEvaluated)
			{
				if (ui->polygonInfoTreeWidget->topLevelItem(index)->text(1) == "∞")
				{
					endTime = ui->polygonInfoTreeWidget->topLevelItem(index)->text(1);
					endTimeEvaluated = true;
				}
				else if (ui->polygonInfoTreeWidget->topLevelItem(index)->text(1).toInt() > endTime.toInt())
				{
					endTime = ui->polygonInfoTreeWidget->topLevelItem(index)->text(1);
				}
			}
		}

		ui->cultureStartTimeValueLabel->setText(QString::number(startTime));
		ui->cultureEndTimeValueLabel->setText(endTime);

		// update the startTime / endTime of the skyCulture
		maker->setSkyCultureStartTime(ui->cultureStartTimeValueLabel->text().toInt());
		maker->setSkyCultureEndTime(ui->cultureEndTimeValueLabel->text());
	}
	else
	{
		ui->cultureStartTimeValueLabel->setText("");
		ui->cultureEndTimeValueLabel->setText("");
	}
}

void ScmSkyCultureDialog::selectLocation(QTreeWidgetItem *item)
{
	ScmPolygonInfoTreeItem *polygonInfoItem = static_cast<ScmPolygonInfoTreeItem *>(item);
	ui->scmGeoLocGraphicsView->selectPolygon(polygonInfoItem->getId());
}

void ScmSkyCultureDialog::checkMutExRegions(const QStringList checkedItems)
{
	// global should not be selectable when any other region is selected and vice versa
	if (checkedItems.empty())
	{
		// enable all regions
		ui->regionComboBox->setItemEnabledState("", true, true);
	}
	else if (checkedItems.contains("Global"))
	{
		// enable "Global"
		ui->regionComboBox->setItemEnabledState("Global", true);
		// disable all other regions
		ui->regionComboBox->setItemEnabledState("Global", false, true);
	}
	else
	{
		// disable "Global"
		ui->regionComboBox->setItemEnabledState("Global", false);
		// enable all other regions
		ui->regionComboBox->setItemEnabledState("Global", true, true);
	}
}

void ScmSkyCultureDialog::confirmAddPolygon()
{
	int startTime = ui->startTimeSpinBox->value();
	QString endTime = ui->endTimeSpinBox->text();

	// increase the polygonCountValueLabel by 1
	ui->polygonCountValueLabel->setText(QString::number(ui->polygonCountValueLabel->text().toInt() + 1));

	// update the cultureStartTimeValueLabel and cultureEndTimeValueLabel accordingly
	if (ui->cultureStartTimeValueLabel->text().isEmpty())
	{
		ui->cultureStartTimeValueLabel->setText(QString::number(startTime));
	}
	else
	{
		if (startTime < ui->cultureStartTimeValueLabel->text().toInt())
		{
			ui->cultureStartTimeValueLabel->setText(QString::number(startTime));
		}
	}

	if (ui->cultureEndTimeValueLabel->text().isEmpty())
	{
		ui->cultureEndTimeValueLabel->setText(endTime);
	}
	else if (ui->cultureEndTimeValueLabel->text() != "∞")
	{
		if (endTime == "∞")
		{
			ui->cultureEndTimeValueLabel->setText(endTime);
		}
		else if (endTime.toInt() > ui->cultureEndTimeValueLabel->text().toInt())
		{
			ui->cultureEndTimeValueLabel->setText(endTime);
		}
	}

	// update the startTime / endTime of the skyCulture
	maker->setSkyCultureStartTime(ui->cultureStartTimeValueLabel->text().toInt());
	maker->setSkyCultureEndTime(ui->cultureEndTimeValueLabel->text());

	// add the current polygon to the map with startTime and endTime as time limits
	ui->scmGeoLocGraphicsView->addCurrentPoly(startTime, ui->endTimeSpinBox->value());

	// hide the popup dialog
	hideAddPolygon();
}

void ScmSkyCultureDialog::cancelAddPolygon()
{
	hideAddPolygon();
}
