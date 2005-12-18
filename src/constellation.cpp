/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include <iostream>
#include <algorithm>
#include "constellation.h"

bool Constellation::gravity_label = false;

Vec3f Constellation::line_color = Vec3f(.4,.4,.8);
Vec3f Constellation::label_color = Vec3f(.4,.4,.8);
Vec3f Constellation::boundary_color = Vec3f(0.8,0.3,0.3);
bool Constellation::singleSelected = false;

Constellation::Constellation() : asterism(NULL), art_tex(NULL)
{
}

Constellation::~Constellation()
{   
	if (asterism) delete[] asterism;
    asterism = NULL;
    
	if (art_tex) delete art_tex;
	art_tex = NULL;
}

// Read Constellation data record and grab cartesian positions of stars
// returns false if can't parse record
bool Constellation::read(const string& record, HipStarMgr * _VouteCeleste)
{   
	unsigned int HP;

	abbreviation.clear();
	nb_segments = 0;

	std::istringstream istr(record);
	if (!(istr >> abbreviation >> nb_segments)) return false;
		
	// make short_name uppercase for case insensitive searches
	transform(abbreviation.begin(),abbreviation.end(), abbreviation.begin(), ::toupper);

    asterism = new HipStar*[nb_segments*2];
    for (unsigned int i=0;i<nb_segments*2;++i)
    {
		HP = 0;
        istr >> HP;
		if(HP == 0)
		{
			delete [] asterism;
			return false;
		}

        asterism[i]=_VouteCeleste->searchHP(HP);
		if (!asterism[i])
		{
			cout << "Error in Constellation " << abbreviation << " asterism : can't find star HP= " << HP << endl;
			delete [] asterism;
			return false;
		}
    }

    for(unsigned int ii=0;ii<nb_segments*2;++ii)
    {
		XYZname+=(*asterism[ii]).XYZ;
    }
    XYZname*=1./(nb_segments*2);

	return true;
}

// Draw the Constellation lines
/*
void Constellation::draw(Projector* prj) const
{
    prj->set_orthographic_projection();	// set 2D coordinate
	draw_optim(prj);
	prj->reset_perspective_projection();
}
*/

// Draw the lines for the Constellation using the coords of the stars
// (optimized for use thru the class ConstellationMgr only)
void Constellation::draw_optim(Projector* prj) const
{
	if(!line_fader.getInterstate()) return;

	glColor3fv(line_color*line_fader.getInterstate());
	Vec3d star1;
	Vec3d star2;
    for(unsigned int i=0;i<nb_segments;++i)
    {
		if(prj->project_prec_earth_equ_line_check(asterism[2*i]->XYZ,star1,asterism[2*i+1]->XYZ,star2) ) 
		{
			glBegin(GL_LINES);
			glVertex2f(star1[0],star1[1]);
			glVertex2f(star2[0],star2[1]);
			glEnd();
		}
	}
}

// Draw the name
void Constellation::draw_name(s_font *constfont, Projector* prj) const
{
	if(!name_fader.getInterstate()) return;
	glColor3fv(label_color*name_fader.getInterstate());
	gravity_label ? prj->print_gravity180(constfont, XYname[0], XYname[1], nameI18, 1, -constfont->getStrLen(nameI18)/2) :
	constfont->print(XYname[0]-constfont->getStrLen(nameI18)/2, XYname[1], nameI18);
}

