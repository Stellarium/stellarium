// Author and Copyright: Johannes Gajdosik, 2006
// License: GPL
// g++ -O2 MakeCombinedCatalogue.C -o MakeCombinedCatalogue


// change catalogue locations according to your needs:

const char *hip_name = "cdsweb.u-strasbg.fr/ftp/cats/I/239/hip_main.dat";

const char *tyc_names[]={
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.00",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.01",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.02",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.03",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.04",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.05",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.06",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.07",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.08",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.09",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.10",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.11",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.12",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.13",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.14",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.15",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.16",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.17",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.18",
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/tyc2.dat.19",
  0
};

const char *nomad_names[]={
  "/sde1/contents/NOMAD/n161a",
  "/sde1/contents/NOMAD/n161b",
  "/sde1/contents/NOMAD/n161c",
  "/sde1/contents/NOMAD/n161d",
  0
};


const char *suppl_names[]={
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/suppl_1.dat",
  0, // suppl_2 contains bad stars from Tyc1
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/suppl_2.dat",
  0
};


#define NR_OF_HIP 120416

#define MAX_HIP_LEVEL 2
#define MAX_TYC_LEVEL 4
#define MAX_LEVEL 7

static
const char *output_file_names[MAX_LEVEL+1] = {
  "/sde1/contents/stars0.cat",
  "/sde1/contents/stars1.cat",
  "/sde1/contents/stars2.cat",
  "/sde1/contents/stars3.cat",
  "/sde1/contents/stars4.cat",
  "/sde1/contents/stars5.cat",
  "/sde1/contents/stars6.cat",
  "/sde1/contents/stars7.cat"
};

static
const char *component_ids_filename = "/sde1/contents/stars_hip_component_ids.cat";

static
const char *sp_filename = "/sde1/contents/stars_hip_sp.cat";

static
const double level_mag_limit[MAX_LEVEL+1] = {
  6,7.5,9,
  10.5,12,
  13.5,15,16.5
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
                       double mag,double b_v,bool &does_not_fit) = 0;
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
                       double mag,double b_v,bool &does_not_fit);
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
                       double mag,double b_v,bool &does_not_fit);
  void writeStarsToOutput(FILE *f);
};

struct FaintZoneData : public ZoneData {
  list<FaintStar> stars;
  int getArraySize(void) const {return stars.size();}
  HipStar *add(int level,
                       int tyc1,int tyc2,int tyc3,
                       int hip,const char *component_ids,
                       double x0,double x1,double dx0,double dx1,
                       double mag,double b_v,bool &does_not_fit);
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
              double mag,double b_v);
  int addData(int hip,const char *component_ids,double plx,const char *sp,
              double ra,double dec,double pm_ra,double pm_dec,
              double mag,double b_v);
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
                       double mag,double b_v,bool &does_not_fit) {
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
                          double mag,double b_v,bool &does_not_fit) {
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
                          double mag,double b_v,bool &does_not_fit) {
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
  s.plx = 0;
  stars.push_back(s);
  does_not_fit = false;
  return &(stars.back());
}


HipStar *Accumulator::ZoneArray::addStar(
                           int tyc1,int tyc2,int tyc3,int hip,const char *component_ids,
                           double ra,double dec,double pma,double pmd,
                           double mag,double b_v,
                           bool &does_not_fit) {
  ra *= (M_PI/180.0);
  dec *= (M_PI/180.0);
  if (ra < 0 || ra >= 2*M_PI || dec < -0.5*M_PI || dec > 0.5*M_PI) {
    cerr << "ZoneData::addStar: bad ra/dec: " << ra << ',' << dec << endl;
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
                        x0,x1,dx0,dx1,mag,b_v,does_not_fit);
  if (!does_not_fit) nr_of_stars++;
//cout << "Accumulator::ZoneArray(l=" << level << ")::addStar: 999";
  return rval;
}

