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

#ifndef EXTERNAL_STEP_SPINBOX_HPP
#define EXTERNAL_STEP_SPINBOX_HPP

#include <QSpinBox>

/*!
 * A spinbox whose stepBy commands are emitted as signals instead of changing the value.
 */
class ExternalStepSpinBox : public QSpinBox
{
	Q_OBJECT

public:
	ExternalStepSpinBox(QWidget*const parent = nullptr)
		: QSpinBox(parent)
	{
		setWrapping(true); // enable step buttons even when at limits
	}
	void stepBy(const int steps) override
	{
		emit stepsRequested(steps);
	}

signals:
	void stepsRequested(int steps);
};

#endif
