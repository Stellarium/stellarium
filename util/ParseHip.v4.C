// Author and Copyright: Johannes Gajdosik, 2007
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
// g++ -O2 ParseHip.C -o ParseHip


// change catalogue locations according to your needs:

const char *hip_main_name = "cdsweb.u-strasbg.fr/ftp/cats/I/239m/hip_main_r.dat";
const char *h_dm_com_name = "cdsweb.u-strasbg.fr/ftp/cats/I/239m/h_dm_com_r.dat";


#define NR_OF_HIP 120416


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <map>
#include <list>
#include <string>
#include <iostream>
#include <assert.h>
#undef NDEBUG

using namespace std;

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



short int Mag2Int(double mag) {
  int rval = (int)trunc(0.5+1000.0*mag);
  if (rval > 30000 || rval < -30000) rval = 30000;
  return (short int)rval;
}


/*
Hip:
1) only 1 Star has no Vmag: no mag at all
2) most stars have Vmag and B_V
3) many others have both BTmag and Tmag
*/

class Accumulator {
public:
  void print(void);
  void addStar(int hip,int tyc1,int tyc2,int tyc3,
               const char *component_ids,int VarFlag,
               double ra,double dec,double Plx,double pm_ra,double pm_dec,
               double BTmag,double VTmag,double Hpmag,
               double Vmag,double B_V,double V_I,
               double V_I_red,const char *sp_type) {
    map[hip].push_back(RawStar(hip,tyc1,tyc2,tyc3,
                               component_ids,VarFlag,
                               ra,dec,Plx,pm_ra,pm_dec,
                               BTmag,VTmag,Hpmag,
                               Vmag,B_V,V_I,
                               V_I_red,sp_type));
  }
private:
  struct RawStar {
    int hip;
    int tyc1,tyc2,tyc3;
    string component_ids;
    int VarFlag;
    double ra,dec;
    double Plx;
    double pm_ra,pm_dec;
    short int BTmag,VTmag,Hpmag,Vmag,B_V,V_I,V_I_red;
    string sp_type;
    RawStar(void) {}
    RawStar(int hip,int tyc1,int tyc2,int tyc3,
            const char *component_ids,int VarFlag,
            double ra,double dec,double Plx,double pm_ra,double pm_dec,
            double BTmag,double VTmag,double Hpmag,
            double Vmag,double B_V,double V_I,
            double V_I_red,const char *sp_type)
     :hip(hip),tyc1(tyc1),tyc2(tyc2),tyc3(tyc3),
      component_ids(component_ids),VarFlag(VarFlag),ra(ra),dec(dec),Plx(Plx),
      pm_ra(pm_ra),pm_dec(pm_dec),
      BTmag(Mag2Int(BTmag)),VTmag(Mag2Int(VTmag)),Hpmag(Mag2Int(Hpmag)),
      Vmag(Mag2Int(Vmag)),B_V(Mag2Int(B_V)),V_I(Mag2Int(V_I)),
      V_I_red(Mag2Int(V_I_red)),sp_type(sp_type) {}
    bool operator<(const RawStar &r) {return (component_ids<r.component_ids);}
    void mergeSameComponents(const RawStar &r);
    void mergeFromMulti(const RawStar &r,bool only_one_component);
  };
  static void PrintSingleStar(const RawStar &r);
  typedef std::list<RawStar> RawStarList;
  typedef std::map<int,RawStarList> RawStarMap;
  RawStarMap map;
};

void Accumulator::RawStar::mergeSameComponents(const Accumulator::RawStar &r) {
  if (tyc1==0||tyc2==0||tyc3==0) {
    tyc1=r.tyc1;tyc2=r.tyc2;tyc3=r.tyc3;
      // prefer hiparcos ra,dec:
    ra = r.ra;
    dec = r.dec;
  }
  if (VarFlag<0) VarFlag = r.VarFlag;
  if ((Plx==0.0||Plx<=-9999) && r.Plx>0) Plx = r.Plx;
  if (pm_ra==0.0 && pm_dec==0.0) {pm_ra=r.pm_ra;pm_dec=r.pm_dec;}
  if (BTmag>=30000) BTmag=r.BTmag;
  if (VTmag>=30000) VTmag=r.VTmag;
  if (Hpmag>=30000) Hpmag=r.Hpmag;
  if (Vmag>=30000) Vmag=r.Vmag;
  if (B_V>=30000) B_V=r.B_V;
  if (V_I>=30000) V_I=r.V_I;
  if (V_I_red>=30000) V_I_red=r.V_I_red;
  if (sp_type.empty()) sp_type=r.sp_type;
}

