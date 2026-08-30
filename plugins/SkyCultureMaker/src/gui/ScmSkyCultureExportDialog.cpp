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
#include <QTemporaryDir>
#include <optional>

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
		maker->showUserErrorMessage(ui->titleBar->title(), q_("No sky culture is set."));
		ScmSkyCultureExportDialog::close();
		return false;
	}

	QString skyCultureId = currentSkyCulture->getId();

	// Let the user choose the export directory with skyCulturesPath as default
	QDir finalDirectory;
	bool exportDirectoryChosen = chooseExportDirectory(skyCultureId, finalDirectory);
	if (!exportDirectoryChosen)
	{
		qWarning() << "SkyCultureMaker: Could not export sky culture. User cancelled or failed to choose "
			      "directory.";
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to choose export directory."));
		return false; // User cancelled or failed to choose directory
	}

	const bool isOverwrite = finalDirectory.exists();
	QDir skyCultureDirectory;
	std::optional<QTemporaryDir> tempDir;

	if (isOverwrite)
	{
		const bool overwrite = maker->showUserConfirmMessage(
			ui->titleBar->title(),
			q_("A sky culture with this ID already exists. Do you want to overwrite it?"));
		if (!overwrite)
		{
			// don't close the dialog here, so the user can change the ID or cancel
			return false;
		}

		tempDir.emplace(finalDirectory.absolutePath() + ".scmtmp-XXXXXX");
		if (!tempDir->isValid())
		{
			qWarning() << "SkyCultureMaker: Failed to create temporary directory for sky culture export at"
				   << finalDirectory.absolutePath();
			maker->showUserErrorMessage(ui->titleBar->title(),
			                            q_("Failed to create a temporary directory for the export."));
			return false;
		}
		skyCultureDirectory = QDir(tempDir->path());
	}
	else
	{
		skyCultureDirectory = finalDirectory;
	}

	// Create the sky culture directory (already exists for the temporary directory case)
	bool createdDirectorySuccessfully = skyCultureDirectory.mkpath(".");
	if (!createdDirectorySuccessfully)
	{
		qWarning() << "SkyCultureMaker: Failed to create sky culture directory at"
			   << skyCultureDirectory.absolutePath();
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to create sky culture directory."));
		return false;
	}

	// save illustrations before json, because the relative illustrations path is required for the json export
	bool savedIllustrationsSuccessfully = currentSkyCulture->saveIllustrations(skyCultureDirectory.absolutePath() +
	                                                                           QDir::separator() + "illustrations");
	if (!savedIllustrationsSuccessfully)
	{
		qWarning() << "SkyCultureMaker: Failed to export sky culture illustrations.";
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to save the illustrations."));
		// delete the created directory
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}

	// Export the sky culture to the index.json file
	bool mergeLinesOnExport =
		StelApp::getInstance().getSettings()->value("SkyCultureMaker/mergeLinesOnExport", true).toBool();
	qDebug() << "SkyCultureMaker: Exporting sky culture. Merge lines on export:" << mergeLinesOnExport;
	QJsonObject scIndexJsonObject = currentSkyCulture->toJson(mergeLinesOnExport);
	QJsonDocument scIndexJsonDoc(scIndexJsonObject);
	if (scIndexJsonDoc.isNull() || scIndexJsonDoc.isEmpty())
	{
		qWarning() << "SkyCultureMaker: Failed to create JSON document for sky culture.";
		maker->showUserErrorMessage(ui->titleBar->title(),
		                            q_("Failed to create JSON document for sky culture."));
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}
	QFile scIndexJsonFile(skyCultureDirectory.absoluteFilePath("index.json"));
	if (!scIndexJsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "SkyCultureMaker: Failed to open index.json for writing.";
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to open index.json for writing."));
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}
	scIndexJsonFile.write(scIndexJsonDoc.toJson(QJsonDocument::Indented));
	scIndexJsonFile.close();

	// Export the locations (polygons) of the sky culture to the territory.json file
	qDebug() << "SkyCultureMaker: Exporting sky culture territory...";
	// clean up potential overlaps / user errors before exporting
	currentSkyCulture->mergeLocations();
	QJsonObject scTerritoryGeoJsonObject = currentSkyCulture->getTerritoryGeoJson();
	QJsonDocument scTerritoryGeoJsonDoc(scTerritoryGeoJsonObject);
	if (scTerritoryGeoJsonDoc.isNull() || scTerritoryGeoJsonDoc.isEmpty())
	{
		qWarning() << "SkyCultureMaker: Failed to create JSON document for sky culture.";
		maker->showUserErrorMessage(ui->titleBar->title(),
		                            q_("Failed to create GeoJSON document for sky culture."));
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}
	QFile scTerritoryGeoJsonFile(skyCultureDirectory.absoluteFilePath("territory.geojson"));
	if (!scTerritoryGeoJsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "SkyCultureMaker: Failed to open territory.geojson for writing.";
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to open territory.geojson for writing."));
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}
	scTerritoryGeoJsonFile.write(scTerritoryGeoJsonDoc.toJson(QJsonDocument::Indented));
	scTerritoryGeoJsonFile.close();

	// Save the sky culture description
	bool savedDescriptionSuccessfully = maker->saveSkyCultureDescription(skyCultureDirectory);
	if (!savedDescriptionSuccessfully)
	{
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to export sky culture description."));
		qWarning() << "SkyCultureMaker: Failed to export sky culture description.";
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}

	// Save the CMakeLists.txt file
	bool savedCMakeListsSuccessfully = saveSkyCultureCMakeListsFile(skyCultureDirectory);
	if (!savedCMakeListsSuccessfully)
	{
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to export CMakeLists.txt."));
		qWarning() << "SkyCultureMaker: Failed to export CMakeLists.txt.";
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	}

	// The export into the working directory fully succeeded. When overwriting, atomically swap the freshly
	// exported directory in for the existing one, keeping a temporary backup so a failed swap can be rolled back.
	if (isOverwrite)
	{
		tempDir->setAutoRemove(false);
		const QString finalPath  = finalDirectory.absolutePath();
		const QString tempPath   = skyCultureDirectory.absolutePath();
		const QString backupPath = finalPath + ".scmbak";

		// remove potential leftover backup from a previous failed run
		QDir(backupPath).removeRecursively();

		// move the existing sky culture aside as a backup
		if (!QDir().rename(finalPath, backupPath))
		{
			qWarning() << "SkyCultureMaker: Failed to back up existing sky culture directory at"
				   << finalPath;
			maker->showUserErrorMessage(
				ui->titleBar->title(),
				q_("Failed to back up the existing sky culture. It was left unchanged."));
			QDir(tempPath).removeRecursively();
			ScmSkyCultureExportDialog::close();
			return false;
		}

		// move the freshly exported sky culture into place
		if (!QDir().rename(tempPath, finalPath))
		{
			qWarning() << "SkyCultureMaker: Failed to move exported sky culture into place at" << finalPath
				   << ". The backup will be restored.";
			// Roll back to the original sky culture.
			QDir().rename(backupPath, finalPath);
			QDir(tempPath).removeRecursively();
			maker->showUserErrorMessage(
				ui->titleBar->title(),
				q_("Failed to replace the existing sky culture. It was restored from backup."));
			ScmSkyCultureExportDialog::close();
			return false;
		}

		// The new sky culture is in place; remove the backup.
		//QDir(backupPath).removeRecursively();
	}

	maker->showUserInfoMessage(ui->titleBar->title(),
	                            q_("Sky culture exported successfully to ") +
	                            finalDirectory.absolutePath());
	qInfo() << "SkyCultureMaker: Sky culture exported successfully to" << finalDirectory.absolutePath();
	ScmSkyCultureExportDialog::close();

	// Reload the sky cultures in Stellarium to make the new one available immediately
	StelSkyCultureMgr &scMgr = StelApp::getInstance().getSkyCultureMgr();
	scMgr.reloadSkyCulture();
	scMgr.setCurrentSkyCultureID(skyCultureId);
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
		maker->showUserErrorMessage(ui->titleBar->title(), q_("The selected directory is not valid"));
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
