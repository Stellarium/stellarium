/*
 *
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "ScriptConsole.hpp"
#include "ui_scriptConsole.h"
#include "StelMainView.hpp"
#include "StelScriptMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelScriptSyntaxHighlighter.hpp"

#include <QDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDateTime>
#include <QSyntaxHighlighter>
#include <QTextDocumentFragment>
#include <QRegularExpression>

ScriptConsole::ScriptConsole(QObject *parent)
	: StelDialog("ScriptConsole", parent)
	, highlighter(nullptr)
	, useUserDir(false)
	, hideWindowAtScriptRun(false)
	, clearOutput(false)	
	, scriptFileName("")
	, isNew(true)
	, dirty(false)
{
	ui = new Ui_scriptConsoleForm;
}

ScriptConsole::~ScriptConsole()
{
	delete ui;
	delete highlighter; highlighter = nullptr;
}

void ScriptConsole::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateQuickRunList();
	}
}

void ScriptConsole::styleChanged(const QString &style)
{
	StelDialog::styleChanged(style);
	if (highlighter)
	{ 
		highlighter->setFormats();
		highlighter->rehighlight();
	}
}

void ScriptConsole::populateQuickRunList()
{
	ui->quickrunCombo->clear();
	ui->quickrunCombo->addItem(""); // First line is empty!
	ui->quickrunCombo->addItem(qc_("selected text as script","command"));
	ui->quickrunCombo->addItem(qc_("remove screen text","command"));
	ui->quickrunCombo->addItem(qc_("remove screen images","command"));
	ui->quickrunCombo->addItem(qc_("remove screen markers","command"));
	ui->quickrunCombo->addItem(qc_("clear map: natural","command"));
	ui->quickrunCombo->addItem(qc_("clear map: starchart","command"));
	ui->quickrunCombo->addItem(qc_("clear map: deepspace","command"));
	ui->quickrunCombo->addItem(qc_("clear map: galactic","command"));
	ui->quickrunCombo->addItem(qc_("clear map: supergalactic","command"));
}

void ScriptConsole::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &ScriptConsole::retranslate);

	highlighter = new StelScriptSyntaxHighlighter(ui->scriptEdit->document());
	ui->includeEdit->setText(StelFileMgr::getInstallationDir() + "/scripts");

	populateQuickRunList();

	connect(ui->scriptEdit, &QPlainTextEdit::cursorPositionChanged, this, &ScriptConsole::rowColumnChanged);
	connect(ui->scriptEdit, &QPlainTextEdit::textChanged, this, &ScriptConsole::setDirty);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &ScriptConsole::handleMovedTo);
	connect(ui->loadButton, &QToolButton::clicked, this, &ScriptConsole::loadScript);
	connect(ui->saveButton, &QToolButton::clicked, this, &ScriptConsole::saveScript);
	connect(ui->clearButton, &QToolButton::clicked, this, &ScriptConsole::clearButtonPressed);
	connect(ui->preprocessSSCButton, &QPushButton::clicked, this, &ScriptConsole::preprocessScript);
	connect(ui->runButton, &QToolButton::clicked, this, &ScriptConsole::runScript);
	connect(ui->stopButton, &QToolButton::clicked, &StelApp::getInstance().getScriptMgr(), &StelScriptMgr::stopScript);
	connect(ui->includeBrowseButton, &QToolButton::clicked, this, &ScriptConsole::includeBrowse);
	connect(ui->quickrunCombo, &QComboBox::currentIndexChanged, this, &ScriptConsole::quickRun);
	connect(&StelApp::getInstance().getScriptMgr(), &StelScriptMgr::scriptRunning, this, &ScriptConsole::scriptStarted);
	connect(&StelApp::getInstance().getScriptMgr(), &StelScriptMgr::scriptStopped, this, &ScriptConsole::scriptEnded);
	connect(&StelApp::getInstance().getScriptMgr(), &StelScriptMgr::scriptDebug, this, &ScriptConsole::appendLogLine);
	connect(&StelApp::getInstance().getScriptMgr(), &StelScriptMgr::scriptOutput, this, &ScriptConsole::appendOutputLine);
	ui->tabs->setCurrentIndex(0);

	// get decent indentation
	QFont font = ui->scriptEdit->font();
	QFontMetrics fontMetrics = QFontMetrics(font);
	int width = fontMetrics.boundingRect("0").width();
	ui->scriptEdit->setTabStopDistance(4*width); // 4 characters
	ui->scriptEdit->setFocus();

	QSettings* conf = StelApp::getInstance().getSettings();
	useUserDir = conf->value("gui/flag_scripts_user_dir", false).toBool();
	ui->useUserDirCheckBox->setChecked(useUserDir);
	hideWindowAtScriptRun = conf->value("gui/flag_scripts_hide_window", false).toBool();
	ui->closeWindowAtScriptRunCheckbox->setChecked(hideWindowAtScriptRun);
	clearOutput = conf->value("gui/flag_scripts_clear_output", false).toBool();
	ui->clearOutputCheckbox->setChecked(clearOutput);
	connect(ui->useUserDirCheckBox, &QCheckBox::toggled, this, &ScriptConsole::setFlagUserDir);
	connect(ui->closeWindowAtScriptRunCheckbox, &QCheckBox::toggled, this, &ScriptConsole::setFlagHideWindow);
	connect(ui->clearOutputCheckbox, &QCheckBox::toggled, this, &ScriptConsole::setFlagClearOutput);

	ui->allowScreenshotDirCheckBox->setChecked(StelApp::getInstance().getScriptMgr().getFlagAllowExternalScreenshotDir());
	connect(ui->allowScreenshotDirCheckBox, &QCheckBox::clicked, &StelApp::getInstance().getScriptMgr(), &StelScriptMgr::setFlagAllowExternalScreenshotDir);

	ui->allowStoreAbsoluteCheckBox->setChecked(StelApp::getInstance().getScriptMgr().getFlagAllowWriteAbsolutePaths());
	connect(ui->allowStoreAbsoluteCheckBox, &QCheckBox::clicked, &StelApp::getInstance().getScriptMgr(), &StelScriptMgr::setFlagAllowWriteAbsolutePaths);

	ui->showWaitMessageCheckBox->setChecked(StelApp::getInstance().getScriptMgr().getFlagShowContinueMessage());
	connect(ui->showWaitMessageCheckBox, &QCheckBox::clicked, &StelApp::getInstance().getScriptMgr(), &StelScriptMgr::setFlagShowContinueMessage);

	dirty = false;
}

void ScriptConsole::setFlagUserDir(bool b)
{
	if (b!=useUserDir)
	{
		useUserDir = b;
		StelApp::getInstance().getSettings()->setValue("gui/flag_scripts_user_dir", b);
	}
}

void ScriptConsole::setFlagHideWindow(bool b)
{
	if (b!=hideWindowAtScriptRun)
	{
		hideWindowAtScriptRun = b;
		StelApp::getInstance().getSettings()->setValue("gui/flag_scripts_hide_window", b);
	}
}

void ScriptConsole::setFlagClearOutput(bool b)
{
	if (b!=clearOutput)
	{
		clearOutput = b;
		StelApp::getInstance().getSettings()->setValue("gui/flag_scripts_clear_output", b);
	}
}

void ScriptConsole::loadScript()
{
	if (dirty)
	{
		// We are loaded and dirty: don't just overwrite!
        if (QMessageBox::question(&StelMainView::getInstance(), q_("Caution!"), q_("Are you sure you want to load script without saving changes?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;
	}
	
	QString openDir;
	if (getFlagUserDir())
	{
		openDir = StelFileMgr::findFile("scripts", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory));		
		if (openDir.isEmpty() || openDir.contains(StelFileMgr::getInstallationDir()))
			openDir = StelFileMgr::getUserDir();
	}
	else
		openDir = StelFileMgr::getInstallationDir() + "/scripts";

	QString filter = q_("Stellarium Script Files");
	filter.append(" (*.ssc *.inc);;");
	filter.append(getFileMask());
	QString aFile = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Load Script"), openDir, filter);
	if (aFile.isNull())
		return;
	scriptFileName = aFile;
	QFile file(scriptFileName);
	if (file.open(QIODevice::ReadOnly))
	{
		ui->scriptEdit->setPlainText(file.readAll());
		dirty = false;
		ui->includeEdit->setText(StelFileMgr::dirName(scriptFileName));
		file.close();
	}
	ui->tabs->setCurrentIndex(0);
}

void ScriptConsole::saveScript()
{
	QString saveDir = StelFileMgr::findFile("scripts", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory));
	if (saveDir.isEmpty())
		saveDir = StelFileMgr::getUserDir();

	QString defaultFilter = q_("Stellarium Script");
	defaultFilter.append(" (*.ssc)");
	// Let's ask file name, when file is new and overwrite it in other case
	if (scriptFileName.isEmpty())
	{
		QString aFile = QFileDialog::getSaveFileName(&StelMainView::getInstance(), q_("Save Script"), saveDir + "/myscript.ssc", getFileMask(), &defaultFilter);
		if (aFile.isNull())
			return;
		scriptFileName = aFile;
	}
	else
	{
		// skip save
		if (!dirty)
			return;
	}
	QFile file(scriptFileName);
	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream out(&file);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		out.setEncoding(QStringConverter::Utf8);
#else
		out.setCodec("UTF-8");
#endif
		out << ui->scriptEdit->toPlainText();
		file.close();
		dirty = false;
	}
	else
		qWarning() << "[ScriptConsole] ERROR - cannot write script file";
}

void ScriptConsole::clearButtonPressed()
{
	if (ui->tabs->currentIndex() == 0)
	{
		if (QMessageBox::question(&StelMainView::getInstance(), q_("Caution!"), q_("Are you sure you want to clear script?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			ui->scriptEdit->clear();
			scriptFileName = ""; // OK, it's a new file!
			dirty = false;
		}
	}
	else if (ui->tabs->currentIndex() == 1)
		ui->logBrowser->clear();
	else if (ui->tabs->currentIndex() == 2)
		ui->outputBrowser->clear();
}

void ScriptConsole::preprocessScript()
{
	//perform pre-processing without an intermediate temp file
	QString dest;
	QString src = ui->scriptEdit->toPlainText();

	int errLoc = 0;
	if (sender() == ui->preprocessSSCButton)
	{
		qDebug() << "[ScriptConsole] Preprocessing with SSC proprocessor";
		StelApp::getInstance().getScriptMgr().preprocessScript( scriptFileName, src, dest, ui->includeEdit->text(), errLoc );
	}
	else
		qWarning() << "[ScriptConsole] Unknown preprocessor type";

	ui->scriptEdit->setPlainText(dest);
	scriptFileName = ""; // OK, it's a new file!
	dirty = true;
	ui->tabs->setCurrentIndex( 0 );
	if( errLoc != -1 ){
		QTextCursor tc = ui->scriptEdit->textCursor();
		tc.setPosition( errLoc );
		ui->scriptEdit->setTextCursor( tc );
	}
}

void ScriptConsole::runScript()
{
	ui->tabs->setCurrentIndex(1);
	ui->logBrowser->clear();
	if( clearOutput )
		ui->outputBrowser->clear();
	
	appendLogLine(QString("Starting script at %1").arg(QDateTime::currentDateTime().toString()));
	int errLoc = 0;
	if (!StelApp::getInstance().getScriptMgr().runScriptDirect(scriptFileName, ui->scriptEdit->toPlainText(), errLoc, ui->includeEdit->text()))
	{
		QString msg = QString("ERROR - cannot run script");
		qWarning() << "[ScriptConsole] " + msg;
		appendLogLine(msg);
		if( errLoc != -1 ){
			QTextCursor tc = ui->scriptEdit->textCursor();
			tc.setPosition( errLoc );
			ui->scriptEdit->setTextCursor( tc );
		}
		return;
	}
}

void ScriptConsole::scriptStarted()
{
	//prevent starting of scripts while any script is running
	ui->quickrunCombo->setEnabled(false);
	ui->runButton->setEnabled(false);
	ui->stopButton->setEnabled(true);
	if (hideWindowAtScriptRun)
		dialog->setVisible(false);
}

void ScriptConsole::scriptEnded()
{
	qDebug() << "ScriptConsole::scriptEnded";
	appendLogLine(QString("Script finished at %1").arg(QDateTime::currentDateTime().toString()));
	ui->quickrunCombo->setEnabled(true);
	ui->runButton->setEnabled(true);
	ui->stopButton->setEnabled(false);
	if (hideWindowAtScriptRun)
		dialog->setVisible(true);
}

void ScriptConsole::appendLogLine(const QString& s)
{
	static const QRegularExpression whitespaceExp("^\\s+");
	QString html = ui->logBrowser->toHtml();
	html.replace(whitespaceExp, "");
	html += s;
	ui->logBrowser->setHtml(html);
}

void ScriptConsole::appendOutputLine(const QString& s)
{
	if (s.isEmpty())
	{
		ui->outputBrowser->clear();
	}
	else
	{
		static const QRegularExpression whitespaceExp("^\\s+");
		QString html = ui->outputBrowser->toHtml();
		html.replace(whitespaceExp, "");
		html += s;
		ui->outputBrowser->setHtml(html);
	}
}

void ScriptConsole::includeBrowse()
{
	QString aDir = QFileDialog::getExistingDirectory(&StelMainView::getInstance(), q_("Select Script Include Directory"), StelFileMgr::getInstallationDir() + "/scripts");
	if (!aDir.isNull())
		ui->includeEdit->setText(aDir);
}

void ScriptConsole::quickRun(int idx)
{
	if (idx==0)
		return;	
	static const QMap<int, QString>map = {
		{2, "LabelMgr.deleteAllLabels();\n"},
		{3, "ScreenImageMgr.deleteAllImages();\n"},
		{4, "MarkerMgr.deleteAllMarkers();\n"},
		{5, "core.clear(\"natural\");\n"},
		{6, "core.clear(\"starchart\");\n"},
		{7, "core.clear(\"deepspace\");\n"},
		{8, "core.clear(\"galactic\");\n"},
		{9, "core.clear(\"supergalactic\");\n"}};
	QString scriptText = map.value(idx, QTextDocumentFragment::fromHtml(ui->scriptEdit->textCursor().selectedText(), ui->scriptEdit->document()).toPlainText());

	if (!scriptText.isEmpty())
	{
		if(clearOutput)
			ui->outputBrowser->clear();
		appendLogLine(QString("Running: %1").arg(scriptText));
		int errLoc;
		StelApp::getInstance().getScriptMgr().runScriptDirect( "<>", scriptText, errLoc );
		ui->quickrunCombo->setCurrentIndex(0);
	}
}

void ScriptConsole::rowColumnChanged()
{
	// TRANSLATORS: The first letter of word "Row"
	QString row = qc_("R", "text cursor");
	// TRANSLATORS: The first letter of word "Column"
	QString column = qc_("C", "text cursor");
	ui->rowColumnLabel->setText(QString("%1:%2 %3:%4")
				    .arg(row).arg(ui->scriptEdit->textCursor().blockNumber())
				    .arg(column).arg(ui->scriptEdit->textCursor().columnNumber()));
}

void ScriptConsole::setDirty()
{
	if (isNew)
		isNew = false;
	else
		dirty = true;
}

const QString ScriptConsole::getFileMask()
{
	QString filter = q_("Stellarium Script");
	filter.append(" (*.ssc);;");
	filter.append(q_("Include File"));
	filter.append(" (*.inc)");
	return  filter;
}