void Accumulator::RawStar::mergeFromMulti(const Accumulator::RawStar &r,
                                          bool only_one_component) {
  if ((Plx==0.0||Plx<=-9999) && r.Plx>0) Plx = r.Plx;
  if (pm_ra==0.0 && pm_dec==0.0) {pm_ra=r.pm_ra;pm_dec=r.pm_dec;}
  if (sp_type.empty() && (component_ids[0]=='A'||only_one_component)) {
    sp_type=r.sp_type;
  }
  if (VarFlag<0 && (component_ids[0]=='A'||only_one_component)) {
    VarFlag=r.VarFlag;
  }
}





bool SpToBv(const string &sp_type,short int &b_v) {
      // try to deduce color from sp_type
      // http://adsabs.harvard.edu/abs/1945CMWCI.712...14.

      //http://domeofthesky.com/clicks/bv.html
      //Color Index 	Spectral Type 	Approximate Color
      //1.41 	M0 	Red
      //0.82 	K0 	Orange
      //0.59 	G0 	Yellow
      //0.31 	F0 	Yellowish
      //0.00 	A0 	White
      //-0.29 	B0 	Blue
  if (sp_type.empty()) return false;
  switch (sp_type[0]) {
    case 'C': b_v = 3400;return true;
    case 'T': b_v = 3400;return true;
    case 'R': b_v = 3400;return true;
    case 'N': b_v = 3400;return true;
    case 'M': b_v = 1410;return true;
    case 'K': b_v =  820;return true;
    case 'G': b_v =  590;return true;
    case 'F': b_v =  310;return true;
    case 'A': b_v =    0;return true;
    case 'B': b_v =  -29;return true;
    case 'O': b_v =  -40;return true;
    case 'W': b_v =  -50;return true;
  }
  return false;
}  


void PrintStar(int hip,int tyc1,int tyc2,int tyc3,
               const string &comp_ids,int VarFlag,
               double ra,double dec,double Plx,double pm_ra,double pm_dec,
               short int mag,short int b_v,string sp_type) {
  for (int i=0;i<sp_type.size();i++) if (sp_type[i]==' ') sp_type[i]='_';
  printf("%6d %5d %5d %d %2s %d %20.15f %20.15f %18.15e %18.11f %18.11f %5d %5d %s\n",
         hip,tyc1,tyc2,tyc3,
         comp_ids.empty()?"?":comp_ids.c_str(),
         ((VarFlag<0)?0:VarFlag),
         ra,dec,
         (Plx<0.0)?0.0:Plx,
         pm_ra,pm_dec,mag,b_v,
         sp_type.empty()?"?":sp_type.c_str());
}

void Accumulator::PrintSingleStar(const RawStar &r) {
  bool do_print = true;
  short int mag = r.Vmag;
  short int b_v = r.B_V;
  if (mag>=30000) {
    if (r.BTmag<30000 && r.VTmag<30000) {
      mag = (short int)floor(0.5+r.VTmag -0.090*(r.BTmag-r.VTmag));  // Johnson photometry V
      if (b_v>=30000) {
        b_v = (short int)floor(0.5+0.850*(r.BTmag-r.VTmag)); // Johnson photometry B-V
      }
    } else {
      if (r.BTmag<30000) {
        if (!SpToBv(r.sp_type,b_v)) b_v = -500; // -0.5
        mag = (short int)floor(0.5+r.BTmag -1.090*b_v/0.85);
      } else if (r.VTmag<30000) {
        if (!SpToBv(r.sp_type,b_v)) b_v = 3499; // +3.499
        mag = (short int)floor(0.5+r.VTmag -0.090*b_v/0.85);
      } else if (r.Hpmag<30000) {
        if (!SpToBv(r.sp_type,b_v)) b_v = 0;    // +0.0
        mag = r.Hpmag;
      } else {
        // no mag at all: forget this star
        do_print = false;
      }
    }
  } else {
      // mag ok.
    if (b_v>=30000) {
      if (r.BTmag<30000 && r.VTmag<30000) {
        b_v = (short int)floor(0.850*(r.BTmag-r.VTmag)); // Johnson photometry B-V
      } else if (!SpToBv(r.sp_type,b_v)) {
        if (r.V_I<30000) {
          b_v = r.V_I; // desperate
        } else if (r.V_I<30000) {
          b_v = r.V_I_red; // even more desperate
        } else {
          b_v = 0; // just no idea
        }
      }
    } else {
      // best case
    }
  }
  if (do_print && b_v<3500 && b_v>-500)
    PrintStar(r.hip,r.tyc1,r.tyc2,r.tyc3,r.component_ids,
              r.VarFlag,
              r.ra,r.dec,
              r.Plx,
              r.pm_ra,r.pm_dec,
              mag,b_v,r.sp_type);
}