int Accumulator::addStar(int tyc1,int tyc2,int tyc3,int hip,const char *component_ids,
                         double ra,double dec,double pma,double pmd,
                         double mag,double b_v) {
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
                                      mag,b_v,does_not_fit);
  if (does_not_fit) s = zone_array[MAX_HIP_LEVEL]
                            ->addStar(tyc1,tyc2,tyc3,hip,component_ids,
                                      ra,dec,pma,pmd,
                                      mag,b_v,does_not_fit);
  if (s) hip_index[hip].push_back(s);
  return 0;
}

int Accumulator::addData(int hip,const char *component_ids,
                         double plx,const char *sp,
                         double ra,double dec,double pm_ra,double pm_dec,
                         double mag,double b_v) {
  if (0<hip && hip<=NR_OF_HIP) {
    if (hip_index[hip].empty()) {
      if (mag > 1000) {
        cout << "missing tycho2, bad hip: " << hip << endl;
        return -1;
      }
      addStar(0,0,0,hip,component_ids,ra,dec,pm_ra,pm_dec,mag,b_v);
//      cout << "missing tycho2, good hip: " << hip << endl;
    }
    for (list<HipStar*>::const_iterator it(hip_index[hip].begin());
         it!=hip_index[hip].end();it++) {
      HipStar *const s = *it;
      if (s) s->setPlxSp(plx,sp);
    }
  }
  return 0;
}













#define FILE_MAGIC 0xde0955a3

