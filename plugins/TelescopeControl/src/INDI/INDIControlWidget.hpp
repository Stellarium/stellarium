/*
 * Copyright (C) 2017 Alessandro Siniscalchi <asiniscalchi@gmail.com>
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

#ifndef INDICONTROLWIDGET_HPP
#define INDICONTROLWIDGET_HPP

#include <QWidget>

#include "TelescopeClient.hpp"

namespace Ui {
class INDIControlWidget;
}

class INDIControlWidget : public QWidget
{
    Q_OBJECT

public:
	explicit INDIControlWidget(QSharedPointer<TelescopeClient> telescope, QWidget *parent = 0);
    ~INDIControlWidget();

private slots:
	void onNorthButtonPressed();
	void onNorthButtonReleased();
	void onEastButtonPressed();
	void onEastButtonReleased();
	void onSouthButtonPressed();
	void onSouthButtonReleased();
	void onWestButtonPressed();
	void onWestButtonReleased();

private:
	double speed() const;

    Ui::INDIControlWidget *ui;
	QSharedPointer<TelescopeClient> mTelescope;
};

#endif // INDICONTROLWIDGET_HPP
