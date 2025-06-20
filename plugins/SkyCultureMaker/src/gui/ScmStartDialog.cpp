#include "ScmStartDialog.hpp"
#include "ui_scmStartDialog.h"
#include <cassert>

#ifdef SCM_CONVERTER_ENABLED_CPP
# include "SkyCultureConverter.hpp"
# include "StelFileMgr.hpp"
# include <cstring> // For strcmp
# include <QDialog>
# include <QFileDialog>
# include <QFileSystemModel>
# include <QFutureWatcher>
# include <QHBoxLayout>
# include <QLabel>
# include <QLineEdit>
# include <QMimeDatabase>
# include <QPushButton>
# include <QVBoxLayout>
# include <QtConcurrent/QtConcurrent>
#else // SCM_CONVERTER_ENABLED_CPP not defined
# include <QLabel>
# include <QVBoxLayout>
# include <QtGlobal> // For QT_VERSION_STR
#endif               // SCM_CONVERTER_ENABLED_CPP

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#ifdef SCM_CONVERTER_ENABLED_CPP
// Code snippert from https://github.com/selmf/unarr/blob/master/test/main.c
ar_archive *ar_open_any_archive(ar_stream *stream, const char *fileext)
{
	ar_archive *ar = ar_open_rar_archive(stream);
	if (!ar)
		ar = ar_open_zip_archive(stream,
		                         fileext && (strcmp(fileext, ".xps") == 0 || strcmp(fileext, ".epub") == 0));
	if (!ar) ar = ar_open_7z_archive(stream);
	if (!ar) ar = ar_open_tar_archive(stream);
	return ar;
}

QString extractArchive(const QString &archivePath, const QString &destinationPath)
{
	ar_stream *stream = ar_open_file(archivePath.toUtf8().constData());
	if (!stream)
	{
		return QString("Failed to open archive: %1").arg(archivePath);
	}
	ar_archive *archive = ar_open_any_archive(stream, QFileInfo(archivePath).suffix().toUtf8().constData());
	if (!archive)
	{
		ar_close(stream);
		return QString("Failed to open archive: %1").arg(archivePath);
	}

	// iterate entries and decompress each
	while (ar_parse_entry(archive))
	{
		QString name    = QString::fromUtf8(ar_entry_get_name(archive));
		QString outPath = destinationPath + "/" + name;
		QDir().mkpath(QFileInfo(outPath).path());
		QFile outFile(outPath);
		if (outFile.open(QIODevice::WriteOnly))
		{
			qint64 remaining     = ar_entry_get_size(archive);
			const qint64 bufSize = 8192;
			while (remaining > 0)
			{
				qint64 chunk = qMin<qint64>(remaining, bufSize);
				QByteArray buffer(chunk, 0);
				if (!ar_entry_uncompress(archive, reinterpret_cast<unsigned char *>(buffer.data()),
				                         chunk))
					break;
				outFile.write(buffer);
				remaining -= chunk;
			}
			outFile.close();
		}
	}
	ar_close_archive(archive);
	ar_close(stream);

	return QString();
}
#endif // SCM_CONVERTER_ENABLED_CPP

ScmStartDialog::ScmStartDialog(SkyCultureMaker *maker)
	: StelDialog("ScmStartDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmStartDialog;
}

ScmStartDialog::~ScmStartDialog()
{
	if (ui != nullptr)
	{
		delete ui;
	}
}

void ScmStartDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmStartDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmStartDialog::closeDialog);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Buttons
	connect(ui->scmStartCancelpushButton, &QPushButton::clicked, this, &ScmStartDialog::closeDialog); // Cancel
	connect(ui->scmStartCreatepushButton, &QPushButton::clicked, this,
	        &ScmStartDialog::startScmCreationProcess); // Create
	connect(ui->scmStartEditpushButton, &QPushButton::clicked, this,
	        &ScmStartDialog::closeDialog); // Edit - TODO: add logic (currently closing the window)

