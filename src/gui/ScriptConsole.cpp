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

#include <QDialog>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDateTime>
#include <QPlainTextEdit>

void dumpFile(QString fileName)
{
	qDebug() << "dumpFile(" + fileName + "):";
	QFile file(fileName);
	if (file.open(QIODevice::ReadOnly))
	{
		while(!file.atEnd()) 
		{
			QString line = file.readLine();
			line.replace("\n", "");
			qDebug() << line;
		}
		file.close();
	}
}

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
	ui->includeEdit->setText(StelApp::getInstance().getFileMgr().getInstallationDir() + "/scripts");

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->clearButton, SIGNAL(clicked()), ui->scriptEdit, SLOT(clear()));
	connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(loadScript()));
	connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveScript()));
	connect(ui->preprocessSSCButton, SIGNAL(clicked()), this, SLOT(preprocessScript()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runScript()));
	connect(ui->stopButton, SIGNAL(clicked()), &StelApp::getInstance().getScriptMgr(), SLOT(stopScript()));
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
	if (!StelApp::getInstance().getScriptMgr().runScript(fileName))
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

QString ScriptConsole::getFileMask()
{
#ifdef ENABLE_STRATOSCRIPT_COMPAT
	return "(*.ssc *.sts *.inc)";
#else
	return "(*.ssc *.inc)";
#endif
}
