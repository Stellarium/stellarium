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

#ifndef SCM_TYPES_DRAWING_HPP
#define SCM_TYPES_DRAWING_HPP

namespace scm
{
//! The possibles states during the drawing.
enum class Drawing
{
	//! No line is available.
	None = 1,

	//! The line as a starting point.
	hasStart = 2,

	//! The line has a not placed end that is attached to the cursor.
	hasFloatingEnd = 4,

	//! The line is complete i.e. has start and end point.
	hasEnd = 8,

	//! The end is an already existing point.
	hasEndExistingPoint = 16,
};
} // namespace scm

#endif