/* =============================================== SkyCultureConverter ============================================== */
#ifdef SCM_CONVERTER_ENABLED_CPP
	ui->scmStartConvertpushButton->setToolTip(
		tr("Convert SkyCultures from the old (fib) format to the new (json) format"));
	connect(ui->scmStartConvertpushButton, &QPushButton::clicked, this,
	        [this]()
	        {
			// Create a new dialog for the conversion process
			QDialog *converterDialog = new QDialog(nullptr);
			converterDialog->setWindowTitle(tr("Convert Old Sky Culture Format"));
			converterDialog->setAttribute(Qt::WA_DeleteOnClose);

			// Create widgets for the dialog
			QVBoxLayout *layout = new QVBoxLayout(converterDialog);
			QLabel *infoLabel   = new QLabel(tr("Select an archive (.zip, .rar, .7z or .tar) with an old "
		                                              "format to convert to the new "
		                                              "format"),
		                                         converterDialog);
			infoLabel->setWordWrap(true);

			QHBoxLayout *fileSelectLayout = new QHBoxLayout();
			QLineEdit *filePathLineEdit   = new QLineEdit(converterDialog);
			filePathLineEdit->setReadOnly(true);
			filePathLineEdit->setPlaceholderText(tr("Select a file…"));
			QPushButton *browseButton = new QPushButton(tr("Browse…"), converterDialog);
			fileSelectLayout->addWidget(filePathLineEdit);
			fileSelectLayout->addWidget(browseButton);

			QPushButton *convertButton = new QPushButton(tr("Convert"), converterDialog);
			QLabel *convertResultLabel = new QLabel(converterDialog);
			convertResultLabel->setWordWrap(true);

			QPushButton *closeButton = new QPushButton(tr("Close"), converterDialog);

			layout->addWidget(infoLabel);
			layout->addLayout(fileSelectLayout);
			layout->addWidget(convertButton);
			layout->addWidget(convertResultLabel);
			layout->addStretch();
			layout->addWidget(closeButton);

			// Connect signals for the new dialog's widgets
			connect(browseButton, &QPushButton::clicked, this,
		                [filePathLineEdit]()
		                {
					const QString file =
						QFileDialog::getOpenFileName(nullptr, tr("Select an archive"),
			                                                     QDir::homePath(),
			                                                     tr("Archives (*.zip *.rar *.7z *.tar)"));
					if (!file.isEmpty())
					{
						filePathLineEdit->setText(file);
					}
				});

			connect(closeButton, &QPushButton::clicked, converterDialog, &QDialog::close);

			connect(convertButton, &QPushButton::clicked, this,
		                [this, filePathLineEdit, convertResultLabel, convertButton]()
		                {
					const QString path = filePathLineEdit->text();
					if (path.isEmpty())
					{
						convertResultLabel->setText(tr("Please select a file."));
						return;
					}

					qDebug() << "Selected file:" << path;

					// Create a temporary directory for extraction
					QString baseName = QFileInfo(path).fileName(); // e.g. "foo.zip"
					int dotPos       = baseName.indexOf('.');
					QString stem     = (dotPos == -1)
			                                           ? baseName
			                                           : baseName.left(
                                                                         dotPos); // Extract the part before the first dot

					const QString tempDir = QDir::tempPath() + "/skycultures/" + stem;
					QDir().mkpath(tempDir);
					QDir tempFolder(tempDir);

					// Destination is where the converted files will be saved temporarily
					const QString tempDestDir = QDir::tempPath() + "/skycultures/results/" + stem;
					QDir tempDestFolder(tempDestDir);

					convertButton->setEnabled(false);

					// Run conversion in a background thread
					QFuture<QString> future = QtConcurrent::run(
						[path, tempDir, tempFolder, tempDestDir, tempDestFolder,
			                         stem]() mutable -> QString
						{
							// Check if the file is a valid archive
							QMimeDatabase db;
							QMimeType mime = db.mimeTypeForFile(path,
				                                                            QMimeDatabase::MatchContent);

							static const QStringList archiveTypes =
								{QStringLiteral("application/zip"),
				                                 QStringLiteral("application/x-tar"),
				                                 QStringLiteral("application/x-7z-compressed"),
				                                 QStringLiteral("application/x-rar-compressed"),
				                                 QStringLiteral("application/vnd.rar")};

							if (!archiveTypes.contains(mime.name()))
							{
								return QStringLiteral(
									"Please select a valid archive file "
									"(zip, tar, rar or 7z)");
							}

							try
							{
								// Extract the archive to the temporary directory
								qDebug() << "Extracting archive:" << path << "to"
									 << tempDir;

								QString error = extractArchive(path, tempDir);
								if (!error.isEmpty())
								{
									return error;
								}

								qDebug() << "Archive extracted to:" << tempDir;
							}
							catch (const std::exception &e)
							{
								return QString("Error extracting archive: %1")
					                                .arg(e.what());
							}

							QStringList extracted_files = tempFolder.entryList(
								QDir::AllEntries | QDir::NoDotAndDotDot);

							qDebug() << "Extracted files:" << extracted_files.length();

							if (extracted_files.isEmpty())
							{
								return "No files found in the archive.";
							}

							// set source as the folder that gets converted
							// Archive can have a single folder with the skyculture files or
							// an the skyculture files directly in the root
							QString source;
							if (extracted_files.contains("info.ini"))
								source = tempDir;
							else if (extracted_files.length() == 1)
								source = tempDir + "/" + extracted_files.first();
							else
								return "Invalid archive structure. Expected 'info.ini' "
								       "or a "
								       "single "
								       "subfolder.";

							qDebug() << "Source for conversion:" << source;
							qDebug() << "Destination for conversion:" << tempDestDir;

							SkyCultureConverter::ReturnValue result;

							try
							{
								result = SkyCultureConverter::convert(source,
					                                                              tempDestDir);
							}
							catch (const std::exception &e)
							{
								return QString("Error during conversion: %1")
					                                .arg(e.what());
							}

							switch (result)
							{
							case SkyCultureConverter::ReturnValue::CONVERT_SUCCESS: break;
							case SkyCultureConverter::ReturnValue::ERR_OUTPUT_DIR_EXISTS:
								return "Output directory (convertion) already exists.";
							case SkyCultureConverter::ReturnValue::ERR_INFO_INI_NOT_FOUND:
								return "info.ini not found in the archive.";
							case SkyCultureConverter::ReturnValue::
								ERR_OUTPUT_DIR_CREATION_FAILED:
								return "Failed to create output directory.";
							case SkyCultureConverter::ReturnValue::ERR_OUTPUT_FILE_WRITE_FAILED:
								return "Failed to write output file.";
								break;
							default: return "Unknown error."; break;
							}

							// move the converted files into skycultures folder inside programm directory
							QString appResourceBasePath = StelFileMgr::getInstallationDir();

							QString mainSkyCulturesPath = QDir(appResourceBasePath)
				                                                              .filePath("skycultures");

							QString targetPath = QDir(mainSkyCulturesPath).filePath(stem);

							qDebug() << "Target path for moved files:" << targetPath;

							QDir targetDir(targetPath); // QDir object for checking existence
							if (targetDir.exists())
							{
								// Target folder already exists. Do not copy/move.
								qDebug() << "Target folder" << targetPath
									 << "already exists. No move operation "
									    "performed.";
								return QString("Target folder already exists: %1")
					                                .arg(targetPath);
							}
							else if (QDir().rename(tempDestFolder.path(), targetPath))
							{
								qDebug() << "Successfully moved contents of"
									 << tempDestFolder.path() << "to" << targetPath;
								return QString("Conversion completed successfully. "
					                                       "Files moved "
					                                       "to: %1")
					                                .arg(targetPath);
							}
							else
							{
								qWarning() << "Failed to move" << tempDestFolder.path()
									   << "to" << targetPath;
								return QString("Failed to move files to: %1")
					                                .arg(targetPath);
							}
						});

					// Watcher to re-enable the button & report result on UI thread
					auto *watcher = new QFutureWatcher<QString>(this);
					connect(watcher, &QFutureWatcher<QString>::finished, this,
			                        [convertResultLabel, convertButton, watcher, tempFolder,
			                         tempDestFolder]() mutable
			                        {
							QString resultText = watcher->future().result();
							convertResultLabel->setText(resultText);
							tempFolder.removeRecursively();
							tempDestFolder.removeRecursively();
							convertButton->setEnabled(true);
							watcher->deleteLater();
						});
					watcher->setFuture(future);

					qDebug() << "Conversion started.";
				});

			converterDialog->exec(); // Show the dialog modally
		});
