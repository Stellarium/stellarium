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
	if (ui && dialog)
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
}

void ScmSkyCultureDialog::saveSkyCulture()
{

}

void ScmSkyCultureDialog::removeSelectedConstellation()
{
	auto selectedItems = ui->constellationsList->selectedItems();
	if (!selectedItems.isEmpty())
	{
		QListWidgetItem *item = selectedItems.first();
		// Get Id by comparing to the display name
		// This will always work, even when the constellation id
		// or name contains special characters
		QString selectedConstellationId = "";
		for (const auto &constellation : *constellations)
		{
			if (item->text() == (getDisplayNameFromConstellation(constellation)))
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
