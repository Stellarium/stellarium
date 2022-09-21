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

#ifndef STEL_CLOSE_BUTTON_HPP
#define STEL_CLOSE_BUTTON_HPP

#include <QPushButton>

class StelCloseButton : public QPushButton
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	using EnterEvent=QEvent;
#else
	using EnterEvent=QEnterEvent;
#endif
public:
	StelCloseButton(QWidget* parent = nullptr);

protected:
	void enterEvent(EnterEvent*) override;
	void leaveEvent(QEvent*) override;

private:
	QIcon iconNormal;
	QIcon iconHover;
};

#endif
