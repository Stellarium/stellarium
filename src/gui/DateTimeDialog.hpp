/*
 * Stellarium
 * Copyright (C) 2008 Nigel Kerr
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

#ifndef _DATETIMEDIALOG_HPP_
#define _DATETIMEDIALOG_HPP_

#include <QObject>

class Ui_dateTimeDialogForm;

class DateTimeDialog : public QObject
{
	Q_OBJECT
public:
	DateTimeDialog();
public slots:
	void setVisible(bool);
	void close();
	//! Called when the time8601 emits textEditing(const QString&)
	void dateTimeEdited(const QString &v);
	//! update the editing display with new JD.
	void setDateTime(double newJd);
signals:
	void closed();
	//! signals that a new, valid JD is available.
	void dateTimeChanged(double newJd);
protected:
	QWidget* dialog;
	Ui_dateTimeDialogForm* ui;
};

#endif // _DATETIMEDIALOG_HPP_
