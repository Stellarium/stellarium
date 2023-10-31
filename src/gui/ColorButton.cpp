/*
 * Stellarium
 * Copyright (C) 2023 Ruslan Kabatsayev
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

#include "ColorButton.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelPropertyMgr.hpp"
#include <QSettings>
#include <QColorDialog>

ColorButton::ColorButton(QWidget* parent)
	: QToolButton(parent)
{
}

void ColorButton::setup(const QString &propertyName, const QString &iniName, const QString &moduleName)
{
	propName_ = propertyName;
	iniName_ = iniName;
	moduleName_ = moduleName;
	const auto prop = StelApp::getInstance().getStelPropertyManager()->getProperty(propertyName);
	const auto color = prop->getValue().value<Vec3f>().toQColor();
	setStyleSheet("ColorButton { background-color:" + color.name() + "; }");
	connect(this, &ColorButton::clicked, this, &ColorButton::askColor);
}

void ColorButton::askColor()
{
	if(propName_.isEmpty() || iniName_.isEmpty())
	{
		qWarning() << "ColorButton not set up properly! Ignoring.";
		Q_ASSERT(0);
		return;
	}

	const auto& propMan = *StelApp::getInstance().getStelPropertyManager();
	const QColor initColor = propMan.getProperty(propName_)->getValue().value<Vec3f>().toQColor();

	const QColor newColor = QColorDialog::getColor(initColor, &StelMainView::getInstance(), toolTip());
	if(!newColor.isValid()) return;

	const Vec3d newVColor(newColor.redF(), newColor.greenF(), newColor.blueF());
	propMan.setStelPropertyValue(propName_, QVariant::fromValue(newVColor.toVec3f()));
	if (moduleName_.isEmpty())
	{
		StelApp::getInstance().getSettings()->setValue(iniName_, newVColor.toStr());
	}
	else
	{
		const auto module = StelApp::getInstance().getModuleMgr().getModule(moduleName_);
		const auto settings = module->getSettings();
		Q_ASSERT(settings);
		settings->setValue(iniName_, newVColor.toStr());
	}
	setStyleSheet("ColorButton { background-color:" + newColor.name() + "; }");
}
