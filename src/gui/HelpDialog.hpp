/*
 * Stellarium
 * Copyright (C) 2007 Matthew Gates
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

#ifndef _HELPDIALOG_HPP_
#define _HELPDIALOG_HPP_

#include <QString>
#include <QObject>
#include <QSettings>

#include "StelDialog.hpp"

class Ui_helpDialogForm;
class QListWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;

class HelpDialog : public StelDialog
{
	Q_OBJECT
public:
	//! @enum UpdateState
	//! Used for keeping for track of the download/update status
	enum UpdateState {
		Updating,		//!< Update in progress
		CompleteNoUpdates,	//!< Update completed, there we no updates
		CompleteUpdates,	//!< Update completed, there were updates
		DownloadError,		//!< Error during download phase
		OtherError		//!< Other error
	};

	HelpDialog(QObject* parent);
	~HelpDialog();

	//! Notify that the application style changed
	void styleChanged();

	//! get whether or not the plugin will try to update data from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled(void) {return updatesEnabled;}
	//! set whether or not the plugin will try to update data from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) {updatesEnabled=b;}
	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}
	//! Get the version from the "latestVersion" value in the updates.json file
	//! @return version string, e.g. "0.12.4"
	QString getLatestVersionFromJson(void);
	int getRequiredOpenGLVersionFromJson(void);
	void setUpdatesMessage(bool hasUpdates, QString version="", int OpenGL=0);
	QString getUpdatesMessage();

public slots:
	void retranslate();
	//! Download JSON from web recources described in the module section of the
	//! module.ini file and update the local JSON file.
	void updateJSON(void);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

	Ui_helpDialogForm* ui;

signals:
	//! @param state the new update state.
	void updateStateChanged(HelpDialog::UpdateState state);

private slots:
	//! Show/bring to foreground the shortcut editor window.
	void showShortcutsWindow();
	
	//! On tab change, if the Log tab is selected, call refreshLog().
	void updateLog(int);

	//! Sync the displayed log.
	void refreshLog();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);

	//! check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void updateDownloadComplete(QNetworkReply* reply);

private:
	//! Return the help text with keys description and external links.
	QString getHelpText(void);

	//! This function concatenates the header, key codes and footer to build
	//! up the help text.
	void updateText(void);

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file.
	void readJsonFile(void);

	// variables and functions for the updater
	UpdateState updateState;
	QSettings* conf;
	QNetworkAccessManager* downloadMgr;
	QString updateUrl;	
	bool updatesEnabled;
	QString jsonDataPath;
	QString currentVersion;
	QString updatesMessage;
};

#endif /*_HELPDIALOG_HPP_*/

