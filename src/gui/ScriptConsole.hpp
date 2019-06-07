/*
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
 
#ifndef SCRIPTCONSOLE_HPP
#define SCRIPTCONSOLE_HPP

#include <QObject>
#include "StelDialog.hpp"

class Ui_scriptConsoleForm;
class StelScriptSyntaxHighlighter;

class ScriptConsole : public StelDialog
{
	Q_OBJECT
public:
	ScriptConsole(QObject* parent);
	virtual ~ScriptConsole();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();

private slots:
	void runScript();
	void loadScript();
	void saveScript();
	void clearButtonPressed();
	void preprocessScript();
	void scriptStarted();
	void scriptEnded();
	void appendLogLine(const QString& s);
	void appendOutputLine(const QString& s);
	void includeBrowse();
	void quickRun(int idx);
	void rowColumnChanged();
	
protected:
	Ui_scriptConsoleForm* ui;
	
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

private:
	static const QString getFileMask();
	StelScriptSyntaxHighlighter* highlighter;
};

#endif // _SCRIPTCONSOLE_HPP