// Draw the art texture, optimized function to be called thru a constellation manager only
void Constellation::draw_art_optim(Projector* prj, Navigator* nav) const
{
	float intensity = art_fader.getInterstate(); 
	if (art_tex && intensity) 
	{
		glColor3f(intensity,intensity,intensity);

		Vec3d v0, v1, v2, v3, v4, v5, v6, v7, v8;
		bool b0, b1, b2, b3, b4, b5, b6, b7, b8; 

		// If one of the point is in the screen
		b0 = prj->project_prec_earth_equ_check(art_vertex[0],v0) || (nav->get_prec_equ_vision().dot(art_vertex[0])>0.9);
		b1 = prj->project_prec_earth_equ_check(art_vertex[1],v1) || (nav->get_prec_equ_vision().dot(art_vertex[1])>0.9);
		b2 = prj->project_prec_earth_equ_check(art_vertex[2],v2) || (nav->get_prec_equ_vision().dot(art_vertex[2])>0.9);
		b3 = prj->project_prec_earth_equ_check(art_vertex[3],v3) || (nav->get_prec_equ_vision().dot(art_vertex[3])>0.9);
		b4 = prj->project_prec_earth_equ_check(art_vertex[4],v4) || (nav->get_prec_equ_vision().dot(art_vertex[4])>0.9);
		b5 = prj->project_prec_earth_equ_check(art_vertex[5],v5) || (nav->get_prec_equ_vision().dot(art_vertex[5])>0.9);
		b6 = prj->project_prec_earth_equ_check(art_vertex[6],v6) || (nav->get_prec_equ_vision().dot(art_vertex[6])>0.9);
		b7 = prj->project_prec_earth_equ_check(art_vertex[7],v7) || (nav->get_prec_equ_vision().dot(art_vertex[7])>0.9);
		b8 = prj->project_prec_earth_equ_check(art_vertex[8],v8) || (nav->get_prec_equ_vision().dot(art_vertex[8])>0.9);
			
		if (b0 || b1 || b2 || b3 || b4 || b5 || b6 || b7 || b8)
		{
			glBindTexture(GL_TEXTURE_2D, art_tex->getID());
		
			if ((b0 || b1 || b2 || b3) && (v0[2]<1 && v1[2]<1 && v2[2]<1 && v3[2]<1))
			{	
				glBegin(GL_QUADS);
					glTexCoord2f(0,0); 		glVertex2f(v0[0],v0[1]);
					glTexCoord2f(0.5,0); 	glVertex2f(v1[0],v1[1]);
					glTexCoord2f(0.5,0.5); 	glVertex2f(v2[0],v2[1]);
					glTexCoord2f(0,0.5); 	glVertex2f(v3[0],v3[1]);
				glEnd();
			}
			if ((b1 || b4 || b5 || b2) && (v1[2]<1 && v4[2]<1 && v5[2]<1 && v2[2]<1))
			{
				glBegin(GL_QUADS);
					glTexCoord2f(0.5,0); glVertex2f(v1[0],v1[1]);
					glTexCoord2f(1,0); glVertex2f(v4[0],v4[1]);
					glTexCoord2f(1,0.5); glVertex2f(v5[0],v5[1]);
					glTexCoord2f(0.5,0.5); glVertex2f(v2[0],v2[1]);
				glEnd();
			}
			if ((b2 || b5 || b6 || b7) && (v2[2]<1 && v5[2]<1 && v6[2]<1 && v7[2]<1))
			{
				glBegin(GL_QUADS);
					glTexCoord2f(0.5,0.5); 	glVertex2f(v2[0],v2[1]);
					glTexCoord2f(1,0.5); 	glVertex2f(v5[0],v5[1]);
					glTexCoord2f(1,1); 		glVertex2f(v6[0],v6[1]);
					glTexCoord2f(0.5,1);	glVertex2f(v7[0],v7[1]);
				glEnd();
			}
			if ((b3 || b2 || b7 || b8) && (v3[2]<1 && v2[2]<1 && v7[2]<1 && v8[2]<1))
			{
				glBegin(GL_QUADS);
					glTexCoord2f(0,0.5); 	glVertex2f(v3[0],v3[1]);
					glTexCoord2f(0.5,0.5); 	glVertex2f(v2[0],v2[1]);
					glTexCoord2f(0.5,1); 	glVertex2f(v7[0],v7[1]);
					glTexCoord2f(0,1);		glVertex2f(v8[0],v8[1]);
				glEnd();
			}
		}
	}
}

// Draw the art texture
void Constellation::draw_art(Projector* prj, Navigator* nav) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	prj->set_orthographic_projection();

	draw_art_optim(prj, nav);

	prj->reset_perspective_projection();

	glDisable(GL_CULL_FACE);
}

const Constellation* Constellation::is_star_in(const HipStar * s) const
{
    for(unsigned int i=0;i<nb_segments*2;++i)
    {
		if (asterism[i]==s) return this;
    }
	return NULL;
}

void Constellation::update(int delta_time)
{
	line_fader.update(delta_time);
	name_fader.update(delta_time);
	art_fader.update(delta_time);
	boundary_fader.update(delta_time);
}

// Draw the Constellation lines
void Constellation::draw_boundary_optim(Projector* prj) const
{
	if(!boundary_fader.getInterstate()) return;

//    if (boundaryXYZ.size() <= 2) return;
//    if (!isolatedBoundarySegments.size()) return;

	if (singleSelected) glColor3fv(boundary_color);
	else glColor3fv(boundary_color*boundary_fader.getInterstate());
/* drawing for original bound_20 data
	i = 0;
	Vec3d pt1;
	Vec3d pt2;
    do
    {
		if(prj->project_prec_earth_equ_line_check(boundaryXYZ[i],pt1,boundaryXYZ[i+1],pt2) ) 
		{
			glBegin(GL_LINES);
				glVertex2f(pt1[0],pt1[1]);
				glVertex2f(pt2[0],pt2[1]);
			glEnd();
		}
		i++;
	} while ( k < boundaryXYZ.size());
*/
	unsigned int i, j, size;
	Vec3d pt1, pt2;
	vector<Vec3f> *points;

	if (singleSelected) size = isolatedBoundarySegments.size();
	else size = sharedBoundarySegments.size();
	
	for (i=0;i<size;i++)
	{
		if (singleSelected) points = isolatedBoundarySegments[i];
		else points = sharedBoundarySegments[i];

		for (j=0;j<points->size()-1;j++)
		{
			if(prj->project_prec_earth_equ_line_check(points->at(j),pt1,points->at(j+1),pt2)) 
			{
				glBegin(GL_LINES);
					glVertex2f(pt1[0],pt1[1]);
					glVertex2f(pt2[0],pt2[1]);
				glEnd();
			}
		}			
	}
}
