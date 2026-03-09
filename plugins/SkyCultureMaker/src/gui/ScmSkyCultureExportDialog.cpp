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

#include "ScmSkyCultureExportDialog.hpp"
#include "QDir"
#include "ScmSkyCulture.hpp"
#include "StelFileMgr.hpp"
#include "ui_scmSkyCultureExportDialog.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

ScmSkyCultureExportDialog::ScmSkyCultureExportDialog(SkyCultureMaker* maker)
	: StelDialogSeparate("ScmSkyCultureExportDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmSkyCultureExportDialog;

	QString appResourceBasePath = StelFileMgr::getUserDir();
	skyCulturesPath             = QDir(appResourceBasePath).filePath("skycultures");
}

ScmSkyCultureExportDialog::~ScmSkyCultureExportDialog()
{
	if (ui != nullptr)
	{
		delete ui;
	}
}

void ScmSkyCultureExportDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmSkyCultureExportDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &ScmSkyCultureExportDialog::handleFontChanged);
	connect(&StelApp::getInstance(), &StelApp::guiFontSizeChanged, this,
	        &ScmSkyCultureExportDialog::handleFontChanged);
	handleFontChanged();

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmSkyCultureExportDialog::close);

	ui->mergeLinesCB->setChecked(
		StelApp::getInstance().getSettings()->value("SkyCultureMaker/mergeLinesOnExport", true).toBool());
	connect(ui->mergeLinesCB, &QCheckBox::toggled, this, [](bool checked)
	        { StelApp::getInstance().getSettings()->setValue("SkyCultureMaker/mergeLinesOnExport", checked); });

	connect(ui->exportBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::exportSkyCulture);
	connect(ui->exportAndExitBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::exportAndExitSkyCulture);
	connect(ui->cancelBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::close);
}

void ScmSkyCultureExportDialog::handleFontChanged()
{
	QFont titleLblFont = QApplication::font();
	titleLblFont.setPixelSize(titleLblFont.pixelSize() + 2);
	titleLblFont.setBold(true);
	ui->titleLbl->setFont(titleLblFont);
}

