/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#ifndef _NEBULA_MGR_H_
#define _NEBULA_MGR_H_

#include <vector>
#include "nebula.h"
#include "s_font.h"

using namespace std;

class Nebula_mgr  
{
 public:
  Nebula_mgr(Vec3f defaultfontcolor = Vec3f(0.4,0.3,0.5), Vec3f defaultcirclecolor = Vec3f(0.8,0.8,0.1));
  virtual ~Nebula_mgr();

  // Read the Nebulas data from a file
  int read(const string& font_fileName, const string& fileName, int barx, int bary);
  
  // Draw all the Nebulas
  void draw(int hints_ON, Projector* prj, const navigator * nav, tone_reproductor* eye,
	    bool _gravity_label, float max_mag_name, bool bright_nebulae);
  
  stel_object * search(string name);  // search by name
  stel_object * search(Vec3f Pos);    // Search the Nebulae by position
  
  // Return a stl vector containing the nebulas located inside the lim_fov circle around position v
  vector<stel_object*> search_around(Vec3d v, double lim_fov);
  
 private:
  void read_one();  // load next nebula from file
  FILE * nebula_fic;
  int total;   // total number of nebulas
  int loaded;  // number loaded so far
  vector<Nebula*> neb_array;	// The nebulas list
  Vec3f defaultfontcolor;
  Vec3f defaultcirclecolor;
};

#endif // _NEBULA_MGR_H_