void Accumulator::HipZoneArray::writeHeaderToOutput(FILE *f) const {
  cout << "Accumulator::HipZoneArray(" << level << ")::writeHeaderToOutput: "
       << nr_of_stars << endl;
  WriteInt(f,FILE_MAGIC+0); // type
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
  WriteInt(f,FILE_MAGIC+1); // type
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
  WriteInt(f,FILE_MAGIC+2); // type
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















int ReadEmptyString(const char *s) {
  if (s) {
    while (*s) {if (*s!=' ') return 0;s++;}
  }
  return 1;
}

int ReadBlankOrDouble(const char *s,double *x) {
  if (s==0) return -1;
  if (ReadEmptyString(s)) {*x=-9999999.99;return 0;}
  if (1==sscanf(s,"%lf",x)) {
//    printf("\"%s\":%f\n",s,*x);
    return 0;
  }
  return -2;
}

int ReadTyc2File(const char *fname,Accumulator &accu) {
  char buff[208];
  int count;
  FILE *f;
  int tyc1,tyc2,tyc3,hip;
  char component_ids[4] = {'\0','\0','\0','\0'};
  double ra,dec,pma,pmd,bt,vt,mag,b_v;
  f = fopen(fname,"rb");
  if (f == 0) {
    fprintf(stderr,"Could not open file \"%s\".\n",fname);
    exit(-1);
  }
  count = 0;
  while (207==fread(buff,1,207,f)) {
    if (buff[4]==' ' && buff[10]==' ' &&
        buff[12]=='|' && buff[14]=='|' && buff[27]=='|' && buff[40]=='|' &&
        buff[48]=='|' && buff[56]=='|' && buff[60]=='|' && buff[64]=='|' &&
        buff[69]=='|' && buff[74]=='|' && buff[82]=='|' && buff[90]=='|' &&
        buff[93]=='|' && buff[97]=='|' && buff[101]=='|' && buff[105]=='|' &&
        buff[109]=='|' && buff[116]=='|' && buff[122]=='|' && buff[129]=='|' &&
        buff[135]=='|' && buff[139]=='|' && buff[141]=='|' && buff[151]=='|' &&
        buff[164]=='|' && buff[177]=='|' && buff[182]=='|' && buff[187]=='|' &&
        buff[193]=='|' && buff[199]=='|' && buff[201]=='|' &&
        buff[206]==0x0A) {
      buff[12]='\0';
      if (3!=sscanf(buff,"%d%d%d",&tyc1,&tyc2,&tyc3)) {
        fprintf(stderr,"File \"%s\", record %d: Error 1\n",fname,count);
        exit(-1);
      }
      if (tyc1<1 || tyc2<1 || tyc3<1 ||
          tyc1>9537 || tyc2>12121 || tyc3>3) {
        fprintf(stderr,"File \"%s\", record %d: Error 2\n",fname,count);
        exit(-1);
      }
      component_ids[0] = buff[148];
      component_ids[1] = buff[149];
      component_ids[2] = buff[150];
      if (component_ids[2] == ' ') {
        component_ids[2] = '\0';
        if (component_ids[1] == ' ') {
          component_ids[1] = '\0';
          if (component_ids[0] == ' ') {
            component_ids[0] = '\0';
          }
        }
      }
      buff[148] = '\0';
      if (1!=sscanf(buff+142,"%d",&hip)) {
        if (buff[142]!=' '||buff[143]!=' '||buff[144]!=' '||
            buff[145]!=' '||buff[146]!=' '||buff[147]!=' '||
            component_ids[0] != '\0') {
          fprintf(stderr,"File \"%s\", record %d: Error 3\n",fname,count);
          exit(-1);
        } else {
          hip = 0;
        }
      } else {
        if (hip<1 || hip>120404) {
          fprintf(stderr,"File \"%s\", record %d: Error 4\n",fname,count);
          exit(-1);
        }
      }
      buff[27]=buff[40]=buff[48]=' ';buff[56]='\0';

      if (buff[13]=='X' || buff[13]=='P') {
        buff[164]=buff[177]=' ';
        if (2!=sscanf(buff+152,"%lf%lf",&ra,&dec)) {
          fprintf(stderr,"File \"%s\", record %d: Error 5\n",fname,count);
          exit(-1);
        }
        if (buff[13]=='P') {
            // do not use the "photocentre of two Tycho-2 entries":
          if (2!=sscanf(buff+41,"%lf%lf",&pma,&pmd)) {
            fprintf(stderr,"File \"%s\", record %d: Error 6\n",fname,count);
            exit(-1);
          }
            // simple proper motion correction:
          buff[182]=' ';buff[187]='\0';
          double EpRA,EpDE;
          if (2!=sscanf(buff+178,"%lf%lf",&EpRA,&EpDE)) {
            fprintf(stderr,"File \"%s\", record %d: Error 7\n",fname,count);
            exit(-1);
          }
          EpRA += 1990.0;
          EpDE += 1990.0;
          ra += (2000.0-EpRA)*(0.001/3600)*pma*cos(dec*M_PI/180);
          dec += (2000.0-EpDE)*(0.001/3600)*pmd;
          if (ra < 0.0) ra+=360.0;
          else if (ra >= 360.0) ra-=360.0;
          if (dec < -90.0 || dec > 90.0) {
            fprintf(stderr,"File \"%s\", "
                    "record %d: no real error, just bad dec, I am sorry.\n",
                    fname,count);
            exit(-1);
          }
        } else {
          if (!ReadEmptyString(buff+15)) {
            fprintf(stderr,"File \"%s\", record %d: Error 8\n",fname,count);
            exit(-1);
          }
          pma = pmd = 0.0;
        }
      } else {
        if (4!=sscanf(buff+15,"%lf%lf%lf%lf",&ra,&dec,&pma,&pmd)) {
          fprintf(stderr,"File \"%s\", record %d: Error 10\n",fname,count);
          exit(-1);
        }
      }

      buff[116]='\0';
      if (ReadBlankOrDouble(buff+110,&bt)) {
        fprintf(stderr,"File \"%s\", record %d: Error 11\n",fname,count);
        exit(-1);
      }
      buff[129]='\0';
      if (ReadBlankOrDouble(buff+123,&vt)) {
        fprintf(stderr,"File \"%s\", record %d: Error 12\n",fname,count);
        exit(-1);
      }
      if (bt>-10.0)
        if (vt>-10.0) {
          mag = vt -0.090*(bt-vt);  // Johnson photometry V
          b_v = 0.850*(bt-vt);      // Johnson photometry B-V
        } else {
          mag = bt;
          b_v = 0.73;
        }
      else
        if (vt>-10.0) {
          mag = vt;
          b_v = 0.73;
        } else {
          fprintf(stderr,"File \"%s\", record %d: Error 11\n",fname,count);
          exit(-1);
        }
//      printf("%04lu %05lu %lu %12.8f %12.8f %7.1f %7.1f %6.3f %6.3f\n",
//             tyc1,tyc2,tyc3,ra,dec,pma,pmd,bt,vt);
      const int rc = accu.addStar(tyc1,tyc2,tyc3,hip,component_ids,
                                  ra, // degrees
                                  dec, // degrees
                                  pma,pmd,mag,b_v);
      if (rc < 0) {
        fprintf(stderr,"File \"%s\", record %d: Error 12 %d %d \"%s\"\n",
                fname,count,rc,hip,component_ids);
        exit(-1);
      }
    } else {
      fprintf(stderr,"File \"%s\", record %d: wrong format\n",fname,count);
      exit(-1);
    }
    count++;
  }
  fclose(f);
  return count;
}


int ReadTyc2SupplFile(const char *fname,Accumulator &accu) {
  char buff[124];
  int count;
  FILE *f;
  int tyc1,tyc2,tyc3,hip;
  char component_ids[4] = {'\0','\0','\0','\0'};
  double ra,dec,pma,pmd,bt,vt,mag,b_v;
  f = fopen(fname,"r");
  if (f == 0) {
    fprintf(stderr,"Could not open file \"%s\".\n",fname);
    exit(-1);
  }
  count = 0;
  while (fgets(buff,124,f)) {
    if (buff[4]==' ' && buff[10]==' ' &&
        buff[12]=='|' && buff[14]=='|' && buff[27]=='|' && buff[40]=='|' &&
        buff[48]=='|' && buff[56]=='|' && buff[62]=='|' && buff[68]=='|' &&
        buff[74]=='|' && buff[80]=='|' && buff[82]=='|' && buff[89]=='|' &&
        buff[95]=='|' && buff[102]=='|' && buff[108]=='|' && buff[112]=='|' &&
        buff[114]=='|') {
      buff[12]='\0';
      if (3!=sscanf(buff,"%d%d%d",&tyc1,&tyc2,&tyc3)) {
        fprintf(stderr,"File \"%s\", record %d: Error 1\n",fname,count);
        exit(-1);
      }
      if (tyc1<1 || tyc2<1 || tyc3<1 ||
          tyc1>9537 || tyc2>12121 || tyc3>4) {
        fprintf(stderr,"File \"%s\", record %d: Error 2\n",fname,count);
        exit(-1);
      }
      component_ids[0] = buff[121];
      if (component_ids[0] == ' ') {
        component_ids[0] = '\0';
      }
      buff[121] = '\0';
      if (1!=sscanf(buff+115,"%d",&hip)) {
        if (buff[115]!=' '||buff[116]!=' '||buff[117]!=' '||
            buff[118]!=' '||buff[119]!=' '||buff[120]!=' '||
            component_ids[0] != '\0') {
          fprintf(stderr,"File \"%s\", record %d: Error 3\n",fname,count);
          exit(-1);
        } else {
          hip = 0;
        }
      } else {
        if (hip<1 || hip>120404) {
          fprintf(stderr,"File \"%s\", record %d: Error 4\n",fname,count);
          exit(-1);
        }
      }
      buff[27]=' ';buff[40]='\0';
      if (2!=sscanf(buff+15,"%lf%lf",&ra,&dec)) {
        fprintf(stderr,"File \"%s\", record %d: Error 5\n",fname,count);
        exit(-1);
      }
      buff[48]='\0';
      if (ReadBlankOrDouble(buff+41,&pma)) {
        fprintf(stderr,"File \"%s\", record %d: Error 6\n",fname,count);
        exit(-1);
      }
      buff[56]='\0';
      if (ReadBlankOrDouble(buff+49,&pmd)) {
        fprintf(stderr,"File \"%s\", record %d: Error 7\n",fname,count);
        exit(-1);
      }
      if (pma<-1000000.0)
        if (pmd<-1000000.0) {pma=pmd=0.0;}
        else {
          fprintf(stderr,"File \"%s\", record %d: Error 8\n",fname,count);
          exit(-1);
        }
      else
        if (pmd<-1000000.0) {
          fprintf(stderr,"File \"%s\", record %d: Error 9\n",fname,count);
          exit(-1);
        }
      buff[89]='\0';
      if (ReadBlankOrDouble(buff+83,&bt)) {
        fprintf(stderr,"File \"%s\", record %d: Error 10\n",fname,count);
        exit(-1);
      }
      buff[102]='\0';
      if (ReadBlankOrDouble(buff+96,&vt)) {
        fprintf(stderr,"File \"%s\", record %d: Error 11\n",fname,count);
        exit(-1);
      }
      switch (buff[81]) {
        case ' ':
          if (bt<-10 || vt<-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 12\n",fname,count);
            exit(-1);
          }
          mag = vt -0.090*(bt-vt);  // Johnson photometry V
          b_v = 0.850*(bt-vt);      // Johnson photometry B-V
          break;
        case 'B':
          if (bt<-10 || vt>-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 13\n",fname,count);
            exit(-1);
          }
          mag = bt;
          b_v = 0.73;
          break;
        case 'V':case 'H':
          if (bt>-10 || vt<-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 14\n",fname,count);
            exit(-1);
          }
          mag = vt;
          b_v = 0.73;
          break;
        default:
          fprintf(stderr,"File \"%s\", record %d: Error 15\n",fname,count);
          exit(-1);
      }
//      printf("%04lu %05lu %lu %12.8f %12.8f %7.1f %7.1f %6.3f %6.3f\n",
//             tyc1,tyc2,tyc3,ra,dec,pma,pmd,bt,vt);
      if (accu.addStar(tyc1,tyc2,tyc3,hip,component_ids,
                       ra, // degrees
                       dec, // degrees
                       pma,pmd,mag,b_v) < 0) {
        fprintf(stderr,"File \"%s\", record %d: Error 16\n",fname,count);
        exit(-1);
      }
    } else {
      fprintf(stderr,"File \"%s\", record %d: wrong format\n",fname,count);
      exit(-1);
    }
    count++;
  }
  fclose(f);
  return count;
}

int ReadHipFile(const char *fname,Accumulator &accu) {
  char buff[452];
  int count;
  FILE *f;
  int hip;
  char sp[13] = {'\0','\0','\0','\0','\0','\0',
                 '\0','\0','\0','\0','\0','\0','\0'};
  double plx;
  f = fopen(fname,"rb");
  if (f == 0) {
    fprintf(stderr,"Could not open file \"%s\".\n",fname);
    exit(-1);
  }
  count = 0;
  while (451==fread(buff,1,451,f)) {
    if (buff[1]=='|' && buff[14]=='|' &&
        buff[78]=='|' && buff[86]=='|' &&
        buff[434]=='|' && buff[447]=='|' &&
        buff[450]==0x0A) {
      buff[14] = '\0';
      if (1!=sscanf(buff+2,"%d",&hip)) {
          fprintf(stderr,"File \"%s\", record %d: Error 3\n",fname,count);
          exit(-1);
      }
      if (hip<1 || hip>NR_OF_HIP) {
        fprintf(stderr,"File \"%s\", record %d: Error 4\n",fname,count);
        exit(-1);
      }

      char component_ids[3];
      component_ids[0] = buff[352];
      component_ids[1] = buff[353];
      component_ids[2] = '\0';
      if (component_ids[1] == ' ') {
        component_ids[1] = '\0';
        if (component_ids[0] == ' ') {
          component_ids[0] = '\0';
        }
      }

      double pm_ra,pm_dec;
      buff[95]='\0';
      if (1!=sscanf(buff+87,"%lf",&pm_ra)) pm_ra = 0.0;
      buff[104]='\0';
      if (1!=sscanf(buff+96,"%lf",&pm_dec)) pm_dec = 0.0;
      
      buff[63]='\0';
      double ra;
      if (1!=sscanf(buff+51,"%lf",&ra)) {
        buff[28]='\0';
        if (buff[19]!=' ' || buff[22]!=' ' || buff[25]!='.') {
          fprintf(stderr,"File \"%s\", record %d: Error 5\n",fname,count);
          exit(-1);
        }
        buff[25] = ' ';
        int h,m,s,c;
        if (4!=sscanf(buff+17,"%d%d%d%d",&h,&m,&s,&c)) {
          fprintf(stderr,"File \"%s\", record %d: Error 6\n",fname,count);
          exit(-1);
        }
        h *= 60;
        h += m;
        h *= 60;
        h += s;
        h *= 100;
        h += c;
        ra = (360/24)*h / (double)(60*60*100);
      }
      
      buff[76]='\0';
      double dec;
      if (1!=sscanf(buff+64,"%lf",&dec)) {
        buff[40]='\0';
        if (buff[32]!=' ' || buff[35]!=' ' || buff[38]!='.') {
          fprintf(stderr,"File \"%s\", record %d: Error 7\n",fname,count);
          exit(-1);
        }
        buff[38] = ' ';
        int d,m,s,c;
        if (4!=sscanf(buff+30,"%d%d%d%d",&d,&m,&s,&c)) {
          fprintf(stderr,"File \"%s\", record %d: Error 8\n",fname,count);
          exit(-1);
        }
        d *= 60;
        d += m;
        d *= 60;
        d += s;
        d *= 10;
        d += c;
        if (buff[29]=='-') {
          d = -d;
        } else if (buff[29]!='+') {
          fprintf(stderr,"File \"%s\", record %d: Error 9\n",fname,count);
          exit(-1);
        }
        dec = d / (double)(60*60*10);
      }
      ChangeEpoch(2000.0-1991.25,ra,dec,pm_ra,pm_dec);

      buff[223]='\0';
      buff[236]='\0';
      double bt,vt;
      if (1!=sscanf(buff+217,"%lf",&bt) ||
          1!=sscanf(buff+230,"%lf",&vt)) {
        bt = vt = 9999.999;
      }

      double mag;
      buff[46]='\0';
      if (1!=sscanf(buff+41,"%lf",&mag)) {
        if (bt<1000 && vt<1000) {
          mag = vt -0.090*(bt-vt);  // Johnson photometry V
        } else {
            // no magnitude at all
          mag = 9999.999;
        }
      }

      double b_v;
      buff[251]='\0';
      if (1!=sscanf(buff+245,"%lf",&b_v)) {
        if (!ReadEmptyString(buff+245)) {
          fprintf(stderr,"File \"%s\", record %d: Error 11\n",fname,count);
          exit(-1);
        }
        if (bt<1000 && vt<1000) {
          b_v = 0.850*(bt-vt);      // Johnson photometry B-V
        } else {
          b_v = 0.73;
        }
      }

      buff[86]='\0';
      if (ReadEmptyString(buff+79)) plx = 0.0;
      else if (1!=sscanf(buff+79,"%lf",&plx)) {
        fprintf(stderr,"File \"%s\", record %d: Error 12\n",fname,count);
        exit(-1);
      }
      for (int i=0;i<12;i++) sp[i] = buff[435+i];
      for (int i=11;i>=0;i--) {
        if (sp[i] == ' ') sp[i] = '\0';
        else break;
      }
      const int rc = accu.addData(hip,component_ids,plx,sp,
                                  ra,dec,pm_ra,pm_dec,mag,b_v);
      if (rc < 0) {
          // never mind: propably no magnitude for Hiparcos star
//        fprintf(stderr,"File \"%s\", record %d: Error 13 %d %d \"%s\"\n",
//                fname,count,rc,hip,sp);
//        exit(-1);
      }
    } else {
      fprintf(stderr,"File \"%s\", record %d: wrong format\n",fname,count);
      exit(-1);
    }
    count++;
  }
  fclose(f);
  return count;
}



struct NOMAD_Record {
  int ra,spd,dev_ra,dev_spd;
  int pm_ra,pm_spd,dev_pm_ra,dev_pm_spd;
  int epoch_ra,epoch_spd;
  int b,v,r,j,h,k;
  int usno_id,_two_mass_id,yb6_id,ucac2_id,tycho2_id;
  int flags;
};

#define READ_SIZE 10000
static NOMAD_Record buff[READ_SIZE];

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
    count = fread(buff,sizeof(NOMAD_Record),READ_SIZE,f);
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
            mag = buff[i].r;
            b_v = 0;
          }
        } else {
          mag = buff[i].b;
          b_v = 0;
        }
      } else {
        mag = buff[i].v;
        if (buff[i].b >= 30000) {
          b_v = 0;
        } else {
          b_v = buff[i].b-buff[i].v;
        }
      }

      int nr_of_measurements = 0;
      if (buff[i].b < 30000) nr_of_measurements++;
      if (buff[i].v < 30000) nr_of_measurements++;
      if (buff[i].r < 30000) nr_of_measurements++;
