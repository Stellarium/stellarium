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
#include "NebulaMgr.hpp"
#include "ScmPolygonInfoTreeItem.hpp"
#include "StarMgr.hpp"
#include "StelObjectMgr.hpp"
#include "ui_scmSkyCultureDialog.h"
#include <cassert>
#include <QCheckBox>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>

namespace
{

class NoEditDelegate : public QStyledItemDelegate
{
public:
	NoEditDelegate(QObject* parent = nullptr)
		: QStyledItemDelegate(parent)
	{
	}
	QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override
	{
		return nullptr;
	}
};

}

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
	// (delete when multiple regions are used)
	for (const auto &region : scm::REGIONS)
	{
		// add name, region type
		ui->regionCB->addItem(q_(region.second.name), QVariant::fromValue(region.first));
		// set the region description as tooltip
		int index = ui->regionCB->count() - 1;
		ui->regionCB->setItemData(index, region.second.description, Qt::ToolTipRole);
		// set NONE as the default region
		if (region.first == scm::RegionType::NONE)
		{
			ui->regionCB->setCurrentIndex(index);
		}
	}

	// preparation for a possible extension of multiple regions for 1 skyculture
	// please set regionComboBox to ScmMultiselectionComboBox in the ui file (instead of a normal QComboBox)
	/*for (const auto &region : scm::REGIONS)
	{
		ui->regionComboBox->addItem(region.second.name, QVariant::fromValue(region.first), region.second.description);
	}
	ui->regionComboBox->setDefaultText("None");
	connect(ui->regionComboBox, &ScmMultiselectionComboBox::checkedItemsChanged, this, &ScmSkyCultureDialog::checkMutExRegions);*/

	// Geographical Location Tab

	initSkyCultureTime();

	connect(ui->skyCultureTimeSlider, &QSlider::valueChanged, this, &ScmSkyCultureDialog::updateSkyCultureTimeValue);
	connect(ui->skyCultureCurrentTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ScmSkyCultureDialog::updateSkyCultureTimeValue);
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

	// prevent beginTimeSpinBox value to be greater than endTimeSpinBox and vice versa
	connect(ui->beginTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->endTimeSpinBox,
	        &ScmAdvancedSpinBox::setMinimum);
	connect(ui->endTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->beginTimeSpinBox,
	        &ScmAdvancedSpinBox::setMaximum);

	connect(ui->addPolygonDialogButtonBox, &QDialogButtonBox::accepted, this, &ScmSkyCultureDialog::confirmAddPolygon);
	connect(ui->addPolygonDialogButtonBox, &QDialogButtonBox::rejected, this, &ScmSkyCultureDialog::cancelAddPolygon);

	hideAddPolygon();

	resetReferences();
	ui->referencesList->setItemDelegateForColumn(0, new NoEditDelegate(ui->referencesList));
	connect(ui->addRefBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::addNewReference);
	connect(ui->removeRefBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::removeReference);
	connect(ui->moveRefUpBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::moveCurrentReferenceUp);
	connect(ui->moveRefDownBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::moveCurrentReferenceDown);
	connect(ui->referencesList, &QTreeWidget::currentItemChanged, this, &ScmSkyCultureDialog::updateReferencesButtons);

	// Common Names Tab
	ui->cnEntriesTable->horizontalHeader()->setStretchLastSection(false);
	ui->cnEntriesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui->cnEntriesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	ui->cnEntriesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui->cnEntriesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	ui->cnEntriesTable->setItemDelegate(new NoEditDelegate(ui->cnEntriesTable));
	cnRefreshTable();
	ui->cnVisibleCB->setEnabled(false);
	ui->cnVisibleLbl->setEnabled(false);
	ui->cnRemoveEntryBtn->setEnabled(false);
	connect(ui->cnObjectTypeCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
	        [this](int i) { cnUpdateVisibleField(static_cast<CnObjectType>(i)); });
	connect(ui->cnRemoveEntryBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::cnRemoveEntry);
	connect(ui->cnUseFromSelectionBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::cnUseSelectedObject);
	connect(ui->cnAddNewBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::cnAddNew);
	connect(ui->cnSaveChangesBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::cnSaveChanges);
	ui->cnSaveChangesBtn->setEnabled(false);
	// Enable Save Changes when the user edits any form field while a row is selected
	const auto onFormEdited = [this]()
	{
		if (cnEditingRow >= 0) ui->cnSaveChangesBtn->setEnabled(true);
	};
	connect(ui->cnObjectTypeCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
	        [onFormEdited](int) { onFormEdited(); });
	connect(ui->cnObjectIdLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnEnglishLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnNativeLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnPronounceLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnTransliterationLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnIpaLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnBynameLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnReferencesLE, &QLineEdit::textChanged, this, onFormEdited);
	connect(ui->cnVisibleCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onFormEdited);
	connect(ui->cnEntriesTable, &QTableWidget::itemSelectionChanged, this,
	        &ScmSkyCultureDialog::cnOnTableSelectionChanged);
	cnUpdateVisibleField(CnObjectType::Star); // initial placeholder text
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
	const auto incompFieldsList = desc.getIncompleteFieldsList();
	if (!incompFieldsList.isEmpty())
	{
		auto msg = q_("The sky culture description is not complete. The following fields are not filled correctly:\n");
		for (const auto& field : incompFieldsList)
			msg += QString(" \u2022 ") + field + "\n";
		maker->showUserWarningMessage(dialog, ui->titleBar->title(), msg);
		return;
	}

	// If valid, set the sky culture description
	maker->setSkyCultureDescription(desc);

	// Push common names into the sky culture object
	QMap<QString, QList<scm::ScmCulturalName>> culturalNamesMap;
	for (const auto &pair : cnEntries) {
		culturalNamesMap[pair.first].append(pair.second);
	}
	maker->getCurrentSkyCulture()->setCulturalNames(culturalNamesMap);

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
		maker->setDialogVisibility(scm::DialogID::ConstellationDialog, true);
		maker->loadDialogFromConstellation(constellation);
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
	return constellation.getCulturalName().translated + " (" + constellation.getId() + ")";
}

