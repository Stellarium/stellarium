/*
 * Stellarium
 * Copyright (C) 2023 Alexander Wolf
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

#ifndef SPECIFIC_TIME_WIDGET
#define SPECIFIC_TIME_WIDGET

#include <memory>
#include <QWidget>
#include "ui_specificTimeWidget.h"

class SpecificTimeWidget : public QWidget
{
    Q_OBJECT
public:
    SpecificTimeWidget(QWidget* parent = nullptr);
    void setup();
    void retranslate();

private slots:
    void setSeasonLabels();
    void setSeasonTimes();
    void setTodayTimes();

private:
    class StelCore* core;
    class SpecificTimeMgr* specMgr;
    class StelLocaleMgr* localeMgr;

    void populateData();

    std::unique_ptr<Ui_specificTimeWidget> ui;
};

#endif
