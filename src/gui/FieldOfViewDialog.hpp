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
#include "StelDialog.hpp"

class Ui_fieldOfViewDialogForm;

class FieldOfViewDialog : public StelDialog
{
	Q_OBJECT
public:
	FieldOfViewDialog(QObject* parent);
	~FieldOfViewDialog() override;
	double getFieldOfView();
	bool makeValidAndApply(int d, int m, int s);
	bool applyFieldOfView(double fov);

public slots:
	void retranslate() override;
	void close() override;
	void setFieldOfView(double newFOV);

protected:
	Ui_fieldOfViewDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;
	void connectSpinnerEvents() const;
	void disconnectSpinnerEvents() const;

private slots:
	//! degree slider or dial changed
	void degreeChanged(int nd);
	//! minute slider or dial changed
	void minuteChanged(int nm);
	//! second slider or dial changed
	void secondChanged(int ns);
	//! decimal degree slider or dial changed
	void fovChanged(double nd);
	//! set FOV limit when projection type is changed
	void setFieldOfViewLimit(QString);

private:
	StelCore* core;	

	int degree;
	int minute;
	int second;
	double fov;

	void pushToWidgets();
};

#endif // FIELDOFVIEWDIALOG_HPP
