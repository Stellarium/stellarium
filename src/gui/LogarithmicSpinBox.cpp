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

#include "LogarithmicSpinBox.hpp"
#include <cmath>

LogarithmicSpinBox::LogarithmicSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
}

void LogarithmicSpinBox::stepBy(const int steps)
{
    auto value = this->value();

    const auto numIterations = std::abs(steps);
    // Doing it one-by-one, to prevent overstepping the step change boundary
    for(int n=0; n < numIterations; ++n)
    {
        const double minOrder = -decimals();
        const auto order = std::max(minOrder, std::floor(std::log10(value)) - 1);
        const auto step = std::pow(10., order);
        value = steps>0 ? value+step : value-step;
    }

    setValue(value);
}