void ScmSkyCultureDialog::resetReferences()
{
	if (ui && dialog)
	{
		ui->referencesList->clear();
		ui->moveRefUpBtn->setEnabled(false);
		ui->moveRefDownBtn->setEnabled(false);
		ui->removeRefBtn->setEnabled(false);
	}
}

void ScmSkyCultureDialog::updateReferencesButtons()
{
	const auto count = ui->referencesList->topLevelItemCount();
	const auto currentItem = ui->referencesList->currentItem();

	ui->removeRefBtn->setDisabled(count == 0 || !currentItem);

	if (count <= 1 || !currentItem)
	{
		ui->moveRefUpBtn->setDisabled(true);
		ui->moveRefDownBtn->setDisabled(true);
		return;
	}
	const auto rootItem = ui->referencesList->invisibleRootItem();
	Q_ASSERT(rootItem);
	const auto row = rootItem->indexOfChild(currentItem);
	ui->moveRefUpBtn->setDisabled(row == 0);
	ui->moveRefDownBtn->setDisabled(row == count - 1);
}

void ScmSkyCultureDialog::updateReferencesNumeration()
{
	for (int row = 0; row < ui->referencesList->topLevelItemCount(); ++row)
	{
		ui->referencesList->topLevelItem(row)->setText(0, QString("#%1").arg(row + 1));
	}
}

void ScmSkyCultureDialog::addNewReference()
{
	const auto num = ui->referencesList->topLevelItemCount() + 1;
	const QStringList labels{QString("#%1").arg(num), ""};
	const auto item = new QTreeWidgetItem(labels);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	ui->referencesList->addTopLevelItem(item);
	ui->referencesList->setCurrentItem(item, 1);
	ui->referencesList->resizeColumnToContents(0);
	ui->referencesList->editItem(item, 1);

	updateReferencesButtons();
}

void ScmSkyCultureDialog::removeReference()
{
	const auto currentItem = ui->referencesList->currentItem();
	if (!currentItem) return;
	const auto rootItem = ui->referencesList->invisibleRootItem();
	Q_ASSERT(rootItem);
	const auto row = rootItem->indexOfChild(currentItem);
	const auto itemToRemove = ui->referencesList->takeTopLevelItem(row);
	Q_ASSERT(itemToRemove == currentItem); Q_UNUSED(itemToRemove);
	updateReferencesNumeration();
	updateReferencesButtons();
}

void ScmSkyCultureDialog::moveCurrentReferenceUp()
{
	const auto currentItem = ui->referencesList->currentItem();
	if (!currentItem) return;

	const auto rootItem = ui->referencesList->invisibleRootItem();
	Q_ASSERT(rootItem);
	const auto row = rootItem->indexOfChild(currentItem);
	if (row == 0) return;

	const auto itemToMove = ui->referencesList->takeTopLevelItem(row);
	Q_ASSERT(itemToMove == currentItem);
	ui->referencesList->insertTopLevelItem(row-1, itemToMove);
	ui->referencesList->setCurrentItem(itemToMove, 1);

	updateReferencesNumeration();
	updateReferencesButtons();
}

