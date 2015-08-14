/*
 * Stellarium
 * Copyright (C) 2015 Guillaume Chereau
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

#include <QObject>
#include <QVariantList>

// This object is only used as a proxy to provide the list of registered
// buttons to the QML gui.
class StelGui : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList buttons MEMBER buttons NOTIFY changed)
	Q_PROPERTY(bool autoHideHorizontalButtonBar MEMBER autoHideHorizontalButtonBar NOTIFY changed)
	Q_PROPERTY(bool autoHideVerticalButtonBar MEMBER autoHideVerticalButtonBar NOTIFY changed)
	Q_PROPERTY(bool helpDialogVisible MEMBER helpDialogVisible NOTIFY changed)
	Q_PROPERTY(bool configurationDialogVisible MEMBER configurationDialogVisible NOTIFY changed)
	Q_PROPERTY(bool searchDialogVisible MEMBER searchDialogVisible NOTIFY changed)
	Q_PROPERTY(bool viewDialogVisible MEMBER viewDialogVisible NOTIFY changed)
	Q_PROPERTY(bool dateTimeDialogVisible MEMBER dateTimeDialogVisible NOTIFY changed)
	Q_PROPERTY(bool locationDialogVisible MEMBER locationDialogVisible NOTIFY changed)
	Q_PROPERTY(bool shortcutsDialogVisible MEMBER shortcutsDialogVisible NOTIFY changed)

signals:
	void changed();

public:
	StelGui();
	void addButton(QString pixOn, QString pixOff,
				   QString action, QString groupName,
				   QString beforeActionName = QString());

private:
	QVariantList buttons;
	bool autoHideHorizontalButtonBar;
	bool autoHideVerticalButtonBar;

	bool helpDialogVisible;
	bool configurationDialogVisible;
	bool searchDialogVisible;
	bool viewDialogVisible;
	bool dateTimeDialogVisible;
	bool locationDialogVisible;
	bool shortcutsDialogVisible;
};
