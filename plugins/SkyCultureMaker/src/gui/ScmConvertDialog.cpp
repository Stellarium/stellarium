#ifdef SCM_CONVERTER_ENABLED_CPP

# include "ScmConvertDialog.hpp"
# include "SkyCultureConverter.hpp"
# include "StelMainView.hpp"
# include "ui_scmConvertDialog.h"
# include <QWidget>

ScmConvertDialog::ScmConvertDialog(SkyCultureMaker *maker)
	: StelDialog("ScmConvertDialog")
	, ui(new Ui_scmConvertDialog)
	, watcher(new QFutureWatcher<QString>(this))
	, conversionCancelled(false)
	, maker(maker)
{
	// The dialog widget is created in StelDialog::setVisible, not in the constructor.
	// The ScmConvertDialog C++ instance is owned by ScmStartDialog.
}

ScmConvertDialog::~ScmConvertDialog()
{
	// We must wait for the background thread to finish before this object is
	// destroyed, otherwise the thread will be operating on a dangling 'this'
	// pointer, which will lead to a crash. This also ensures that
	// onConversionFinished() is called and temporary files are cleaned up.
	// We send a cancel request to the watcher, which will stop the
	// background task if it is still running, while finishing major steps
	if (watcher->isRunning())
	{
		conversionCancelled = true; // Signal the thread to cancel
		watcher->waitForFinished();
	}
	if (ui != nullptr)
	{
		delete ui;
	}
}

void ScmConvertDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmConvertDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// Connect signals
	connect(ui->browseButton, &QPushButton::clicked, this, &ScmConvertDialog::browseFile);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmConvertDialog::closeDialog);
	connect(ui->titleBar, &TitleBar::movedTo, this, &StelDialog::handleMovedTo);
	connect(ui->convertButton, &QPushButton::clicked, this, &ScmConvertDialog::convert);
	connect(watcher, &QFutureWatcher<QString>::finished, this, &ScmConvertDialog::onConversionFinished);
}

void ScmConvertDialog::closeDialog()
{
	StelDialog::close();
	maker->setToolbarButtonState(false); // Toggle the toolbar button to disabled
}

void ScmConvertDialog::browseFile()
{
	const QString file = QFileDialog::getOpenFileName(&StelMainView::getInstance(), tr("Select an archive"),
	                                                  QDir::homePath(), tr("Archives (*.zip *.rar *.7z *.tar)"));
	if (!file.isEmpty())
	{
		ui->filePathLineEdit->setText(file);
	}
}

void ScmConvertDialog::onConversionFinished()
{
	QString resultText = watcher->future().result();
	ui->convertResultLabel->setText(resultText);
	if (!tempDirPath.isEmpty())
	{
		QDir(tempDirPath).removeRecursively();
	}
	if (!tempDestDirPath.isEmpty())
	{
		QDir(tempDestDirPath).removeRecursively();
	}
	ui->convertButton->setEnabled(true);
}

namespace
{
// Code snippet from https://github.com/selmf/unarr/blob/master/test/main.c
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

	QString errorString;
	// iterate entries and decompress each
	while (ar_parse_entry(archive))
	{
		QString name    = QString::fromUtf8(ar_entry_get_name(archive));
		QString outPath = destinationPath + QDir::separator() + name;
		QDir().mkpath(QFileInfo(outPath).path());
		QFile outFile(outPath);
		if (outFile.open(QIODevice::WriteOnly))
		{
			qint64 remaining     = ar_entry_get_size(archive);
			const qint64 bufSize = 8192;
			QByteArray buffer;
			while (remaining > 0)
			{
				qint64 chunk = qMin<qint64>(remaining, bufSize);
				buffer.resize(chunk);
				if (!ar_entry_uncompress(archive, reinterpret_cast<unsigned char *>(buffer.data()),
				                         chunk))
				{
					errorString = QString("Failed to decompress entry: %1").arg(name);
					break;
				}
				outFile.write(buffer);
				remaining -= chunk;
			}
			outFile.close();
		}
		else
		{
			errorString = QString("Failed to open file for writing: %1").arg(outPath);
		}

		if (!errorString.isEmpty())
		{
			break;
		}
	}
	ar_close_archive(archive);
	ar_close(stream);

