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

using namespace std;

class Nebula_mgr  
{
public:
    Nebula_mgr();
    virtual ~Nebula_mgr();
    int Read(char * font_fileName, char * fileName);
    void Draw(int names_ON, draw_utility * du);            // Draw all the Nebulaes
    int ReadTexture();
    stel_object * search(vec3_t Pos);             // Search the Nebulae by position
private:
    vector<Nebula*> Liste;              // list of Nebulaes*
    Nebula * Selectionnee;              // Selected nebulae
};

#endif // _NEBULA_MGR_H_
