/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#ifndef _ADDONWIDGET_HPP
#define _ADDONWIDGET_HPP

#include <QListWidgetItem>
#include <QWidget>

#include "StelAddOnMgr.hpp"
#include "StelApp.hpp"

class Ui_AddOnWidget;

class AddOnWidget : public QWidget
{
Q_OBJECT
public:
	AddOnWidget(QWidget* parent, int row);
	virtual ~AddOnWidget();

	void init(int addonId);
	QStringList getTexturesToInstall() { return m_sSelectedTexturesToInstall; }
	QStringList getTexturesToRemove() { return m_sSelectedTexturesToRemove; }

	void paintEvent(QPaintEvent*);

signals:
	void checkRow(int pRow, int checked);

private slots:
	void slotItemChanged(QListWidgetItem* item);
	void slotCheckAllTextures(int pRow, bool checked);

private:
	const int m_iRow;
	Ui_AddOnWidget* ui;
	StelAddOnDAO* m_pStelAddOnDAO;
	QString m_sThumbnailDir;
	QStringList m_sSelectedTexturesToInstall;
	QStringList m_sSelectedTexturesToRemove;
};

#endif // _ADDONWIDGET_HPP