void ScmSkyCultureDialog::moveCurrentReferenceDown()
{
	const auto currentItem = ui->referencesList->currentItem();
	if (!currentItem) return;

	const auto rootItem = ui->referencesList->invisibleRootItem();
	Q_ASSERT(rootItem);
	const auto row = rootItem->indexOfChild(currentItem);
	const auto count = ui->referencesList->topLevelItemCount();
	if (row == count - 1) return;

	const auto itemToMove = ui->referencesList->takeTopLevelItem(row);
	Q_ASSERT(itemToMove == currentItem);
	ui->referencesList->insertTopLevelItem(row+1, itemToMove);
	ui->referencesList->setCurrentItem(itemToMove, 1);

	updateReferencesNumeration();
	updateReferencesButtons();
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
		text += constellation->getCulturalName().translated;
		text += "\n\n";
		text += descr;
	}
	return text.trimmed();
}

QString ScmSkyCultureDialog::makeReferencesSection() const
{
	QString refs;
	for (int row = 0; row < ui->referencesList->topLevelItemCount(); ++row)
	{
		const auto text = ui->referencesList->topLevelItem(row)->text(1);
		refs += QString(" - [#%1]: %2\n").arg(row + 1).arg(text.trimmed());
	}
	if (!refs.isEmpty())
		refs.chop(1); // remove the final newline char
	return refs;
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

	desc.references       = makeReferencesSection();

	desc.authors            = ui->authorsTE->toPlainText();
	desc.about              = ui->aboutTE->toPlainText();
	desc.acknowledgements = ui->acknowledgementsTE->toPlainText();

	desc.license            = ui->licenseCB->currentData().value<scm::LicenseType>();
	desc.classification   = ui->classificationCB->currentData().value<scm::ClassificationType>();

	// (delete when multiple regions are used)
	desc.region = ui->regionCB->currentData().value<scm::RegionType>();

	// (uncomment when multiple regions are used)
	/*desc.region.clear();
	for (const auto &itemData : ui->regionCB->checkedItemsData())
	{
		desc.region.push_back(itemData.value<scm::RegionType>());
	}*/

	return desc;
}

void ScmSkyCultureDialog::populateDescriptionTab(const scm::Description &desc)
{
	// Name
	// block textChanged so the signal doesnt overwrite the loaded ID
	ui->skyCultureNameLE->blockSignals(true);
	ui->skyCultureNameLE->setText(desc.name);
	ui->skyCultureNameLE->blockSignals(false);
	name = desc.name;
	ui->ExportSkyCultureBtn->setEnabled(!desc.name.isEmpty());

	// Description text fields
	ui->introTE->setPlainText(desc.introduction);
	ui->cultureDescriptionTE->setPlainText(desc.cultureDescription);
	ui->skyTE->setPlainText(desc.sky);
	ui->moonSunTE->setPlainText(desc.moonAndSun);
	ui->planetsTE->setPlainText(desc.planets);
	ui->zodiacTE->setPlainText(desc.zodiac);
	ui->milkyWayTE->setPlainText(desc.milkyWay);
	ui->otherObjectsTE->setPlainText(desc.otherObjects);
	ui->authorsTE->setPlainText(desc.authors);
	ui->aboutTE->setPlainText(desc.about);
	ui->acknowledgementsTE->setPlainText(desc.acknowledgements);

	// License combo
	for (int i = 0; i < ui->licenseCB->count(); ++i)
	{
		if (ui->licenseCB->itemData(i).value<scm::LicenseType>() == desc.license)
		{
			ui->licenseCB->setCurrentIndex(i);
			break;
		}
	}

	// Classification combo
	for (int i = 0; i < ui->classificationCB->count(); ++i)
	{
		if (ui->classificationCB->itemData(i).value<scm::ClassificationType>() == desc.classification)
		{
			ui->classificationCB->setCurrentIndex(i);
			break;
		}
	}

	// Region combo
	for (int i = 0; i < ui->regionCB->count(); ++i)
	{
		if (ui->regionCB->itemData(i).value<scm::RegionType>() == desc.region)
		{
			ui->regionCB->setCurrentIndex(i);
			break;
		}
	}
}

void ScmSkyCultureDialog::populateReferences(const QString &references)
{
	resetReferences();
	// matches lines like: " - [#1]: Reference text"
	// the original reference number is preserved
	static const QRegularExpression refRx(R"(^\s*-\s*\[(#\d+)\]:\s*(.+)$)");
	for (const QString &line : references.split('\n'))
	{
		const auto m = refRx.match(line.trimmed());
		if (!m.hasMatch()) continue;
		auto *item = new QTreeWidgetItem({m.captured(1).trimmed(), m.captured(2).trimmed()});
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		ui->referencesList->addTopLevelItem(item);
	}
	updateReferencesButtons();
}

