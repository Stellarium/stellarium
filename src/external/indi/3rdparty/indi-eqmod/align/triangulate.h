/* Copyright 2013 Geehalel (geehalel AT gmail DOT com) */
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

#include "pointset.h"

class Face
{
  public:
    Face(HtmID v0, HtmID v1, HtmID v2) : v(3, 0)
    {
        v[0] = v0;
        v[1] = v1;
        v[2] = v2;
    }
    std::vector<HtmID> v;
};

class Triangulate
{
    //  typedef struct Face {
    //  HtmID v[3];
    //} Face;
  public:
    Triangulate(std::map<HtmID, PointSet::Point> *p);
    virtual void Reset();
    virtual void AddPoint(HtmID id);
    virtual XMLEle *toXML();
    virtual std::vector<Face *> getFaces();
    virtual bool isValid();

  protected:
    std::map<HtmID, PointSet::Point> *pmap;
    std::vector<HtmID> vvertices;
    std::vector<Face *> vfaces;
    bool isvalid;
};
