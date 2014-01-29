/*
 * Copyright (C) 2013 Felix Zeltner
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

#ifndef PLANESDIALOG_HPP
#define PLANESDIALOG_HPP

#include "StelDialog.hpp"
#include "Flight.hpp"
#include "DBCredentials.hpp"

namespace Ui {
class PlanesDialog;
}

//! @class PlanesDialog
//! Provides the user with a GUI to customise the plugin behaviour.
class PlanesDialog : public StelDialog
{
	Q_OBJECT

public:
	explicit PlanesDialog();
	~PlanesDialog();

public slots:
	void retranslate();

	//! Display status string for database
	void setDBStatus(QString status);

	//! Display status string for data port
	void setBSStatus(QString status);

signals:
	//!@{
	//! Signals emitted, when settings change or buttons are clicked
	void pathColourModeChanged(Flight::PathColour mode);
	void pathDrawModeChanged(Flight::PathDrawMode mode);
	void showLabelsChanged(bool enabled);
	void fileSelected(QString filename);
	void useInterpChanged(bool interp);
	void dbUsedSet(bool dbUsedSet);
	void bsUsedSet(bool bsUsedSet);
	void credentialsChanged(DBCredentials creds);
	void connectDBRequested();
	void disconnectDBRequested();
	void bsHostChanged(QString host);
	void bsPortChanged(quint16 port);
	void bsUseDBChanged(bool dbUsedSet);
	void connectBSRequested();
	void disconnectBSRequested();
	void connectOnStartupChanged(bool enabled);
	//!@}

protected:
	//! Prepare the dialot for first time useage
	void createDialogContent();

private:
	//! Toggle certain settings that depend on other settings being enabled
	void updateDBFields();

	Ui::PlanesDialog *ui;
	QString cachedDBStatus;
	QString cachedBSStatus;

private slots:
	//!@{
	//! Slots to connect UI elements to, handle events such as button clicks and
	//! value changes.
	void setSolidCol()
	{
		emit pathColourModeChanged(Flight::SolidColour);
	}
	void setHeightCol()
	{
		emit pathColourModeChanged(Flight::EncodeHeight);
	}
	void setVelocityCol()
	{
		emit pathColourModeChanged(Flight::EncodeVelocity);
	}
	void setNoPaths()
	{
		emit pathDrawModeChanged(Flight::NoPaths);
	}
	void setSelectedPath()
	{
		emit pathDrawModeChanged(Flight::SelectedOnly);
	}
	void setInviewPaths()
	{
		emit pathDrawModeChanged(Flight::InViewOnly);
	}
	void setAllPaths()
	{
		emit pathDrawModeChanged(Flight::AllPaths);
	}
	void setLabels(bool enabled)
	{
		emit showLabelsChanged(enabled);
	}
	void setInterp(bool checked)
	{
		emit useInterpChanged(checked);
	}
	void setMinHeight(double val) {
		Flight::setMinHeight(val);
	}
	void setMaxHeight(double val) {
		Flight::setMaxHeight(val);
	}
	void setMinVel(double val) {
		Flight::setMinVelocity(val);
	}
	void setMaxVel(double val) {
		Flight::setMaxVelocity(val);
	}
	void setMinVertRate(double val) {
		Flight::setMinVertRate(val);
	}
	void setMaxVertRate(double val) {
		Flight::setMaxVertRate(val);
	}

	void openFile();

	void connectDB()
	{
		emit connectDBRequested();
	}
	void disconnectDB()
	{
		emit disconnectDBRequested();
	}

	void setUseDB();
	void updateCreds();

	void setBSHost(const QString &host)
	{
		emit bsHostChanged(host);
	}
	void setBSPort(const QString &port)
	{
		emit bsPortChanged(port.toUInt());
	}
	void setBSUseDB(bool enabled)
	{
		emit bsUseDBChanged(enabled);
	}
	void connectBS()
	{
		emit connectBSRequested();
	}
	void disconnectBS()
	{
		emit disconnectBSRequested();
	}
	void setUseBS();
	void setConnectOnStartup();
	//!@}
};

#endif // PLANESDIALOG_HPP