#else   // SCM_CONVERTER_ENABLED_CPP is not defined
	// Converter is disabled, so disable the button
	ui->scmStartConvertpushButton->setEnabled(false);
	ui->scmStartConvertpushButton->setToolTip(
		tr("Converter is only available from Qt6.5 onwards, currently build with version %1")
			.arg(QT_VERSION_STR));
#endif  // SCM_CONVERTER_ENABLED_CPP
/* ================================================================================================================== */
}

void ScmStartDialog::startScmCreationProcess()
{
	dialog->setVisible(false);                  // Close the dialog before starting the editor
	maker->setSkyCultureDialogVisibility(true); // Start the editor dialog for creating a new Sky Culture
	maker->setNewSkyCulture();

	SkyCultureMaker::setActionToggle("actionShow_DateTime_Window_Global", true);
	SkyCultureMaker::setActionToggle("actionShow_Location_Window_Global", true);
	SkyCultureMaker::setActionToggle("actionShow_Ground", false);
	SkyCultureMaker::setActionToggle("actionShow_Atmosphere", false);
	SkyCultureMaker::setActionToggle("actionShow_MeteorShowers", false);
	SkyCultureMaker::setActionToggle("actionShow_Satellite_Hints", false);
}

void ScmStartDialog::closeDialog()
{
	StelDialog::close();
	maker->setIsScmEnabled(false); // Disable the Sky Culture Maker
}
