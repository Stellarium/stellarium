/*
 * Copyright (C) 2010 Pep Pujols
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

#ifndef _PRINTSKYDIALOG_HPP_
#define _PRINTSKYDIALOG_HPP_

#include <QObject>
#include "StelDialogPrintSky.hpp"
#include "StelStyle.hpp"
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QPrintPreviewDialog>
#include "StelGui.hpp"

class Ui_printskyDialogForm;


class PrintSkyDialog : public StelDialogPrintSky
{
	Q_OBJECT

public:
	PrintSkyDialog();
	virtual ~PrintSkyDialog();
	void languageChanged();
	void updateStyle();

	//! Notify that the application style changed
	void styleChanged();
	void enableOutputOptions(bool enable);

public slots:
	void closeWindow();

	//! Print report on a preview window
	void previewSky();
	//! Read the printer parameters and run the output option selected (Print/Preview)
	void executePrinterOutputOption();
	//! Print report direct to printer
	void printSky();

signals:
	void invertColorsChanged(bool state);
	void scaleToFitChanged(bool state);
	void orientationChanged(bool state);
	void printDataChanged(bool state);
	void printSSEphemeridesChanged(bool state);

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_printskyDialogForm* ui;

private slots:
	void invertColorsStateChanged(int state);
	//! Draw contents report
	//! @param printer the paint device to paint on a print
	void printDataSky(QPrinter * printer);
	void scaleToFitStateChanged(int state);
	void orientationStateChanged(bool state);
	void printDataStateChanged(int state);
	void printSSEphemeridesStateChanged(int state);

private:
	QFont font;
	//Gui
	StelGui* gui;
	bool currentVisibilityGui;

	//! Printing options
	bool outputOption;
	bool invertColorsOption;
	bool scaleToFitOption;
	QString orientationOption;
	bool printDataOption;
	bool printSSEphemeridesOption;
	QString printableTime(double time, int shift);
	QList< QPair<float, float> > getListMagnitudeRadius(StelCore *core);


};

#endif // _OCULARDIALOG_HPP_
