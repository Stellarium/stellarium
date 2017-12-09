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

#include "triangulate.h"

Triangulate::Triangulate(std::map<HtmID, PointSet::Point> *p)
{
    pmap = p;
}

void Triangulate::Reset()
{
    isvalid = false;
    vvertices.clear();
    vfaces.clear();
}

void Triangulate::AddPoint(HtmID id)
{
    INDI_UNUSED(id);
    isvalid = false;
}

XMLEle *Triangulate::toXML()
{
    XMLEle *root;
    std::vector<Face *>::iterator it;

    root = addXMLEle(NULL, "triangulation");

    for (it = vfaces.begin(); it != vfaces.end(); it++)
    {
        XMLEle *face;
        XMLEle *vertex;
        char pcdata[10];
        face   = addXMLEle(root, "face");
        vertex = addXMLEle(face, "vindex");
        snprintf(pcdata, sizeof(pcdata), "%d", pmap->at((*it)->v[0]).index);
        editXMLEle(vertex, pcdata);
        vertex = addXMLEle(face, "vindex");
        snprintf(pcdata, sizeof(pcdata), "%d", pmap->at((*it)->v[1]).index);
        editXMLEle(vertex, pcdata);
        vertex = addXMLEle(face, "vindex");
        snprintf(pcdata, sizeof(pcdata), "%d", pmap->at((*it)->v[2]).index);
        editXMLEle(vertex, pcdata);
    }

    return (root);
}

std::vector<Face *> Triangulate::getFaces()
{
    isvalid = true;
    return vfaces;
}

bool Triangulate::isValid()
{
    return isvalid;
}
