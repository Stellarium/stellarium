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

#ifndef DATETIMEDIALOG_HPP
#define DATETIMEDIALOG_HPP

#include <QObject>
#include <QTimer>
#include "StelDialog.hpp"

class Ui_dateTimeDialogForm;

class DateTimeDialog : public StelDialog
{
	Q_OBJECT
public:
	DateTimeDialog(QObject* parent);
	~DateTimeDialog() Q_DECL_OVERRIDE;
	double newJd();
	bool valid(int y, int m, int d, int h, int min, int s);
	bool validJd(double jday);	
public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;
	virtual void close() Q_DECL_OVERRIDE;
	//! update the editing display with new JD.
	void setDateTime(double newJd);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent() Q_DECL_OVERRIDE;
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
	//! JD slider or dial changed
	void jdChanged(double njd);
	//! MJD slider or dial changed
	void mjdChanged(double nmjd);
	//! handle timer-triggered update
	void onTimerTimeout(void);
	void setFlagEnableFocus(bool b);

private:
	StelCore* core;
	Ui_dateTimeDialogForm* ui;
	QTimer *updateTimer;
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	double jd;
	void pushToWidgets();
	void setMjd(double mjd) { jd = 2400000.5 + mjd; }
	double getMjd() const { return jd - 2400000.5 ; }

	int oldyear;
	int oldmonth;
	int oldday;

	bool enableFocus;
};

#endif // DATETIMEDIALOG_HPP
