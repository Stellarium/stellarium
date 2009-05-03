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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "ScriptConsole.hpp"
#include "ui_scriptConsole.h"
#include "StelMainGraphicsView.hpp"
#include "StelScriptMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelScriptSyntaxHighlighter.hpp"

#include <QDialog>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDateTime>
#include <QSyntaxHighlighter>
#include <QTextDocumentFragment>

ScriptConsole::ScriptConsole()
{
	ui = new Ui_scriptConsoleForm;
}

ScriptConsole::~ScriptConsole()
{
	delete ui;
}

void ScriptConsole::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ScriptConsole::styleChanged()
{
	// Nothing for now
}

void ScriptConsole::createDialogContent()
{
	ui->setupUi(dialog);

	highlighter = new StelScriptSyntaxHighlighter(ui->scriptEdit->document());
	ui->includeEdit->setText(StelApp::getInstance().getFileMgr().getInstallationDir() + "/scripts");

	ui->quickrunCombo->addItem(q_("quickrun..."));
	ui->quickrunCombo->addItem(q_("selected text"));
	ui->quickrunCombo->addItem(q_("clear text"));
	ui->quickrunCombo->addItem(q_("clear images"));
	ui->quickrunCombo->addItem(q_("natural"));
	ui->quickrunCombo->addItem(q_("starchart"));

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(loadScript()));
	connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveScript()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clearButtonPressed()));
	connect(ui->preprocessSSCButton, SIGNAL(clicked()), this, SLOT(preprocessScript()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runScript()));
	connect(ui->stopButton, SIGNAL(clicked()), &StelApp::getInstance().getScriptMgr(), SLOT(stopScript()));
	connect(ui->includeBrowseButton, SIGNAL(clicked()), this, SLOT(includeBrowse()));
	connect(ui->quickrunCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(quickRun(int)));
	connect(&StelApp::getInstance().getScriptMgr(), SIGNAL(scriptStopped()), this, SLOT(scriptEnded()));
	connect(&StelApp::getInstance().getScriptMgr(), SIGNAL(scriptDebug(const QString&)), this, SLOT(appendLogLine(const QString&)));
#ifndef ENABLE_STRATOSCRIPT_COMPAT
	ui->preprocessSTSButton->setHidden(true);
#else
	connect(ui->preprocessSTSButton, SIGNAL(clicked()), this, SLOT(preprocessScript()));
#endif
	ui->tabs->setCurrentIndex(0);
}

void ScriptConsole::loadScript()
{
	QString fileName = QFileDialog::getOpenFileName(&StelMainGraphicsView::getInstance(), 
	                                                tr("Load Script"), 
	                                                StelApp::getInstance().getFileMgr().getInstallationDir() + "/scripts", 
	                                                tr("Script Files") + " " + getFileMask());
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
	QString saveDir;
	try
	{
		saveDir = StelApp::getInstance().getFileMgr().findFile("scripts", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::Directory));
	}
	catch (std::runtime_error& e)
	{
		saveDir = StelApp::getInstance().getFileMgr().getUserDir();
	}

	QString fileName = QFileDialog::getSaveFileName(&StelMainGraphicsView::getInstance(), 
	                                                tr("Save Script"), 
	                                                saveDir,
	                                                tr("Script Files") + " " + getFileMask());
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
		ui->outputBrowser->clear();
}

void ScriptConsole::preprocessScript()
{
	qDebug() << "ScriptConsole::preprocessScript";
	QTemporaryFile src(QDir::tempPath() + "/stelscriptXXXXXX");
	QTemporaryFile dest(QDir::tempPath() + "/stelscriptXXXXXX");
	QString srcName;
	if (src.open())
	{
		QTextStream out(&src);
		out << ui->scriptEdit->toPlainText();
		srcName = src.fileName();
		src.close();
		src.open();
		if (dest.open())
		{
			if (sender() == ui->preprocessSSCButton)
			{
				qDebug() << "Preprocessing with SSC proprocessor";
				StelApp::getInstance().getScriptMgr().preprocessScript(src, dest, ui->includeEdit->text());
			}
#ifdef ENABLE_STRATOSCRIPT_COMPAT
			else if (sender() == ui->preprocessSTSButton)
			{
				qDebug() << "Preprocessing with STS proprocessor";
				StelApp::getInstance().getScriptMgr().preprocessStratoScript(src, dest, ui->includeEdit->text());
			}
#endif
			else
				qWarning() << "WARNING: unknown preprocessor type";
				
			dest.seek(0);
			ui->scriptEdit->setPlainText(dest.readAll());
			dest.close();
		}
	}
	ui->tabs->setCurrentIndex(0);
}

void ScriptConsole::runScript()
{
	ui->tabs->setCurrentIndex(1);
	ui->outputBrowser->setHtml("");
	QTemporaryFile file(QDir::tempPath() + "/stelscriptXXXXXX.ssc");
	QString fileName;
	if (file.open()) {
		QTextStream out(&file);
		out << ui->scriptEdit->toPlainText() << "\n";
		fileName = file.fileName();
		file.close();
	}
	else
	{	
		QString msg = "ERROR - cannot open tmp file for writing script text";
		qWarning() << "ScriptConsole::runScript " + msg;
		appendLogLine(msg);
		return;
	}

	appendLogLine(QString("Starting script at %1").arg(QDateTime::currentDateTime().toString()));
	if (!StelApp::getInstance().getScriptMgr().runScript(fileName, ui->includeEdit->text()))
	{
		QString msg = QString("ERROR - cannot run script from temp file: \"%1\"").arg(fileName);
		qWarning() << "ScriptConsole::runScript " + msg;
		appendLogLine(msg);
		if (file.open())
		{
			int n=0;
			while(!file.atEnd()) 
			{
				appendLogLine(QString("%1:%2").arg(n,2).arg(QString(file.readLine())));
			}
			file.close();
		}
		return;
	}
	ui->runButton->setEnabled(false);
	ui->stopButton->setEnabled(true);
}

void ScriptConsole::scriptEnded()
{
	QString html = ui->outputBrowser->toHtml();
	appendLogLine(QString("Script finished at %1").arg(QDateTime::currentDateTime().toString()));
	ui->runButton->setEnabled(true);
	ui->stopButton->setEnabled(false);
}

void ScriptConsole::appendLogLine(const QString& s)
{
	QString html = ui->outputBrowser->toHtml();
	// if (html!="")
	// 	html += "<br />";

	html += s;
	ui->outputBrowser->setHtml(html);
}

void ScriptConsole::includeBrowse()
{
	ui->includeEdit->setText(QFileDialog::getExistingDirectory(&StelMainGraphicsView::getInstance(), 
	                                                           tr("Select Script Includ Directory"), 
	                                                           StelApp::getInstance().getFileMgr().getInstallationDir() + "/scripts"));
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

	QTemporaryFile file(QDir::tempPath() + "/stelscriptXXXXXX.ssc");
	if (file.open())
	{
		QString fileName = file.fileName();
		QTextStream out(&file);
		out << scriptText;
		file.close();
		appendLogLine(QString("Running: %1").arg(scriptText));
		StelApp::getInstance().getScriptMgr().runScript(fileName);
	}
	else
	{
		appendLogLine("Can't run quick script (could not open temp file)");
	}
}

QString ScriptConsole::getFileMask()
{
#ifdef ENABLE_STRATOSCRIPT_COMPAT
	return "(*.ssc *.sts *.inc)";
#else
	return "(*.ssc *.inc)";
#endif
}
