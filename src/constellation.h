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

#ifndef _CONSTELLATION_H_
#define _CONSTELLATION_H_

#include "star.h"
#include "star_mgr.h"
#include "stellarium.h"
#include "s_utility.h"
#include "s_font.h"

class constellation  
{
    friend class Constellation_mgr;
public:
    constellation();
    virtual ~constellation();
    int Read(FILE *, Star_mgr * _VouteCeleste);
    void Draw();
    void DrawSeule();
    void DrawName();
    void ComputeName();
private:
    char * Name;
    char Abreviation[4];
    char * Inter;
    float Xnom,Ynom,Znom;
    double x_c,y_c;
    unsigned int NbSegments;
    bool inFov;
    Star ** Asterism;
};

#endif // _CONSTELLATION_H_
