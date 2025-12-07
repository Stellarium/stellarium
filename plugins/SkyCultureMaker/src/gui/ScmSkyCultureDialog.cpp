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
#include "ui_scmSkyCultureDialog.h"
#include <cassert>
#include <QStyledItemDelegate>
#include <QDebug>

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

	resetReferences();
	ui->referencesList->setItemDelegateForColumn(0, new NoEditDelegate(ui->referencesList));
	connect(ui->addRefBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::addNewReference);
	connect(ui->removeRefBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::removeReference);
	connect(ui->moveRefUpBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::moveCurrentReferenceUp);
	connect(ui->moveRefDownBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::moveCurrentReferenceDown);
	connect(ui->referencesList, &QTreeWidget::currentItemChanged, this, &ScmSkyCultureDialog::updateReferencesButtons);
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

QString ScmSkyCultureDialog::getDisplayNameFromConstellation(const scm::ScmConstellation &constellation) const
{
	return constellation.getEnglishName() + " (" + constellation.getId() + ")";
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
		text += constellation->getEnglishName();
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

	return desc;
}

void ScmSkyCultureDialog::resetDialog()
{
	if (ui && dialog)
	{
		ui->skyCultureNameLE->clear();
		ui->authorsTE->clear();
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

		name.clear();
		setIdFromName(name);
		resetConstellations();
		maker->setSkyCultureDescription(getDescriptionFromTextEdit());

		updateAddConstellationButtons(true);
		updateEditConstellationButton();
		updateRemoveConstellationButton();
	}
}
