/*
 * Stellarium
 * Copyright (C) 2003 Fabien Ch?reau
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

#include "landscape.h"
#include "init_parser.h"

Landscape::Landscape(float _radius) : radius(_radius), sky_brightness(1.)
{
}

Landscape::~Landscape()
{
}

Landscape* Landscape::create_from_file(const string& landscape_file, const string& section_name)
{
	init_parser pd;	// The landscape data ini file parser
	pd.load(landscape_file);
	string s;
	s = pd.get_str(section_name, "type");
	if (s.empty())
	{
		cout << "ERROR : can't find type for landscape section " << section_name << endl;
		exit(-1);
	}
	if (s=="old_style")
	{
		Landscape_old_style* ldscp = new Landscape_old_style();
		ldscp->load(landscape_file, section_name);
		return ldscp;
	}
	if (s=="fisheye")
	{
		Landscape_fisheye* ldscp = new Landscape_fisheye();
		ldscp->load(landscape_file, section_name);
		return ldscp;
	}

	cout << "ERROR : can't understand landscape type for landscape section " << section_name << endl;
	exit(-1);

	return NULL;
}

string Landscape::get_file_content(const string& landscape_file)
{
	init_parser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string result;

	for (int i=0; i<pd.get_nsec();i++)
	{
		result += pd.get_secname(i) + '\n';
	}
	return result;
}

Landscape_old_style::Landscape_old_style(float _radius) : Landscape(_radius), side_texs(NULL), sides(NULL)
{
}

Landscape_old_style::~Landscape_old_style()
{
	if (side_texs)
	{
		for (int i=0;i<nb_side_texs;++i)
		{
			if (side_texs[i]) delete side_texs[i];
			side_texs[i] = NULL;
		}
		delete side_texs;
	}
	if (sides) delete sides;
	if (ground_tex) delete ground_tex;
	if (fog_tex) delete fog_tex;
}

void Landscape_old_style::load(const string& landscape_file, const string& section_name)
{
	init_parser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	name = pd.get_str(section_name, "name");

	// Load sides textures
	nb_side_texs = pd.get_int(section_name, "nbsidetex", 0);
	side_texs = new s_texture*[nb_side_texs];
	char tmp[255];
	for (int i=0;i<nb_side_texs;++i)
	{
		sprintf(tmp,"tex%d",i);
		side_texs[i] = new s_texture(pd.get_str(section_name, tmp),TEX_LOAD_TYPE_PNG_ALPHA);
	}

	// Init sides parameters
	nb_side = pd.get_int(section_name, "nbside", 0);
	sides = new landscape_tex_coord[nb_side];
	string s;
	int texnum;
	float a,b,c,d;
	for (int i=0;i<nb_side;++i)
	{
		sprintf(tmp,"side%d",i);
		s = pd.get_str(section_name, tmp);
		sscanf(s.c_str(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		sides[i].tex = side_texs[texnum];
		sides[i].tex_coords[0] = a;
		sides[i].tex_coords[1] = b;
		sides[i].tex_coords[2] = c;
		sides[i].tex_coords[3] = d;
		//printf("%f %f %f %f\n",a,b,c,d);
	}

	nb_decor_repeat = pd.get_int(section_name, "nb_decor_repeat", 1);

	ground_tex = new s_texture(pd.get_str(section_name, "groundtex"),TEX_LOAD_TYPE_PNG_SOLID);
	s = pd.get_str(section_name, "ground");
	sscanf(s.c_str(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	ground_tex_coord.tex = ground_tex;
	ground_tex_coord.tex_coords[0] = a;
	ground_tex_coord.tex_coords[1] = b;
	ground_tex_coord.tex_coords[2] = c;
	ground_tex_coord.tex_coords[3] = d;

	fog_tex = new s_texture(pd.get_str(section_name, "fogtex"),TEX_LOAD_TYPE_PNG_SOLID_REPEAT);
	s = pd.get_str(section_name, "fog");
	sscanf(s.c_str(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	fog_tex_coord.tex = fog_tex;
	fog_tex_coord.tex_coords[0] = a;
	fog_tex_coord.tex_coords[1] = b;
	fog_tex_coord.tex_coords[2] = c;
	fog_tex_coord.tex_coords[3] = d;

	fog_alt_angle = pd.get_double(section_name, "fog_alt_angle", 0.);
	fog_angle_shift = pd.get_double(section_name, "fog_angle_shift", 0.);
	decor_alt_angle = pd.get_double(section_name, "decor_alt_angle", 0.);
	decor_angle_shift = pd.get_double(section_name, "decor_angle_shift", 0.);
	ground_angle_shift = pd.get_double(section_name, "ground_angle_shift", 0.);
}

void Landscape_old_style::draw(tone_reproductor * eye, const Projector* prj, const navigator* nav,
		bool flag_fog, bool flag_decor, bool flag_ground)
{
	if (flag_ground) draw_ground(eye, prj, nav);
	if (flag_decor) draw_decor(eye, prj, nav);
	if (flag_fog) draw_fog(eye, prj, nav);
}


// Draw the horizon fog
void Landscape_old_style::draw_fog(tone_reproductor * eye, const Projector* prj, const navigator* nav) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glPushMatrix();
	glColor3f(0.1f+0.1f*sky_brightness, 0.1f+0.1f*sky_brightness, 0.1f+0.1f*sky_brightness);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, fog_tex->getID());
	prj->sCylinder(radius, radius*sinf(fog_alt_angle*M_PI/180.), 32, 1, nav->get_local_to_eye_mat() *
		Mat4d::translation(Vec3d(0.,0.,radius*sinf(fog_angle_shift*M_PI/180.))), 1);
	glDisable(GL_CULL_FACE);
	glPopMatrix();
}

// Draw the mountains with a few pieces of texture
void Landscape_old_style::draw_decor(tone_reproductor * eye, const Projector* prj, const navigator* nav) const
{
	Mat4d mat = nav->get_local_to_eye_mat();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	glColor3f(sky_brightness, sky_brightness, sky_brightness);
	glPushMatrix();
	glLoadMatrixd(mat);

	int subdiv = 32/(nb_decor_repeat*nb_side);
	if (subdiv<=0) subdiv = 1;
	float da = (2.*M_PI)/(nb_side*subdiv*nb_decor_repeat);
	float dz = radius * sinf(decor_alt_angle*M_PI/180.f);
	float z = radius*sinf(ground_angle_shift*M_PI/180.);
	float x,y;
	float a;

	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, ground_tex->getID());
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f (ground_tex_coord.tex_coords[0],ground_tex_coord.tex_coords[1]);
		prj->sVertex3(-radius, -radius, z, mat);
		glTexCoord2f (ground_tex_coord.tex_coords[2], ground_tex_coord.tex_coords[1]);
		prj->sVertex3(-radius, radius, z, mat);
		glTexCoord2f (ground_tex_coord.tex_coords[2], ground_tex_coord.tex_coords[3]);
		prj->sVertex3(radius, radius, z, mat);
		glTexCoord2f (ground_tex_coord.tex_coords[0], ground_tex_coord.tex_coords[3]);
		prj->sVertex3(radius, -radius, z, mat);
		glTexCoord2f (ground_tex_coord.tex_coords[0],ground_tex_coord.tex_coords[1]);
		prj->sVertex3(-radius, -radius, z, mat);
	glEnd ();		
	
	z=radius*sinf(decor_angle_shift*M_PI/180.);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);	
	
	for (int n=0;n<nb_decor_repeat;++n)
	{
		a = 2.f*M_PI*n/nb_decor_repeat;
		for (int i=0;i<nb_side;++i)
		{
			glBindTexture(GL_TEXTURE_2D, sides[i].tex->getID());
			glBegin(GL_QUAD_STRIP);
			for (int j=0;j<=subdiv;++j)
			{
				x = radius * sinf(a + da * j + da * subdiv * i);
				y = radius * cosf(a + da * j + da * subdiv * i);
				glNormal3f(-x, -y, 0);
				glTexCoord2f(sides[i].tex_coords[0] + (float)j/subdiv * (sides[i].tex_coords[2]-sides[i].tex_coords[0]),
					sides[j].tex_coords[3]);
				prj->sVertex3(x, y, z + dz * (sides[i].tex_coords[3]-sides[i].tex_coords[1]), mat);
				glTexCoord2f(sides[i].tex_coords[0] + (float)j/subdiv * (sides[i].tex_coords[2]-sides[i].tex_coords[0]),
					sides[j].tex_coords[1]);
				prj->sVertex3(x, y, z , mat);
			}
			glEnd();
		}
	}
	glDisable(GL_CULL_FACE);
	glPopMatrix();
}


// Draw the ground
void Landscape_old_style::draw_ground(tone_reproductor * eye, const Projector* prj, const navigator* nav) const
{
	if (prj->get_type()==FISHEYE_PROJECTOR)
	{
		// Need to draw a half sphere ground
		glColor3f(sky_brightness, sky_brightness, sky_brightness);
		glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, ground_tex->getID());
		prj->sHalfSphere(radius,30,10, nav->get_local_to_eye_mat(), 1);
	    glDisable(GL_CULL_FACE);
	}
	else
	{
		// Just a horizontal quad for the ground is enought
		float z=radius*sinf(decor_angle_shift*M_PI/180.);
		glPushMatrix();
		glColor3f(sky_brightness, sky_brightness, sky_brightness);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, ground_tex->getID());
		glBegin(GL_QUADS);
			glTexCoord2f (ground_tex_coord.tex_coords[0],ground_tex_coord.tex_coords[1]);
			glVertex3f (-radius/2, -radius/2, 0);
			glTexCoord2f (ground_tex_coord.tex_coords[2], ground_tex_coord.tex_coords[1]);
			glVertex3f(-radius/2, radius/2, 0);
			glTexCoord2f (ground_tex_coord.tex_coords[2], ground_tex_coord.tex_coords[3]);
			glVertex3f( radius/2, radius/2, 0);
			glTexCoord2f (ground_tex_coord.tex_coords[0], ground_tex_coord.tex_coords[3]);
			glVertex3f( radius/2, -radius/2, 0);
		glEnd ();
		glPopMatrix();
	}
}

Landscape_fisheye::Landscape_fisheye(float _radius) : Landscape(_radius), map_tex(NULL)
{
}

Landscape_fisheye::~Landscape_fisheye()
{
	if (map_tex) delete map_tex;
	map_tex = NULL;
}

void Landscape_fisheye::load(const string& landscape_file, const string& section_name)
{
	init_parser pd;	// The landscape data ini file parser
	pd.load(landscape_file);
	name = pd.get_str(section_name, "name");
	map_tex = new s_texture(pd.get_str(section_name, "maptex"),TEX_LOAD_TYPE_PNG_ALPHA);
	tex_fov = pd.get_double(section_name, "texturefov", 360) * M_PI/180.;
}

void Landscape_fisheye::draw(tone_reproductor * eye, const Projector* prj, const navigator* nav,
		bool flag_fog, bool flag_decor, bool flag_ground)
{
	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor3f(sky_brightness, sky_brightness, sky_brightness);

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, map_tex->getID());
	if (flag_ground) prj->sSphere_map(radius,40,20, nav->get_local_to_eye_mat(), tex_fov, 1);

	glDisable(GL_CULL_FACE);
}

