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

// Module to manage User Interface

#ifndef _STELLARIUM_UI_H
#define _STELLARIUM_UI_H

void initUi(void);
void clearUi(void);
void renderUi();
void HandleClic(int x, int y, int state, int button);
void HandleMove(int x, int y);
void HandleNormalKey(unsigned char key, int state);

#endif  //_STELLARIUM_UI_H
