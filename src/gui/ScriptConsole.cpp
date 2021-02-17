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

ScriptConsole::ScriptConsole(QObject *parent)
	: StelDialog("ScriptConsole", parent)
	, highlighter(Q_NULLPTR)
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
	delete highlighter; highlighter = Q_NULLPTR;
}

void ScriptConsole::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateQuickRunList();
	}
}

void ScriptConsole::styleChanged()
{
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
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	highlighter = new StelScriptSyntaxHighlighter(ui->scriptEdit->document());
	ui->includeEdit->setText(StelFileMgr::getInstallationDir() + "/scripts");

	populateQuickRunList();

	connect(ui->scriptEdit, SIGNAL(cursorPositionChanged()), this, SLOT(rowColumnChanged()));
	connect(ui->scriptEdit, SIGNAL(textChanged()), this, SLOT(setDirty()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(loadScript()));
	connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveScript()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clearButtonPressed()));
	connect(ui->preprocessSSCButton, SIGNAL(clicked()), this, SLOT(preprocessScript()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runScript()));
	connect(ui->stopButton, SIGNAL(clicked()), &StelApp::getInstance().getScriptMgr(), SLOT(stopScript()));
	connect(ui->includeBrowseButton, SIGNAL(clicked()), this, SLOT(includeBrowse()));
	connect(ui->quickrunCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(quickRun(int)));
	connect(&StelApp::getInstance().getScriptMgr(), SIGNAL(scriptRunning()), this, SLOT(scriptStarted()));
	connect(&StelApp::getInstance().getScriptMgr(), SIGNAL(scriptStopped()), this, SLOT(scriptEnded()));
	connect(&StelApp::getInstance().getScriptMgr(), SIGNAL(scriptDebug(const QString&)), this, SLOT(appendLogLine(const QString&)));
	connect(&StelApp::getInstance().getScriptMgr(), SIGNAL(scriptOutput(const QString&)), this, SLOT(appendOutputLine(const QString&)));
	ui->tabs->setCurrentIndex(0);

	// get decent indentation
	QFont font = ui->scriptEdit->font();
	QFontMetrics fontMetrics = QFontMetrics(font);
	int width = fontMetrics.width("0");
	ui->scriptEdit->setTabStopWidth(4*width); // 4 characters
	ui->scriptEdit->setFocus();

	QSettings* conf = StelApp::getInstance().getSettings();
	useUserDir = conf->value("gui/flag_scripts_user_dir", false).toBool();
	ui->useUserDirCheckBox->setChecked(useUserDir);
	hideWindowAtScriptRun = conf->value("gui/flag_scripts_hide_window", false).toBool();
	ui->closeWindowAtScriptRunCheckbox->setChecked(hideWindowAtScriptRun);
	clearOutput = conf->value("gui/flag_scripts_clear_output", false).toBool();
	ui->clearOutputCheckbox->setChecked(clearOutput);
	connect(ui->useUserDirCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFlagUserDir(bool)));
	connect(ui->closeWindowAtScriptRunCheckbox, SIGNAL(toggled(bool)), this, SLOT(setFlagHideWindow(bool)));
	connect(ui->clearOutputCheckbox, SIGNAL(toggled(bool)), this, SLOT(setFlagClearOutput(bool)));

	// Let's improve visibility of the text
	QString style = "QLabel { color: rgb(238, 238, 238); }";
	ui->quickrunLabel->setStyleSheet(style);
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
	QString aFile = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Load Script"), openDir, filter);
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

	QString defaultFilter("(*.ssc)");
	// Let's ask file name, when file is new and overwrite him in other case	
	if (scriptFileName.isEmpty())
	{
		QString aFile = QFileDialog::getSaveFileName(Q_NULLPTR, q_("Save Script"), saveDir + "/myscript.ssc", getFileMask(), &defaultFilter);
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
		out.setCodec("UTF-8");
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
		qWarning() << "[ScriptConsole] WARNING - unknown preprocessor type";

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
	//prevent strating of scripts while any script is running
	ui->quickrunCombo->setEnabled(false);
	ui->runButton->setEnabled(false);
	ui->stopButton->setEnabled(true);
	if (hideWindowAtScriptRun)
		dialog->setVisible(false);
}

void ScriptConsole::scriptEnded()
{
	qDebug() << "ScriptConsole::scriptEnded";
	QString html = ui->logBrowser->toHtml();
	appendLogLine(QString("Script finished at %1").arg(QDateTime::currentDateTime().toString()));
	ui->quickrunCombo->setEnabled(true);
	ui->runButton->setEnabled(true);
	ui->stopButton->setEnabled(false);
	if (hideWindowAtScriptRun)
		dialog->setVisible(true);
}

void ScriptConsole::appendLogLine(const QString& s)
{
	QString html = ui->logBrowser->toHtml();
	html.replace(QRegExp("^\\s+"), "");
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
		QString html = ui->outputBrowser->toHtml();
		html.replace(QRegExp("^\\s+"), "");
		html += s;
		ui->outputBrowser->setHtml(html);
	}
}

void ScriptConsole::includeBrowse()
{
	QString aDir = QFileDialog::getExistingDirectory(Q_NULLPTR, q_("Select Script Include Directory"), StelFileMgr::getInstallationDir() + "/scripts");
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