void ScmSkyCultureDialog::populateCommonNames(const QMap<QString, QList<scm::ScmCulturalName>> &culturalNames)
{
	cnEntries.clear();
	for (auto it = culturalNames.constBegin(); it != culturalNames.constEnd(); ++it)
	{
		for (const auto &cn : it.value())
			cnEntries.append({it.key(), cn});
	}
	cnEditingRow = -1;
	cnRefreshTable();
}

void ScmSkyCultureDialog::populateLocationsTab(scm::ScmSkyCulture *sc)
{
	ui->polygonInfoTreeWidget->clear();
	ui->scmGeoLocGraphicsView->reset();
	for (const auto &poly : sc->getLocations())
	{
		ui->scmGeoLocGraphicsView->addExistingPolygon(poly);
		QString endTimeStr = QString::number(poly.endTime);
		if (poly.endTime >= ui->skyCultureCurrentTimeSpinBox->maximum()) endTimeStr = "∞";
		ui->polygonInfoTreeWidget->addTopLevelItem(
			new ScmPolygonInfoTreeItem(poly.id, poly.beginTime, endTimeStr, poly.polygon.size()));
	}
	ui->polygonCountValueLabel->setText(QString::number(sc->getLocations().size()));
}

void ScmSkyCultureDialog::populateFromSkyCulture(scm::ScmSkyCulture *sc)
{
	if (!sc || !ui || !dialog) return;

	const scm::Description &desc = sc->getDescription();
	populateDescriptionTab(desc);
	populateReferences(desc.references);

	setConstellations(sc->getConstellations());
	updateAddConstellationButtons(true);

	populateCommonNames(sc->getCulturalNames());
	populateLocationsTab(sc);

	// Set the time slider to the sky culture's end year
	int displayYear   = sc->getEndTime();
	const int maxYear = QDateTime::currentDateTime().date().year();
	if (displayYear <= 0 || displayYear > maxYear) displayYear = maxYear;
	updateSkyCultureTimeValue(displayYear);
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
		resetReferences();
		ui->acknowledgementsTE->clear();

		ui->licenseCB->setCurrentIndex(0);
		ui->classificationCB->setCurrentIndex(0);

		// (delete when multiple regions are used)
		ui->regionCB->setCurrentIndex(0);

		// (uncomment when multiple regions are used)
		//ui->regionComboBox->reset();

		name.clear();
		setIdFromName(name);
		resetConstellations();
		maker->setSkyCultureDescription(getDescriptionFromTextEdit());

		updateAddConstellationButtons(true);
		updateEditConstellationButton();
		updateRemoveConstellationButton();

		ui->polygonCountValueLabel->setText(QString::number(0));
		ui->cultureBeginTimeValueLabel->setText("");
		ui->cultureEndTimeValueLabel->setText("");
		initSkyCultureTime(); // reuse init function to set skyCultureTimeSlider / skyCultureCurrentTimeSpinBox
		ui->polygonInfoTreeWidget->clear(); // reset the location list
		ui->scmGeoLocGraphicsView->reset(); // reset the map used for digitizing
		updateRemovePolygonButton();

		cnEntries.clear();
		cnEditingRow = -1;
		cnSkipObjectExistCheck = false;
		cnClearForm();
		cnRefreshTable();
	}
}

