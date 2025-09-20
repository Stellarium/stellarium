/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCM_TYPES_LINE_HPP
#define SCM_TYPES_LINE_HPP

#include "CoordinateLine.hpp"
#include "StarLine.hpp"
#include <vector>

namespace scm
{
//! The lines of the current drawn constellation
struct Lines
{
	//! The coordinate pairs of a line.
	std::vector<CoordinateLine> coordinates;

	//! The optional available stars to the coordinates.
	std::vector<StarLine> stars;
};
} // namespace scm

#endif