	return errorString;
}

QString validateArchivePath(const QString &path)
{
	QMimeDatabase db;
	QMimeType mime = db.mimeTypeForFile(path, QMimeDatabase::MatchDefault);

	static const QStringList archiveTypes = {QStringLiteral("application/zip"), QStringLiteral("application/x-tar"),
	                                         QStringLiteral("application/x-7z-compressed"),
	                                         QStringLiteral("application/x-rar-compressed"),
	                                         QStringLiteral("application/vnd.rar")};

	if (!archiveTypes.contains(mime.name()))
	{
		qWarning() << "Unsupported MIME type:" << mime.name() << "for file" << path;
		return QStringLiteral("Please select a valid archive file "
		                      "(zip, tar, rar or 7z)");
	}
	return QString(); // No error
}

QString extractAndDetermineSource(const QString &archivePath, const QString &tempDirPath, QString &outSourcePath)
{
	try
	{
		qDebug() << "Extracting archive:" << archivePath << "to" << tempDirPath;

		QString error = extractArchive(archivePath, tempDirPath);
		if (!error.isEmpty())
		{
			return error;
		}

		qDebug() << "Archive extracted to:" << tempDirPath;
	}
	catch (const std::exception &e)
	{
		return QString("Error extracting archive: %1").arg(e.what());
	}

	QStringList extracted_files = QDir(tempDirPath).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);

	qDebug() << "Extracted files:" << extracted_files.length();

	if (extracted_files.isEmpty())
	{
		return "No files found in the archive.";
	}

	// set source as the folder that gets converted
	// Archive can have a single folder with the skyculture files or
	// an the skyculture files directly in the root
	bool sourceFound = false;
	if (extracted_files.contains("info.ini"))
	{
		outSourcePath = tempDirPath;
		sourceFound   = true;
	}
	else if (extracted_files.length() == 1)
	{
		const QString singleItemPath = QDir(tempDirPath).filePath(extracted_files.first());
		if (QFileInfo(singleItemPath).isDir() && QFile::exists(QDir(singleItemPath).filePath("info.ini")))
		{
			outSourcePath = singleItemPath;
			sourceFound   = true;
		}
	}

	if (!sourceFound)
	{
		return "Invalid archive structure. Expected 'info.ini' "
		       "or a "
		       "single "
		       "subfolder.";
	}

	return QString(); // No error
}

QString performConversion(const QString &sourcePath, const QString &destPath)
{
	qDebug() << "Source for conversion:" << sourcePath;
	qDebug() << "Destination for conversion:" << destPath;

	SkyCultureConverter::ReturnValue result;

	try
	{
		result = SkyCultureConverter::convert(sourcePath, destPath);
	}
	catch (const std::exception &e)
	{
		return QString("Error during conversion: %1").arg(e.what());
	}

	switch (result)
	{
	case SkyCultureConverter::ReturnValue::CONVERT_SUCCESS: return QString(); // Success
	case SkyCultureConverter::ReturnValue::ERR_OUTPUT_DIR_EXISTS:
		return "Output directory (convertion) already exists.";
	case SkyCultureConverter::ReturnValue::ERR_INFO_INI_NOT_FOUND: return "info.ini not found in the archive.";
	case SkyCultureConverter::ReturnValue::ERR_OUTPUT_DIR_CREATION_FAILED:
		return "Failed to create output directory.";
	case SkyCultureConverter::ReturnValue::ERR_OUTPUT_FILE_WRITE_FAILED: return "Failed to write output file.";
	default: return "Unknown error.";
	}
}

