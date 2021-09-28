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
	virtual ~ScriptConsole() Q_DECL_OVERRIDE;
	//! Notify that the application style changed
	virtual void styleChanged() Q_DECL_OVERRIDE;

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

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
	void setDirty();

	void setFlagUserDir(bool b);
	void setFlagHideWindow(bool b);
	void setFlagClearOutput(bool b);
	void populateQuickRunList();
	
protected:
	Ui_scriptConsoleForm* ui;
	
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent() Q_DECL_OVERRIDE;

private:
	static const QString getFileMask();
	StelScriptSyntaxHighlighter* highlighter;
	bool useUserDir;
	bool hideWindowAtScriptRun;
	bool clearOutput;

	// The script editor FSM has four states: dirty (t/f), file name.
	// Input events are: buttons load, save, clear and keyboard input.
	// Actions are ld/sv: a file, fn: set file name, cb: clear buffer
	// and pre: preprocessing.
	//           load     kbd  save       cb        pre
	// 1: f/-   2|ld      3|-  2|sv,fn    1|cb      3: t/-
	// 2: f/fn  2|ld      4|-  3|no-op    1|cb      3: t/-
	// 3: t/-   Y: 2|ld   3|-  2|sv,fn    Y: 1|cb   3: t/-
	//          N: 3|--                   N: 3|-
	// 4: t/fn  Y: 2|ld   4|-  2|sv       Y: 1|cb   3: t/-
	//  	    N: 4|--                   N: 4|-
	QString scriptFileName;
	bool isNew;
	bool dirty;

	bool getFlagUserDir() { return useUserDir; }
};

#endif // _SCRIPTCONSOLE_HPP