void Accumulator::print(void) {
  for (RawStarMap::iterator mit(map.begin());mit!=map.end();mit++) {
    if (mit->first == 0) {
      for (RawStarList::iterator it(mit->second.begin());
           it!=mit->second.end();it++) {
          // non-hip
        PrintSingleStar(*it);
      }
    } else {
      int list_size = mit->second.size();
      if (list_size == 1) {
          // ordinary hip star
        PrintSingleStar(mit->second.front());
      } else {
        RawStar last_star;
        RawStarList tmp_list;
          // remove double entries
        mit->second.sort();
        RawStarList::iterator it1(mit->second.begin());
        RawStarList::iterator it2(it1);
        it2++;
        while (it2!=mit->second.end()) {
          if (it1->component_ids == it2->component_ids) {
            it1->mergeSameComponents(*it2);
            mit->second.erase(it2);
          } else {
            it1++;
          }
          it2 = it1;
          it2++;
        }
        list_size = mit->second.size();
        if (list_size == 1) {
            // just 1 star
          PrintSingleStar(mit->second.front());
        } else {
            // filter out single components:
          RawStarList single_comp,multi_comp;
          for (it1=mit->second.begin();it1!=mit->second.end();it1++) {
            if (it1->component_ids.empty()) {
              multi_comp.push_back(*it1);
            } else if (it1->component_ids.size()==1) {
              single_comp.push_back(*it1);
            } else {
              multi_comp.push_back(*it1);
            }
          }
          if (single_comp.empty()) {
            assert(0);
          }
          for (it1=single_comp.begin();it1!=single_comp.end();it1++) {
            for (it2=multi_comp.begin();it2!=multi_comp.end();it2++) {
              if (it2->component_ids.empty() ||
                  it2->component_ids.find(it1->component_ids[0])!=string::npos) {
                it1->mergeFromMulti(*it2,(single_comp.size()==1));
                  // mark *it2 so that it will not be printed:
                it2->component_ids += "~";
              }
            }
            PrintSingleStar(*it1);
          }
            // erase stars that have been merged
          it2=multi_comp.begin();
          while (it2!=multi_comp.end()) {
            if (it2->component_ids.find('~')!=string::npos) {
              it1=it2;
              it2++;
              multi_comp.erase(it1);
            } else {
              it2++;
            }
          }
          
          if (!multi_comp.empty()) {
            if (multi_comp.size() == 1) {
              PrintSingleStar(multi_comp.front());
            } else {
              for (it2=multi_comp.begin();it2!=multi_comp.end();it2++) {
                if (it2->component_ids.empty()) {
                  cerr << "bad multi-comps star: " << it2->hip << endl;
                  assert(0);
                }
                PrintSingleStar(*it2);
              }
            }
          }
        }
      }
    }
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

      accu.addStar(hip,tyc1,tyc2,tyc3,component_ids,-9999,
                   ra,dec,-9999,pma,pmd,bt,vt,
                   -9999,-9999,-9999,-9999,-9999,"");
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
      double hp = -9999;
      switch (buff[81]) {
        case ' ':
          if (bt<-10 || vt<-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 12\n",fname,count);
            exit(-1);
          }
          break;
        case 'B':
          if (bt<-10 || vt>-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 13\n",fname,count);
            exit(-1);
          }
          vt = -9999;
          break;
        case 'V':
          if (bt>-10 || vt<-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 14\n",fname,count);
            exit(-1);
          }
          bt = -9999;
          break;
        case 'H':
          if (bt>-10 || vt<-10) {
            fprintf(stderr,"File \"%s\", record %d: Error 14a\n",fname,count);
            exit(-1);
          }
          hp = vt;
          vt = -9999;
          bt = -9999;
          break;
        default:
          fprintf(stderr,"File \"%s\", record %d: Error 15\n",fname,count);
          exit(-1);
      }
      
      if (buff[13]!='H') {
      accu.addStar(hip,tyc1,tyc2,tyc3,component_ids,-9999,
                   ra,dec,-9999,pma,pmd,bt,vt,hp,
                   -9999,-9999,-9999,-9999,"");
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



#define ASSERT(cond) if(!(cond)) \
{fprintf(stderr,"File \"%s\", record %d\n",fname,count);assert(0);}

int ReadHipMain(const char *fname,Accumulator &accu) {
  FILE *f = fopen(fname,"rb");
  assert(f);
  int count = 0;
  char buff[466];
  while (465==fread(buff,1,465,f)) {
    ASSERT(buff[  1]=='|' && buff[ 14]=='|' && buff[ 16]=='|');
    ASSERT(buff[ 28]=='|' && buff[ 40]=='|' && buff[ 46]=='|');
    ASSERT(buff[ 48]=='|' && buff[ 50]=='|' && buff[ 63]=='|');
    ASSERT(buff[ 76]=='|' && buff[ 78]=='|' && buff[ 86]=='|');
    ASSERT(buff[ 95]=='|' && buff[104]=='|' && buff[111]=='|');
    ASSERT(buff[118]=='|' && buff[125]=='|' && buff[132]=='|');
    ASSERT(buff[139]=='|' && buff[145]=='|' && buff[151]=='|');
    ASSERT(buff[157]=='|' && buff[163]=='|' && buff[169]=='|');
    ASSERT(buff[175]=='|' && buff[181]=='|' && buff[181]=='|');
    ASSERT(buff[193]=='|' && buff[199]=='|' && buff[203]=='|');
    ASSERT(buff[209]=='|' && buff[216]=='|' && buff[223]=='|');
    ASSERT(buff[229]=='|' && buff[236]=='|' && buff[242]=='|');
    ASSERT(buff[244]=='|' && buff[251]=='|' && buff[257]=='|');
    ASSERT(buff[259]=='|' && buff[264]=='|' && buff[269]=='|');
    ASSERT(buff[271]=='|' && buff[273]=='|' && buff[281]=='|');
    ASSERT(buff[288]=='|' && buff[294]=='|' && buff[298]=='|');
    ASSERT(buff[300]=='|' && buff[306]=='|' && buff[312]=='|');
    ASSERT(buff[320]=='|' && buff[322]=='|' && buff[324]=='|');
    ASSERT(buff[326]=='|' && buff[337]=='|' && buff[339]=='|');
    ASSERT(buff[342]=='|' && buff[345]=='|' && buff[347]=='|');
    ASSERT(buff[349]=='|' && buff[351]=='|' && buff[354]=='|');
    ASSERT(buff[358]=='|' && buff[366]=='|' && buff[372]=='|');
    ASSERT(buff[378]=='|' && buff[383]=='|' && buff[385]=='|');
    ASSERT(buff[387]=='|' && buff[389]=='|' && buff[396]=='|');
    ASSERT(buff[407]=='|' && buff[418]=='|' && buff[429]=='|');
    ASSERT(buff[434]=='|' && buff[461]=='|' && buff[464]=='\n');

    buff[  1]='\0'; buff[ 14]='\0'; buff[ 16]='\0';
    buff[ 28]='\0'; buff[ 40]='\0'; buff[ 46]='\0';
    buff[ 48]='\0'; buff[ 50]='\0'; buff[ 63]='\0';
    buff[ 76]='\0'; buff[ 78]='\0'; buff[ 86]='\0';
    buff[ 95]='\0'; buff[104]='\0'; buff[111]='\0';
    buff[118]='\0'; buff[125]='\0'; buff[132]='\0';
    buff[139]='\0'; buff[145]='\0'; buff[151]='\0';
    buff[157]='\0'; buff[163]='\0'; buff[169]='\0';
    buff[175]='\0'; buff[181]='\0'; buff[181]='\0';
    buff[193]='\0'; buff[199]='\0'; buff[203]='\0';
    buff[209]='\0'; buff[216]='\0'; buff[223]='\0';
    buff[229]='\0'; buff[236]='\0'; buff[242]='\0';
    buff[244]='\0'; buff[251]='\0'; buff[257]='\0';
    buff[259]='\0'; buff[264]='\0'; buff[269]='\0';
    buff[271]='\0'; buff[273]='\0'; buff[281]='\0';
    buff[288]='\0'; buff[294]='\0'; buff[298]='\0';
    buff[300]='\0'; buff[306]='\0'; buff[312]='\0';
    buff[320]='\0'; buff[322]='\0'; buff[324]='\0';
    buff[326]='\0'; buff[337]='\0'; buff[339]='\0';
    buff[342]='\0'; buff[345]='\0'; buff[347]='\0';
    buff[349]='\0'; buff[351]='\0'; buff[354]='\0';
    buff[358]='\0'; buff[366]='\0'; buff[372]='\0';
    buff[378]='\0'; buff[383]='\0'; buff[385]='\0';
    buff[387]='\0'; buff[389]='\0'; buff[396]='\0';
    buff[407]='\0'; buff[418]='\0'; buff[429]='\0';
    buff[434]='\0'; buff[461]='\0'; buff[464]='\0';

    int hip;
    ASSERT(1==sscanf(buff+2,"%d",&hip));
    ASSERT(1<=hip && hip<=NR_OF_HIP);

    double Vmag = 10000.0;
    if (!ReadEmptyString(buff+41)) ASSERT(1==sscanf(buff+41,"%lf",&Vmag));
    int VarFlag = 0;
    if (!ReadEmptyString(buff+47)) ASSERT(1==sscanf(buff+47,"%d",&VarFlag));

    double ra;
    if (1!=sscanf(buff+51,"%lf",&ra)) {
    //if (true) {
      ASSERT(buff[19]==' ' && buff[22]==' ' && buff[25]=='.');
      buff[25] = ' ';
      int h,m,s,c;
      ASSERT(4==sscanf(buff+17,"%d%d%d%d",&h,&m,&s,&c));
      h *= 60;
      h += m;
      h *= 60;
      h += s;
      h *= 100;
      h += c;
      ra = (360/24)*h / (double)(60*60*100);
    }

    double dec;
    if (1!=sscanf(buff+64,"%lf",&dec)) {
    //if (true) {
      ASSERT(buff[32]==' ' && buff[35]==' ' && buff[38]=='.');
      buff[38] = ' ';
      int d,m,s,c;
      ASSERT(4==sscanf(buff+30,"%d%d%d%d",&d,&m,&s,&c));
      d *= 60;
      d += m;
      d *= 60;
      d += s;
      d *= 10;
      d += c;
      if (buff[29]=='-') {
        d = -d;
      } else ASSERT(buff[29]=='+');
      dec = d / (double)(60*60*10);
    }

    double Plx = 0.0;
    if (!ReadEmptyString(buff+79)) ASSERT(1==sscanf(buff+79,"%lf",&Plx));
    double pm_ra = 0.0;
    if (!ReadEmptyString(buff+87)) ASSERT(1==sscanf(buff+87,"%lf",&pm_ra));
    double pm_dec = 0.0;
    if (!ReadEmptyString(buff+96)) ASSERT(1==sscanf(buff+96,"%lf",&pm_dec));

    //ChangeEpoch(2000.0-1991.25,ra,dec,pm_ra,pm_dec);

    int hip2;
    ASSERT(1==sscanf(buff+210,"%d",&hip2));
    assert(hip==hip2);

    double BTmag = 10000.0;
    if (!ReadEmptyString(buff+217)) ASSERT(1==sscanf(buff+217,"%lf",&BTmag));
    double VTmag = 10000.0;
    if (!ReadEmptyString(buff+230)) ASSERT(1==sscanf(buff+230,"%lf",&VTmag));

    double B_V = 10000.0;
    if (!ReadEmptyString(buff+245)) ASSERT(1==sscanf(buff+245,"%lf",&B_V));
    double V_I = 10000.0;
    if (!ReadEmptyString(buff+260)) ASSERT(1==sscanf(buff+260,"%lf",&V_I));
    
    double Hpmag = 10000.0;
    if (!ReadEmptyString(buff+274)) ASSERT(1==sscanf(buff+274,"%lf",&Hpmag));

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

    double V_I_red = 10000.0;
    if (!ReadEmptyString(buff+430)) ASSERT(1==sscanf(buff+430,"%lf",&V_I_red));
      
    char SpType[27];
    for (int i=0;i<26;i++) SpType[i] = buff[435+i];
    SpType[26] = '\0';
    for (int i=26;i>=0;i--) {
      if (SpType[i] != ' ') break;
      SpType[i] = '\0';
    }

      // see errata.htx:
    if (hip==85069) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"B9/B9.5II/III");
    } else if (hip==90062) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"B0/B0.5II/III");
    } else if (hip==4266 || hip==14055 || hip==21392 ||
               hip==29564 || hip==88170 || hip==113840) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"M");
    } else if (hip==22767) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"A0");
    } else if (hip==24548) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"F8");
    } else if (hip==35015) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"Be");
    } else if (hip==40765) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"G1V");
    } else if (hip==83404) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"B7/B8Ib/II");
    } else if (hip==105680) {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,"Be");
    } else if (hip==114110 || hip==114176) {
      //non-real
    } else {
      accu.addStar(hip,0,0,0,component_ids,VarFlag,
                   ra,dec,Plx,pm_ra,pm_dec,
                   BTmag,VTmag,Hpmag,
                   Vmag,B_V,V_I,
                   V_I_red,SpType);
    }
