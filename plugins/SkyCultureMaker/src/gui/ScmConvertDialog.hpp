#ifndef SCMCONVERTDIALOG_HPP
#define SCMCONVERTDIALOG_HPP

#include "StelDialog.hpp"

#ifdef SCM_CONVERTER_ENABLED_CPP

# include "SkyCultureMaker.hpp"
# include "StelFileMgr.hpp"
# include "ui_scmConvertDialog.h"
# include "unarr.h"
# include <atomic>
# include <QDebug>
# include <QDir>
# include <QFileDialog>
# include <QFileInfo>
# include <QFutureWatcher>
# include <QHBoxLayout>
# include <QLabel>
# include <QLineEdit>
# include <QMimeDatabase>
# include <QPushButton>
# include <QVBoxLayout>
# include <QtConcurrent/QtConcurrent>

class Ui_scmConvertDialog;

class ScmConvertDialog : public StelDialog
{
	Q_OBJECT

public:
	explicit ScmConvertDialog(SkyCultureMaker *maker);
	~ScmConvertDialog() override;
	void retranslate() override;

protected:
	void createDialogContent() override;

private slots:
	void browseFile();
	void convert();
	void onConversionFinished();
	void closeDialog();
	bool chooseFallbackDirectory(QString &skyCulturesPath, QString &skyCultureId);

private:
	Ui_scmConvertDialog *ui;
	QFutureWatcher<QString> *watcher;
	QString tempDirPath;
	QString tempDestDirPath;
	std::atomic<bool> conversionCancelled;
	SkyCultureMaker *maker = nullptr;
};

#endif // SCM_CONVERTER_ENABLED_CPP
#endif // SCMCONVERTDIALOG_HPP
