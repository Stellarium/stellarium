/*
 * Stellarium Observability Plug-in GUI
 * 
 * Copyright (C) 2012 Ivan Marti-Vidal
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
 
#ifndef OBSERVABILITYDIALOG_HPP
#define OBSERVABILITYDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"
#include "Observability.hpp"

class Ui_ObservabilityDialog;

//! Configuration window of the %Observability Analysis plug-in.
//! @see Observability
//! @ingroup observability
class ObservabilityDialog : public StelDialog
{
	Q_OBJECT

public:
	ObservabilityDialog();
	~ObservabilityDialog();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent();

public slots:
	void retranslate();

private slots:
	//! Read the color values from the sliders and set the font color.
	void setColor();
	
	void updateAltitudeLabel(int altitude);
	void updateHorizonLabel(int horizon);
	//! Updates the displayed values of controls from the plug-in.
	//! Called when the configuration is changed.
	void updateControls();
	void restoreDefaults();

private:
	Ui_ObservabilityDialog* ui;
	void setAboutHtml(void);
};

#endif // OBSERVABILITYDIALOG_HPP
