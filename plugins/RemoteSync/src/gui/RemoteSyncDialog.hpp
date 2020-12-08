/*
 * Stellarium Remote Sync plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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

#ifndef REMOTESYNCDIALOG_HPP
#define REMOTESYNCDIALOG_HPP

#include <QString>
#include <QListWidgetItem>

#include "StelDialog.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

class Ui_remoteSyncDialog;
class RemoteSync;

class RemoteSyncDialog : public StelDialog
{
	Q_OBJECT

public:
	RemoteSyncDialog();
	~RemoteSyncDialog();

public slots:
	void retranslate();

protected:
	void createDialogContent();

private slots:
	void printErrorMessage(const QString error);
	void updateState();
	void updateIPlabel(bool running);

	void updateCheckboxesFromSyncOptions();
	void checkboxToggled(int id, bool state);

	void addPropertiesForExclusion();
	void removePropertiesForExclusion();
	void populateExclusionLists();

	void setConnectionLostBehavior(int idx);
	void setQuitBehavior(int idx);
	void restoreDefaults();

private:
	Ui_remoteSyncDialog* ui;
	RemoteSync* rs;

	void setAboutHtml();
	static QStringList listFromItemList(QList<QListWidgetItem*> list);
};


#endif /* REMOTESYNCDIALOG_HPP */
