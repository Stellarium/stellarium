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
#include "ScmSkyCulture.hpp"
#include "StelFileMgr.hpp"
#include "StelMainView.hpp"
#include "ui_scmSkyCultureExportDialog.h"
#include <optional>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTemporaryDir>

namespace
{
// Returns a unique backup path "<basePath>_old", "<basePath>_old(1)", ... that does not yet exist,
// so successive backups never overwrite each other.
QString makeUniqueBackupPath(const QString& basePath)
{
	QString candidate = basePath + "_old";
	for (int n = 1; QFileInfo::exists(candidate); ++n)
	{
		candidate = basePath + "_old(" + QString::number(n) + ")";
	}
	return candidate;
}
} // namespace

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
	if (!chooseExportDirectory(skyCultureId, finalDirectory)) return false;

	const bool isOverwrite = finalDirectory.exists();
	QDir skyCultureDirectory;
	std::optional<QTemporaryDir> tempDir;
	bool keepBackup                   = false;
	bool createdDirectorySuccessfully = false;

	if (isOverwrite)
	{
		QMessageBox confirmBox(QMessageBox::Question, ui->titleBar->title(),
		                       q_("A sky culture with this ID already exists. Do you want to overwrite it?"),
		                       QMessageBox::Yes | QMessageBox::No, &StelMainView::getInstance());
		confirmBox.setDefaultButton(QMessageBox::No);
		QCheckBox* keepBackupCB = new QCheckBox(q_("Keep a backup of the original sky culture"), &confirmBox);
		confirmBox.setCheckBox(keepBackupCB);
		if (confirmBox.exec() != QMessageBox::Yes)
		{
			// don't close the dialog here, so the user can change the ID or cancel
			return false;
		}
		keepBackup = keepBackupCB->isChecked();

		tempDir.emplace(finalDirectory.absolutePath() + ".scmtmp-XXXXXX");
		createdDirectorySuccessfully = tempDir->isValid();
		skyCultureDirectory          = QDir(tempDir->path());
	}
	else
	{
		skyCultureDirectory          = finalDirectory;
		createdDirectorySuccessfully = skyCultureDirectory.mkpath(".");
	}

	if (!createdDirectorySuccessfully)
	{
		qWarning() << "SkyCultureMaker: Failed to create sky culture directory at"
		           << skyCultureDirectory.absolutePath();
		maker->showUserErrorMessage(ui->titleBar->title(), q_("Failed to create sky culture directory."));
		return false;
	}

	// Failure path once the working directory exists: log, notify the user, clean up and close.
	auto fail = [&](const QString& logMsg, const QString& userMsg) -> bool
	{
		qWarning() << "SkyCultureMaker:" << logMsg;
		maker->showUserErrorMessage(ui->titleBar->title(), userMsg);
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return false;
	};

	// Serialize jsonObject into fileName inside the working directory. On failure calls fail() and returns
	// false: buildErrorMsg is shown when the document is empty, writeErrorMsg when the file cannot be opened.
	auto writeJson = [&](const QString& fileName, const QJsonObject& jsonObject, const QString& buildErrorMsg,
	                     const QString& writeErrorMsg) -> bool
	{
		QJsonDocument doc(jsonObject);
		if (doc.isNull() || doc.isEmpty())
		{
			return fail("Failed to create JSON document for " + fileName + ".", buildErrorMsg);
		}
		QFile file(skyCultureDirectory.absoluteFilePath(fileName));
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return fail("Failed to open " + fileName + " for writing.", writeErrorMsg);
		}
		file.write(doc.toJson(QJsonDocument::Indented));
		file.close();
		return true;
	};

	// save illustrations before json, because the relative illustrations path is required for the json export
	bool savedIllustrationsSuccessfully = currentSkyCulture->saveIllustrations(skyCultureDirectory.absolutePath() +
	                                                                           QDir::separator() + "illustrations");
	if (!savedIllustrationsSuccessfully)
	{
		return fail("Failed to export sky culture illustrations.", q_("Failed to save the illustrations."));
	}

	// Export the sky culture to the index.json file
	bool mergeLinesOnExport =
		StelApp::getInstance().getSettings()->value("SkyCultureMaker/mergeLinesOnExport", true).toBool();
	qDebug() << "SkyCultureMaker: Exporting sky culture. Merge lines on export:" << mergeLinesOnExport;
	if (!writeJson("index.json", currentSkyCulture->toJson(mergeLinesOnExport),
	               q_("Failed to create JSON document for sky culture."),
	               q_("Failed to open index.json for writing.")))
	{
		return false;
	}

	// Export the locations (polygons) of the sky culture to the territory.geojson file
	qDebug() << "SkyCultureMaker: Exporting sky culture territory...";
	// clean up potential overlaps / user errors before exporting
	currentSkyCulture->mergeLocations();
	if (!writeJson("territory.geojson", currentSkyCulture->getTerritoryGeoJson(),
	               q_("Failed to create GeoJSON document for sky culture."),
	               q_("Failed to open territory.geojson for writing.")))
	{
		return false;
	}

	// Save the sky culture description
	bool savedDescriptionSuccessfully = maker->saveSkyCultureDescription(skyCultureDirectory);
	if (!savedDescriptionSuccessfully)
	{
		return fail("Failed to export sky culture description.",
		            q_("Failed to export sky culture description."));
	}

	// Save the CMakeLists.txt file
	bool savedCMakeListsSuccessfully = saveSkyCultureCMakeListsFile(skyCultureDirectory);
	if (!savedCMakeListsSuccessfully)
	{
		return fail("Failed to export CMakeLists.txt.", q_("Failed to export CMakeLists.txt."));
	}

	// When overwriting, move the existing sky culture aside before moving the new one in, so a failed swap can be
	// rolled back.
	QString backupPath;
	if (isOverwrite)
	{
		tempDir->setAutoRemove(false);
		const QString finalPath = finalDirectory.absolutePath();
		const QString tempPath  = skyCultureDirectory.absolutePath();
		backupPath              = makeUniqueBackupPath(finalPath);

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
			if (QDir().rename(backupPath, finalPath))
			{
				QDir(tempPath).removeRecursively();
				maker->showUserErrorMessage(
					ui->titleBar->title(),
					q_("Failed to replace the existing sky culture. It was restored from backup."));
			}
			else
			{
				// keep both copies so nothing is lost
				qWarning() << "SkyCultureMaker: Failed to restore backup from" << backupPath << "to"
				           << finalPath;
				maker->showUserErrorMessage(
					ui->titleBar->title(),
					q_("Failed to replace the existing sky culture and could not restore it. The "
					   "original sky culture is at %1, the new export is at %2.")
						.arg(backupPath, tempPath));
			}
			ScmSkyCultureExportDialog::close();
			return false;
		}

		if (!keepBackup)
		{
			if (!QDir(backupPath).removeRecursively())
				qWarning() << "SkyCultureMaker: Failed to remove backup at" << backupPath;
		}
	}

	QString successMessage = q_("Sky culture exported successfully to %1").arg(finalDirectory.absolutePath());
	if (keepBackup)
	{
		successMessage += "\n\n" + q_("The original sky culture was backed up to %1").arg(backupPath);
	}
	maker->showUserInfoMessage(ui->titleBar->title(), successMessage);
	qInfo() << "SkyCultureMaker: Sky culture exported successfully to" << finalDirectory.absolutePath();
	ScmSkyCultureExportDialog::close();

	// Reload the sky cultures in Stellarium to make the new one available immediately
	StelSkyCultureMgr& scMgr = StelApp::getInstance().getSkyCultureMgr();
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
		qDebug() << "SkyCultureMaker: Export directory selection cancelled";
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
	if (exportSkyCulture())
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
