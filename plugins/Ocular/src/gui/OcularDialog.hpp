/*
 * Copyright (C) 2009 Timothy Reaves
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

#ifndef _OCULARDIALOG_HPP_
#define _OCULARDIALOG_HPP_

#include <QObject>
#include "StelDialogOculars.hpp"
#include "Ocular.hpp"
#include "StelStyle.hpp"

#include <QSqlRecord>

class Ui_ocularDialogForm;

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
class QDoubleValidator;
class QIntValidator;
class QRegExpValidator;
class QModelIndex;
class QSqlRecord;
class QSqlTableModel;
QT_END_NAMESPACE


class OcularDialog : public StelDialogOculars
{
	Q_OBJECT

public:
	OcularDialog(QSqlTableModel *ocularsTableModel, QSqlTableModel *telescopesTableModel);
	virtual ~OcularDialog();
	void languageChanged();
	virtual void setStelStyle(const StelStyle& style);

	//! Notify that the application style changed
	void styleChanged();
	void setOculars(QList<Ocular*> theOculars);

public slots:
	void closeWindow();
	void deleteSelectedOcular();
	void deleteSelectedTelescope();
	void insertNewOcular();
	void insertNewTelescope();
	void ocularSelected(const QModelIndex &index);
	void telescopeSelected(const QModelIndex &index);
	void updateOcular();
	void updateTelescope();

signals:
	void scaleImageCircleChanged(bool state);
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_ocularDialogForm* ui;

private slots:
	void scaleImageCircleStateChanged(int state);

private:
	QDataWidgetMapper *ocularMapper;
	QSqlTableModel *ocularsTableModel;
	QDataWidgetMapper *telescopeMapper;
	QSqlTableModel *telescopesTableModel;
	QIntValidator *validatorOcularAFOV;
	QDoubleValidator *validatorOcularEFL;
	QDoubleValidator *validatorTelescopeDiameter;
	QDoubleValidator *validatorTelescopeFL;
	QRegExpValidator *validatorName;

};

#endif // _OCULARDIALOG_HPP_
