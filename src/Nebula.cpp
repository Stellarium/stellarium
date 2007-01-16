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
#include <sstream>
#include "Nebula.hpp"
#include "NebulaMgr.hpp"
#include "STexture.hpp"
#include "stellarium.h"
#include "SFont.hpp"
#include "Navigator.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"

STexture * Nebula::tex_circle = NULL;
SFont* Nebula::nebula_font = NULL;
float Nebula::circleScale = 1.f;
float Nebula::hints_brightness = 0;
Vec3f Nebula::label_color = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circle_color = Vec3f(0.8,0.8,0.1);
bool Nebula::flagShowTexture = false;
bool Nebula::flagBright = false;
const float Nebula::RADIUS_NEB = 1.f;

Nebula::Nebula() :
		M_nb(0),
		NGC_nb(0),
		IC_nb(0),
		/*UGC_nb(0),*/
		neb_tex(NULL)
{
	nameI18 = L"";
}

Nebula::~Nebula()
{
	delete neb_tex;
	neb_tex = NULL;

	delete tex_circle;
	tex_circle = NULL;
}

wstring Nebula::getInfoString(const Navigator* nav) const
{
	double tempDE, tempRA;

	Vec3d equPos = nav->j2000_to_earth_equ(XYZ);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,equPos);

	wostringstream oss;
	if (nameI18!=L"")
	{
		oss << nameI18 << L" (";
	}
	if ((M_nb > 0) && (M_nb < 111))
	{
		oss << L"M " << M_nb << L" - ";
	}
	if (NGC_nb > 0)
	{
		oss << L"NGC " << NGC_nb;
	}
	if (IC_nb > 0)
	{
		oss << L"IC " << IC_nb;
	}
	/*if (UGC_nb > 0)
	{
		oss << L"UGC " << UGC_nb;
	}*/
	if (nameI18!=L"")
	{
		oss << L")";
	}
	oss << endl;
	
	oss.setf(ios::fixed);
	oss.precision(2);
	oss << _("Magnitude: ") << mag << endl;	
	
	oss << _("RA/DE: ") << StelUtils::printAngleHMS(tempRA) << L"/" << StelUtils::printAngleDMS(tempDE) << endl;
	// calculate alt az
	Vec3d localPos = nav->earth_equ_to_local(equPos);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,localPos);
	tempRA = 3*M_PI - tempRA;  // N is zero, E is 90 degrees
	if(tempRA > M_PI*2) tempRA -= M_PI*2;	
	oss << _("Az/Alt: ") << StelUtils::printAngleDMS(tempRA) << L"/" << StelUtils::printAngleDMS(tempDE) << endl;
	
	oss << _("Type: ") << getTypeString() << endl;
	oss << _("Size: ") << StelUtils::printAngleDMS(angular_size*M_PI/180.) << endl;

	return oss.str();
}

wstring Nebula::getShortInfoString(const Navigator*) const
{
	if (nameI18!=L"")
	{
		wostringstream oss;
		oss << nameI18 << L"  ";
		if (mag < 99) oss << _("Magnitude: ") << mag;
		
		return oss.str();
	}
	else
	{
		if (M_nb > 0)
		{
			return L"M " + StelUtils::intToWstring(M_nb);
		}
		else if (NGC_nb > 0)
		{
			return L"NGC " + StelUtils::intToWstring(NGC_nb);
		}
		else if (IC_nb > 0)
		{
			return L"IC " + StelUtils::intToWstring(IC_nb);
		}
	}
	
	// All nebula have at least an NGC or IC number
	assert(false);
	return L"";
}

 float Nebula::getSelectPriority(const Navigator *nav) const
 {
	if( ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("nebulas"))->getFlagHints() )
	{
		// make very easy to select IF LABELED
		return -10;
	}
	else
	{
		if (get_mag(nav)>20) return 20;
		return get_mag(nav);
	}
 }

Vec3f Nebula::getInfoColor(void) const
{
	return ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("nebulas"))->getNamesColor();
}

double Nebula::get_close_fov(const Navigator*) const
{
	return angular_size * 180./M_PI * 4;
}

