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

#ifndef _CONSTELLATION_MGR_H_
#define _CONSTELLATION_MGR_H_

#include <vector>
#include "constellation.h"

class Constellation_mgr  
{
public:
    Constellation_mgr();
    virtual ~Constellation_mgr();
    int Read(FILE *, Star_mgr * _VouteCeleste);
    void Load(char *, Star_mgr * _VouteCeleste);
    void Draw();
    void Draw(char * Abr);
    void DrawName();
private:
    vector<constellation*> Liste;
};


extern Constellation_mgr * ConstellCeleste;   // Class to manage the constellation boundary and name


#endif // _CONSTELLATION_MGR_H_
