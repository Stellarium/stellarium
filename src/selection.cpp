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

 #include "selection.h"

 /*
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
*/

char * selection::get_info_string(void)
{
	if (type==0)  //Star
    {
		glColor3f(0.4f,0.7f,0.3f);
        sprintf(objectInfo,"Info : %s\nName :%s\nHip : %.4d\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nMag : %.2f",
            global.SelectedObject.CommonName==NULL ? "-" : global.SelectedObject.CommonName,
            global.SelectedObject.Name==NULL ? "-" : global.SelectedObject.Name,
            global.SelectedObject.HR,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Mag);
    }
    if (type==1)  //Nebulae
    {   glColor3f(0.4f,0.5f,0.8f);
        sprintf(objectInfo,"Name: %s\nNGC : %d\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nMag : %.2f",
            global.SelectedObject.Name,
            global.SelectedObject.NGCNum,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Mag);
    }
    if (type==2)  //Planet
    {   glColor3f(1.0f,0.3f,0.3f);
        sprintf(objectInfo,"Name: %s\nRA  : %.2dh %.2dm %.2fs\nDE  : %.2fdeg\nDistance : %.4f AU",
            global.SelectedObject.Name,
            global.SelectedObject.RAh,global.SelectedObject.RAm,global.SelectedObject.RAs,
            global.SelectedObject.DE*180/PI,
            global.SelectedObject.Distance);
    }
}
