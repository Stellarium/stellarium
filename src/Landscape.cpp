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

#include "Landscape.hpp"
#include "InitParser.hpp"
#include "STexture.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"

#include <cassert>
#include <vector>

Landscape::Landscape(float _radius) : radius(_radius), sky_brightness(1.)
{
	valid_landscape = 0;
}

Landscape::~Landscape()
{}


// Load attributes common to all landscapes
void Landscape::loadCommon(const string& landscape_file, const string& landscapeId)
{
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);
	cout << "DEBUG MNG: section name is: " << landscapeId << "file name is " << landscape_file << endl;
	name = StelUtils::stringToWstring(pd.get_str("landscape", "name"));
	author = StelUtils::stringToWstring(pd.get_str("landscape", "author"));
	description = StelUtils::stringToWstring(pd.get_str("landscape", "description"));
	if(name == L"" )
	{
		cerr << "No valid landscape definition found for section "<< landscapeId << " in file " << landscape_file << ". No landscape in use." << endl;
		valid_landscape = 0;
		return;
	}
	else
	{
		valid_landscape = 1;
	}
}

const string Landscape::getTexturePath(const string& basename, const string& landscapeId)
{
	// look in the landscape directory first, and if not found default to global textures directory
	string path;
	try {
		path = StelApp::getInstance().getFileMgr().findFile("landscapes/" + landscapeId + "/" + basename).string();
		return path;
	}
	catch (exception& e)
	{
		path = StelApp::getInstance().getFileMgr().findFile("textures/" + basename).string();
		return path;
	}
}

LandscapeOldStyle::LandscapeOldStyle(float _radius) : Landscape(_radius), side_texs(NULL), sides(NULL), tanMode(false)
{}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (side_texs)
	{
		for (int i=0;i<nb_side_texs;++i)
		{
			// TODO I am not sure that this decrement the reference count of the shared pointers
			// side_texs[i] = NULL;
		}
		delete [] side_texs;
		side_texs = NULL;
	}

	if (sides) delete [] sides;

}

