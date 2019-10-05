// Author and Copyright: Johannes Gajdosik, 2006
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
//
// g++ -O2 MakeCombinedCatalogue.C -o MakeCombinedCatalogue


// change catalogue locations according to your needs:

const char *nomad_names[]={
  "/sdb1/nomad.sml/Nomad00.sml",
  "/sdb1/nomad.sml/Nomad01.sml",
  "/sdb1/nomad.sml/Nomad02.sml",
  "/sdb1/nomad.sml/Nomad03.sml",
  "/sdb1/nomad.sml/Nomad04.sml",
  "/sdb1/nomad.sml/Nomad05.sml",
  "/sdb1/nomad.sml/Nomad06.sml",
  "/sdb1/nomad.sml/Nomad07.sml",
  "/sdb1/nomad.sml/Nomad08.sml",
  "/sdb1/nomad.sml/Nomad09.sml",
  "/sdb1/nomad.sml/Nomad10.sml",
  "/sdb1/nomad.sml/Nomad11.sml",
  "/sdb1/nomad.sml/Nomad12.sml",
  "/sdb1/nomad.sml/Nomad13.sml",
  "/sdb1/nomad.sml/Nomad14.sml",
  "/sdb1/nomad.sml/Nomad15.sml",
  "/sdb1/nomad.sml/Nomad16.sml",
  0
};



#define NR_OF_HIP 120416

#define MAX_HIP_LEVEL 2
#define MAX_TYC_LEVEL 4
#define MAX_LEVEL 9

static
const char *output_file_names[MAX_LEVEL+1] = {
  "stars0.cat",
  "stars1.cat",
  "stars2.cat",
  "stars3.cat",
  "stars4.cat",
  "stars5.cat",
  "stars6.cat",
  "stars7.cat",
  "stars8.cat",
  "stars9.cat"
};

static
const char *component_ids_filename = "stars_hip_component_ids.cat";

static
const char *sp_filename = "stars_hip_sp.cat";

static
const double level_mag_limit[MAX_LEVEL+1] = {
  6,7.5,9,
  10.5,12,
  13.5,15,16.5,18,19.5
};









/*

Vector (simple 3d-Vector)

Author and Copyright: Johannes Gajdosik, 2006

License: GPL

*/


#include <math.h>

struct Vector {
  double x[3];
  inline double length(void) const;
  inline void normalize(void);
  const Vector &operator+=(const Vector &a) {
    x[0] += a.x[0];
    x[1] += a.x[1];
    x[2] += a.x[2];
  }
  const Vector &operator-=(const Vector &a) {
    x[0] -= a.x[0];
    x[1] -= a.x[1];
    x[2] -= a.x[2];
  }
};

static inline
Vector operator^(const Vector &a,const Vector &b) {
  Vector c;
  c.x[0] = a.x[1]*b.x[2] - a.x[2]*b.x[1];
  c.x[1] = a.x[2]*b.x[0] - a.x[0]*b.x[2];
  c.x[2] = a.x[0]*b.x[1] - a.x[1]*b.x[0];
  return c;
}

static inline
Vector operator-(const Vector &a,const Vector &b) {
  Vector c;
  c.x[0] = a.x[0]-b.x[0];
  c.x[1] = a.x[1]-b.x[1];
  c.x[2] = a.x[2]-b.x[2];
  return c;
}

static inline
Vector operator+(const Vector &a,const Vector &b) {
  Vector c;
  c.x[0] = a.x[0]+b.x[0];
  c.x[1] = a.x[1]+b.x[1];
  c.x[2] = a.x[2]+b.x[2];
  return c;
}

static inline
Vector operator*(double a,const Vector &b) {
  Vector c;
  c.x[0] = a*b.x[0];
  c.x[1] = a*b.x[1];
  c.x[2] = a*b.x[2];
  return c;
}

static inline
Vector operator*(const Vector &b,double a) {
  Vector c;
  c.x[0] = a*b.x[0];
  c.x[1] = a*b.x[1];
  c.x[2] = a*b.x[2];
  return c;
}

static inline
double operator*(const Vector &a,const Vector &b) {
  return a.x[0]*b.x[0]+a.x[1]*b.x[1]+a.x[2]*b.x[2];
}

double Vector::length(void) const {
  return sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
}

void Vector::normalize(void) {
  const double l = 1.0/length();
  x[0] *= l;
  x[1] *= l;
  x[2] *= l;
}




void ChangeEpoch(double delta_years,
                 double &ra,double &dec, // degrees
                 double pm_ra,double pm_dec // mas/yr
                ) {
  ra *= (M_PI/180);
  dec *= (M_PI/180);

  const double cdec = cos(dec);
  Vector x = {cos(ra)*cdec,sin(ra)*cdec,sin(dec)};
  const Vector north = {0.0,0.0,1.0};

  Vector axis0 = north ^ x;
  axis0.normalize();
  const Vector axis1 = x ^ axis0;

  const double f = delta_years*(0.001/3600)*(M_PI/180);

  x += pm_ra*f*axis1 + pm_dec*f*axis0;

  ra = atan2(x.x[1],x.x[0]);
  if (ra < 0.0) ra += 2*M_PI;
  dec = atan2(x.x[2],sqrt(x.x[0]*x.x[0]+x.x[1]*x.x[1]));

  ra *= (180/M_PI);
  dec *= (180/M_PI);
}






//------------------------------------------------

