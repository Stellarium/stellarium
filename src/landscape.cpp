/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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
	valid_landscape = 0;
}

Landscape::~Landscape()
{}

Landscape* Landscape::create_from_file(const string& landscape_file, const string& section_name)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);
	string s;
	s = pd.get_str(section_name, "type");
	if (s=="old_style")
	{
		LandscapeOldStyle* ldscp = new LandscapeOldStyle();
		ldscp->load(landscape_file, section_name);
		return ldscp;
	}
	else if (s=="spherical")
	{
		LandscapeSpherical* ldscp = new LandscapeSpherical();
		ldscp->load(landscape_file, section_name);
		return ldscp;
	}
	else
	{   //	if (s=="fisheye")
		LandscapeFisheye* ldscp = new LandscapeFisheye();
		ldscp->load(landscape_file, section_name);
		return ldscp;
	}

}

// create landscape from parameters passed in a hash (same keys as with ini file)
// NOTE: maptex must be full path and filename
Landscape* Landscape::create_from_hash(stringHash_t param)
{

	// NOTE: textures should be full filename (and path)
	if (param["type"]=="old_style")
	{
		LandscapeOldStyle* ldscp = new LandscapeOldStyle();
		ldscp->create(1, param);
		return ldscp;
	}
	else if (param["type"]=="spherical")
	{
		LandscapeSpherical* ldscp = new LandscapeSpherical();
		ldscp->create(param["name"], 1, param["path"] + param["maptex"]);
		return ldscp;
	}
	else
	{   //	if (s=="fisheye")
		LandscapeFisheye* ldscp = new LandscapeFisheye();
		ldscp->create(param["name"], 1, param["path"] + param["maptex"], str_to_double(param["texturefov"]));
		return ldscp;
	}
}


string Landscape::get_file_content(const string& landscape_file)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string result;

	for (int i=0; i<pd.get_nsec();i++)
	{
		result += pd.get_secname(i) + '\n';
	}
	return result;
}

LandscapeOldStyle::LandscapeOldStyle(float _radius) : Landscape(_radius), side_texs(NULL), sides(NULL), fog_tex(NULL), ground_tex(NULL)
{}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (side_texs)
	{
		for (int i=0;i<nb_side_texs;++i)
		{
			if (side_texs[i]) delete side_texs[i];
			side_texs[i] = NULL;
		}
		delete [] side_texs;
		side_texs = NULL;
	}

	if (sides) delete [] sides;
	if (ground_tex) delete ground_tex;
	if (fog_tex) delete fog_tex;

}

void LandscapeOldStyle::load(const string& landscape_file, const string& section_name)
{
	// TODO: put values into hash and call create method to consolidate code

	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	name = pd.get_str(section_name, "name");
	if(name == "" )
	{
		printf("ERROR : No valid landscape definition found for %s.  No landscape in use.\n", section_name.c_str());
		valid_landscape = 0;
		return;
	}
	else
	{
		valid_landscape = 1;
	}

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
	decor_angle_rotatez = pd.get_double(section_name, "decor_angle_rotatez", 0.);
	ground_angle_shift = pd.get_double(section_name, "ground_angle_shift", 0.);
	ground_angle_rotatez = pd.get_double(section_name, "ground_angle_rotatez", 0.);
	draw_ground_first = pd.get_int(section_name, "draw_ground_first", 0);
}


