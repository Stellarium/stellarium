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
		ui->retranslateUi(dialog);
}

void ScriptConsole::styleChanged()
{
	if (highlighter)
	{ 
		highlighter->setFormats();
		highlighter->rehighlight();
	}
}

void ScriptConsole::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	highlighter = new StelScriptSyntaxHighlighter(ui->scriptEdit->document());
	ui->includeEdit->setText(StelFileMgr::getInstallationDir() + "/scripts");

	ui->quickrunCombo->addItem(q_("quickrun..."));
	ui->quickrunCombo->addItem(q_("selected text"));
	ui->quickrunCombo->addItem(q_("clear text"));
	ui->quickrunCombo->addItem(q_("clear images"));
	ui->quickrunCombo->addItem(q_("natural"));
	ui->quickrunCombo->addItem(q_("starchart"));

	connect(ui->scriptEdit, SIGNAL(cursorPositionChanged()), this, SLOT(rowColumnChanged()));
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
	ui->scriptEdit->setFocus();
}

void ScriptConsole::loadScript()
{
	QString fileName = QFileDialog::getOpenFileName(Q_NULLPTR,
							q_("Load Script"),
	                                                StelFileMgr::getInstallationDir() + "/scripts", 
							q_("Script Files") + " " + getFileMask());
	QFile file(fileName);
	if (file.open(QIODevice::ReadOnly))
	{
		ui->scriptEdit->setPlainText(file.readAll());
		ui->includeEdit->setText(StelFileMgr::dirName(fileName));
		file.close();
	}
	ui->tabs->setCurrentIndex(0);
}

void ScriptConsole::saveScript()
{
	QString saveDir = StelFileMgr::findFile("scripts", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory));
	if (saveDir.isEmpty())
		saveDir = StelFileMgr::getUserDir();

	QString fileName = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save Script"),
	                                                saveDir,
							q_("Script Files") + " " + getFileMask());
	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream out(&file);
		out << ui->scriptEdit->toPlainText();
		file.close();
	}
	else
		qWarning() << "ERROR - cannot write script file";
}

void ScriptConsole::clearButtonPressed()
{
	if (ui->tabs->currentIndex() == 0)
		ui->scriptEdit->clear();
	else if (ui->tabs->currentIndex() == 1)
		ui->logBrowser->clear();
	else if (ui->tabs->currentIndex() == 2)
		ui->outputBrowser->clear();
}

void ScriptConsole::preprocessScript()
{
	qDebug() << "ScriptConsole::preprocessScript";
	//perform pre-processing without an intermediate temp file
	QString dest;
	QString src = ui->scriptEdit->toPlainText();

	if (sender() == ui->preprocessSSCButton)
	{
		qDebug() << "Preprocessing with SSC proprocessor";
		StelApp::getInstance().getScriptMgr().preprocessScript(src, dest, ui->includeEdit->text());
	}
	else
		qWarning() << "WARNING: unknown preprocessor type";

	ui->scriptEdit->setPlainText(dest);
	ui->tabs->setCurrentIndex(0);
}

void ScriptConsole::runScript()
{
	ui->tabs->setCurrentIndex(1);
	ui->logBrowser->setHtml("");

	appendLogLine(QString("Starting script at %1").arg(QDateTime::currentDateTime().toString()));
	if (!StelApp::getInstance().getScriptMgr().runScriptDirect(ui->scriptEdit->toPlainText(), ui->includeEdit->text()))
	{
		QString msg = QString("ERROR - cannot run script");
		qWarning() << "ScriptConsole::runScript " + msg;
		appendLogLine(msg);
		return;
	}
}

void ScriptConsole::scriptStarted()
{
	//prevent strating of scripts while any script is running
	ui->quickrunCombo->setEnabled(false);
	ui->runButton->setEnabled(false);
	ui->stopButton->setEnabled(true);
}

void ScriptConsole::scriptEnded()
{
	qDebug() << "ScriptConsole::scriptEnded";
	QString html = ui->logBrowser->toHtml();
	appendLogLine(QString("Script finished at %1").arg(QDateTime::currentDateTime().toString()));
	ui->quickrunCombo->setEnabled(true);
	ui->runButton->setEnabled(true);
	ui->stopButton->setEnabled(false);
}

void ScriptConsole::appendLogLine(const QString& s)
{
	QString html = ui->logBrowser->toHtml();
	html.replace(QRegExp("^\\s+"), "");
	// if (html!="")
	// 	html += "<br />";

	html += s;
	ui->logBrowser->setHtml(html);
}

void ScriptConsole::appendOutputLine(const QString& s)
{
	QString html = ui->outputBrowser->toHtml();
	html.replace(QRegExp("^\\s+"), "");
	// if (html!="")
	// 	html += "<br />";

	html += s;
	ui->outputBrowser->setHtml(html);
}


void ScriptConsole::includeBrowse()
{
	ui->includeEdit->setText(QFileDialog::getExistingDirectory(Q_NULLPTR,
								   q_("Select Script Include Directory"),
	                                                           StelFileMgr::getInstallationDir() + "/scripts"));
}

void ScriptConsole::quickRun(int idx)
{
	ui->quickrunCombo->setCurrentIndex(0);
	QString scriptText;
	if (idx==1)
	{
		scriptText = QTextDocumentFragment::fromHtml(ui->scriptEdit->textCursor().selectedText(), ui->scriptEdit->document()).toPlainText();
		qDebug() << "selected script text is:" << scriptText; 
	}
	if (idx==2)
	{
		scriptText = "LabelMgr.deleteAllLabels();\n";
	}
	if (idx==3)
	{
		scriptText = "ScreenImageMgr.deleteAllImages()\n";
	}
	if (idx==4)
	{
		scriptText = "core.clear(\"natural\");\n";
	}
	if (idx==5)
	{
		scriptText = "core.clear(\"starchart\");\n";
	}

	if (scriptText.isEmpty())
		return;

	appendLogLine(QString("Running: %1").arg(scriptText));
	StelApp::getInstance().getScriptMgr().runScriptDirect(scriptText);
}

void ScriptConsole::rowColumnChanged()
{
	ui->rowColumnLabel->setText(QString("R:%1 C:%2").arg(ui->scriptEdit->textCursor().blockNumber())
	                                                .arg(ui->scriptEdit->textCursor().columnNumber()));
}

const QString ScriptConsole::getFileMask()
{
	return "(*.ssc *.inc)";
}
