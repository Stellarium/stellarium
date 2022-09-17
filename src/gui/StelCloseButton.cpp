/*
 * Stellarium
 * Copyright (C) 2022 Ruslan Kabatsayev
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

#include "StelCloseButton.hpp"


StelCloseButton::StelCloseButton(QWidget* parent)
	: QPushButton(parent)
	, iconNormal(":/graphicGui/uieCloseButton.png")
	, iconHover(":/graphicGui/uieCloseButton-hover.png")
{
	// Qt 5.15 regression: QBushButton without borders does not respond to clicks
	setStyleSheet("border: 1px solid rgba(0, 0, 0, 0); background: none;");
	setIcon(iconNormal);
}

void StelCloseButton::enterEvent(EnterEvent*)
{
	setIcon(iconHover);
}

void StelCloseButton::leaveEvent(QEvent*)
{
	setIcon(iconNormal);
}