void ScmSkyCultureDialog::showAddPolygon()
{
	ui->scmGeoLocGraphicsView->clearFocus();

	// show 'popup' dialog
	ui->mapForegroundWidget->setVisible(true);
	ui->addPolygonFrame->setVisible(true);

	// set start / end time SpinBoxes to the respective values of currentTimeSpinBox
	ui->beginTimeSpinBox->setMinimum(ui->skyCultureCurrentTimeSpinBox->minimum());
	ui->beginTimeSpinBox->setUseCustomMaximum(true);
	ui->beginTimeSpinBox->setCustomMaximum(ui->skyCultureCurrentTimeSpinBox->maximum());
	ui->endTimeSpinBox->setMaximum(ui->skyCultureCurrentTimeSpinBox->maximum());

	// display a fitting char for cultures that still exist
	ui->endTimeSpinBox->setDisplayCustomStringForValue(true);
	ui->endTimeSpinBox->setCustomStringForMax("∞");

	// connection in createDialogContent correctly sets the min / max when value is changed
	ui->endTimeSpinBox->setValue(ui->skyCultureCurrentTimeSpinBox->value());
	ui->beginTimeSpinBox->setValue(ui->skyCultureCurrentTimeSpinBox->value());
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
	QString endTimeString = QString::number(culturePoly.endTime);
	if (culturePoly.endTime >= ui->skyCultureCurrentTimeSpinBox->maximum())
	{
		culturePoly.endTime = 9146; // special value for existing cultures
		endTimeString = "∞";
	}

	// add polygon to list (polygonInfoTreeWidget)
	ui->polygonInfoTreeWidget->addTopLevelItem(new ScmPolygonInfoTreeItem(culturePoly.id, culturePoly.beginTime, endTimeString, culturePoly.polygon.size()));

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
		int beginTime = ui->polygonInfoTreeWidget->topLevelItem(0)->text(0).toInt();
		QString endTime = ui->polygonInfoTreeWidget->topLevelItem(0)->text(1);

		bool endTimeEvaluated = (endTime == "∞"); // can't use break when endTime is done because of beginTime
		for (int index = 1; index < ui->polygonInfoTreeWidget->topLevelItemCount(); index++)
		{
			if (ui->polygonInfoTreeWidget->topLevelItem(index)->text(0).toInt() < beginTime)
			{
				beginTime = ui->polygonInfoTreeWidget->topLevelItem(index)->text(0).toInt();
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

		ui->cultureBeginTimeValueLabel->setText(QString::number(beginTime));
		ui->cultureEndTimeValueLabel->setText(endTime);

		// update the beginTime / endTime of the skyCulture
		maker->setSkyCultureBeginTime(ui->cultureBeginTimeValueLabel->text().toInt());
		maker->setSkyCultureEndTime(ui->cultureEndTimeValueLabel->text() == "∞" ? 9146 : ui->cultureEndTimeValueLabel->text().toInt());
	}
	else
	{
		ui->cultureBeginTimeValueLabel->setText("");
		ui->cultureEndTimeValueLabel->setText("");
	}
}

void ScmSkyCultureDialog::selectLocation(QTreeWidgetItem *item)
{
	ScmPolygonInfoTreeItem *polygonInfoItem = static_cast<ScmPolygonInfoTreeItem *>(item);
	ui->scmGeoLocGraphicsView->selectPolygon(polygonInfoItem->getId());
}

// (uncomment when multiple regions are used)
/*void ScmSkyCultureDialog::checkMutExRegions(const QStringList checkedItems)
{
	// world should not be selectable when any other region is selected and vice versa
	if (checkedItems.empty())
	{
		// enable all regions
		ui->regionComboBox->setItemEnabledState("", true, true);
	}
	else if (checkedItems.contains("World"))
	{
		// enable "World"
		ui->regionComboBox->setItemEnabledState("World", true);
		// disable all other regions
		ui->regionComboBox->setItemEnabledState("World", false, true);
	}
	else
	{
		// disable "World"
		ui->regionComboBox->setItemEnabledState("World", false);
		// enable all other regions
		ui->regionComboBox->setItemEnabledState("World", true, true);
	}
}*/

void ScmSkyCultureDialog::confirmAddPolygon()
{
	int beginTime = ui->beginTimeSpinBox->value();
	QString endTime = ui->endTimeSpinBox->text();

	// increase the polygonCountValueLabel by 1
	ui->polygonCountValueLabel->setText(QString::number(ui->polygonCountValueLabel->text().toInt() + 1));

	// update the cultureBeginTimeValueLabel and cultureEndTimeValueLabel accordingly
	if (ui->cultureBeginTimeValueLabel->text().isEmpty())
	{
		ui->cultureBeginTimeValueLabel->setText(QString::number(beginTime));
	}
	else
	{
		if (beginTime < ui->cultureBeginTimeValueLabel->text().toInt())
		{
			ui->cultureBeginTimeValueLabel->setText(QString::number(beginTime));
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

	// update the beginTime / endTime of the skyCulture
	maker->setSkyCultureBeginTime(ui->cultureBeginTimeValueLabel->text().toInt());
	maker->setSkyCultureEndTime(ui->cultureEndTimeValueLabel->text() == "∞" ? 9146 : ui->cultureEndTimeValueLabel->text().toInt());

	// add the current polygon to the map with beginTime and endTime as time limits
	ui->scmGeoLocGraphicsView->addCurrentPoly(beginTime, ui->endTimeSpinBox->value());

	// hide the popup dialog
	hideAddPolygon();
}

void ScmSkyCultureDialog::cancelAddPolygon()
{
	hideAddPolygon();
}

void ScmSkyCultureDialog::cnUpdateVisibleField(CnObjectType type)
{
	const bool isPlanet = (type == CnObjectType::Planet);
	ui->cnVisibleCB->setEnabled(isPlanet);
	if (!isPlanet) ui->cnVisibleCB->setCurrentIndex(0); // disabling the CB should reset the value
	ui->cnVisibleLbl->setEnabled(isPlanet);

	switch (type)
	{
	case CnObjectType::Star:
		ui->cnObjectIdLE->setPlaceholderText(q_("HIP or Gaia DR3 ID (e.g. HIP 1234 or Gaia DR3 1234567890)"));
		break;
	case CnObjectType::Planet: ui->cnObjectIdLE->setPlaceholderText(q_("Planet name (e.g. Venus)")); break;
	case CnObjectType::DSO: ui->cnObjectIdLE->setPlaceholderText(q_("Catalog designation (e.g. M31)")); break;
	}
}

QString ScmSkyCultureDialog::cnBuildKey() const
{
	QString id = ui->cnObjectIdLE->text().trimmed();
	switch (static_cast<CnObjectType>(ui->cnObjectTypeCB->currentIndex()))
	{
	// removal and re-addition of prefix to ensure correct casing and spacing
	case CnObjectType::Star:
		if (id.startsWith(QLatin1String("HIP"), Qt::CaseInsensitive)) id = id.mid(3).trimmed();
		// Normalize Gaia DR3 prefix casing; keep the numeric part as-is
		if (id.startsWith(QLatin1String("Gaia DR3"), Qt::CaseInsensitive))
			return QLatin1String("Gaia DR3 ") + id.mid(8).trimmed();
		return QLatin1String("HIP ") + id;
	case CnObjectType::Planet:
		if (id.startsWith(QLatin1String("NAME"), Qt::CaseInsensitive)) id = id.mid(4).trimmed();
		return QLatin1String("NAME ") + id;
	default: return id;
	}
}

void ScmSkyCultureDialog::cnClearForm()
{
	ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::Star));
	ui->cnObjectIdLE->clear();
	ui->cnEnglishLE->clear();
	ui->cnNativeLE->clear();
	ui->cnPronounceLE->clear();
	ui->cnTransliterationLE->clear();
	ui->cnIpaLE->clear();
	ui->cnBynameLE->clear();
	ui->cnReferencesLE->clear();
	ui->cnVisibleCB->setCurrentIndex(0);
	cnUpdateVisibleField(CnObjectType::Star);
}

scm::ScmCulturalName ScmSkyCultureDialog::cnReadForm() const
{
	scm::ScmCulturalName name;
	name.translated      = ui->cnEnglishLE->text().trimmed();
	name.native          = ui->cnNativeLE->text().trimmed();
	name.pronounce       = ui->cnPronounceLE->text().trimmed();
	name.transliteration = ui->cnTransliterationLE->text().trimmed();
	name.IPA             = ui->cnIpaLE->text().trimmed();
	name.byname          = ui->cnBynameLE->text().trimmed();

	const QString refsText = ui->cnReferencesLE->text().trimmed();
	if (!refsText.isEmpty())
	{
		for (const auto &part : refsText.split(','))
		{
			bool ok       = false;
			const int ref = part.trimmed().toInt(&ok);
			if (ok)
			{
				name.references.append(ref);
			}
		}
	}

	switch (ui->cnVisibleCB->currentIndex())
	{
	case 1: name.special = StelObject::CulturalNameSpecial::Morning; break;
	case 2: name.special = StelObject::CulturalNameSpecial::Evening; break;
	default: name.special = StelObject::CulturalNameSpecial::None; break;
	}
	return name;
}

void ScmSkyCultureDialog::cnPopulateForm(const QString &key, const scm::ScmCulturalName &name)
{
	// Determine type from key
	if (key.startsWith(QLatin1String("HIP "), Qt::CaseInsensitive) ||
	    key.startsWith(QLatin1String("Gaia DR3 "), Qt::CaseInsensitive))
	{
		ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::Star));
	}
	else if (key.startsWith(QLatin1String("NAME "), Qt::CaseInsensitive))
	{
		ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::Planet));
	}
	else
	{
		ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::DSO));
	}
	ui->cnObjectIdLE->setText(key);
	ui->cnEnglishLE->setText(name.translated);
	ui->cnNativeLE->setText(name.native);
	ui->cnPronounceLE->setText(name.pronounce);
	ui->cnTransliterationLE->setText(name.transliteration);
	ui->cnIpaLE->setText(name.IPA);
	ui->cnBynameLE->setText(name.byname);

	QStringList refStrings;
	for (int r : name.references)
	{
		refStrings.append(QString::number(r));
	}
	ui->cnReferencesLE->setText(refStrings.join(", "));

	switch (name.special)
	{
	case StelObject::CulturalNameSpecial::Morning: ui->cnVisibleCB->setCurrentIndex(1); break;
	case StelObject::CulturalNameSpecial::Evening: ui->cnVisibleCB->setCurrentIndex(2); break;
	default: ui->cnVisibleCB->setCurrentIndex(0); break;
	}
}

