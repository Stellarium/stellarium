#include "ScmSkyCultureExportDialog.hpp"
#include "QDir"
#include "ScmSkyCulture.hpp"
#include "StelFileMgr.hpp"
#include "ui_scmSkyCultureExportDialog.h"
#include <filesystem>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

ScmSkyCultureExportDialog::ScmSkyCultureExportDialog(SkyCultureMaker* maker)
	: StelDialogSeparate("ScmSkyCultureExportDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmSkyCultureExportDialog;

	QString appResourceBasePath = StelFileMgr::getInstallationDir();
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

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmSkyCultureExportDialog::close);
	connect(ui->saveBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::saveSkyCulture);
	connect(ui->saveAndExitBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::saveAndExitSkyCulture);
	connect(ui->cancelBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::close);
}

void ScmSkyCultureExportDialog::saveSkyCulture()
{
	if (maker == nullptr)
	{
		qWarning() << "SkyCultureMaker: maker is nullptr. Cannot export sky culture.";
		ScmSkyCultureExportDialog::close();
		return;
	}

	scm::ScmSkyCulture* currentSkyCulture = maker->getCurrentSkyCulture();
	if (currentSkyCulture == nullptr)
	{
		qWarning() << "SkyCultureMaker: current sky culture is nullptr. Cannot export.";
		maker->setSkyCultureDialogInfoLabel("ERROR: No sky culture is set.");
		ScmSkyCultureExportDialog::close();
		return;
	}

	QString skyCultureId     = currentSkyCulture->getId();
	QDir skyCultureDirectory = QDir(skyCulturesPath + QDir::separator() + skyCultureId);
	if (skyCultureDirectory.exists())
	{
		qWarning() << "SkyCultureMaker: Sky culture with ID" << skyCultureId
			   << "already exists. Cannot export.";
		maker->setSkyCultureDialogInfoLabel("ERROR: Sky culture with this ID already exists.");
		// dont close the dialog here, so the user can delete the folder first
		return;
	}

	// Create the sky culture directory
	bool directorySuccessfully = skyCultureDirectory.mkpath(".");
	if (!directorySuccessfully) // not possible use alternative user selected directory.
	{
		bool fallbackDirectorySuccessfully = chooseFallbackDirectory(skyCultureId, skyCultureDirectory);
		if (!fallbackDirectorySuccessfully)
		{
			return;
		}
	}

	// save illustrations before json, because the relative illustrations path is required for the json export
	bool savedIllustrationsSuccessfully = currentSkyCulture->saveIllustrations(skyCultureDirectory.absolutePath() +
	                                                                           QDir::separator() + "illustrations");
	if (!savedIllustrationsSuccessfully)
	{
		maker->setSkyCultureDialogInfoLabel("WARNING: Failed to save the illustrations.");
		qWarning() << "SkyCultureMaker: Failed to export sky culture illustrations.";
		// delete the created directory
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return;
	}

	// Export the sky culture to the index.json file
	qDebug() << "Exporting sky culture...";
	QJsonObject scJsonObject = currentSkyCulture->toJson();
	QJsonDocument scJsonDoc(scJsonObject);
	if (scJsonDoc.isNull() || scJsonDoc.isEmpty())
	{
		qWarning() << "SkyCultureMaker: Failed to create JSON document for sky culture.";
		maker->setSkyCultureDialogInfoLabel("ERROR: Failed to create JSON document for sky culture.");
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return;
	}
	QFile scJsonFile(skyCultureDirectory.absoluteFilePath("index.json"));
	if (!scJsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "SkyCultureMaker: Failed to open index.json for writing.";
		maker->setSkyCultureDialogInfoLabel("ERROR: Failed to open index.json for writing.");
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return;
	}
	scJsonFile.write(scJsonDoc.toJson(QJsonDocument::Indented));
	scJsonFile.close();

	// Save the sky culture description
	bool savedDescriptionSuccessfully = maker->saveSkyCultureDescription(skyCultureDirectory);
	if (!savedDescriptionSuccessfully)
	{
		maker->setSkyCultureDialogInfoLabel("WARNING: Failed to export sky culture description.");
		qWarning() << "SkyCultureMaker: Failed to export sky culture description.";
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return;
	}

	// Save the CMakeLists.txt file
	bool savedCMakeListsSuccessfully = saveSkyCultureCMakeListsFile(skyCultureDirectory);
	if (!savedCMakeListsSuccessfully)
	{
		maker->setSkyCultureDialogInfoLabel("WARNING: Failed to export CMakeLists.txt.");
		qWarning() << "SkyCultureMaker: Failed to export CMakeLists.txt.";
		skyCultureDirectory.removeRecursively();
		ScmSkyCultureExportDialog::close();
		return;
	}

	maker->setSkyCultureDialogInfoLabel("Sky culture exported successfully!");
	ScmSkyCultureExportDialog::close();
}

bool ScmSkyCultureExportDialog::chooseFallbackDirectory(const QString& skyCultureId, QDir& skyCultureDirectory)
{
	// 10 is maximum number of tries the user have to select a fallback directory
	for (size_t i = 0; i < 10; i++)
	{
		QString selectedDirectory = QFileDialog::getExistingDirectory(nullptr, tr("Open Directory"));
		if (!QDir(selectedDirectory).exists())
		{
			maker->setSkyCultureDialogInfoLabel("ERROR: The selected directory is not valid");
			qDebug() << "Selected not existing fallback directory";
			continue;
		}

		QDir newSkyCultureDir(selectedDirectory + QDir::separator() + skyCultureId);
		if (newSkyCultureDir.mkpath("."))
		{
			skyCultureDirectory = newSkyCultureDir;
			return true;
		}
	}

	maker->setSkyCultureDialogInfoLabel("ERROR: Exceeded maximum attempts to set a fallback directory.");
	qDebug() << "User exceeded maximum number (10) of attempts to set a fallback directory.";
	return false;
}

void ScmSkyCultureExportDialog::saveAndExitSkyCulture()
{
	saveSkyCulture();
	maker->resetScmDialogs();
	maker->hideAllDialogs();
	maker->setIsScmEnabled(false);
}

bool ScmSkyCultureExportDialog::saveSkyCultureCMakeListsFile(const QDir &directory)
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
