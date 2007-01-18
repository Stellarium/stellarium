/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Author of this file: 2007 Johannes Gajdosik
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _EQUAL_AREA_PROJECTOR_HPP_
#define _EQUAL_AREA_PROJECTOR_HPP_

// Lambert azimuthal equal area projection

#include "CustomProjector.hpp"

class EqualAreaProjector : public CustomProjector {
public:
  EqualAreaProjector(const Vec4i & viewport, double _fov);
private:
  PROJECTOR_TYPE getType(void) const {return EQUAL_AREA_PROJECTOR;}
  bool project_custom(const Vec3d &v, Vec3d &win, const Mat4d &mat) const;
  void unproject(double x, double y, const Mat4d& m, Vec3d& v) const;
};


#endif
