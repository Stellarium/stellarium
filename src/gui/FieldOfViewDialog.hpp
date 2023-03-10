/*
 * Stellarium
 * Copyright (C) 2023 Alexander Wolf
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

#ifndef FIELDOFVIEWDIALOG_HPP
#define FIELDOFVIEWDIALOG_HPP

#include <QObject>
#include <QTimer>
#include "StelDialog.hpp"

class Ui_fieldOfViewDialogForm;

class FieldOfViewDialog : public StelDialog
{
	Q_OBJECT
public:
	FieldOfViewDialog(QObject* parent);
	~FieldOfViewDialog() Q_DECL_OVERRIDE;
	double getFieldOfView();
	bool makeValidAndApply(int d, int m, int s);
	bool applyFieldOfView(double fov);
public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;
	virtual void close() Q_DECL_OVERRIDE;	
	void setFieldOfView(double newFOV);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent() Q_DECL_OVERRIDE;
	void connectSpinnerEvents() const;
	void disconnectSpinnerEvents()const;

private slots:
	//! degree slider or dial changed
	void degreeChanged(int nd);
	//! minute slider or dial changed
	void minuteChanged(int nm);
	//! second slider or dial changed
	void secondChanged(int ns);
	//! set FOV limit when projection type is changed
	void setFieldOfViewLimit(QString);

private:
	StelCore* core;	
	Ui_fieldOfViewDialogForm* ui;
	int degree;
	int minute;
	int second;
	void pushToWidgets();
};

#endif // FIELDOFVIEWDIALOG_HPP