QString moveConvertedFiles(const QString &tempDestDirPath, const QString &stem)
{
	// move the converted files into skycultures folder inside programm directory
	QString appResourceBasePath = StelFileMgr::getInstallationDir();

	QString mainSkyCulturesPath = QDir(appResourceBasePath).filePath("skycultures");

	QString targetPath = QDir(mainSkyCulturesPath).filePath(stem);

	QDir targetDir(targetPath); // QDir object for checking existence
	const QString absoluteTargetPath      = targetDir.absolutePath();
	const QString absoluteTempDestDirPath = QDir(tempDestDirPath).absolutePath();

	qDebug() << "Target path for moved files:" << absoluteTargetPath;

	if (targetDir.exists())
	{
		// Target folder already exists. Do not copy/move.
		qDebug() << "Target folder" << absoluteTargetPath
			 << "already exists. No move operation "
			    "performed.";
		return QString("Target folder already exists: %1").arg(absoluteTargetPath);
	}
	else if (QDir().rename(absoluteTempDestDirPath, absoluteTargetPath))
	{
		qDebug() << "Successfully moved contents of" << absoluteTempDestDirPath << "to" << absoluteTargetPath;
		return QString("Conversion completed successfully. "
		               "Files moved "
		               "to: %1")
		        .arg(absoluteTargetPath);
	}
	else
	{
		qWarning() << "Failed to move" << absoluteTempDestDirPath << "to" << absoluteTargetPath;
		return QString("Failed to move files to: %1").arg(absoluteTargetPath);
	}
}

} // namespace

void ScmConvertDialog::convert()
{
	const QString path = ui->filePathLineEdit->text();
	if (path.isEmpty())
	{
		ui->convertResultLabel->setText(tr("Please select a file to convert."));
		return;
	}

	qDebug() << "Selected file:" << path;

	// Create a temporary directory for extraction
	QString stem = QFileInfo(path).baseName(); // e.g. "foo.tar.gz" -> "foo"

	tempDirPath = QDir::tempPath() + QDir::separator() + "skycultures" + QDir::separator() + stem;
	QDir().mkpath(tempDirPath);
	QDir tempFolder(tempDirPath);

	// Destination is where the converted files will be saved temporarily
	// Important: the converter checks if the destination folder already exists
	// and will not overwrite it, so we do not create it here.
	// If the destination folder already exists, the converter will return an error.
	tempDestDirPath = QDir::tempPath() + QDir::separator() + "skycultures" + QDir::separator() + "results" +
	                  QDir::separator() + stem;
	QDir tempDestFolder(tempDestDirPath);

	conversionCancelled = false; // Reset the flag before starting a new conversion
	ui->convertButton->setEnabled(false);

	// Run conversion in a background thread.
	QFuture<QString> future = QtConcurrent::run(
		[this, path, stem]() -> QString
		{
			// Validate the archive path (whether it is a valid archive file)
			QString error = validateArchivePath(path);
			if (!error.isEmpty()) return error;

			// Check for cancellation between major steps
			if (conversionCancelled) return "Conversion cancelled.";

			// Extract the archive to a temporary directory
			// Check if the skyculture files are in the root or in a subfolder in the archive
			QString sourcePath;
			error = extractAndDetermineSource(path, tempDirPath, sourcePath);
			if (!error.isEmpty()) return error;

			// Check for cancellation between major steps
			if (conversionCancelled) return "Conversion cancelled.";

			// Call the actual converter
			error = performConversion(sourcePath, tempDestDirPath);
			if (!error.isEmpty()) return error;

			// Check for cancellation between major steps
			if (conversionCancelled) return "Conversion cancelled.";

			// Move the converted files to the skycultures folder in the program directory
			return moveConvertedFiles(tempDestDirPath, stem);
		});

	watcher->setFuture(future);

	qDebug() << "Conversion started.";
}

#endif // SCM_CONVERTER_ENABLED_CPP
