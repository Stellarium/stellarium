/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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
 
#ifndef _VIEWDIALOG_HPP_
#define _VIEWDIALOG_HPP_

#include <QObject>

class Ui_viewDialogForm;

class ViewDialog : public QObject
{
Q_OBJECT
public:
	ViewDialog(QObject* parent);
	virtual ~ViewDialog();
public slots:
	void setVisible(bool);
	void close();
signals:
	void closed();
protected:
	QWidget* dialog;
	Ui_viewDialogForm* ui;
private slots:
	void skyCultureChanged(const QString& cultureName);
	void projectionChanged(const QString& projectionName);
	void landscapeChanged(const QString& landscapeName);
	void shoutingStarsZHRChanged();
private:
	void updateSkyCultureText();	
};

#endif // _VIEWDIALOG_HPP_
