/* Copyright 2012 Geehalel (geehalel AT gmail DOT com) */
/* This file is part of the Skywatcher Protocol INDI driver.

    The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "htm.h"

#include <map>
#include <set>
#include <vector>

// to get access to lat/long data
#include <inditelescope.h>

typedef struct AlignData
{
    double lst;
    double jd;
    double targetRA, targetDEC;
    double telescopeRA, telescopeDEC;
} AlignData;

//class Triangulate;
class TriangulateCHull;
class Face;

class PointSet
{
  public:
    typedef struct Point
    {
        int index;
        HtmID htmID;
        HtmName htmname;
        double celestialALT, celestialAZ, telescopeALT, telescopeAZ;
        double cx, cy, cz;
        double tx, ty, tz;
        AlignData aligndata;
    } Point;
    typedef struct Distance
    {
        HtmID htmID;
        double value;
    } Distance;
    typedef enum PointFilter { None, SameQuadrant } PointFilter;
    PointSet(INDI::Telescope *);
    const char *getDeviceName();
    void AddPoint(AlignData aligndata, struct ln_lnlat_posn *pos);
    Point *getPoint(HtmID htmid);
    int getNbPoints();
    int getNbTriangles();
    bool isInitialized();
    void Init();
    void Reset();
    char *LoadDataFile(const char *filename);
    char *WriteDataFile(const char *filename);
    XMLEle *toXML();
    void setBlobData(IBLOBVectorProperty *bp);
    void setPointBlobData(IBLOB *blob);
    void setTriangulationBlobData(IBLOB *blob);
    std::set<Distance, bool (*)(Distance, Distance)> *ComputeDistances(double alt, double az, PointFilter filter,
                                                                       bool ingoto);
    std::vector<HtmID> findFace(double currentRA, double currentDEC, double jd, double pointalt, double pointaz,
                                ln_lnlat_posn *position, bool ingoto);
    double lat, lon, alt;
    double range24(double r);
    double range360(double r);
    // should be elsewhere
    void AltAzFromRaDec(double ra, double dec, double jd, double *alt, double *az, struct ln_lnlat_posn *pos);
    void AltAzFromRaDecSidereal(double ra, double dec, double lst, double *alt, double *az, struct ln_lnlat_posn *pos);
    void RaDecFromAltAz(double alt, double az, double jd, double *ra, double *dec, struct ln_lnlat_posn *pos);
    double scalarTripleProduct(Point *p, Point *e1, Point *e2, bool ingoto);
    bool isPointInside(Point *p, std::vector<HtmID> f, bool ingoto);

  protected:
  private:
    XMLEle *PointSetXmlRoot;
    std::map<HtmID, Point> *PointSetMap;
    bool PointSetInitialized;
    TriangulateCHull *Triangulation;
    Face *currentFace;
    std::vector<HtmID> current;
    // to get access to lat/long data
    INDI::Telescope *telescope;
    // from align data file
    struct ln_lnlat_posn *lnalignpos;
    friend class TriangulateCHull;
};