/*
    if (Vmag>30.0) {
      printf("VERY STRANGE Hip Star: %5d %2s|%5d %5d %5d|%5d %5d %5d %5d|%s\n",
           hip,component_ids,
           Mag2Int(BTmag),Mag2Int(VTmag),Mag2Int(Hpmag),
           Mag2Int(Vmag),Mag2Int(B_V),Mag2Int(V_I),Mag2Int(V_I_red),
           SpType);
    } else if (B_V<30.0) {
      // very ok
    } else if (BTmag<30.0 && VTmag<30.0) {
      // still ok
    } else if (SpType[0] && strchr("WOBAFGKMNRTC",SpType[0])) {
      // try to deduce color from SpType
      // http://adsabs.harvard.edu/abs/1945CMWCI.712...14.

      //http://domeofthesky.com/clicks/bv.html
      //Color Index 	Spectral Type 	Approximate Color
      //1.41 	M0 	Red
      //0.82 	K0 	Orange
      //0.59 	G0 	Yellow
      //0.31 	F0 	Yellowish
      //0.00 	A0 	White
      //-0.29 	B0 	Blue

// http://www.daviddarling.info/encyclopedia/C/color_index.html
// The system is defined so that for a star of spectral type A0
// (equivalent to a surface temperature of 9,200 K), B - V = U - B = 0.
// The full range runs from about -0.4 (blue) to +2.0 (red carbon stars).

//http://adsbit.harvard.edu/cgi-bin/nph-iarticle_query?bibcode=1979PASP...91..589B
    } else if (V_I<30.0) {

      assert(0);
    } else if (V_I_red<30.0) {
      // I will try to do what I can
    } else {
      assert(0);
    }
*/