// Read nebula data from file and compute x,y and z;
// returns false if can't parse record
bool Nebula::readTexture(const string& record)
{
	string tex_name;
	string name;
	float ra;
	float de;
	float tex_angular_size;
	float tex_rotation;
	int ngc;
	
	std::istringstream istr(record);

	if (!(istr >> ngc >> ra >> de >> mag >> tex_angular_size >> tex_rotation >> name >> tex_name >> credit)) return false ;

	if (credit  == "none")
		credit = "";
	else
		credit = string("Credit: ") + credit;

	for (string::size_type i=0;i<credit.length();++i)
	{
		if (credit[i]=='_') credit[i]=' ';
	}

	// Only set name if not already set from NGC data
	if(englishName == "") {
		for (string::size_type i=0;i<name.length();++i)
			{
				if (name[i]=='_') name[i]=' ';
			}
		englishName = name;
	}

	// Calc the RA and DE from the datas
	float RaRad = ra*M_PI/180.;
	float DecRad = de*M_PI/180.;

	// Calc the Cartesian coord with RA and DE
	StelUtils::sphe_to_rect(RaRad,DecRad,XYZ);
	XYZ*=RADIUS_NEB;

	// Calc the angular size in radian : TODO this should be independant of tex_angular_size
	angular_size = tex_angular_size/2/60*M_PI/180;

	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setMipmapsMode(true);
	neb_tex = &StelApp::getInstance().getTextureManager().createTexture(tex_name, true);

	luminance = ToneReproducer::mag_to_luminance(mag, tex_angular_size*tex_angular_size*3600);


	float tex_size = RADIUS_NEB * sin(tex_angular_size/2/60*M_PI/180);

	// Precomputation of the rotation/translation matrix
	Mat4f mat_precomp = Mat4f::translation(XYZ) *
	                    Mat4f::zrotation(RaRad) *
	                    Mat4f::yrotation(-DecRad) *
	                    Mat4f::xrotation(tex_rotation*M_PI/180.);

	tex_quad_vertex[0] = mat_precomp * Vec3f(0.,-tex_size,-tex_size); // Bottom Right
	tex_quad_vertex[1] = mat_precomp * Vec3f(0., tex_size,-tex_size); // Bottom Right
	tex_quad_vertex[2] = mat_precomp * Vec3f(0.,-tex_size, tex_size); // Bottom Right
	tex_quad_vertex[3] = mat_precomp * Vec3f(0., tex_size, tex_size); // Bottom Right

	return true;
}


void Nebula::draw_tex(const Projector* prj, const Navigator* nav, ToneReproducer* eye)
{
	if (!neb_tex || !flagShowTexture) return;

	neb_tex->lazyBind();
	
	// if start zooming in, turn up brightness to full for DSO images
	// gradual change might be better

	if(flagBright && get_on_screen_size(prj, nav)>12.)
	{
		glColor4f(1.0,1.0,1.0,1.0);
	}
	else
	{
		float ad_lum=eye->adapt_luminance(luminance);

		// TODO this should be revisited to be less ad hoc
		// 3 is a fudge factor since only about 1/3 of a texture is not black background
		float cmag = 3 * ad_lum / neb_tex->getAverageLuminance();
		glColor3f(cmag,cmag,cmag);

		//		printf("%s: lum %f ad_lum %f cmag %f angle %f\n", name.c_str(), luminance, ad_lum, cmag, angular_size);
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	Vec3d v;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
		prj->project_j2000(tex_quad_vertex[0],v); glVertex3dv(v);
        glTexCoord2i(0,0);              // Bottom Left
		prj->project_j2000(tex_quad_vertex[1],v); glVertex3dv(v);
        glTexCoord2i(1,1);              // Top Right
		prj->project_j2000(tex_quad_vertex[2],v); glVertex3dv(v);
        glTexCoord2i(0,1);              // Top Left
		prj->project_j2000(tex_quad_vertex[3],v); glVertex3dv(v);
    glEnd();
}

void Nebula::draw_circle(const Projector* prj, const Navigator * nav)
{
	if (2.f/get_on_screen_size(prj, nav)<0.1) return;
	glBlendFunc(GL_ONE, GL_ONE);
	float lum = MY_MIN(1,2.f/get_on_screen_size(prj, nav))*0.8;
	glColor3f(circle_color[0]*lum*hints_brightness, circle_color[1]*lum*hints_brightness, circle_color[2]*lum*hints_brightness);
	Nebula::tex_circle->bind();
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(1,0);              // Bottom Right
		glVertex3f(XY[0] + 4, XY[1] - 4, 0.0f);
		glTexCoord2i(0,0);              // Bottom Left
		glVertex3f(XY[0] - 4, XY[1] - 4, 0.0f);
		glTexCoord2i(1,1);              // Top Right
		glVertex3f(XY[0] + 4, XY[1] + 4,0.0f);
		glTexCoord2i(0,1);              // Top Left
		glVertex3f(XY[0] - 4, XY[1] + 4,0.0f);
	glEnd ();
}

void Nebula::draw_no_tex(const Projector* prj, const Navigator * nav,ToneReproducer* eye)
{
	float r = (get_on_screen_size(prj, nav)/2);
	float cmag = 0.20 * hints_brightness;

	glColor3f(cmag,cmag,cmag);
	tex_circle->bind();
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);    glVertex2f(XY[0]-r,XY[1]-r);	// Bottom left
		glTexCoord2i(1,0);    glVertex2f(XY[0]+r,XY[1]-r);	// Bottom right
		glTexCoord2i(1,1);    glVertex2f(XY[0]+r,XY[1]+r);	// Top right
		glTexCoord2i(0,1);    glVertex2f(XY[0]-r,XY[1]+r);	// Top left
	glEnd();
}