// create from a hash of parameters (no ini file needed)
void LandscapeOldStyle::create(bool _fullpath, stringHash_t param)
{

	/*
	for ( stringHashIter_t iter = param.begin(); iter != param.end(); ++iter ) {
		cout << iter->first << " : " << iter->second << endl;
	}
	*/

	name = param["name"];
	valid_landscape = 1;  // assume valid if got here

	// Load sides textures
	nb_side_texs = str_to_int(param["nbsidetex"]);
	side_texs = new s_texture*[nb_side_texs];

	char tmp[255];
	for (int i=0;i<nb_side_texs;++i)
	{

		sprintf(tmp,"tex%d",i);
		side_texs[i] = new s_texture(_fullpath, param["path"] + param[tmp],TEX_LOAD_TYPE_PNG_ALPHA);

	}

	// Init sides parameters
	nb_side = str_to_int(param["nbside"]);
	sides = new landscape_tex_coord[nb_side];
	string s;
	int texnum;
	float a,b,c,d;
	for (int i=0;i<nb_side;++i)
	{
		sprintf(tmp,"side%d",i);
		s = param[tmp];
		sscanf(s.c_str(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		sides[i].tex = side_texs[texnum];
		sides[i].tex_coords[0] = a;
		sides[i].tex_coords[1] = b;
		sides[i].tex_coords[2] = c;
		sides[i].tex_coords[3] = d;
		//printf("%f %f %f %f\n",a,b,c,d);
	}

	nb_decor_repeat = str_to_int(param["nb_decor_repeat"], 1);

	ground_tex = new s_texture(_fullpath, param["path"] + param["groundtex"],TEX_LOAD_TYPE_PNG_SOLID);
	s = param["ground"];
	sscanf(s.c_str(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	ground_tex_coord.tex = ground_tex;
	ground_tex_coord.tex_coords[0] = a;
	ground_tex_coord.tex_coords[1] = b;
	ground_tex_coord.tex_coords[2] = c;
	ground_tex_coord.tex_coords[3] = d;

	fog_tex = new s_texture(_fullpath, param["path"] + param["fogtex"],TEX_LOAD_TYPE_PNG_SOLID_REPEAT);
	s = param["fog"];
	sscanf(s.c_str(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	fog_tex_coord.tex = fog_tex;
	fog_tex_coord.tex_coords[0] = a;
	fog_tex_coord.tex_coords[1] = b;
	fog_tex_coord.tex_coords[2] = c;
	fog_tex_coord.tex_coords[3] = d;

	fog_alt_angle = str_to_double(param["fog_alt_angle"]);
	fog_angle_shift = str_to_double(param["fog_angle_shift"]);
	decor_alt_angle = str_to_double(param["decor_alt_angle"]);
	decor_angle_shift = str_to_double(param["decor_angle_shift"]);
	decor_angle_rotatez = str_to_double(param["decor_angle_rotatez"]);
	ground_angle_shift = str_to_double(param["ground_angle_shift"]);
	ground_angle_rotatez = str_to_double(param["ground_angle_rotatez"]);
	draw_ground_first = str_to_int(param["draw_ground_first"]);
}

void LandscapeOldStyle::draw(ToneReproductor * eye, const Projector* prj, const Navigator* nav)
{
	if(!valid_landscape) return;
	if (draw_ground_first) draw_ground(eye, prj, nav);
	draw_decor(eye, prj, nav);
	if (!draw_ground_first) draw_ground(eye, prj, nav);
	draw_fog(eye, prj, nav);
}


// Draw the horizon fog
void LandscapeOldStyle::draw_fog(ToneReproductor * eye, const Projector* prj, const Navigator* nav) const
{
	if(!fog_fader.getInterstate()) return;
	glBlendFunc(GL_ONE, GL_ONE);
	glPushMatrix();
	glColor3f(fog_fader.getInterstate()*(0.1f+0.1f*sky_brightness), fog_fader.getInterstate()*(0.1f+0.1f*sky_brightness), fog_fader.getInterstate()*(0.1f+0.1f*sky_brightness));
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, fog_tex->getID());
	prj->sCylinder(radius, radius*sinf(fog_alt_angle*M_PI/180.), 128, 1, nav->get_local_to_eye_mat() *
	               Mat4d::translation(Vec3d(0.,0.,radius*sinf(fog_angle_shift*M_PI/180.))), 1);
	glDisable(GL_CULL_FACE);
	glPopMatrix();
}

// Draw the mountains with a few pieces of texture
void LandscapeOldStyle::draw_decor(ToneReproductor * eye, const Projector* prj, const Navigator* nav) const
{
	if (!land_fader.getInterstate()) return;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());

	int subdiv = 128/(nb_decor_repeat*nb_side);
	if (subdiv<=0) subdiv = 1;
	float da = (2.*M_PI)/(nb_side*subdiv*nb_decor_repeat);
	float dz = radius * sinf(decor_alt_angle*M_PI/180.f);
	float z = radius*sinf(ground_angle_shift*M_PI/180.);
	float x,y;
	float a;

	//	Mat4d mat = nav->get_local_to_eye_mat() * Mat4d::zrotation(decor_angle_rotatez*M_PI/180.f);
	Mat4d mat = nav->get_local_to_eye_mat();
	glPushMatrix();
	glLoadMatrixd(mat);

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
				x = radius * sinf(a + da * j + da * subdiv * i + decor_angle_rotatez*M_PI/180);
				y = radius * cosf(a + da * j + da * subdiv * i + decor_angle_rotatez*M_PI/180);
				glNormal3f(-x, -y, 0);
				glTexCoord2f(sides[i].tex_coords[0] + (float)j/subdiv * (sides[i].tex_coords[2]-sides[i].tex_coords[0]),
				             sides[i].tex_coords[3]);
				prj->sVertex3(x, y, z + dz * (sides[i].tex_coords[3]-sides[i].tex_coords[1]), mat);
				glTexCoord2f(sides[i].tex_coords[0] + (float)j/subdiv * (sides[i].tex_coords[2]-sides[i].tex_coords[0]),
				             sides[i].tex_coords[1]);
				prj->sVertex3(x, y, z , mat);
			}
			glEnd();
		}
	}
	glDisable(GL_CULL_FACE);
	glPopMatrix();
}


// Draw the ground
void LandscapeOldStyle::draw_ground(ToneReproductor * eye, const Projector* prj, const Navigator* nav) const
{
	if (!land_fader.getInterstate()) return;
	Mat4d mat = nav->get_local_to_eye_mat() * Mat4d::zrotation(ground_angle_rotatez*M_PI/180.f) * Mat4d::translation(Vec3d(0,0,radius*sinf(ground_angle_shift*M_PI/180.)));
	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, ground_tex->getID());
	int subdiv = 128/(nb_decor_repeat*nb_side);
	if (subdiv<=0) subdiv = 1;
	prj->sDisk(radius,nb_side*subdiv*nb_decor_repeat,5, mat, 1);
	glDisable(GL_CULL_FACE);
}

