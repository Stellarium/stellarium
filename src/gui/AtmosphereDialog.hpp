/*
 * Stellarium
 * 
 * Copyright (C) 2011 Georg Zotti (Refraction/Extinction feature)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/


// GZ: Methods copied largely from AddRemoveLandscapesDialog

#ifndef ATMOSPHEREDIALOG_HPP
#define ATMOSPHEREDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"
#include "RefractionExtinction.hpp"

class Ui_atmosphereDialogForm;

class AtmosphereDialog : public StelDialog
{
	Q_OBJECT

public:
	AtmosphereDialog();
	virtual ~AtmosphereDialog();

public slots:
        void retranslate();

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
        virtual void createDialogContent();
        Ui_atmosphereDialogForm *ui;

private:
        Refraction *refraction;
        Extinction *extinction;
};

#endif // ATMOSPHEREDIALOG_HPP
