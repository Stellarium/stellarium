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


#ifndef ATMOSPHEREDIALOG_HPP
#define ATMOSPHEREDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"

class Ui_atmosphereDialogForm;

class AtmosphereDialog : public StelDialog
{
	Q_OBJECT

public:
	AtmosphereDialog();
	~AtmosphereDialog() override;

public slots:
	void retranslate() override;

protected:
        //! Initialize the dialog widgets and connect the signals/slots.
	void createDialogContent() override;
        Ui_atmosphereDialogForm *ui;

private:
	bool hasValidModelPath() const;
	void onModelChoiceChanged(const QString& model);
	void onPathToModelEditingFinished();
	void onPathToModelChanged();
	void updatePathToModelStyle();
	void onErrorStateChanged(bool error);
	void browsePathToModel();
	void setCurrentValues();
	void clearStatus();
	bool eventFilter(QObject* object, QEvent* event) override;

private slots:
	void setStandardAtmosphere();
	void setTfromK(double k);
};

#endif // ATMOSPHEREDIALOG_HPP