void Nebula::draw_name(const Projector* prj)
{
	glColor4f(label_color[0], label_color[1], label_color[2], hints_brightness);
	float size = get_on_screen_size(prj);
	float shift = 8.f + size/2.f;

	wstring nebulaname = getNameI18n();

	if (prj->getFlagGravityLabels())
		prj->print_gravity180(nebula_font, XY[0]+shift, XY[1]+shift, nebulaname, 1, 0, 0);
	else
		nebula_font->print(XY[0]+shift, XY[1]+shift, nebulaname);

	// draw image credit, if it fits easily
	if(flagShowTexture && credit != "" && size > nebula_font->getStrLen(credit))
	{
		if (prj->getFlagGravityLabels())
			prj->print_gravity180(nebula_font, XY[0]-shift-40, XY[1]+-shift-40, credit, 1, 0, 0);
		else
			nebula_font->print(XY[0]-shift, XY[1]-shift-60, credit);
	}
}

bool Nebula::readNGC(char *recordstr)
{
	int rahr;
	float ramin;
	int dedeg;
	float demin;
	float tex_angular_size;
	int nb;

	sscanf(&recordstr[1],"%d",&nb);

	if (recordstr[0] == 'I')
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}

	sscanf(&recordstr[12],"%d %f",&rahr, &ramin);
	sscanf(&recordstr[22],"%d %f",&dedeg, &demin);
	float RaRad = (double)rahr+ramin/60;
	float DecRad = (float)dedeg+demin/60;
	if (recordstr[21] == '-') DecRad *= -1.;

	RaRad*=M_PI/12.;     // Convert from hours to rad
	DecRad*=M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::sphe_to_rect(RaRad,DecRad,XYZ);
	XYZ*=Nebula::RADIUS_NEB;

	// Calc the angular size in radian : TODO this should be independant of tex_angular_size
	sscanf(&recordstr[47],"%f",&mag);
	if (mag < 1) mag = 99;

	sscanf(&recordstr[40],"%f",&tex_angular_size);
	if (tex_angular_size < 0)
		tex_angular_size = 1;
	if (tex_angular_size > 150)
		tex_angular_size = 150;

	angular_size = tex_angular_size/2/60*M_PI/180;

	luminance = ToneReproducer::mag_to_luminance(mag, tex_angular_size*tex_angular_size*3600);
	if (luminance < 0)
		luminance = .0075;

	// this is a huge performance drag if called every frame, so cache here
	if (neb_tex) delete neb_tex;
	neb_tex = NULL;

	if (!strncmp(&recordstr[8],"Gx",2)) { nType = NEB_GX;}
	else if (!strncmp(&recordstr[8],"OC",2)) { nType = NEB_OC;}
	else if (!strncmp(&recordstr[8],"Gb",2)) { nType = NEB_GC;}
	else if (!strncmp(&recordstr[8],"Nb",2)) { nType = NEB_N;}
	else if (!strncmp(&recordstr[8],"Pl",2)) { nType = NEB_PN;}
	else if (!strncmp(&recordstr[8],"  ",2)) { return false;}
	else if (!strncmp(&recordstr[8]," -",2)) { return false;}
	else if (!strncmp(&recordstr[8]," *",2)) { return false;}
	else if (!strncmp(&recordstr[8],"D*",2)) { return false;}
	else if (!strncmp(&recordstr[7],"***",3)) { return false;}
	else if (!strncmp(&recordstr[7],"C+N",3)) { nType = NEB_CN;}
	else if (!strncmp(&recordstr[8]," ?",2)) { nType = NEB_UNKNOWN;}
	else { nType = NEB_UNKNOWN;}

	return true;
}

wstring Nebula::getTypeString(void) const
{
	wstring wsType;

	switch(nType)
	   {
		case NEB_GX:
			wsType = L"Galaxy";
			break;
		case NEB_OC:
			wsType = L"Open cluster";
			break;
		case NEB_GC:
			wsType = L"Globular cluster";
			break;
		case NEB_N:
			wsType = L"Nebula";
			break;
		case NEB_PN:
			wsType = L"Planetary nebula";
			break;
		case NEB_CN:
			wsType = L"Cluster associated with nebulosity";
			break;
		case NEB_UNKNOWN:
			wsType = L"Unknown";
			break;
		default:
			wsType = L"Undocumented type";
			break;
	}
	return wsType;
}