void ScmSkyCultureDialog::cnRefreshTable()
{
	ui->cnEntriesTable->setRowCount(0);
	for (const auto &pair : cnEntries)
	{
		const int row = ui->cnEntriesTable->rowCount();
		ui->cnEntriesTable->insertRow(row);

		QString displayKey = pair.first;
		switch (pair.second.special)
		{
		case StelObject::CulturalNameSpecial::Morning:
			displayKey += QLatin1String(" (") + qc_("morning", "celestial object visibility period") +
			              QLatin1String(")");
			break;
		case StelObject::CulturalNameSpecial::Evening:
			displayKey += QLatin1String(" (") + qc_("evening", "celestial object visibility period") +
			              QLatin1String(")");
			break;
		default: break;
		}

		ui->cnEntriesTable->setItem(row, 0, new QTableWidgetItem(displayKey));
		ui->cnEntriesTable->setItem(row, 1, new QTableWidgetItem(pair.second.translated));
		ui->cnEntriesTable->setItem(row, 2, new QTableWidgetItem(pair.second.native));
		ui->cnEntriesTable->setItem(row, 3, new QTableWidgetItem(pair.second.pronounce));
	}
}

bool ScmSkyCultureDialog::cnIsDuplicate(const QString &key, StelObject::CulturalNameSpecial special,
                                        int excludeRow) const
{
	for (int i = 0; i < cnEntries.size(); ++i)
	{
		if (i == excludeRow) continue;
		if (cnEntries[i].first == key && cnEntries[i].second.special == special) return true;
	}
	return false;
}

