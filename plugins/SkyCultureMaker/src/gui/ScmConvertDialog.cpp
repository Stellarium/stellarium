#include "ScmConvertDialog.hpp"
#include "StelMainView.hpp"
#include "ui_scmConvertDialog.h"
#include <QWidget>

#ifdef SCM_CONVERTER_ENABLED_CPP

# include "SkyCultureConverter.hpp"

ScmConvertDialog::ScmConvertDialog()
	: StelDialog("ScmConvertDialog")
	, ui(new Ui_scmConvertDialog)
	, watcher(new QFutureWatcher<QString>(this))
{
	// The dialog widget is created in StelDialog::setVisible, not in the constructor.
	// The ScmConvertDialog C++ instance is owned by ScmStartDialog.
}

ScmConvertDialog::~ScmConvertDialog()
{
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
	QDir(tempDirPath).removeRecursively();
	QDir(tempDestDirPath).removeRecursively();
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
		QString outPath = destinationPath + "/" + name;
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
	QMimeType mime = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);

	static const QStringList archiveTypes = {QStringLiteral("application/zip"), QStringLiteral("application/x-tar"),
	                                         QStringLiteral("application/x-7z-compressed"),
	                                         QStringLiteral("application/x-rar-compressed"),
	                                         QStringLiteral("application/vnd.rar")};

	if (!archiveTypes.contains(mime.name()))
	{
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
	if (extracted_files.contains("info.ini"))
		outSourcePath = tempDirPath;
	else if (extracted_files.length() == 1)
		outSourcePath = tempDirPath + "/" + extracted_files.first();
	else
		return "Invalid archive structure. Expected 'info.ini' "
		       "or a "
		       "single "
		       "subfolder.";

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

	qDebug() << "Target path for moved files:" << targetPath;

	QDir targetDir(targetPath); // QDir object for checking existence
	if (targetDir.exists())
	{
		// Target folder already exists. Do not copy/move.
		qDebug() << "Target folder" << targetPath
			 << "already exists. No move operation "
			    "performed.";
		return QString("Target folder already exists: %1").arg(targetPath);
	}
	else if (QDir().rename(tempDestDirPath, targetPath))
	{
		qDebug() << "Successfully moved contents of" << tempDestDirPath << "to" << targetPath;
		return QString("Conversion completed successfully. "
		               "Files moved "
		               "to: %1")
		        .arg(targetPath);
	}
	else
	{
		qWarning() << "Failed to move" << tempDestDirPath << "to" << targetPath;
		return QString("Failed to move files to: %1").arg(targetPath);
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
	QString baseName = QFileInfo(path).fileName(); // e.g. "foo.zip"
	int dotPos       = baseName.indexOf('.');
	QString stem     = (dotPos == -1) ? baseName : baseName.left(dotPos); // Extract the part before the first dot

	const QString tempDirPath = QDir::tempPath() + "/skycultures/" + stem;
	QDir().mkpath(tempDirPath);
	QDir tempFolder(tempDirPath);

	// Destination is where the converted files will be saved temporarily
	// Important: the converter checks if the destination folder already exists
	// and will not overwrite it, so we do not create it here.
	// If the destination folder already exists, the converter will return an error.
	const QString tempDestDirPath = QDir::tempPath() + "/skycultures/results/" + stem;
	QDir tempDestFolder(tempDestDirPath);

	ui->convertButton->setEnabled(false);

	// Run conversion in a background thread
	QFuture<QString> future = QtConcurrent::run(
		[path, tempDirPath, tempDestDirPath, stem]() -> QString
		{
			QString error = validateArchivePath(path);
			if (!error.isEmpty())
			{
				return error;
			}

			QString sourcePath;
			error = extractAndDetermineSource(path, tempDirPath, sourcePath);
			if (!error.isEmpty())
			{
				return error;
			}

			error = performConversion(sourcePath, tempDestDirPath);
			if (!error.isEmpty())
			{
				return error;
			}

			return moveConvertedFiles(tempDestDirPath, stem);
		});

	// Watcher to re-enable the button & report result on UI thread
	connect(watcher, &QFutureWatcher<QString>::finished, this, &ScmConvertDialog::onConversionFinished);
	watcher->setFuture(future);

	qDebug() << "Conversion started.";
}

#endif // SCM_CONVERTER_ENABLED_CPP
