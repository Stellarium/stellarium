/*
 * Copyright (C) 2015 Kirill Snezhko
 * Copyright (C) 2018 Alexander Wolf
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

#ifndef OBSERVABILITYWINDOW_HPP
#define OBSERVABILITYWINDOW_HPP

#include "StelDialog.hpp"

class Ui_observabilityWindowForm;

class ObservabilityWindow : public StelDialog
{
    Q_OBJECT

public:
    ObservabilityWindow();
    ~ObservabilityWindow();

public slots:
    void retranslate();

protected:
    void createDialogContent();

private:
    Ui_dynamicPluginTemplateWindowForm *ui;
    DynamicPluginTemplate *dynamicPluginTemplate;

};

#endif // OBSERVABILITYWINDOW_HPP