bool ScmSkyCultureDialog::cnValidateForm(QString &outKey, scm::ScmCulturalName &outName)
{
	const QString id = ui->cnObjectIdLE->text().trimmed();
	if (id.isEmpty())
	{
		maker->showUserWarningMessage(
			dialog, ui->titleBar->title(),
			qc_("Please enter an object identifier.",
		            "Prompt for missing celestial object identifier (e.g. HIP number, Gaia DR3 ID, or"
		            "planet name, DSO catalog designation)"));
		return false;
	}
	if (ui->cnObjectTypeCB->currentIndex() == static_cast<int>(CnObjectType::Star))
	{
		if (!id.startsWith(QLatin1String("Gaia DR3 "), Qt::CaseInsensitive))
		{
			// HIP: strip optional prefix, then validate the numeric part
			// Component letter suffixes are allowed (e.g. "32349 A" for multiple-star systems)
			QString numStr = id;
			if (numStr.startsWith(QLatin1String("HIP"), Qt::CaseInsensitive))
				numStr = numStr.mid(3).trimmed();
			bool ok = false;
			numStr.split(' ').first().toInt(&ok);
			if (!ok)
			{
				maker->showUserWarningMessage(
					dialog, ui->titleBar->title(),
					q_("The star identifier must be a HIP number (e.g. 1234 or HIP 1234 A) "
				           "or a Gaia DR3 ID (e.g. Gaia DR3 1234567890)."));
				return false;
			}
		}
	}
	outKey  = cnBuildKey();
	outName = cnReadForm();
	if (outName.translated.isEmpty())
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(), q_("The \"English\" field is required."));
		return false;
	}

	return true;
}

bool ScmSkyCultureDialog::cnCheckObjectExists(const QString &key)
{
	if (cnSkipObjectExistCheck) return true;

	StelObjectMgr *objMgr = GETSTELMODULE(StelObjectMgr);
	StelObjectP obj;

	if (key.startsWith(QLatin1String("HIP "), Qt::CaseInsensitive) ||
	    key.startsWith(QLatin1String("Gaia DR3 "), Qt::CaseInsensitive))
	{
		obj = objMgr->searchByID(QLatin1String("Star"), key);
	}
	else if (key.startsWith(QLatin1String("NAME "), Qt::CaseInsensitive))
	{
		obj = objMgr->searchByID(QLatin1String("Planet"), key.mid(5).trimmed());
	}
	else
	{
		obj = objMgr->searchByID(QLatin1String("Nebula"), key);
	}

	// the object was found
	if (obj) return true;

	QMessageBox msgBox(dialog);
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(q_("The object \"%1\" could not be found in the current Stellarium database.").arg(key));
	msgBox.setInformativeText(q_("Do you still want to save this entry?"));

	auto *dontShowAgainCB = new QCheckBox(q_("Don't show this warning again"), &msgBox);
	msgBox.setCheckBox(dontShowAgainCB);

	QPushButton *saveAnywayBtn = msgBox.addButton(q_("Save Anyway"), QMessageBox::AcceptRole);
	QPushButton *dontSaveBtn   = msgBox.addButton(q_("Don't Save"), QMessageBox::RejectRole);
	msgBox.setDefaultButton(dontSaveBtn);

	msgBox.exec();

	// deactivate this check in the current session
	if (dontShowAgainCB->isChecked()) cnSkipObjectExistCheck = true;

	return msgBox.clickedButton() == saveAnywayBtn;
}

