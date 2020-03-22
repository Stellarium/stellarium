/*
 * Copyright (C) 2010 Pep Pujols
 * Copyright (C) 2020 Georg Zotti
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

#ifndef PRINTSKYDIALOG_HPP
#define PRINTSKYDIALOG_HPP

#include <QObject>
//#include "StelDialogPrintSky.hpp"
#include "StelDialog.hpp"
#include "StelStyle.hpp"
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QPrintPreviewDialog>
#include "StelGui.hpp"

class Ui_printskyDialogForm;


class PrintSkyDialog : public StelDialog // PrintSky
{
	Q_OBJECT

public:
	PrintSkyDialog();
	virtual ~PrintSkyDialog();
	virtual void retranslate();
	void updateStyle();

	//! Notify that the application style changed
	void styleChanged();
	void enableOutputOptions(bool enable);

public slots:
	void closeWindow();

	//! Print report on a preview window
	void previewSky();
	//! Print report direct to printer
	void printSky();
	//! Read the printer parameters and run the output option selected (Print/Preview)
	void executePrinterOutputOption();


protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_printskyDialogForm* ui;

private slots:
	//! Draw contents report
	//! @param printer the paint device to paint on a print
	void printDataSky(QPrinter * printer);

private:
	QFont font;
	StelGui* gui; // main StelGui
	bool stelGuiVisible;

	//! Printing options
	bool outputPreview;
	QString printableTime(double time, double shift);
	QList< QPair<float, float> > getListMagnitudeRadius(StelCore *core);


};

#endif // PRINTSKYDIALOG_HPP
