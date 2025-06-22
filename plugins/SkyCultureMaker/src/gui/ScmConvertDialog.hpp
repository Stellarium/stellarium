#ifndef SCMCONVERTDIALOG_HPP
#define SCMCONVERTDIALOG_HPP

#include "StelDialogSeparate.hpp"

#ifdef SCM_CONVERTER_ENABLED_CPP

# include "unarr.h"
# include "StelFileMgr.hpp"
# include <QDebug>
# include <QFileDialog>
# include <QFileInfo>
# include <QHBoxLayout>
# include <QLabel>
# include <QLineEdit>
# include <QMimeDatabase>
# include <QPushButton>
# include <QVBoxLayout>
# include <QtConcurrent/QtConcurrent>
# include <QFutureWatcher>
# include <QDir>

class TitleBar;

class ScmConvertDialog : public StelDialogSeparate
{
	Q_OBJECT

public:
	explicit ScmConvertDialog();
	~ScmConvertDialog() override;
	void retranslate() override;
	QWidget *getDialog() const { return dialog; }

protected:
	void createDialogContent() override;

private slots:
	void browseFile();
	void convert();
	void onConversionFinished();

private:
	QLineEdit *filePathLineEdit;
	QLabel *convertResultLabel;
	QPushButton *convertButton;
	QFutureWatcher<QString> *watcher;
	QString tempDirPath;
	QString tempDestDirPath;

	// For retranslation
	TitleBar *titleBar;
	QLabel *infoLabel;
	QPushButton *browseButton;
};

#else // SCM_CONVERTER_ENABLED_CPP

class ScmConvertDialog : public StelDialogSeparate
{
	Q_OBJECT
public:
	explicit ScmConvertDialog() : StelDialogSeparate("ScmConvertDialog") {}
	void retranslate() override {}
protected:
	void createDialogContent() override {}
};

#endif // SCM_CONVERTER_ENABLED_CPP
#endif // SCMCONVERTDIALOG_HPP
