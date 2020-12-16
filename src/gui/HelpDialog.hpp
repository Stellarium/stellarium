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

#ifndef HELPDIALOG_HPP
#define HELPDIALOG_HPP

#include <QString>
#include <QObject>
#include <QSettings>
#include <QNetworkReply>

#include "StelDialog.hpp"

class Ui_helpDialogForm;
class QListWidgetItem;
class QNetworkAccessManager;

class HelpDialog : public StelDialog
{
	Q_OBJECT
public:
	//! @enum UpdateState
	//! Used for keeping for track of the download/update status
	enum UpdateState {
		Updating,				//!< Update in progress
		CompleteNoUpdates,	//!< Update completed, there we no updates
		CompleteUpdates,		//!< Update completed, there were updates
		DownloadError,			//!< Error during download phase
		OtherError				//!< Other error
	};

	HelpDialog(QObject* parent);
	~HelpDialog() Q_DECL_OVERRIDE;

	//! Notify that the application style changed
	virtual void styleChanged() Q_DECL_OVERRIDE;

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent() Q_DECL_OVERRIDE;

	Ui_helpDialogForm* ui;

private:
	QString message;
	UpdateState updateState;
	QNetworkAccessManager * networkManager;
	QNetworkReply * downloadReply;

signals:
	void checkUpdatesComplete(void);

private slots:
	//! Show/bring to foreground the shortcut editor window.
	void showShortcutsWindow();
	
	//! On tab change, if the Log tab is selected, call refreshLog().
	void updateLog(int);

	//! Updated text in Help tab.
	void updateHelpText(void) const;

	//! Updated text in About tab.
	void updateAboutText(void) const;

	//! Sync the displayed log.
	void refreshLog() const;

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);

	void checkUpdates(void);
	void downloadComplete(QNetworkReply * reply);
	void setKeyButtonState(bool state);
};

#endif /*_HELPDIALOG_HPP*/

