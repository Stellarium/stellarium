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

#ifndef STEL_COLOR_BUTTON_HPP
#define STEL_COLOR_BUTTON_HPP

#include <QToolButton>

class ColorButton : public QToolButton
{
	Q_OBJECT
public:
	ColorButton(QWidget* parent = nullptr);
	//! Prepare this button so that it can work properly with the StelProperty and config entry it represents.
	//! @param propertyName a StelProperty name which must represent a color (coded as Vec3f)
	//! @param iniName the associated entry for config.ini, in the form
	//! group/name. Usually "color/some_feature_name_color".
	//! @param moduleName if the iniName is for a module (plugin)-specific ini file, add the module name
	//! here. The module needs an implementation of getSettings()
	//! @warning If the action with \c propName is invalid/unregistered, or cannot be converted
	//! to the required datatype, the application will crash
	void setup(const QString& propertyName, const QString& iniName, const QString& moduleName = "");
private:
	void askColor();

	QString propName_;
	QString iniName_;
	QString moduleName_;
};

#endif
