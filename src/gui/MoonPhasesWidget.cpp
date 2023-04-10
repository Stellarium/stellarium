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

#include "MoonPhasesWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "StelLocaleMgr.hpp"
#include "SolarSystem.hpp"

#include <QPushButton>
#include <QToolTip>
#include <QSettings>

MoonPhasesWidget::MoonPhasesWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui_moonPhasesWidget)
{
}

void MoonPhasesWidget::setup()
{
	ui->setupUi(this);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &MoonPhasesWidget::retranslate);

	core = StelApp::getInstance().getCore();
	localeMgr = &StelApp::getInstance().getLocaleMgr();
}

void MoonPhasesWidget::retranslate()
{
	ui->retranslateUi(this);
}

void MoonPhasesWidget::setMoonPhases()
{
}