void ScmSkyCultureDialog::cnAddNew()
{
	QString key;
	scm::ScmCulturalName name;
	if (!cnValidateForm(key, name)) return;

	if (cnIsDuplicate(key, name.special))
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(),
		                              q_("An entry for this object already exists."));
		return;
	}

	if (!cnCheckObjectExists(key)) return;

	cnEntries.append({key, name});
	cnRefreshTable();
	cnClearForm();
}

void ScmSkyCultureDialog::cnSaveChanges()
{
	if (cnEditingRow < 0 || cnEditingRow >= cnEntries.size()) return;

	QString key;
	scm::ScmCulturalName name;
	if (!cnValidateForm(key, name)) return;

	if (cnIsDuplicate(key, name.special, cnEditingRow))
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(),
		                              q_("An entry for this object already exists."));
		return;
	}

	if (!cnCheckObjectExists(key)) return;

	cnEntries[cnEditingRow] = {key, name};
	// capture before cnRefreshTable resets selection
	const int savedRow = cnEditingRow;
	cnRefreshTable();
	ui->cnEntriesTable->selectRow(savedRow);
}

void ScmSkyCultureDialog::cnRemoveEntry()
{
	const auto selectedRows = ui->cnEntriesTable->selectionModel()->selectedRows();
	if (selectedRows.isEmpty()) return;
	const int row = selectedRows.first().row();
	Q_ASSERT(row >= 0 && row < cnEntries.size());
	cnEntries.removeAt(row);
	cnEditingRow = -1;
	cnRefreshTable();
	// Select the next entry if possible, otherwise clear the form
	if (!cnEntries.isEmpty())
	{
		ui->cnEntriesTable->selectRow(qMin(row, static_cast<int>(cnEntries.size()) - 1));
	}
	else
	{
		cnClearForm();
	}
}

void ScmSkyCultureDialog::cnUseSelectedObject()
{
	StelObjectMgr *objMgr              = GETSTELMODULE(StelObjectMgr);
	const QList<StelObjectP> &selected = objMgr->getSelectedObject();
	if (selected.isEmpty())
	{
		maker->showUserWarningMessage(dialog, ui->titleBar->title(),
		                              q_("No object is currently selected in Stellarium."));
		return;
	}

	const StelObjectP &obj = selected.first();
	const QString type     = obj->getType();

	QString englishName;
	if (type == QLatin1String("Star"))
	{
		ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::Star));
		ui->cnObjectIdLE->setText(obj->getID());
		// StarWrapper1::getEnglishName() returns the catalog designation ("HIP XXXXX"),
		// not the common proper name. Use StarMgr to look it up from the HIP number.
		const QString engId = obj->getEnglishName();
		if (engId.startsWith(QLatin1String("HIP "), Qt::CaseInsensitive))
		{
			bool ok          = false;
			const StarId hip = engId.mid(4).toULongLong(&ok);
			if (ok) englishName = StarMgr::getCommonEnglishName(hip);
		}
	}
	else if (type == QLatin1String("Planet"))
	{
		ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::Planet));
		ui->cnObjectIdLE->setText(QLatin1String("NAME ") + obj->getEnglishName());
		englishName = obj->getEnglishName();
	}
	else
	{
		ui->cnObjectTypeCB->setCurrentIndex(static_cast<int>(CnObjectType::DSO));
		QString id = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignationWIC();
		if (id.isEmpty()) id = obj->getEnglishName();
		ui->cnObjectIdLE->setText(id);
		englishName = obj->getEnglishName();
	}
	ui->cnEnglishLE->setText(englishName);
}

void ScmSkyCultureDialog::cnOnTableSelectionChanged()
{
	const auto selectedRows = ui->cnEntriesTable->selectionModel()->selectedRows();
	const bool hasSelection = !selectedRows.isEmpty();

	ui->cnRemoveEntryBtn->setEnabled(hasSelection);
	ui->cnSaveChangesBtn->setEnabled(false);

	if (hasSelection)
	{
		const int row = selectedRows.first().row();
		Q_ASSERT(row >= 0 && row < cnEntries.size());
		cnEditingRow = -1; // suppress form-change signals during populate
		cnPopulateForm(cnEntries[row].first, cnEntries[row].second);
		cnEditingRow = row;
	}
	else
	{
		cnEditingRow = -1; // no row selected, so edits should not enable Save Changes
	}
}
