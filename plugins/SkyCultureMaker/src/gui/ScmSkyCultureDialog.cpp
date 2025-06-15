#include "ScmSkyCultureDialog.hpp"
#include "ui_scmSkyCultureDialog.h"

ScmSkyCultureDialog::ScmSkyCultureDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmSkyCultureDialog")
	, maker(maker)
{
	ui = new Ui_scmSkyCultureDialog;
}

ScmSkyCultureDialog::~ScmSkyCultureDialog()
{
	delete ui;
}

void ScmSkyCultureDialog::setConstellations(std::vector<scm::ScmConstellation> *constellations)
{
	ScmSkyCultureDialog::constellations = constellations;
	if (ui && dialog && constellations != nullptr)
	{
		ui->constellationsList->clear();
		for (const auto &constellation : *constellations)
		{
			// Add the constellation to the list widget
			ui->constellationsList->addItem(getDisplayNameFromConstellation(constellation));
		}
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
	maker->setSkyCultureDialogVisibility(false);
}

void ScmSkyCultureDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmSkyCultureDialog::close);

	// Overview Tab
	connect(ui->skyCultureNameTE, &QTextEdit::textChanged, this,
	        [this]()
	        {
			name = ui->skyCultureNameTE->toPlainText();
			if (name.isEmpty())
			{
				ui->SaveSkyCultureBtn->setEnabled(false);
			}
			else
			{
				ui->SaveSkyCultureBtn->setEnabled(true);
			}
			setIdFromName(name);
		});

	ui->SaveSkyCultureBtn->setEnabled(false);
	ui->RemoveConstellationBtn->setEnabled(false);
	connect(ui->SaveSkyCultureBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::saveSkyCulture);
	connect(ui->AddConstellationBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::constellationDialog);
	connect(ui->RemoveConstellationBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::removeSelectedConstellation);
	connect(ui->constellationsList, &QListWidget::itemSelectionChanged, this,
	        &ScmSkyCultureDialog::updateRemoveConstellationButton);

	// License Tab

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
		ui->classificationCB->addItem(classification.second.name, QVariant::fromValue(classification.first));
		// set the classification description as tooltip
		int index = ui->classificationCB->count() - 1;
		ui->classificationCB->setItemData(index, classification.second.description, Qt::ToolTipRole);
		// set NONE as the default classification
		if (classification.first == scm::ClassificationType::NONE)
		{
			ui->classificationCB->setCurrentIndex(index);
		}
	}

	// enable/disable the save button based on the current license and authors
	connect(ui->licenseCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
	        [this](int) { setIsLicenseSavable(); });
	connect(ui->authorsTE, &QTextEdit::textChanged, this, &ScmSkyCultureDialog::setIsLicenseSavable);
	connect(ui->classificationCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
	        [this](int) { setIsLicenseSavable(); });
	setIsLicenseSavable();

	connect(ui->saveLicenseBtn, &QPushButton::clicked, this, &ScmSkyCultureDialog::saveLicense);
}

void ScmSkyCultureDialog::saveSkyCulture()
{
// only for debugging purposes
	if (constellations != nullptr)
	{
		qDebug() << "[Constellations as JSON]:";
		for (const auto &constellation : *constellations)
		{
			QJsonObject obj = constellation.toJson(name);
			QJsonDocument doc(obj);
			qDebug().noquote() << doc.toJson(QJsonDocument::Compact);
		}
	}
}

void ScmSkyCultureDialog::saveLicense()
{
	if (maker->getCurrentSkyCulture() != nullptr)
	{
		// set license type
		int index = ui->licenseCB->currentIndex();
		if (index >= 0 && index < ui->licenseCB->count())
		{
			auto licenseType = ui->licenseCB->itemData(index).value<scm::LicenseType>();
			maker->getCurrentSkyCulture()->setLicense(licenseType);
		}
		// set authors
		maker->getCurrentSkyCulture()->setAuthors(ui->authorsTE->toPlainText());
		// set classification type
		index = ui->classificationCB->currentIndex();
		if (index >= 0 && index < ui->classificationCB->count())
		{
			auto classificationType = ui->classificationCB->itemData(index).value<scm::ClassificationType>();
			maker->getCurrentSkyCulture()->setClassificationType(classificationType);
		}
	}
}

void ScmSkyCultureDialog::removeSelectedConstellation()
{
	auto selectedItems = ui->constellationsList->selectedItems();
	if (!selectedItems.isEmpty() && constellations != nullptr)
	{
		QListWidgetItem *item = selectedItems.first();
		QString constellationName = item->text();
		
		// Get Id by comparing to the display name
		// This will always work, even when the constellation id
		// or name contains special characters
		QString selectedConstellationId = "";
		for (const auto &constellation : *constellations)
		{
			if (constellationName == (getDisplayNameFromConstellation(constellation)))
			{
				selectedConstellationId = constellation.getId();
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

void ScmSkyCultureDialog::constellationDialog()
{
	maker->setConstellationDialogVisibility(true);
}

void ScmSkyCultureDialog::setIdFromName(QString &name)
{
	QString id = name.toLower().replace(" ", "_");
	maker->getCurrentSkyCulture()->setId(id);
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

void ScmSkyCultureDialog::setIsLicenseSavable()
{
	if (maker->getCurrentSkyCulture() != nullptr)
	{
		bool isLicenseNotNone = false;
		bool isAuthorsListNotEmpty = !ui->authorsTE->toPlainText().isEmpty();
		bool isClassificationNotNone = false;

		// check if the license is not NONE
		int index = ui->licenseCB->currentIndex();
		if (index >= 0 && index < ui->licenseCB->count())
		{
			auto licenseType = ui->licenseCB->itemData(index).value<scm::LicenseType>();
			isLicenseNotNone = (licenseType != scm::LicenseType::NONE);
		}
		// check if the classification is not NONE
		index = ui->classificationCB->currentIndex();
		if (index >= 0 && index < ui->classificationCB->count())
		{
			auto classificationType = ui->classificationCB->itemData(index).value<scm::ClassificationType>();
			isClassificationNotNone = (classificationType != scm::ClassificationType::NONE);
		}
		// set state of the save button
		ui->saveLicenseBtn->setEnabled(isLicenseNotNone && isAuthorsListNotEmpty && isClassificationNotNone);
	}
	else
	{
		ui->saveLicenseBtn->setEnabled(false);
	}
}