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

#ifndef _SELECTION_H_
#define _SELECTION_H_

typedef struct
{
    char * Name;
    double Distance;
}planet_info;

typedef struct
{
    char * Name;
    unsigned int HR;
	unsigned int Hiparcos;
    char * CommonName;
    vec3_t RGB;
}star_info;

typedef struct
{
    char * Name;
    unsigned int MessierNum;
    unsigned int NGCNum;
    float Size;
}nebula_info;

class selection
{
public:
	selection();
    ~selection();
	void update(void);
	void draw_pointer(void);
	char * get_info_string(void);
	char get_type(void);
	Vec3d get_equ_pos(void) {return equ_pos;}
private:
	unsigned int enabled;
    char type;
	nebula_info nebula;
	star_info star;
	planet_info planet;
    float mag;
	Vec3d equ_pos;
	char * info_string;
};

extern selection selected_object; // Selection instance used in stellarium

#endif // _SELECTION_H_
