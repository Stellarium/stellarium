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

#ifndef SCM_TYPES_SKY_POINT_HPP
#define SCM_TYPES_SKY_POINT_HPP

#include "VecMath.hpp"
#include <optional>
#include <QString>

namespace scm
{
//! A point in the sky with coordinates, optionally associated with a star ID.
struct SkyPoint
{
	//! The coordinate of the point.
	Vec3d coordinate = Vec3d(0, 0, 0);

	//! The optional star at that coordinate.
	QString star;

	//! Resets the SkyPoint to default values.
	void reset()
	{
		coordinate.set(0, 0, 0);
		star.clear();
	}
};
} // namespace scm

#endif
