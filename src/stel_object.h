/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _STEL_OBJECT_H_
#define _STEL_OBJECT_H_

#include "vecmath.h"
#include "stel_object_type.h"

#include <iostream>

using namespace std;

class Navigator;
class Projector;
class STexture;
class StelObjectBase;

class StelObject {
public:
  StelObject(void);
  ~StelObject(void);
  StelObject(StelObjectBase *r);
  StelObject(const StelObject &o);
  const StelObject &operator=(const StelObject &o);
  operator bool(void) const;
  bool operator==(const StelObject &o) const;

  void update(void);
  void drawPointer(int delta_time,
                    const Projector *prj,
                    const Navigator *nav);

  //! Write I18n information about the object in wstring. 
  wstring getInfoString(const Navigator *nav) const;

  //! The returned wstring can typically be used for object labeling in the sky
  wstring getShortInfoString(const Navigator *nav) const;

  //! Return object's type
  STEL_OBJECT_TYPE get_type(void) const;

  //! Return object's name
  string getEnglishName(void) const;
  wstring getNameI18n(void) const;

  //! Get position in earth equatorial frame
  Vec3d get_earth_equ_pos(const Navigator *nav) const;

  //! observer centered J2000 coordinates
  Vec3d getObsJ2000Pos(const Navigator *nav) const;

  //! Return object's magnitude
  float get_mag(const Navigator *nav) const;

  //! Get object main color, used to display infos
  Vec3f get_RGB(void) const;

  StelObject getBrightestStarInConstellation(void) const;

    // only needed for AutoZoomIn/Out, whatever this is:
  //! Return the best FOV in degree to use for a close view of the object
  double get_close_fov(const Navigator *nav) const;
  //! Return the best FOV in degree to use for a global view
  //! of the object satellite system (if there are satellites)
  double get_satellites_fov(const Navigator *nav) const;
  double get_parent_satellites_fov(const Navigator *nav) const;

  static void init_textures(void);
  static void delete_textures(void);
private:
  StelObjectBase *rep;
};

#endif