bool ScmSkyCultureExportDialog::exportSkyCulture()
{
	if (maker == nullptr)
	{
		qWarning() << "SkyCultureMaker: maker is nullptr. Cannot export sky culture.";
		ScmSkyCultureExportDialog::close();
		return false;
	}

	scm::ScmSkyCulture* currentSkyCulture = maker->getCurrentSkyCulture();
	if (currentSkyCulture == nullptr)
	{
		qWarning() << "SkyCultureMaker: current sky culture is nullptr. Cannot export.";
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("No sky culture is set."));
		ScmSkyCultureExportDialog::close();
		return false;
	}

	QString skyCultureId = currentSkyCulture->getId();

	// Let the user choose the export directory with skyCulturesPath as default
	QDir skyCultureDirectory;
	bool exportDirectoryChosen = chooseExportDirectory(skyCultureId, skyCultureDirectory);
	if (!exportDirectoryChosen)
	{
		qWarning() << "SkyCultureMaker: Could not export sky culture. User cancelled or failed to choose "
			      "directory.";
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to choose export directory."));
		return false; // User cancelled or failed to choose directory
	}

	if (skyCultureDirectory.exists())
	{
		qWarning() << "SkyCultureMaker: Sky culture with ID" << skyCultureId
			   << "already exists. Cannot export.";
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Sky culture with this ID already exists."));
		// don't close the dialog here, so the user can delete the folder first
		return false;
	}

	// Create the sky culture directory
	bool createdDirectorySuccessfully = skyCultureDirectory.mkpath(".");
	if (!createdDirectorySuccessfully)
	{
		qWarning() << "SkyCultureMaker: Failed to create sky culture directory at"
			   << skyCultureDirectory.absolutePath();
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to create sky culture directory."));
		return false;
	}

	// save illustrations before json, because the relative illustrations path is required for the json export
	bool savedIllustrationsSuccessfully = currentSkyCulture->saveIllustrations(skyCultureDirectory.absolutePath() +
	                                                                           QDir::separator() + "illustrations");
	if (!savedIllustrationsSuccessfully)
	{
		qWarning() << "SkyCultureMaker: Failed to export sky culture illustrations.";
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to save the illustrations."));
		// delete the created directory
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}

	// Export the sky culture to the index.json file
	bool mergeLinesOnExport =
		StelApp::getInstance().getSettings()->value("SkyCultureMaker/mergeLinesOnExport", true).toBool();
	qDebug() << "SkyCultureMaker: Exporting sky culture. Merge lines on export:" << mergeLinesOnExport;
	QJsonObject scJsonObject = currentSkyCulture->toJson(mergeLinesOnExport);
	QJsonDocument scJsonDoc(scJsonObject);
	if (scJsonDoc.isNull() || scJsonDoc.isEmpty())
	{
		qWarning() << "SkyCultureMaker: Failed to create JSON document for sky culture.";
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to create JSON document for sky culture."));
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}
	QFile scJsonFile(skyCultureDirectory.absoluteFilePath("index.json"));
	if (!scJsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "SkyCultureMaker: Failed to open index.json for writing.";
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to open index.json for writing."));
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}
	scJsonFile.write(scJsonDoc.toJson(QJsonDocument::Indented));
	scJsonFile.close();

	// Save the sky culture description
	bool savedDescriptionSuccessfully = maker->saveSkyCultureDescription(skyCultureDirectory);
	if (!savedDescriptionSuccessfully)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to export sky culture description."));
		qWarning() << "SkyCultureMaker: Failed to export sky culture description.";
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}

	// Save the CMakeLists.txt file
	bool savedCMakeListsSuccessfully = saveSkyCultureCMakeListsFile(skyCultureDirectory);
	if (!savedCMakeListsSuccessfully)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("Failed to export CMakeLists.txt."));
		qWarning() << "SkyCultureMaker: Failed to export CMakeLists.txt.";
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}

	maker->showUserInfoMessage(this->dialog, ui->titleBar->title(),
	                            q_("Sky culture exported successfully to ") +
	                            skyCultureDirectory.absolutePath());
	qInfo() << "SkyCultureMaker: Sky culture exported successfully to" << skyCultureDirectory.absolutePath();
	ScmSkyCultureExportDialog::close();

	// Reload the sky cultures in Stellarium to make the new one available immediately
	StelApp::getInstance().getSkyCultureMgr().reloadSkyCulture();

	return true;
}

bool ScmSkyCultureExportDialog::chooseExportDirectory(const QString& skyCultureId, QDir& skyCultureDirectory)
{
	QString selectedDirectory = QFileDialog::getExistingDirectory(nullptr, q_("Choose Export Directory"),
	                                                              skyCulturesPath);
	if (selectedDirectory.isEmpty())
	{
		// User cancelled the dialog
		return false;
	}

	if (!QDir(selectedDirectory).exists())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(), q_("The selected directory is not valid"));
		qDebug() << "SkyCultureMaker: Selected non-existing export directory";
		return false;
	}

	skyCultureDirectory = QDir(selectedDirectory + QDir::separator() + skyCultureId);
	return true;
}

void ScmSkyCultureExportDialog::exportAndExitSkyCulture()
{
	if(exportSkyCulture())
	{
		maker->stopScm();
	}
}

bool ScmSkyCultureExportDialog::saveSkyCultureCMakeListsFile(const QDir& directory)
{
	QFile cmakeListsFile(directory.absoluteFilePath("CMakeLists.txt"));
	if (!cmakeListsFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "SkyCultureMaker: Failed to open CMakeLists.txt for writing.";
		return false;
	}

	QTextStream out(&cmakeListsFile);
	out << "get_filename_component(skyculturePath \"${CMAKE_CURRENT_SOURCE_DIR}\" REALPATH)\n";
	out << "get_filename_component(skyculture ${skyculturePath} NAME)\n";
	out << "install(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/${skyculture}\n";
	out << "        FILES_MATCHING PATTERN \"*\"\n";
	out << "        PATTERN \"CMakeLists.txt\" EXCLUDE)\n";

	cmakeListsFile.close();
	return true;
}
