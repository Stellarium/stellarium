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

#include <QString>
#include "StelObject.hpp"
#include "Navigator.hpp"
#if defined (_MSC_VER)
#include <winsock2.h>
#endif

#include <list>
#include <string>

using namespace std;

long long int GetNow(void);

#ifdef __MINGW32__
struct fd_set;
#endif

class Telescope : public StelObject {
public:
  static Telescope *create(const string &url);
  virtual ~Telescope(void) {}
  QString getEnglishName(void) const {return name;}
  QString getNameI18n(void) const {return nameI18n;}
  QString getInfoString(const Navigator * nav) const;
  QString getShortInfoString(const Navigator * nav) const;
  QString getType(void) const {return "Telescope";}
  Vec3d getEarthEquatorialPos(const Navigator *nav) const
    {return nav->j2000_to_earth_equ(getObsJ2000Pos(nav));}
  virtual void telescopeGoto(const Vec3d &j2000_pos) = 0;

  virtual bool isConnected(void) const = 0;
  virtual bool hasKnownPosition(void) const = 0;
    // all TCP (and all possible other style) communication shall be done in
    // these functions:
  virtual void prepareSelectFds(fd_set &read_fds,fd_set &write_fds,
                                int &fdmax) = 0;
  virtual void handleSelectFds(const fd_set &read_fds,
                               const fd_set &write_fds) {}
  void addOcular(double fov) {if (fov>=0.0) oculars.push_back(fov);}
  const std::list<double> &getOculars(void) const {return oculars;}
protected:
  Telescope(const string &name);
  QString nameI18n;
  const QString name;
private:
  bool isInitialized(void) const {return true;}
  float getSelectPriority(const Navigator *nav) const {return -10.f;}
private:
  std::list<double> oculars; // fov of the oculars
};


#endif
