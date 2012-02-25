/*
 * Stellarium
 * Copyright (C) 2008 Nigel Kerr
 * Copyright (C) 2012 Timothy Reaves
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

#ifndef _DATETIMEDIALOG_HPP_
#define _DATETIMEDIALOG_HPP_

#include <QObject>
#include "StelDialog.hpp"

class Ui_dateTimeDialogForm;

class DateTimeDialog : public StelDialog
{
	Q_OBJECT
public:
	DateTimeDialog();
	~DateTimeDialog();
	double newJd();
	bool valid(int y, int m, int d, int h, int min, int s);
	//! Notify that the application style changed
	void styleChanged();
public slots:
	void retranslate();
	//! update the editing display with new JD.
	void setDateTime(double newJd);

	void close();


protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	void connectSpinnerEvents() const;
	void disconnectSpinnerEvents()const;

private slots:
	//! year slider or dial changed
	void yearChanged(int ny);
	//! year slider or dial changed
	void monthChanged(int nm);
	//! year slider or dial changed
	void dayChanged(int nd);
	//! year slider or dial changed
	void hourChanged(int nh);
	//! year slider or dial changed
	void minuteChanged(int nm);
	//! year slider or dial changed
	void secondChanged(int ns);

private:
	Ui_dateTimeDialogForm* ui;
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	void pushToWidgets();
};

#endif // _DATETIMEDIALOG_HPP_
