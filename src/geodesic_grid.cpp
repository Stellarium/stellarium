/*

GeodesicGrid: a library for dividing the sphere into triangle zones
by subdividing the icosahedron

Author and Copyright: Johannes Gajdosik, 2006

This library requires a simple Vector library,
which may have different copyright and license,
for example Vec3d from vecmath.h.

In the moment I choose to distribute the library under the GPL,
later I may choose to additionally distribute it under a more
relaxed license like the LGPL. If you want to have the library
under another license, please ask me.



This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "geodesic_grid.h"

#include <math.h>
#include <stdlib.h>

#include <iostream>

using namespace std;

static const double icosahedron_G = 0.5*(1.0+sqrt(5.0));
static const double icosahedron_b = 1.0/sqrt(1.0+icosahedron_G*icosahedron_G);
static const double icosahedron_a = icosahedron_b*icosahedron_G;

static const Vec3d icosahedron_corners[12] = {
  Vec3d( icosahedron_a, -icosahedron_b,            0.0),
  Vec3d( icosahedron_a,  icosahedron_b,            0.0),
  Vec3d(-icosahedron_a,  icosahedron_b,            0.0),
  Vec3d(-icosahedron_a, -icosahedron_b,            0.0),
  Vec3d(           0.0,  icosahedron_a, -icosahedron_b),
  Vec3d(           0.0,  icosahedron_a,  icosahedron_b),
  Vec3d(           0.0, -icosahedron_a,  icosahedron_b),
  Vec3d(           0.0, -icosahedron_a, -icosahedron_b),
  Vec3d(-icosahedron_b,            0.0,  icosahedron_a),
  Vec3d( icosahedron_b,            0.0,  icosahedron_a),
  Vec3d( icosahedron_b,            0.0, -icosahedron_a),
  Vec3d(-icosahedron_b,            0.0, -icosahedron_a)
};

struct TopLevelTriangle {
  int corners[3];   // index der Ecken
};

static const TopLevelTriangle icosahedron_triangles[20] = {
  { 0, 1, 9},
  { 1, 0,10},
  { 2, 3, 8},
  { 3, 2,11},
  { 4, 5, 1},
  { 5, 4, 2},
  { 6, 7, 0},
  { 7, 6, 3},
  { 8, 9, 5},
  { 9, 8, 6},
  {10,11, 4},
  {11,10, 7},
  { 0, 9, 6},
  { 5, 9, 1},
  { 6, 8, 3},
  { 2, 8, 5},
  { 0, 7,10},
  { 4, 1,10},
  { 7, 3,11},
  { 2, 4,11}
};








GeodesicGrid::GeodesicGrid(const int lev)
             :max_level(lev<0?0:lev) {
  if (max_level > 0) {
    triangles = new Triangle*[max_level];
    int nr_of_triangles = 20;
    for (int i=0;i<max_level;i++) {
      triangles[i] = new Triangle[nr_of_triangles];
      nr_of_triangles *= 4;
    }
    for (int i=0;i<20;i++) {
      const int *const corners = icosahedron_triangles[i].corners;
      initTriangle(0,i,
                   icosahedron_corners[corners[0]],
                   icosahedron_corners[corners[1]],
                   icosahedron_corners[corners[2]]);
    }
  } else {
    triangles = 0;
  }
}

GeodesicGrid::~GeodesicGrid(void) {
  if (max_level > 0) {
    for (int i=max_level-1;i>=0;i--) delete[] triangles[i];
    delete triangles;
  }
}

void GeodesicGrid::initTriangle(int lev,int index,
                                const Vec3d &c0,
                                const Vec3d &c1,
                                const Vec3d &c2) {
  Triangle &t(triangles[lev][index]);
  t.e0 = c1+c2;
  t.e0.normalize();
  t.e1 = c2+c0;
  t.e1.normalize();
  t.e2 = c0+c1;
  t.e2.normalize();
  lev++;
  if (lev < max_level) {
    index *= 4;
    initTriangle(lev,index+0,c0,t.e2,t.e1);
    initTriangle(lev,index+1,c1,t.e0,t.e2);
    initTriangle(lev,index+2,c2,t.e1,t.e0);
    initTriangle(lev,index+3,t.e0,t.e1,t.e2);
  }
}


void GeodesicGrid::visitTriangles(int max_visit_level,
                                  VisitFunc *func,
                                  void *context) const {
  if (func && max_visit_level >= 0) {
    if (max_visit_level > max_level) max_visit_level = max_level;
    for (int i=0;i<20;i++) {
      const int *const corners = icosahedron_triangles[i].corners;
      visitTriangles(0,i,
                     icosahedron_corners[corners[0]],
                     icosahedron_corners[corners[1]],
                     icosahedron_corners[corners[2]],
                     max_visit_level,func,context);
    }
  }
}

void GeodesicGrid::visitTriangles(int lev,int index,
                                  const Vec3d &c0,
                                  const Vec3d &c1,
                                  const Vec3d &c2,
                                  int max_visit_level,
                                  VisitFunc *func,
                                  void *context) const {
  (*func)(lev,index,c0,c1,c2,context);
  Triangle &t(triangles[lev][index]);
  lev++;
  if (lev <= max_visit_level) {
    index *= 4;
    visitTriangles(lev,index+0,c0,t.e2,t.e1,max_visit_level,func,context);
    visitTriangles(lev,index+1,c1,t.e0,t.e2,max_visit_level,func,context);
    visitTriangles(lev,index+2,c2,t.e1,t.e0,max_visit_level,func,context);
    visitTriangles(lev,index+3,t.e0,t.e1,t.e2,max_visit_level,func,context);
  }
}


int GeodesicGrid::searchZone(const Vec3d &v,int search_level) const {
  for (int i=0;i<20;i++) {
    const int *const corners = icosahedron_triangles[i].corners;
    const Vec3d &c0(icosahedron_corners[corners[0]]);
    const Vec3d &c1(icosahedron_corners[corners[1]]);
    const Vec3d &c2(icosahedron_corners[corners[2]]);
    if (((c0^c1)*v >= 0.0) && ((c1^c2)*v >= 0.0) && ((c2^c0)*v >= 0.0)) {
        // v lies inside this icosahedron triangle
      for (int lev=0;lev<search_level;lev++) {
        Triangle &t(triangles[lev][i]);
        i <<= 2;
        if ((t.e1^t.e2)*v <= 0.0) {
          // i += 0;
        } else if ((t.e2^t.e0)*v <= 0.0) {
          i += 1;
        } else if ((t.e0^t.e1)*v <= 0.0) {
          i += 2;
        } else {
          i += 3;
        }
      }
      return i;
    }
  }
  cerr << "software error: not found" << endl;
  exit(-1);
    // shut up the compiler
  return -1;
}

void GeodesicGrid::searchZones(const HalfSpace *half_spaces,
                               const int nr_of_half_spaces,
                               int **inside_list,int **border_list,
                               int max_search_level) const {
  if (max_search_level < 0) max_search_level = 0;
  else if (max_search_level > max_level) max_search_level = max_level;
  int halfs_used[nr_of_half_spaces];
  for (int h=0;h<nr_of_half_spaces;h++) {halfs_used[h] = h;}
  bool corner_inside[12][nr_of_half_spaces];
  for (int h=0;h<nr_of_half_spaces;h++) {
    const HalfSpace &half_space(half_spaces[h]);
    for (int i=0;i<12;i++) {
      corner_inside[i][h] = half_space.inside(icosahedron_corners[i]);
    }
  }
  for (int i=0;i<20;i++) {
    searchZones(0,i,
                half_spaces,nr_of_half_spaces,halfs_used,nr_of_half_spaces,
                corner_inside[icosahedron_triangles[i].corners[0]],
                corner_inside[icosahedron_triangles[i].corners[1]],
                corner_inside[icosahedron_triangles[i].corners[2]],
                inside_list,border_list,max_search_level);
  }
}

void GeodesicGrid::searchZones(int lev,int index,
                               const HalfSpace *half_spaces,
                               const int nr_of_half_spaces,
                               const int *index_of_used_half_spaces,
                               const int half_spaces_used,
                               const bool *corner0_inside,
                               const bool *corner1_inside,
                               const bool *corner2_inside,
                               int **inside_list,int **border_list,
                               const int max_search_level) const {
  int halfs_used[half_spaces_used];
  int halfs_used_count = 0;
  for (int h=0;h<half_spaces_used;h++) {
    const int i = index_of_used_half_spaces[h];
    if (!corner0_inside[i] && !corner1_inside[i] && !corner2_inside[i]) {
        // totally outside this HalfSpace
      return;
    } else if (corner0_inside[i] && corner1_inside[i] && corner2_inside[i]) {
        // totally inside this HalfSpace
    } else {
        // on the border of this HalfSpace
      halfs_used[halfs_used_count++] = i;
    }
  }
  if (halfs_used_count == 0) {
      // this triangle(lev,index) lies inside all halfspaces
    **inside_list = index;
    (*inside_list)++;
  } else {
    (*border_list)--;
    **border_list = index;
    if (lev < max_search_level) {
      Triangle &t(triangles[lev][index]);
      lev++;
      index <<= 2;
      inside_list++;
      border_list++;
      bool edge0_inside[nr_of_half_spaces];
      bool edge1_inside[nr_of_half_spaces];
      bool edge2_inside[nr_of_half_spaces];
      for (int h=0;h<halfs_used_count;h++) {
        const int i = halfs_used[h];
        const HalfSpace &half_space(half_spaces[i]);
        edge0_inside[i] = half_space.inside(t.e0);
        edge1_inside[i] = half_space.inside(t.e1);
        edge2_inside[i] = half_space.inside(t.e2);
      }
      searchZones(lev,index+0,
                  half_spaces,nr_of_half_spaces,halfs_used,halfs_used_count,
                  corner0_inside,edge2_inside,edge1_inside,
                  inside_list,border_list,max_search_level);
      searchZones(lev,index+1,
                  half_spaces,nr_of_half_spaces,halfs_used,halfs_used_count,
                  corner1_inside,edge0_inside,edge2_inside,
                  inside_list,border_list,max_search_level);
      searchZones(lev,index+2,
                  half_spaces,nr_of_half_spaces,halfs_used,halfs_used_count,
                  corner2_inside,edge1_inside,edge0_inside,
                  inside_list,border_list,max_search_level);
      searchZones(lev,index+3,
                  half_spaces,nr_of_half_spaces,halfs_used,halfs_used_count,
                  edge0_inside,edge1_inside,edge2_inside,
                  inside_list,border_list,max_search_level);
    }
  }
}



GeodesicSearchResult::GeodesicSearchResult(const GeodesicGrid &grid)
                     :grid(grid),
                      zones(new int*[grid.getMaxLevel()+1]),
                      inside(new int*[grid.getMaxLevel()+1]),
                      border(new int*[grid.getMaxLevel()+1]) {
  for (int i=0;i<=grid.getMaxLevel();i++) {
    zones[i] = new int[GeodesicGrid::nrOfZones(i)];
  }
}

GeodesicSearchResult::~GeodesicSearchResult(void) {
  for (int i=grid.getMaxLevel();i>=0;i--) {
    delete zones[i];
  }
  delete border;
  delete inside;
  delete zones;
}

void GeodesicSearchResult::search(const HalfSpace *half_spaces,
                                  const int nr_of_half_spaces,
                                  int max_search_level) {
  for (int i=grid.getMaxLevel();i>=0;i--) {
    inside[i] = zones[i];
    border[i] = zones[i]+GeodesicGrid::nrOfZones(i);
  }
  grid.searchZones(half_spaces,nr_of_half_spaces,inside,border,max_search_level);
}

void GeodesicSearchResult::search(const Vec3d &e0,const Vec3d &e1,
                                  const Vec3d &e2,const Vec3d &e3,
                                  int max_search_level) {
  HalfSpace half_spaces[4];
  half_spaces[0].d = e0*((e1-e0)^(e2-e0));
  if (half_spaces[0].d > 0) {
    half_spaces[0].n = e0^e1;
    half_spaces[1].n = e1^e2;
    half_spaces[2].n = e2^e3;
    half_spaces[3].n = e3^e0;
    half_spaces[0].d = 0.0;
    half_spaces[1].d = 0.0;
    half_spaces[2].d = 0.0;
    half_spaces[3].d = 0.0;
    search(half_spaces,4,max_search_level);
  } else {
    half_spaces[0].n = (e1-e0)^(e2-e0);
    search(half_spaces,1,max_search_level);
  }
}



void GeodesicSearchInsideIterator::reset(void) {
  level = 0;
  max_count = 1<<(max_level<<1); // 4^max_level
  index_p = r.zones[0];
  end_p = r.inside[0];
  index = (*index_p) * max_count;
  count = (index_p < end_p) ? 0 : max_count;
}

int GeodesicSearchInsideIterator::next(void) {
  if (count < max_count) return index+(count++);
  index_p++;
  if (index_p < end_p) {
    index = (*index_p) * max_count;
    count = 1;
    return index;
  }
  while (level < max_level) {
    level++;
    max_count >>= 2;
    index_p = r.zones[level];
    end_p = r.inside[level];
    if (index_p < end_p) {
      index = (*index_p) * max_count;
      count = 1;
      return index;
    }
  }
  return -1;
}


  // global variables: very ugly
GeodesicGrid *geodesic_grid = 0;
GeodesicSearchResult *geodesic_search_result = 0;