//      if (buff[i].j < 30000) nr_of_measurements++;
//      if (buff[i].h < 30000) nr_of_measurements++;
//      if (buff[i].k < 30000) nr_of_measurements++;
      
#define AST_SRC 0x00000007
#define UBBIT 0x00001000
#define SPIKE 0x02000000
//USNO-B1.0 diffraction spike bit set
#define USEME 0x20000000

#define TMONLY 0x00800000
// Object found only in 2MASS cat
#define BSART 0x10000000
//Faint source is bright star artifact

      if (mag < 16500 && buff[i].tycho2_id==0 &&
          ((buff[i].flags&USEME) ||
           (((buff[i].flags&AST_SRC)!=1 ||
              (((buff[i].flags&UBBIT)==0)
               &&
                  (mag>14000 || nr_of_measurements>1) &&
                  (mag>13000 || nr_of_measurements>2)
                   )) &&
            ((buff[i].flags&SPIKE)==0) &&
            ((buff[i].flags&BSART)==0) &&
            ((buff[i].flags&TMONLY)==0)))) {
        if (accu.addStar(0,0,0,0,"",
                         buff[i].ra/(3600.0*1000.0), // degrees
                         (buff[i].spd-90*3600*1000)/(3600.0*1000.0), // degrees
                         0.1*buff[i].pm_ra,0.1*buff[i].pm_spd,
                         0.001*mag,0.001*b_v) < 0) {
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
  for (int c=0;tyc_names[c];c++) {
    n += ReadTyc2File(tyc_names[c],accu);
    printf("%s: %d records processed.\n",tyc_names[c],n);
  }
  for (int c=0;suppl_names[c];c++) {
    n += ReadTyc2SupplFile(suppl_names[c],accu);
    printf("%s: %d records processed.\n",suppl_names[c],n);
  }
  if (restrict_output_level_min <= MAX_HIP_LEVEL) {
    n = ReadHipFile(hip_name,accu);
    printf("%s: %d records processed.\n",hip_name,n);
    SqueezeHip();
  }
  
  for (int c=0;nomad_names[c];c++) {
    ReadNOMADFile(nomad_names[c],accu);
  }

  accu.writeOutput(output_file_names);

  return 0;
}