LandscapeFisheye::LandscapeFisheye(float _radius) : Landscape(_radius), map_tex(NULL)
{}

LandscapeFisheye::~LandscapeFisheye()
{
	if (map_tex) delete map_tex;
	map_tex = NULL;
}

void LandscapeFisheye::load(const string& landscape_file, const string& section_name)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string type;
	type = pd.get_str(section_name, "type");
	name = pd.get_str(section_name, "name");
	if(type != "fisheye" || name == "" )
	{
		printf("ERROR : No valid landscape definition found for %s.  No landscape in use.\n", section_name.c_str());
		valid_landscape = 0;
		return;
	}

	create(name, 0, pd.get_str(section_name, "maptex"), pd.get_double(section_name, "texturefov", 360));

}


// create a fisheye landscape from basic parameters (no ini file needed)
void LandscapeFisheye::create(const string _name, bool _fullpath, const string _maptex, double _texturefov)
{
	//	cout << _name << " " << _fullpath << " " << _maptex << " " << _texturefov << "\n";
	valid_landscape = 1;  // assume ok...
	name = _name;
	map_tex = new s_texture(_fullpath,_maptex,TEX_LOAD_TYPE_PNG_ALPHA);
	tex_fov = _texturefov*M_PI/180.;
}


void LandscapeFisheye::draw(ToneReproductor * eye, const Projector* prj, const Navigator* nav)
{
	if(!valid_landscape) return;
	if(!land_fader.getInterstate()) return;

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, map_tex->getID());
	prj->sSphere_map(radius,40,20, nav->get_local_to_eye_mat(), tex_fov, 1);

	glDisable(GL_CULL_FACE);
}


// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius) : Landscape(_radius), map_tex(NULL)
{}

LandscapeSpherical::~LandscapeSpherical()
{
	if (map_tex) delete map_tex;
	map_tex = NULL;
}

void LandscapeSpherical::load(const string& landscape_file, const string& section_name)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string type;
	type = pd.get_str(section_name, "type");
	name = pd.get_str(section_name, "name");
	if(type != "spherical" )
	{
		printf("ERROR : No valid landscape definition found for %s.  No landscape in use.\n", section_name.c_str());
		valid_landscape = 0;
		return;
	}

	create(name, 0, pd.get_str(section_name, "maptex"));

}


// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const string _name, bool _fullpath, const string _maptex)
{
	//	cout << _name << " " << _fullpath << " " << _maptex << " " << _texturefov << "\n";
	valid_landscape = 1;  // assume ok...
	name = _name;
	map_tex = new s_texture(_fullpath,_maptex,TEX_LOAD_TYPE_PNG_ALPHA);
}


void LandscapeSpherical::draw(ToneReproductor * eye, const Projector* prj, const Navigator* nav)
{
	if(!valid_landscape) return;
	if(!land_fader.getInterstate()) return;

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, map_tex->getID());

	// TODO: verify that this works correctly for custom projections
	// seam is at East
	prj->sSphere(radius,1.0,40,20, nav->get_local_to_eye_mat(), 1);

	glDisable(GL_CULL_FACE);
}

