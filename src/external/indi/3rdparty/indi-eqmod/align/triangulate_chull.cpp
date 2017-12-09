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

#include "triangulate_chull.h"

#include "chull.h"

TriangulateCHull::TriangulateCHull(std::map<HtmID, PointSet::Point> *p) : Triangulate::Triangulate(p)
{
    tVertex v;
    vnum     = 0;
    vertices = NULL;
    edges    = NULL;
    faces    = NULL;
    v        = MakeNullVertex();
    v->v[X]  = 0;
    v->v[Y]  = 0;
    v->v[Z]  = 0;
    v->vnum  = vnum++;
}

void TriangulateCHull::Reset()
{
    tVertex v;
    Triangulate::Reset();
    vnum = 0;
    //SHOULD Free everything
    vertices = NULL;
    edges    = NULL;
    faces    = NULL;
    v        = MakeNullVertex();
    v->v[X]  = 0;
    v->v[Y]  = 0;
    v->v[Z]  = 0;
    v->vnum  = vnum++;
}

void TriangulateCHull::AddPoint(HtmID id)
{
    tVertex v, vnext;
    PointSet::Point p;
    tFace f;
    Triangulate::AddPoint(id);
    //fprintf(stderr, "Triangulate addpoint: %ld\n", id);
    p = pmap->at(id);
    //fprintf(stderr, "Triangulate addpoint: point retrieved %d\n", p.index);
    v       = MakeNullVertex();
    v->v[X] = (int)(p.cx * 1000000);
    v->v[Y] = (int)(p.cy * 1000000);
    v->v[Z] = (int)(p.cz * 1000000);
    v->vnum = vnum++;
    //fprintf(stderr, "Triangulate addpoint: vertex created %d\n", v->vnum);
    vvertices.push_back(id);
    //fprintf(stderr, "Triangulate addpoint: vertex added (%d total)\n", vvertices.size());
    if (vnum < 4)
        return;
    if (vnum == 4)
    {
        DoubleTriangle();
        ConstructHull();
    }
    if (vnum > 4)
    {
        vnext = v->next;
        AddOne(v);
        CleanUp(&vnext);
    }
    vfaces.clear();
    f = faces;
    do
    {
        //fprintf(stderr, "Triangulate addpoint: adding face (%d total)\n", vfaces.size());
        //skip faces containing the origin vertex
        if ((f->vertex[0]->vnum == 0) || (f->vertex[1]->vnum == 0) || (f->vertex[2]->vnum == 0))
        {
            f = f->next;
            continue;
        }
        vfaces.push_back(new Face(vvertices.at(f->vertex[0]->vnum - 1), vvertices.at(f->vertex[1]->vnum - 1),
                                  vvertices.at(f->vertex[2]->vnum - 1)));
        //fprintf(stderr, "Triangulate addpoint: added face (%d total)\n", vfaces.size());
        f = f->next;
    } while (f != faces);
}

//XMLEle *TriangulateCHull::toXML()
//{
//}
