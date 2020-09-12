/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com>
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

#ifndef TELESCOPECLIENTASCOMWIDGET_HPP
#define TELESCOPECLIENTASCOMWIDGET_HPP

#include <QWidget>
#include "ASCOMDevice.hpp"


namespace Ui {
class TelescopeClientASCOMWidget;
}

class TelescopeClientASCOMWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TelescopeClientASCOMWidget(QWidget *parent = Q_NULLPTR);
	~TelescopeClientASCOMWidget();
	QString selectedDevice() const;
	void setSelectedDevice(const QString& device);
	bool useDeviceEqCoordType() const;
	void setUseDeviceEqCoordType(const bool& useDevice);

private slots:
	void onChooseButtonClicked();
	void retranslate();

private:
	Ui::TelescopeClientASCOMWidget *ui;
	ASCOMDevice *ascom;
	QString mSelectedDevice = Q_NULLPTR;
};

#endif // TELESCOPECLIENTASCOMUI_HPP