//          mag = vt -0.090*(bt-vt);  // Johnson photometry V
//          b_v = 0.850*(bt-vt);      // Johnson photometry B-V


    count++;
  }
  fclose(f);
  return count;
}




int ReadHDmCom(const char *fname,Accumulator &accu) {
  FILE *f = fopen(fname,"rb");
  assert(f);
  int count = 0;
  char buff[240];
  while (239==fread(buff,1,239,f)) {
    ASSERT(buff[ 10]=='|' && buff[ 12]=='|' && buff[ 14]=='|');
    ASSERT(buff[ 16]=='|' && buff[ 18]=='|' && buff[ 20]=='|');
    ASSERT(buff[ 22]=='|' && buff[ 25]=='|' && buff[ 28]=='|');
    ASSERT(buff[ 31]=='|' && buff[ 36]=='|' && buff[ 39]=='|');
    ASSERT(buff[ 41]=='|' && buff[ 48]=='|' && buff[ 55]=='|');
    ASSERT(buff[ 61]=='|' && buff[ 68]=='|' && buff[ 74]=='|');
    ASSERT(buff[ 81]=='|' && buff[ 87]=='|' && buff[100]=='|');
    ASSERT(buff[113]=='|' && buff[121]=='|' && buff[130]=='|');
    ASSERT(buff[139]=='|' && buff[146]=='|' && buff[153]=='|');
    ASSERT(buff[160]=='|' && buff[167]=='|' && buff[174]=='|');
    ASSERT(buff[176]=='|' && buff[184]=='|' && buff[193]=='|');
    ASSERT(buff[202]=='|' && buff[209]=='|' && buff[212]=='|');
    ASSERT(buff[238]=='\n');

    buff[ 10]='\0'; buff[ 12]='\0'; buff[ 14]='\0';
    buff[ 16]='\0'; buff[ 18]='\0'; buff[ 20]='\0';
    buff[ 22]='\0'; buff[ 25]='\0'; buff[ 28]='\0';
    buff[ 31]='\0'; buff[ 36]='\0'; buff[ 39]='\0';
    buff[ 41]='\0'; buff[ 48]='\0'; buff[ 55]='\0';
    buff[ 61]='\0'; buff[ 68]='\0'; buff[ 74]='\0';
    buff[ 81]='\0'; buff[ 87]='\0'; buff[100]='\0';
    buff[113]='\0'; buff[121]='\0'; buff[130]='\0';
    buff[139]='\0'; buff[146]='\0'; buff[153]='\0';
    buff[160]='\0'; buff[167]='\0'; buff[174]='\0';
    buff[176]='\0'; buff[184]='\0'; buff[193]='\0';
    buff[202]='\0'; buff[209]='\0'; buff[212]='\0';
    buff[238]='\0';

    char comp_ids[2] = {buff[40],'\0'};

    int hip;
    ASSERT(1==sscanf(buff+42,"%d",&hip));
    ASSERT(1<=hip && hip<=NR_OF_HIP);

    double Hp = 10000.0;
    if (!ReadEmptyString(buff+49)) ASSERT(1==sscanf(buff+49,"%lf",&Hp));
    double BT = 10000.0;
    if (!ReadEmptyString(buff+62)) ASSERT(1==sscanf(buff+62,"%lf",&BT));
    double VT = 10000.0;
    if (!ReadEmptyString(buff+75)) ASSERT(1==sscanf(buff+75,"%lf",&VT));

    double ra;
    ASSERT(1==sscanf(buff+88,"%lf",&ra));
    double dec;
    ASSERT(1==sscanf(buff+101,"%lf",&dec));
    
    double Plx = 0.0;
    if (!ReadEmptyString(buff+114)) ASSERT(1==sscanf(buff+114,"%lf",&Plx));
    double pm_ra = 0.0;
    if (!ReadEmptyString(buff+122)) ASSERT(1==sscanf(buff+122,"%lf",&pm_ra));
    double pm_dec = 0.0;
    if (!ReadEmptyString(buff+131)) ASSERT(1==sscanf(buff+131,"%lf",&pm_dec));

    //ChangeEpoch(2000.0-1991.25,ra,dec,pm_ra,pm_dec);

    accu.addStar(hip,0,0,0,comp_ids,-9999,
                 ra,dec,Plx,pm_ra,pm_dec,
                 BT,VT,Hp,-9999,-9999,-9999,-9999,"");

    count++;
  }
  fclose(f);
  return count;
}

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

const char *suppl_names[]={
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/suppl_1.dat",
  0, // suppl_2 contains bad stars from Tyc1
  "cdsweb.u-strasbg.fr/ftp/cats/I/259/suppl_2.dat",
  0
};


int main(int argc,char *argv[]) {
  Accumulator accu;
  int n;
  n = ReadHDmCom(h_dm_com_name,accu);
  fprintf(stderr,"%s: %d records processed.\n",h_dm_com_name,n);
  n = ReadHipMain(hip_main_name,accu);
  fprintf(stderr,"%s: %d records processed.\n",hip_main_name,n);
  for (int c=0;tyc_names[c];c++) {
    n += ReadTyc2File(tyc_names[c],accu);
    fprintf(stderr,"%s: %d records processed.\n",tyc_names[c],n);
  }
  // no supplement files
  /*
  for (int c=0;suppl_names[c];c++) {
    n += ReadTyc2SupplFile(suppl_names[c],accu);
    fprintf(stderr,"%s: %d records processed.\n",suppl_names[c],n);
  }
  */
  
  accu.print();
  return 0;
}
