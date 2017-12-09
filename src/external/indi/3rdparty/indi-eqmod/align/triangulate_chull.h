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

#include "triangulate.h"

class TriangulateCHull : public Triangulate
{
  public:
    TriangulateCHull(std::map<HtmID, PointSet::Point> *p);
    void Reset();
    void AddPoint(HtmID id);
    //XMLEle *toXML();

  private:
    int vnum;
};
