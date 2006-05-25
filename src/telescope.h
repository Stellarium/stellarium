/*
 * Author and Copyright of this file and of the stellarium telescope feature:
 * Johannes Gajdosik, 2006
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

#ifndef _TELESCOPE_H_
#define _TELESCOPE_H_

#include "stel_object.h"
#include "navigator.h"

class Telescope : public StelObject {
public:
  static Telescope *create(const string &url);
  virtual ~Telescope(void) {}
  const wstring &getNameI18n(void) const {return nameI18n;}
  wstring getInfoString(const Navigator * nav) const;
  wstring getShortInfoString(const Navigator * nav) const;
  STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_TELESCOPE;}
  Vec3d get_earth_equ_pos(const Navigator *nav) const
    {return nav->j2000_to_earth_equ(XYZ);}
  Vec3d getObsJ2000Pos(const Navigator *nav=0) const {return XYZ;}
  virtual void telescopeGoto(const Vec3d &j2000_pos) = 0;

    // all TCP (and all possible other style) communication shall be done in
    // these functions:
  virtual void prepareSelectFds(fd_set &read_fds,fd_set &write_fds,
                                int &fdmax) = 0;
  virtual void handleSelectFds(const fd_set &read_fds,
                               const fd_set &write_fds) {}
protected:
  Telescope(const string &name);
  Vec3d XYZ;  // j2000 position
  wstring nameI18n;
private:
  bool isInitialized(void) const {return true;}
private:
  float get_mag(const Navigator *nav) const {return -10.f;}
};


#endif
