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
#include <QMultiMap>
#include <QPair>
#include <QHash>

#include "StelDialog.hpp"

class Ui_helpDialogForm;
class QListWidgetItem;

class HelpDialog : public StelDialog
{
	Q_OBJECT
public:
	HelpDialog();
	~HelpDialog();

	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();
	void updateIconsColor();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

	Ui_helpDialogForm* ui;

private slots:
	//! Slot that's called when the current tab changes. Updates log file
	//! if Log tab is selected.
	void updateLog(int);

	//! Sync the displayed log.
	void refreshLog();

	void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
	//! Return the help text with keys description and external links.
	QString getHelpText(void);

	//! This function concatenates the header, key codes and footer to build
	//! up the help text.
	void updateText(void);

	//! Sort function for qSort to compare QPair<QString,QString> values.
	//! This is used when displaying the hlp text to sort the items in a group
	//! by the key code (first item of the QPair)
	static bool helpItemSort(const QPair<QString, QString>& p1, const QPair<QString, QString>& p2);

	//! Sort function for putting the Misc group at the end of the list of groups
	static bool helpGroupSort(const QString& s1, const QString& s2);
};

#endif /*_HELPDIALOG_HPP_*/