/*

GeodesicGrid: a library for dividing the sphere into triangle zones
by subdividing the icosahedron

Author and Copyright: Johannes Gajdosik, 2006

This library requires a simple Vector library,
which may have different copyright and license.

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

using namespace std;

struct HalfSpace {
    // all x with x*n-d >= 0
  Vector n;
  double d;
  bool inside(const Vector &x) const {return (x*n>=d);}
};

class GeodesicGrid {
    // Grid of triangles (zones) on the sphere with radius 1,
    // generated by subdividing the icosahedron.
    // level 0: just the icosahedron, 20 zones
    // level 1: 80 zones, level 2: 320 zones, ...
    // Each zone has a unique integer number in the range
    // [0,getNrOfZones()-1].
public:
  GeodesicGrid(int max_level);
  ~GeodesicGrid(void);
  int getMaxLevel(void) const {return max_level;}
  static int nrOfZones(int level)
    {return (20<<(level<<1));} // 20*4^level
  int getNrOfZones(void) const {return nrOfZones(max_level);}
  void getTriangleCorners(int lev, int index, Vector& c0, Vector& c1, Vector& c2) const;
    // Return the position of the 3 corners for the triangle at the given level and index
  int getPartnerTriangle(int lev, int index) const;
    // Return the index of the partner triangle with which to form a paralelogram
  typedef void (VisitFunc)(int lev,int index,
                           const Vector &c0,
                           const Vector &c1,
                           const Vector &c2,
                           void *context);
  void visitTriangles(int max_visit_level,
                      VisitFunc *func,void *context) const;

  int searchZone(const Vector &v,int search_level) const;
    // find the zone number in which a given point lies.
    // prerequisite: v*v==1
    // When the point lies on the border of two or more zones,
    // one such zone is returned (always the same one,
    // because the algorithm is deterministic).

  void searchZones(const HalfSpace *half_spaces,int nr_of_half_spaces,
                   int **inside,int **border,int max_search_level) const;
    // find all zones that lie fully(inside) or partly(border)
    // in the intersection of the given half spaces.
    // The result is accurate when (0,0,0) lies on the border of
    // each half space. If this is not the case,
    // the result may be inaccurate, because it is assumed, that
    // a zone lies in a half space when its 3 corners lie in
    // this half space.
    // inside[l] points to the begin of an integer array of size nrOfZones(l),
    // border[l] points to one after the end of the same integer array.
    // The array will be filled from the beginning with the inside zone numbers
    // and from the end with the border zone numbers of the given level l
    // for 0<=l<=getMaxLevel().
    // inside[l] will not contain zones that are already contained
    // in inside[l1] for some l1 < l.
    // In order to restrict search depth set max_search_level < max_level,
    // for full search depth set max_search_level = max_level,

private:
  const Vector& getTriangleCorner(int lev, int index, int cornerNumber) const;
  void initTriangle(int lev,int index,
                    const Vector &c0,
                    const Vector &c1,
                    const Vector &c2);
  void visitTriangles(int lev,int index,
                      const Vector &c0,
                      const Vector &c1,
                      const Vector &c2,
                      int max_visit_level,
                      VisitFunc *func,
                      void *context) const;
  void searchZones(int lev,int index,
                   const HalfSpace *half_spaces,
                   const int nr_of_half_spaces,
                   const int *index_of_used_half_spaces,
                   const int half_spaces_used,
                   const bool *corner0_inside,
                   const bool *corner1_inside,
                   const bool *corner2_inside,
                   int **inside,int **border,int max_search_level) const;
private:
  const int max_level;
  struct Triangle {
    Vector e0,e1,e2;   // Seitenmittelpunkte
  };
  Triangle **triangles;
  // 20*(4^0+4^1+...+4^n)=20*(4*(4^n)-1)/3 triangles total
  // 2+10*4^n corners
};

class GeodesicSearchResult {
public:
  GeodesicSearchResult(const GeodesicGrid &grid);
  ~GeodesicSearchResult(void);
  void search(const HalfSpace *half_spaces,
              const int nr_of_half_spaces,
              int max_search_level);
  void search(const Vector &e0,const Vector &e1,const Vector &e2,const Vector &e3,
              int max_search_level);
  void print(void) const;
private:
  friend class GeodesicSearchInsideIterator;
  friend class GeodesicSearchBorderIterator;
  const GeodesicGrid &grid;
  int **const zones;
  int **const inside;
  int **const border;
};

class GeodesicSearchBorderIterator {
public:
  GeodesicSearchBorderIterator(const GeodesicSearchResult &r,int level)
    : r(r),level((level<0)?0:(level>r.grid.getMaxLevel())
                          ?r.grid.getMaxLevel():level),
      end(r.zones[GeodesicSearchBorderIterator::level]+
                  GeodesicGrid::nrOfZones(GeodesicSearchBorderIterator::level))
      {reset();}
  void reset(void) {index = r.border[level];}
  int next(void) // returns -1 when finished
    {if (index < end) {return *index++;} return -1;}
private:
  const GeodesicSearchResult &r;
  const int level;
  const int *const end;
  const int *index;
};


class GeodesicSearchInsideIterator {
public:
  GeodesicSearchInsideIterator(const GeodesicSearchResult &r,int level)
    : r(r),max_level((level<0)?0:(level>r.grid.getMaxLevel())
                              ?r.grid.getMaxLevel():level)
     {reset();}
  void reset(void);
  int next(void); // returns -1 when finished
private:
  const GeodesicSearchResult &r;
  const int max_level;
  int level;
  int max_count;
  int *index_p;
  int *end_p;
  int index;
  int count;
};

#include <stdlib.h>
#include <cassert>
#include <iostream>

using namespace std;

static const double icosahedron_G = 0.5*(1.0+sqrt(5.0));
static const double icosahedron_b = 1.0/sqrt(1.0+icosahedron_G*icosahedron_G);
static const double icosahedron_a = icosahedron_b*icosahedron_G;

static const Vector icosahedron_corners[12] = {
  { icosahedron_a, -icosahedron_b,            0.0},
  { icosahedron_a,  icosahedron_b,            0.0},
  {-icosahedron_a,  icosahedron_b,            0.0},
  {-icosahedron_a, -icosahedron_b,            0.0},
  {           0.0,  icosahedron_a, -icosahedron_b},
  {           0.0,  icosahedron_a,  icosahedron_b},
  {           0.0, -icosahedron_a,  icosahedron_b},
  {           0.0, -icosahedron_a, -icosahedron_b},
  {-icosahedron_b,            0.0,  icosahedron_a},
  { icosahedron_b,            0.0,  icosahedron_a},
  { icosahedron_b,            0.0, -icosahedron_a},
  {-icosahedron_b,            0.0, -icosahedron_a}
};

struct TopLevelTriangle {
  int corners[3];   // index der Ecken
};


static const TopLevelTriangle icosahedron_triangles[20] = {
  { 1, 0,10}, //  1
  { 0, 1, 9}, //  0
  { 0, 9, 6}, // 12
  { 9, 8, 6}, //  9
  { 0, 7,10}, // 16
  { 6, 7, 0}, //  6
  { 7, 6, 3}, //  7
  { 6, 8, 3}, // 14
  {11,10, 7}, // 11
  { 7, 3,11}, // 18
  { 3, 2,11}, //  3
  { 2, 3, 8}, //  2
  {10,11, 4}, // 10
  { 2, 4,11}, // 19
  { 5, 4, 2}, //  5
  { 2, 8, 5}, // 15
  { 4, 1,10}, // 17
  { 4, 5, 1}, // 4
  { 5, 9, 1}, // 13
  { 8, 9, 5}  //  8
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

void GeodesicGrid::getTriangleCorners(int lev,int index,
                                      Vector &h0,
                                      Vector &h1,
                                      Vector &h2) const {
  if (lev <= 0) {
    const int *const corners = icosahedron_triangles[index].corners;
    h0 = icosahedron_corners[corners[0]];
    h1 = icosahedron_corners[corners[1]];
    h2 = icosahedron_corners[corners[2]];
  } else {
    lev--;
    const int i = index>>2;
    Triangle &t(triangles[lev][i]);
    switch (index&3) {
      case 0: {
        Vector c0,c1,c2;
        getTriangleCorners(lev,i,c0,c1,c2);
        h0 = c0;
        h1 = t.e2;
        h2 = t.e1;
      } break;
      case 1: {
        Vector c0,c1,c2;
        getTriangleCorners(lev,i,c0,c1,c2);
        h0 = t.e2;
        h1 = c1;
        h2 = t.e0;
      } break;
      case 2: {
        Vector c0,c1,c2;
        getTriangleCorners(lev,i,c0,c1,c2);
        h0 = t.e1;
        h1 = t.e0;
        h2 = c2;
      } break;
      case 3:
        h0 = t.e0;
        h1 = t.e1;
        h2 = t.e2;
        break;
    }
  }
}

int GeodesicGrid::getPartnerTriangle(int lev, int index) const
{
	if (lev==0)
	{
		assert(index<20);
		return (index&1) ? index+1 : index-1;
	}
	switch(index&7)
	{
		case 2:
		case 6:
			return index+1;
		case 3:
		case 7:
			return index-1;
		case 0:
			return (lev==1) ? index+5 : (getPartnerTriangle(lev-1, index>>2)<<2)+1;
		case 1:
			return (lev==1) ? index+3 : (getPartnerTriangle(lev-1, index>>2)<<2)+0;
		case 4:
			return (lev==1) ? index-3 : (getPartnerTriangle(lev-1, index>>2)<<2)+1;
		case 5:
			return (lev==1) ? index-5 : (getPartnerTriangle(lev-1, index>>2)<<2)+0;
		default:
			assert(0);
	}
}

void GeodesicGrid::initTriangle(int lev,int index,
                                const Vector &c0,
                                const Vector &c1,
                                const Vector &c2) {
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
    initTriangle(lev,index+1,t.e2,c1,t.e0);
    initTriangle(lev,index+2,t.e1,t.e0,c2);
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
                                  const Vector &c0,
                                  const Vector &c1,
                                  const Vector &c2,
                                  int max_visit_level,
                                  VisitFunc *func,
                                  void *context) const {
  (*func)(lev,index,c0,c1,c2,context);
  Triangle &t(triangles[lev][index]);
  lev++;
  if (lev <= max_visit_level) {
    index *= 4;
    visitTriangles(lev,index+0,c0,t.e2,t.e1,max_visit_level,func,context);
    visitTriangles(lev,index+1,t.e2,c1,t.e0,max_visit_level,func,context);
    visitTriangles(lev,index+2,t.e1,t.e0,c2,max_visit_level,func,context);
    visitTriangles(lev,index+3,t.e0,t.e1,t.e2,max_visit_level,func,context);
  }
}


int GeodesicGrid::searchZone(const Vector &v,int search_level) const {
  for (int i=0;i<20;i++) {
    const int *const corners = icosahedron_triangles[i].corners;
    const Vector &c0(icosahedron_corners[corners[0]]);
    const Vector &c1(icosahedron_corners[corners[1]]);
    const Vector &c2(icosahedron_corners[corners[2]]);
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
                  edge2_inside,corner1_inside,edge0_inside,
                  inside_list,border_list,max_search_level);
      searchZones(lev,index+2,
                  half_spaces,nr_of_half_spaces,halfs_used,halfs_used_count,
                  edge1_inside,edge0_inside,corner2_inside,
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

void GeodesicSearchResult::search(const Vector &e0,const Vector &e1,
                                  const Vector &e2,const Vector &e3,
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

//------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <map>
#include <list>
#include <string>
#include <iostream>

using namespace std;



static int restrict_output_level_min = 0;
static int restrict_output_level_max = MAX_LEVEL;



struct FaintStar {  // 6 byte
  int x0:18;
  int x1:18;
  unsigned int b_v:7;
  unsigned int mag:5;
  bool operator<(const FaintStar &s) const {return (mag < s.mag);}
} __attribute__ ((__packed__)) ;


struct TycStar {  // 10 byte
  int x0:20;
  int x1:20;
  int dx0:14;
  int dx1:14;
  unsigned int b_v:7;
  unsigned int mag:5;
  bool operator<(const TycStar &s) const {return (mag < s.mag);}
} __attribute__ ((__packed__));


struct HipStarCompressed {
  int hip:24;                  // 17 bits needed
  unsigned char comp_ids_int;  //  5 bits needed
  int x0;                      // 32 bits needed
  int x1;                      // 32 bits needed
  unsigned char b_v;           //  8 bits needed
  unsigned char mag;           //  5 bits needed
  unsigned short sp_int;       // 14 bits needed
  int dx0,dx1,plx;
} __attribute__ ((__packed__));

struct HipStar : public HipStarCompressed {
  string component_ids,sp;
  void setPlxSp(double plx,const string &sp) {
    HipStar::plx = (int)floor(0.5+100.0*plx);
    if (HipStar::plx < 0) HipStar::plx = 0;
    HipStar::sp = sp;
  }
  bool operator<(const HipStar &s) const {return (mag < s.mag);}
};



struct ZoneData {
  virtual ~ZoneData(void) {}
  virtual int getArraySize(void) const = 0;
  virtual HipStar *add(int level,
                       int tyc1,int tyc2,int tyc3,
                       int hip,const char *component_ids,
                       double x0,double x1,double dx0,double dx1,
                       double mag,double b_v,
                       double plx,const char *sp,
                       bool &does_not_fit) = 0;
  void writeInfoToOutput(FILE *f) const;
  virtual void writeStarsToOutput(FILE *f) = 0;
  Vector center;
  Vector axis0;
  Vector axis1;
};

struct HipZoneData : public ZoneData {
  list<HipStar> stars;
  int getArraySize(void) const {return stars.size();}
  HipStar *add(int level,
                       int tyc1,int tyc2,int tyc3,
                       int hip,const char *component_ids,
                       double x0,double x1,double dx0,double dx1,
                       double mag,double b_v,double plx,const char *sp,
                       bool &does_not_fit);
  void writeStarsToOutput(FILE *f);
};


static list<HipStar*> hip_index[NR_OF_HIP+1];

static void SqueezeHip(void) {
    // build lookup maps
  map<string,int> component_ids_map,sp_map;
  for (int i=0;i<=NR_OF_HIP;i++) {
    for (list<HipStar*>::const_iterator it(hip_index[i].begin());
         it!=hip_index[i].end();it++) {
      HipStar *h = *it;
      component_ids_map[h->component_ids]++;
      sp_map[h->sp]++;
    }
  }
  int component_ids_size = 0;
  for (map<string,int>::iterator it(component_ids_map.begin());
       it!=component_ids_map.end();it++,component_ids_size++) {
    it->second = component_ids_size;
  }
  if (component_ids_size >= 32) {
    cerr << "SqueezeHip: too many component_ids: "
         << component_ids_size << endl;
  }
  int sp_size = 0;
  for (map<string,int>::iterator it(sp_map.begin());
       it!=sp_map.end();it++,sp_size++) {
    it->second = sp_size;
  }
  if (sp_size >= 16384) {
    cerr << "SqueezeHip: too many sp: " << sp_size << endl;
  }
    // translate the strings for the hip stars into integers:
  for (int i=0;i<=NR_OF_HIP;i++) {
    for (list<HipStar*>::const_iterator it(hip_index[i].begin());
         it!=hip_index[i].end();it++) {
      HipStar *h = *it;
      h->comp_ids_int = component_ids_map[h->component_ids];
      h->sp_int = sp_map[h->sp];
    }
  }
    // write output files for component_ids and sp
  FILE *f = fopen(component_ids_filename,"wb");
  if (f==0) {
    cout << "fopen(" << component_ids_filename << ") failed" << endl;
    exit(-1);
  }
  for (map<string,int>::const_iterator it(component_ids_map.begin());
       it!=component_ids_map.end();it++,component_ids_size++) {
    fprintf(f,"%s\n",it->first.c_str());
  }
  fclose(f);
  f = fopen(sp_filename,"wb");
  if (f==0) {
    cout << "fopen(" << sp_filename << ") failed" << endl;
    exit(-1);
  }
  for (map<string,int>::const_iterator it(sp_map.begin());
       it!=sp_map.end();it++,sp_size++) {
    fprintf(f,"%s\n",it->first.c_str());
  }
  fclose(f);
}

struct TycZoneData : public ZoneData {
  list<TycStar> stars;
  int getArraySize(void) const {return stars.size();}
  HipStar *add(int level,
                       int tyc1,int tyc2,int tyc3,
                       int hip,const char *component_ids,
                       double x0,double x1,double dx0,double dx1,
                       double mag,double b_v,double plx,const char *sp,
                       bool &does_not_fit);
  void writeStarsToOutput(FILE *f);
};

struct FaintZoneData : public ZoneData {
  list<FaintStar> stars;
  int getArraySize(void) const {return stars.size();}
  HipStar *add(int level,
                       int tyc1,int tyc2,int tyc3,
                       int hip,const char *component_ids,
                       double x0,double x1,double dx0,double dx1,
                       double mag,double b_v,double plx,const char *sp,
                       bool &does_not_fit);
  void writeStarsToOutput(FILE *f);
};


class Accumulator {
public:
  Accumulator(void);
  ~Accumulator(void);
  int addStar(int tyc1,int tyc2,int tyc3,int hip,const char *component_ids,
              double ra, // degrees
              double dec, // degrees
              double pma,double pmd,
              double mag,double b_v,
              double plx,const char *sp);
  void writeOutput(const char *fnames[]);
private:
  GeodesicGrid grid;
  struct ZoneArray {
    ZoneArray(int level,GeodesicGrid &grid)
      : level(level),nr_of_zones(GeodesicGrid::nrOfZones(level)),grid(grid)
        {scale = 0.0;nr_of_stars = 0;}
    virtual ~ZoneArray(void) {}
    virtual ZoneData *getZone(int index) const = 0;
    virtual void writeHeaderToOutput(FILE *f) const = 0;
    const int level;
    const int nr_of_zones;
    const GeodesicGrid &grid;
    double scale;
//    int scale_int;
    int nr_of_stars;
    HipStar *addStar(int tyc1,int tyc2,int tyc3,
                     int hip,const char *component_ids,
                     double ra, // degrees
                     double dec, // degrees
                     double pma,double pmd,
                     double mag,double b_v,
                     double plx,const char *sp,
                     bool &does_not_fit);
    void writeOutput(const char *fname);
  };
  struct HipZoneArray : public ZoneArray {
    HipZoneArray(int level,GeodesicGrid &grid)
      : ZoneArray(level,grid),zones(new HipZoneData[nr_of_zones]) {}
    ~HipZoneArray(void) {delete[] zones;}
    void writeHeaderToOutput(FILE *f) const;
    HipZoneData *const zones;
    ZoneData *getZone(int index) const {
      if (index<0 || index>=nr_of_zones) {
        cerr << "getZone: bad index" << endl;
        exit(-1);
      }
      return zones+index;
    }
  };
  struct TycZoneArray : public ZoneArray {
    TycZoneArray(int level,GeodesicGrid &grid)
      : ZoneArray(level,grid),zones(new TycZoneData[nr_of_zones]) {}
    ~TycZoneArray(void) {delete[] zones;}
    void writeHeaderToOutput(FILE *f) const;
    TycZoneData *zones;
    ZoneData *getZone(int index) const {
      if (index<0 || index>=nr_of_zones) {
        cerr << "getZone: bad index" << endl;
        exit(-1);
      }
      return zones+index;
    }
  };
  struct FaintZoneArray : public ZoneArray {
    FaintZoneArray(int level,GeodesicGrid &grid)
      : ZoneArray(level,grid),zones(new FaintZoneData[nr_of_zones]) {}
    ~FaintZoneArray(void) {delete[] zones;}
    void writeHeaderToOutput(FILE *f) const;
    FaintZoneData *zones;
    ZoneData *getZone(int index) const {
      if (index<0 || index>=nr_of_zones) {
        cerr << "getZone: bad index" << endl;
        exit(-1);
      }
      return zones+index;
    }
  };
  ZoneArray *zone_array[MAX_LEVEL+1];
  
private:
  static void initTriangleFunc(int lev,int index,
                    const Vector &c0,
                    const Vector &c1,
                    const Vector &c2,
                    void *user_data) {
    reinterpret_cast<Accumulator*>(user_data)
      ->initTriangle(lev,index,c0,c1,c2);
  }
  void initTriangle(int lev,int index,
                    const Vector &c0,
                    const Vector &c1,
                    const Vector &c2);
};









static
void WriteByte(FILE *f,int x) {
  unsigned char c = (unsigned int)x;
  if (1!=fwrite(&c,1,1,f)) {
    cerr << "WriteByte: fwrite failed" << endl;
    exit(-1);
  }
}

static
void WriteInt(FILE *f,int x) {
  if (4!=fwrite(&x,1,4,f)) {
    cerr << "WriteInt: fwrite failed" << endl;
    exit(-1);
  }
}

//static
//int DoubleToInt(double x) {
//  return (int)floor(0.5+x*0x7FFFFFFF);
//}

//static
//double IntToDouble(int x) {
//  double rval = x;
//  rval /= 0x7FFFFFFF;
//  return rval;
//}

void ZoneData::writeInfoToOutput(FILE *f) const {
//  WriteInt(f,DoubleToInt(center.x[0]));
//  WriteInt(f,DoubleToInt(center.x[1]));
//  WriteInt(f,DoubleToInt(center.x[2]));
  WriteInt(f,getArraySize());
}

void HipZoneData::writeStarsToOutput(FILE *f) {
  stars.sort();
  for (list<HipStar>::const_iterator it(stars.begin());
       it!=stars.end();it++) {
    if (sizeof(HipStarCompressed)!=
        fwrite(&(*it),1,sizeof(HipStarCompressed),f)) {
      cerr << "HipZoneData::writeStarsToOutput: fwrite failed" << endl;
      exit(-1);
    }
  }
}

void TycZoneData::writeStarsToOutput(FILE *f) {
  stars.sort();
  for (list<TycStar>::const_iterator it(stars.begin());
       it!=stars.end();it++) {
    if (sizeof(TycStar)!=fwrite(&(*it),1,sizeof(TycStar),f)) {
      cerr << "TycZoneData::writeStarsToOutput: fwrite failed" << endl;
      exit(-1);
    }
  }
}


void FaintZoneData::writeStarsToOutput(FILE *f) {
  stars.sort();
  for (list<FaintStar>::const_iterator it(stars.begin());
       it!=stars.end();it++) {
    if (sizeof(FaintStar)!=fwrite(&(*it),1,sizeof(FaintStar),f)) {
      cerr << "FaintZoneData::writeStarsToOutput: fwrite failed" << endl;
      exit(-1);
    }
  }
}



















Accumulator::Accumulator(void)
            :grid(MAX_LEVEL) {
  int l=0;
  for (;l<=MAX_HIP_LEVEL;l++) {
    zone_array[l] = new HipZoneArray(l,grid);
  }
  for (;l<=MAX_TYC_LEVEL;l++) {
    zone_array[l] = new TycZoneArray(l,grid);
  }
  for (;l<=MAX_LEVEL;l++) {
    zone_array[l] = new FaintZoneArray(l,grid);
  }
  grid.visitTriangles(MAX_LEVEL,initTriangleFunc,this);
  for (int l=0;l<=MAX_LEVEL;l++) {
//    zone_array[l]->scale_int = DoubleToInt(zone_array[l]->scale);
//    while (IntToDouble(zone_array[l]->scale_int) > zone_array[l]->scale) {
//      zone_array[l]->scale_int--;
//    }
//    zone_array[l]->scale = IntToDouble(zone_array[l]->scale_int);
    cout << "Accumulator::Accumulator: level " << zone_array[l]->level
         << ", scale: " << zone_array[l]->scale << endl;
  }
}

Accumulator::~Accumulator(void) {
  for (int l=0;l<=MAX_LEVEL;l++) {
    delete zone_array[l];
  }
}

static const Vector north = {0.0,0.0,1.0};

void Accumulator::initTriangle(int lev,int index,
                               const Vector &c0,
                               const Vector &c1,
                               const Vector &c2) {
  ZoneData &z(*zone_array[lev]->getZone(index));
  double &scale(zone_array[lev]->scale);
  z.center = c0+c1+c2;
  z.center.normalize();
  z.axis0 = north ^ z.center;
  z.axis0.normalize();
  z.axis1 = z.center ^ z.axis0;

  double mu0,mu1,f,h;

  mu0 = (c0-z.center)*z.axis0;
  mu1 = (c0-z.center)*z.axis1;
  f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
  h = fabs(mu0)*f;
  if (scale < h) scale = h;
  h = fabs(mu1)*f;
  if (scale < h) scale = h;

  mu0 = (c1-z.center)*z.axis0;
  mu1 = (c1-z.center)*z.axis1;
  f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
  h = fabs(mu0)*f;
  if (scale < h) scale = h;
  h = fabs(mu1)*f;
  if (scale < h) scale = h;

  mu0 = (c2-z.center)*z.axis0;
  mu1 = (c2-z.center)*z.axis1;
  f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
  h = fabs(mu0)*f;
  if (scale < h) scale = h;
  h = fabs(mu1)*f;
  if (scale < h) scale = h;
}

static
unsigned char BvToColorIndex(double b_v) {
  b_v *= 1000.0;
  if (b_v < -500) {
    b_v = -500;
  } else if (b_v > 3499) {
    b_v = 3499;
  }
  return (unsigned int)floor(0.5+127.0*((500.0+b_v)/4000.0));
}

HipStar *FaintZoneData::add(int level,
                       int tyc1,int tyc2,int tyc3,
                       int hip,const char *component_ids,
                       double x0,double x1,double dx0,double dx1,
                       double mag,double b_v,double plx,const char *sp,
                       bool &does_not_fit) {
  if (mag>=level_mag_limit[level]) {
    cout << "too faint" << endl;
    exit(-1);
  }
  FaintStar s;
  s.x0  = ((int)floor(0.5+x0*((1<<17)-1))); // 18 bit signed
  s.b_v = BvToColorIndex(b_v);       // 0..127: 7 Bit unsigned
  s.x1  = ((int)floor(0.5+x1*((1<<17)-1))); // 18 bit signed
  s.mag = (unsigned int)floor(30*(mag-level_mag_limit[level-1])/
                        (level_mag_limit[level]-level_mag_limit[level-1]));
                 // steps(accuracy) of 0.05mag, 5 bit unsigned
  stars.push_back(s);
  does_not_fit = false;
  return 0;
}

HipStar *TycZoneData::add(int level,
                          int tyc1,int tyc2,int tyc3,
                          int hip,const char *component_ids,
                          double x0,double x1,double dx0,double dx1,
                          double mag,double b_v,double plx,const char *sp,
                          bool &does_not_fit) {
  TycStar s;
  s.x0  = ((int)floor(0.5+x0*((1<<19)-1))); // 20 bit signed
  s.b_v = BvToColorIndex(b_v);       // 0..127: 7 Bit unsigned
  s.x1  = ((int)floor(0.5+x1*((1<<19)-1))); // 20 bit signed
  s.mag = (unsigned int)floor(30*(mag-level_mag_limit[level-1])/
                        (level_mag_limit[level]-level_mag_limit[level-1]));
                 // steps(accuracy) of 0.05mag, 5 bit unsigned
  const int dx0_int = (int)floor(0.5+dx0*10.0); // 14 bit signed
  const int dx1_int = (int)floor(0.5+dx1*10.0); // 14 bit signed
  if (dx0_int >= 8192 || dx0_int<-8192 ||
      dx1_int >= 8192 || dx1_int<-8192) {
      // does not fit, must store in Hip format
    does_not_fit = true;
    return 0;
  }
  s.dx0 = dx0_int;
  s.dx1 = dx1_int;
  stars.push_back(s);
  does_not_fit = false;
  return 0;
}


HipStar *HipZoneData::add(int level,
                          int tyc1,int tyc2,int tyc3,
                          int hip,const char *component_ids,
                          double x0,double x1,double dx0,double dx1,
                          double mag,double b_v,double plx,const char *sp,
                          bool &does_not_fit) {
  HipStar s;
  s.x0  = ((int)floor(0.5+x0*((1<<31)-1))); // 32 bit signed
  s.b_v = BvToColorIndex(b_v);       // 0..127: 7 Bit unsigned
  s.x1  = ((int)floor(0.5+x1*((1<<31)-1))); // 32 bit signed
  const int mag_int = (int)floor(20*
             (mag-((level==0)?-2.0:level_mag_limit[level-1])));
  if (mag_int < 0 || mag_int > 255) {
    cerr << "HipZoneData(" << level << ")::add: bad mag: " << mag << endl;
    exit(-1);
  }
  s.mag = mag_int;
  s.dx0 = (int)floor(0.5+dx0*10.0);
  s.dx1 = (int)floor(0.5+dx1*10.0);
  s.hip = hip;
  s.component_ids = component_ids;
  s.comp_ids_int = 0;
  s.sp_int = 0;
  s.plx = (int)floor(0.5+100.0*plx);
  s.sp = sp;
  stars.push_back(s);
  does_not_fit = false;
  return &(stars.back());
}


HipStar *Accumulator::ZoneArray::addStar(
                           int tyc1,int tyc2,int tyc3,int hip,const char *component_ids,
                           double ra,double dec,double pma,double pmd,
                           double mag,double b_v,double plx,const char *sp,
                           bool &does_not_fit) {
  ra *= (M_PI/180.0);
  dec *= (M_PI/180.0);
  if (ra < 0 || ra >= 2*M_PI || dec < -0.5*M_PI || dec > 0.5*M_PI) {
    cerr << "ZoneArray(l=" << level << ")::addStar: "
            "bad ra/dec: " << ra << ',' << dec << endl;
    exit (-1);
  }
  const double cdec = cos(dec);
  Vector pos;
  pos.x[0] = cos(ra)*cdec;
  pos.x[1] = sin(ra)*cdec;
  pos.x[2] = sin(dec);
  const int zone = grid.searchZone(pos,level);

  ZoneData &z(*getZone(zone));
  const double mu0 = (pos-z.center) * z.axis0;
  const double mu1 = (pos-z.center) * z.axis1;
  const double d = 1.0 - mu0*mu0 - mu1*mu1;
  const double sd = sqrt(d);
  const double x0 = mu0/(sd*scale);
  const double x1 = mu1/(sd*scale);
  const double h = mu0*pma + mu1*pmd;
  const double dx0 = (pma*d + mu0*h) / (sd*d);
  const double dx1 = (pmd*d + mu1*h) / (sd*d);

//cout << "Accumulator::ZoneArray(l=" << level << ")::addStar: " << zone << endl;
  HipStar *rval = z.add(level,tyc1,tyc2,tyc3,hip,component_ids,
                        x0,x1,dx0,dx1,mag,b_v,plx,sp,does_not_fit);
  if (!does_not_fit) nr_of_stars++;
//cout << "Accumulator::ZoneArray(l=" << level << ")::addStar: 999";
  return rval;
}

int Accumulator::addStar(int tyc1,int tyc2,int tyc3,int hip,const char *component_ids,
                         double ra,double dec,double pma,double pmd,
                         double mag,double b_v,double plx,const char *sp) {
//  const int packed_hip = PackHip(hip,component_ids);
//  const int packed_tyc = PackTyc(tyc1,tyc2,tyc3);

  int l=0;
  while (l<MAX_LEVEL && mag>=level_mag_limit[l]) l++;
  if (hip>0 && l>MAX_HIP_LEVEL) l = MAX_HIP_LEVEL;
//  store the faint tyc2 stars as faint stars
//  else if (tyc1>0 && l>MAX_TYC_LEVEL) l = MAX_TYC_LEVEL;


  if (restrict_output_level_max < l ||
      restrict_output_level_min > l) {
      // you may want to restrict the output
      // in order to restrict the amount of main memory used.
      // Calling the program twice may be faster than calling once
      // without enough main memory (avoid swapping).
    return 0;
  }

  bool does_not_fit;
  HipStar *s = zone_array[l]->addStar(tyc1,tyc2,tyc3,hip,component_ids,
                                      ra,dec,pma,pmd,
                                      mag,b_v,plx,sp,does_not_fit);
  if (does_not_fit) s = zone_array[MAX_HIP_LEVEL]
                            ->addStar(tyc1,tyc2,tyc3,hip,component_ids,
                                      ra,dec,pma,pmd,
                                      mag,b_v,plx,sp,does_not_fit);
  if (s) hip_index[hip].push_back(s);
  return 0;
}














#define FILE_MAGIC 0x835f040a

void Accumulator::HipZoneArray::writeHeaderToOutput(FILE *f) const {
  cout << "Accumulator::HipZoneArray(" << level << ")::writeHeaderToOutput: "
       << nr_of_stars << endl;
  WriteInt(f,FILE_MAGIC);
  WriteInt(f,0); // type
  WriteInt(f,0); // major version
  WriteInt(f,1); // minor version
  WriteInt(f,level);
//  WriteInt(f,scale_int);
  if (level == 0) {
    WriteInt(f,-2000); // min_mag
  } else {
  WriteInt(f,(int)floor(0.5+1000.0*level_mag_limit[level-1])); // min_mag
  }
  WriteInt(f,12800); // mag_range
  WriteInt(f,256);   // mag_steps
}

void Accumulator::TycZoneArray::writeHeaderToOutput(FILE *f) const {
  cout << "Accumulator::TycZoneArray(" << level << ")::writeHeaderToOutput: "
       << nr_of_stars << endl;
  WriteInt(f,FILE_MAGIC);
  WriteInt(f,1); // type
  WriteInt(f,0); // major version
  WriteInt(f,0); // minor version
  WriteInt(f,level);
//  WriteInt(f,scale_int);
  WriteInt(f,(int)floor(0.5+1000.0*level_mag_limit[level-1])); // min_mag
  WriteInt(f,(int)floor(0.5+1000.0*(level_mag_limit[level]
                                   -level_mag_limit[level-1]))); // mag_range
  WriteInt(f,30);   // mag_steps
}

void Accumulator::FaintZoneArray::writeHeaderToOutput(FILE *f) const {
  cout << "Accumulator::FaintZoneArray(" << level << ")::writeHeaderToOutput: "
       << nr_of_stars << endl;
  WriteInt(f,FILE_MAGIC);
  WriteInt(f,2); // type
  WriteInt(f,0); // major version
  WriteInt(f,0); // minor version
  WriteInt(f,level);
//  WriteInt(f,scale_int);
  WriteInt(f,(int)floor(0.5+1000.0*level_mag_limit[level-1])); // min_mag
  WriteInt(f,(int)floor(0.5+1000.0*(level_mag_limit[level]
                                   -level_mag_limit[level-1]))); // mag_range
  WriteInt(f,30);   // mag_steps
}


void Accumulator::ZoneArray::writeOutput(const char *fname) {
  if (nr_of_stars <= 0) return;
  FILE *f = fopen(fname,"wb");
  if (f==0) {
    cerr << "Accumulator::writeOutput(" << fname << "): "
            "fopen failed" << endl;
    return;
  }
  writeHeaderToOutput(f);
  for (int zone=0;zone<nr_of_zones;zone++) {
    getZone(zone)->writeInfoToOutput(f);
  }
  int permille = 0;
  for (int zone=0;zone<nr_of_zones;zone++) {
    getZone(zone)->writeStarsToOutput(f);
    int p = (1000*zone)/nr_of_zones;
    if (p != permille) {
      permille = p;
      cout << "\rAccumulator::writeOutput(" << fname << "): "
           << permille << "permille written" << flush;
    }
  }
  cout << endl;
  fclose(f);
}

void Accumulator::writeOutput(const char *fnames[]) {
  for (int l=0;l<=MAX_LEVEL;l++) {
    zone_array[l]->writeOutput(fnames[l]);
  }
}















int ReadHipTycFile(Accumulator &accu) {
  int count = 0;
  FILE *f;
  const char *fname = "HipTyc";
  f = fopen(fname,"r");
  if (f == 0) {
    fprintf(stderr,"Could not open file \"%s\".\n",fname);
    exit(-1);
  }

  int hip,tyc1,tyc2,tyc3;
  char cids[32];
  char sp[256];
  int mag,b_v,VarFlag;
  double ra,dec,Plx,pm_ra,pm_dec;
  while (14==fscanf(f,"%d%d%d%d%s%d%lf%lf%lf%lf%lf%d%d%s",
                    &hip,&tyc1,&tyc2,&tyc3,cids,&VarFlag,
                    &ra,&dec,&Plx,&pm_ra,&pm_dec,&mag,&b_v,sp)) {
      const int rc = accu.addStar(tyc1,tyc2,tyc3,hip,
                                  cids[0]=='?'?"":cids,
                                  ra, // degrees
                                  dec, // degrees
                                  pm_ra,pm_dec,0.001*mag,0.001*b_v,
                                  Plx,sp[0]=='?'?"":sp);
      if (rc < 0) {
          // never mind: propably no magnitude for Hiparcos star
//        fprintf(stderr,"File \"%s\", record %d: Error 13 %d %d \"%s\"\n",
//                fname,count,rc,hip,sp);
//        exit(-1);
      }
    count++;
  }
  fclose(f);
  return count;
}


const unsigned short int SHORT_ASTSRCBIT0 = 0x0001; // Astrometry source bit 0
const unsigned short int SHORT_ASTSRCBIT1 = 0x0002; // Astrometry source bit 1
const unsigned short int SHORT_ASTSRCBIT2 = 0x0004; // Astrometry source bit 2
const unsigned short int SHORT_UBBIT   = 0x0008;
const unsigned short int SHORT_TMBIT   = 0x0010;
const unsigned short int SHORT_XRBIT   = 0x0020;
const unsigned short int SHORT_IUCBIT  = 0x0040;
const unsigned short int SHORT_ITYBIT  = 0x0080;
const unsigned short int SHORT_OMAGBIT = 0x0100;
const unsigned short int SHORT_EMAGBIT = 0x0200;
const unsigned short int SHORT_TMONLY  = 0x0400;
const unsigned short int SHORT_SPIKE   = 0x0800;
const unsigned short int SHORT_TYCONF  = 0x1000;
const unsigned short int SHORT_BSCONF  = 0x2000;
const unsigned short int SHORT_BSART   = 0x4000;
const unsigned short int SHORT_USEME   = 0x8000;

struct Short_NOMAD_Record {
  int ra,spd,pm_ra,pm_spd;
  short int b,v,r;
  unsigned short int flags;
};



#define READ_SIZE 10000
static Short_NOMAD_Record buff[READ_SIZE];

void ReadNOMADFile(const char *fname,Accumulator &accu) {
  int count;
  FILE *f;
  f = fopen(fname,"r");
  if (f == 0) {
    fprintf(stderr,"Could not open file \"%s\".\n",fname);
    exit(-1);
  }
  int total = 0;
  do {
    count = fread(buff,sizeof(Short_NOMAD_Record),READ_SIZE,f);
    total += count;
    printf("\rfread(%s,...) returned %6d, total = %8d",
           fname,count,total);
    fflush(stdout);
    int i;
    for (i=0;i<count;i++) {
      int mag = 30000;
      int b_v;
      if (buff[i].v >= 30000) {
        if (buff[i].b >= 30000) {
          if (buff[i].r >= 30000) {
//          cerr << "no magnitude at all" << endl;
          } else {
            mag = buff[i].r + 3499; // just an assumption
            b_v = 3499;
          }
        } else {
          mag = buff[i].b + 500; // just an assumption
          b_v = -500;
        }
      } else {
        mag = buff[i].v;
        if (buff[i].b >= 30000) {
          if (buff[i].r >= 30000) {
            b_v = 0;
          } else {
            b_v = buff[i].v-buff[i].r; // desperate
          }
        } else {
          b_v = buff[i].b-buff[i].v;
        }
      }

      int nr_of_measurements = 0;
      if (buff[i].b < 30000) nr_of_measurements++;
      if (buff[i].v < 30000) nr_of_measurements++;
      if (buff[i].r < 30000) nr_of_measurements++;

      if (mag < 19500 &&
          ((buff[i].flags&SHORT_USEME) ||
           (
           ((buff[i].flags&(SHORT_ASTSRCBIT0|SHORT_ASTSRCBIT1|SHORT_ASTSRCBIT2))!=1 ||
              (((buff[i].flags&SHORT_UBBIT)==0)
               &&
                  (mag>14000 || nr_of_measurements>1) &&
                  (mag>13000 || nr_of_measurements>2)
                   )) &&
            ((buff[i].flags&SHORT_SPIKE)==0) &&
            ((buff[i].flags&SHORT_BSART)==0) &&
            ((buff[i].flags&SHORT_TMONLY)==0)))) {
        if (accu.addStar(0,0,0,0,"",
                         buff[i].ra/(3600.0*1000.0), // degrees
                         (buff[i].spd-90*3600*1000)/(3600.0*1000.0), // degrees
                         0.1*buff[i].pm_ra,0.1*buff[i].pm_spd,
                         0.001*mag,0.001*b_v,
                         0,"") < 0) {
          fprintf(stderr,"File \"%s\", record %d: Error 16\n",fname,count);
          exit(-1);
        }
      }
    }
  } while (count == READ_SIZE);
  printf("\n");
  fclose(f);
}


int main(int argc,char *argv[]) {
  if (argc != 3 ||
      1 != sscanf(argv[1],"%d",&restrict_output_level_min) ||
      1 != sscanf(argv[2],"%d",&restrict_output_level_max)) {
    cerr << "Usage: " << argv[0] << " level_min level_max" << endl
         << " (like " << argv[0] << " 0 6)" << endl << endl;
    return -1;
  }

  Accumulator accu;
  int n=0;
  n = ReadHipTycFile(accu);
  printf("HipTyc: %d records processed.\n",n);
  SqueezeHip();
  
  for (int c=0;nomad_names[c];c++) {
    ReadNOMADFile(nomad_names[c],accu);
  }

  accu.writeOutput(output_file_names);

  return 0;
}