void LandscapeOldStyle::load(const string& landscape_file, const string& landscapeId)
{
	loadCommon(landscape_file, landscapeId);
	
	// TODO: put values into hash and call create method to consolidate code

	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string type = pd.get_str("landscape", "type");
	if(type != "old_style")
	{
		cerr << "Landscape type mismatch for landscape "<< landscapeId << ", expected old_style, found " << type << ".  No landscape in use.\n";
		valid_landscape = 0;
		return;
	}

	// Load sides textures
	nb_side_texs = pd.get_int("landscape", "nbsidetex", 0);
	side_texs = new STextureSP[nb_side_texs];
	char tmp[255];
	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setWrapMode(GL_CLAMP_TO_EDGE);
	for (int i=0;i<nb_side_texs;++i)
	{
		sprintf(tmp,"tex%d",i);
		side_texs[i] = StelApp::getInstance().getTextureManager().createTexture(getTexturePath(pd.get_str("landscape", tmp), landscapeId));
	}

	// Init sides parameters
	nb_side = pd.get_int("landscape", "nbside", 0);
	sides = new landscape_tex_coord[nb_side];
	string s;
	int texnum;
	float a,b,c,d;
	for (int i=0;i<nb_side;++i)
	{
		sprintf(tmp,"side%d",i);
		s = pd.get_str("landscape", tmp);
		sscanf(s.c_str(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		sides[i].tex = side_texs[texnum];
		sides[i].tex_coords[0] = a;
		sides[i].tex_coords[1] = b;
		sides[i].tex_coords[2] = c;
		sides[i].tex_coords[3] = d;
		//printf("%f %f %f %f\n",a,b,c,d);
	}

	nb_decor_repeat = pd.get_int("landscape", "nb_decor_repeat", 1);

	StelApp::getInstance().getTextureManager().setDefaultParams();
	ground_tex = StelApp::getInstance().getTextureManager().createTexture(getTexturePath(pd.get_str("landscape", "groundtex"), landscapeId));
	s = pd.get_str("landscape", "ground");
	sscanf(s.c_str(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	ground_tex_coord.tex = ground_tex;
	ground_tex_coord.tex_coords[0] = a;
	ground_tex_coord.tex_coords[1] = b;
	ground_tex_coord.tex_coords[2] = c;
	ground_tex_coord.tex_coords[3] = d;

	StelApp::getInstance().getTextureManager().setWrapMode(GL_REPEAT);
	fog_tex = StelApp::getInstance().getTextureManager().createTexture(getTexturePath(pd.get_str("landscape", "fogtex"), landscapeId));
	s = pd.get_str("landscape", "fog");
	sscanf(s.c_str(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	fog_tex_coord.tex = fog_tex;
	fog_tex_coord.tex_coords[0] = a;
	fog_tex_coord.tex_coords[1] = b;
	fog_tex_coord.tex_coords[2] = c;
	fog_tex_coord.tex_coords[3] = d;

	fog_alt_angle = pd.get_double("landscape", "fog_alt_angle", 0.);
	fog_angle_shift = pd.get_double("landscape", "fog_angle_shift", 0.);
	decor_alt_angle = pd.get_double("landscape", "decor_alt_angle", 0.);
	decor_angle_shift = pd.get_double("landscape", "decor_angle_shift", 0.);
	decor_angle_rotatez = pd.get_double("landscape", "decor_angle_rotatez", 0.);
	ground_angle_shift = pd.get_double("landscape", "ground_angle_shift", 0.);
	ground_angle_rotatez = pd.get_double("landscape", "ground_angle_rotatez", 0.);
	draw_ground_first = pd.get_int("landscape", "draw_ground_first", 0);
	tanMode = pd.get_boolean("landscape", "tan_mode", false);
}


// create from a hash of parameters (no ini file needed)
void LandscapeOldStyle::create(bool _fullpath, stringHash_t param)
{
	name = StelUtils::stringToWstring(param["name"]);
	valid_landscape = 1;  // assume valid if got here

	// Load sides textures
	nb_side_texs = StelUtils::stringToInt(param["nbsidetex"]);
	side_texs = new STextureSP[nb_side_texs];
	
	char tmp[255];
	//StelApp::getInstance().getTextureManager().setMipmapsMode(true);
	//StelApp::getInstance().getTextureManager().setMagFilter(GL_NEAREST);
	for (int i=0;i<nb_side_texs;++i)
	{
		sprintf(tmp,"tex%d",i);
		side_texs[i] = StelApp::getInstance().getTextureManager().createTexture(param["path"] + param[tmp]);
	}

	// Init sides parameters
	nb_side = StelUtils::stringToInt(param["nbside"]);
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

	nb_decor_repeat = StelUtils::stringToInt(param["nb_decor_repeat"], 1);

	ground_tex = StelApp::getInstance().getTextureManager().createTexture(param["path"] + param["groundtex"]);
	s = param["ground"];
	sscanf(s.c_str(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	ground_tex_coord.tex = ground_tex;
	ground_tex_coord.tex_coords[0] = a;
	ground_tex_coord.tex_coords[1] = b;
	ground_tex_coord.tex_coords[2] = c;
	ground_tex_coord.tex_coords[3] = d;

	StelApp::getInstance().getTextureManager().setWrapMode(GL_REPEAT);
	fog_tex = StelApp::getInstance().getTextureManager().createTexture(param["path"] + param["fogtex"]);
	s = param["fog"];
	sscanf(s.c_str(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	fog_tex_coord.tex = fog_tex;
	fog_tex_coord.tex_coords[0] = a;
	fog_tex_coord.tex_coords[1] = b;
	fog_tex_coord.tex_coords[2] = c;
	fog_tex_coord.tex_coords[3] = d;

	fog_alt_angle = StelUtils::stringToDouble(param["fog_alt_angle"]);
	fog_angle_shift = StelUtils::stringToDouble(param["fog_angle_shift"]);
	decor_alt_angle = StelUtils::stringToDouble(param["decor_alt_angle"]);
	decor_angle_shift = StelUtils::stringToDouble(param["decor_angle_shift"]);
	decor_angle_rotatez = StelUtils::stringToDouble(param["decor_angle_rotatez"]);
	ground_angle_shift = StelUtils::stringToDouble(param["ground_angle_shift"]);
	ground_angle_rotatez = StelUtils::stringToDouble(param["ground_angle_rotatez"]);
	draw_ground_first = StelUtils::stringToInt(param["draw_ground_first"]);
}

void LandscapeOldStyle::draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav)
{
	if(!valid_landscape) return;
	if (draw_ground_first) draw_ground(eye, prj, nav);
	draw_decor(eye, prj, nav);
	if (!draw_ground_first) draw_ground(eye, prj, nav);
	draw_fog(eye, prj, nav);
}


// Draw the horizon fog
void LandscapeOldStyle::draw_fog(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const
{
	if(!fog_fader.getInterstate()) return;
	glBlendFunc(GL_ONE, GL_ONE);
	glColor3f(fog_fader.getInterstate()*(0.1f+0.1f*sky_brightness), fog_fader.getInterstate()*(0.1f+0.1f*sky_brightness), fog_fader.getInterstate()*(0.1f+0.1f*sky_brightness));
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	fog_tex->bind();
	
	const double vpos = tanMode ? radius*std::tan(fog_angle_shift*M_PI/180.) : radius*std::sin(fog_angle_shift*M_PI/180.);
	prj->setCustomFrame(nav->get_local_to_eye_mat() * Mat4d::translation(Vec3d(0.,0.,vpos)));
	
	const double height = tanMode ? radius*std::tan(fog_alt_angle*M_PI/180.) : radius*std::sin(fog_alt_angle*M_PI/180.);
	prj->sCylinder(radius, height, 128, 1, 1);
	
	glDisable(GL_CULL_FACE);
}

// Draw the mountains with a few pieces of texture
void LandscapeOldStyle::draw_decor(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const
{
	if (!land_fader.getInterstate()) return;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glColor4f(sky_brightness, sky_brightness, sky_brightness,
	          land_fader.getInterstate());
	prj->setCurrentFrame(Projector::FRAME_LOCAL);

	const int stacks = 8;
	  // make slices_per_side=(3<<K) so that the innermost polygon of the
	  // fandisk becomes a triangle:
	int slices_per_side = 3*64/(nb_decor_repeat*nb_side);
	if (slices_per_side<=0) slices_per_side = 1;
	const double z0 = tanMode ? radius * std::tan(decor_angle_shift*M_PI/180.0) : 
		radius * std::sin(decor_angle_shift*M_PI/180.0);
	const double d_z = tanMode ? radius * std::tan(decor_alt_angle*M_PI/180.0) / stacks : 
		radius * std::sin(decor_alt_angle*M_PI/180.0) / stacks;
	const double alpha = 2.0*M_PI/(nb_decor_repeat*nb_side*slices_per_side);
	const double ca = cos(alpha);
	const double sa = sin(alpha);
	double y0 = radius*cos(decor_angle_rotatez*M_PI/180.0);
	double x0 = radius*sin(decor_angle_rotatez*M_PI/180.0);
	for (int n=0;n<nb_decor_repeat;n++) for (int i=0;i<nb_side;i++) {
		sides[i].tex->bind();
		double tx0 = sides[i].tex_coords[0];
		const float d_tx0 = (sides[i].tex_coords[2]-sides[i].tex_coords[0])
		                  / slices_per_side;
		const float d_ty = (sides[i].tex_coords[3]-sides[i].tex_coords[1])
		                 / stacks;
		for (int j=0;j<slices_per_side;j++) {
			const double y1 = y0*ca - x0*sa;
			const double x1 = y0*sa + x0*ca;
			const float tx1 = tx0 + d_tx0;
			double z = z0;
			float ty0 = sides[i].tex_coords[1];
			glBegin(GL_QUAD_STRIP);
			for (int k=0;k<=stacks;k++) {
				glTexCoord2f(tx0,ty0);
				prj->drawVertex3(x0, y0, z);
				glTexCoord2f(tx1,ty0);
				prj->drawVertex3(x1, y1, z);
				z += d_z;
				ty0 += d_ty;
			}
			glEnd();
			y0 = y1;
			x0 = x1;
			tx0 = tx1;
		}
	}
	glDisable(GL_CULL_FACE);
}


// Draw the ground
void LandscapeOldStyle::draw_ground(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const
{
	if (!land_fader.getInterstate()) return;
	
	const double vshift = tanMode ? radius*std::tan(ground_angle_shift*M_PI/180.) : radius*std::sin(ground_angle_shift*M_PI/180.);
	Mat4d mat = nav->get_local_to_eye_mat() * Mat4d::zrotation(ground_angle_rotatez*M_PI/180.f) * Mat4d::translation(Vec3d(0,0,vshift));

	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	ground_tex->bind();
	  // make slices_per_side=(3<<K) so that the innermost polygon of the
	  // fandisk becomes a triangle:
	int slices_per_side = 3*64/(nb_decor_repeat*nb_side);
	if (slices_per_side<=0) slices_per_side = 1;
	prj->setCustomFrame(mat);

// draw a fan disk instead of a ordinary disk to that the inner slices
// are not so slender. When they are too slender, culling errors occur
// in cylinder projection mode.
//	prj->sDisk(radius,nb_side*slices_per_side*nb_decor_repeat,5, 1);
	int slices_inside = nb_side*slices_per_side*nb_decor_repeat;
	int level = 0;
	while ((slices_inside&1)==0 && slices_inside > 4) {
		level++;
		slices_inside>>=1;
	}
	prj->sFanDisk(radius,slices_inside,level);

	glDisable(GL_CULL_FACE);
}

LandscapeFisheye::LandscapeFisheye(float _radius) : Landscape(_radius)
{}

LandscapeFisheye::~LandscapeFisheye()
{
}

void LandscapeFisheye::load(const string& landscape_file, const string& landscapeId)
{
	loadCommon(landscape_file, landscapeId);
	
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string type = pd.get_str("landscape", "type");
	if(type != "fisheye")
	{
		cerr << "Landscape type mismatch for landscape "<< landscapeId << ", expected fisheye, found " << type << ".  No landscape in use.\n";
		valid_landscape = 0;
		return;
	}
	create(name, 0, getTexturePath(pd.get_str("landscape", "maptex"), landscapeId), pd.get_double("landscape", "texturefov", 360));
}


// create a fisheye landscape from basic parameters (no ini file needed)
void LandscapeFisheye::create(const wstring _name, bool _fullpath, const string _maptex, double _texturefov)
{
	//	cout << _name << " " << _fullpath << " " << _maptex << " " << _texturefov << "\n";
	valid_landscape = 1;  // assume ok...
	name = _name;
	StelApp::getInstance().getTextureManager().setDefaultParams();
	map_tex = StelApp::getInstance().getTextureManager().createTexture(_maptex);
	tex_fov = _texturefov*M_PI/180.;
}


void LandscapeFisheye::draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav)
{
	if(!valid_landscape) return;
	if(!land_fader.getInterstate()) return;

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	map_tex->bind();
	prj->setCurrentFrame(Projector::FRAME_LOCAL);
	prj->sSphere_map(radius,40,20, tex_fov, 1);

	glDisable(GL_CULL_FACE);
}


// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius) : Landscape(_radius)
{}

LandscapeSpherical::~LandscapeSpherical()
{
}

void LandscapeSpherical::load(const string& landscape_file, const string& landscapeId)
{
	loadCommon(landscape_file, landscapeId);
	
	InitParser pd;	// The landscape data ini file parser
	pd.load(landscape_file);

	string type = pd.get_str("landscape", "type");
	if(type != "spherical" )
	{
		cerr << "Landscape type mismatch for landscape "<< landscapeId << ", expected spherical, found " << type << ".  No landscape in use.\n";
		valid_landscape = 0;
		return;
	}

	create(name, 0, getTexturePath(pd.get_str("landscape", "maptex"),landscapeId));

}


// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const wstring _name, bool _fullpath, const string _maptex)
{
	//	cout << _name << " " << _fullpath << " " << _maptex << " " << _texturefov << "\n";
	valid_landscape = 1;  // assume ok...
	name = _name;
	StelApp::getInstance().getTextureManager().setDefaultParams();
	map_tex = StelApp::getInstance().getTextureManager().createTexture(_maptex);
}


void LandscapeSpherical::draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav)
{
	if(!valid_landscape) return;
	if(!land_fader.getInterstate()) return;

	// Need to flip texture usage horizontally due to glusphere convention
	// so that left-right is consistent in source texture and rendering
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glScalef(-1,1,1);
	glTranslatef(-1,0,0);
	glMatrixMode(GL_MODELVIEW);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(sky_brightness, sky_brightness, sky_brightness, land_fader.getInterstate());

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	map_tex->bind();

	// TODO: verify that this works correctly for custom projections
	// seam is at East
	prj->setCurrentFrame(Projector::FRAME_LOCAL);
	prj->sSphere(radius,1.0,40,20, 1);

	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

