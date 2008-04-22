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

#include <QTextStream>
#include <QFile>
#include <QString>

#include "Nebula.hpp"
#include "NebulaMgr.hpp"
#include "STexture.hpp"
#include "SFont.hpp"
#include "Navigator.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "SFont.hpp"
#include "ToneReproducer.hpp"
#include "StelModuleMgr.hpp"
#include <QDebug>

//#include <iostream>

STextureSP Nebula::tex_circle;
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
		IC_nb(0)
{
	nameI18 = "";
	angularSize = -1;
}

Nebula::~Nebula()
{
}

QString Nebula::getInfoString(const Navigator* nav) const
{
	double tempDE, tempRA;

	Vec3d equPos = nav->j2000_to_earth_equ(XYZ);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,equPos);

	QString str;
	QTextStream oss(&str);
	if (nameI18!="")
	{
		oss << nameI18 << " (";
	}
	if ((M_nb > 0) && (M_nb < 111))
	{
		oss << "M " << M_nb << " - ";
	}
	if (NGC_nb > 0)
	{
		oss << "NGC " << NGC_nb;
	}
	if (IC_nb > 0)
	{
		oss << "IC " << IC_nb;
	}
	/*if (UGC_nb > 0)
	{
		oss << L"UGC " << UGC_nb;
	}*/
	if (nameI18!="")
	{
		oss << ")";
	}
	oss << endl;

	oss.setRealNumberNotation(QTextStream::FixedNotation);
	oss.setRealNumberPrecision(2);
	oss << q_("Magnitude: ") << mag << endl;	
	
	oss << q_("RA/DE: ") << StelUtils::radToHmsStr(tempRA) << "/" << StelUtils::radToDmsStr(tempDE) << endl;
	// calculate alt az
	Vec3d localPos = nav->earth_equ_to_local(equPos);
	StelUtils::rect_to_sphe(&tempRA,&tempDE,localPos);
	tempRA = 3*M_PI - tempRA;  // N is zero, E is 90 degrees
	if(tempRA > M_PI*2) tempRA -= M_PI*2;	
	oss << q_("Az/Alt: ") << StelUtils::radToDmsStr(tempRA) << "/" << StelUtils::radToDmsStr(tempDE) << endl;
	
	oss << q_("Type: ") << getTypeString() << endl;
	if (angularSize>0)
		oss << q_("Size: ") << StelUtils::radToDmsStr(angularSize*M_PI/180.) << endl;
	
	return str;
}

QString Nebula::getShortInfoString(const Navigator*) const
{
	if (nameI18!="")
	{
		QString str;
		QTextStream oss(&str);
		oss << nameI18 << L"  ";
		if (mag < 99)
			oss << q_("Magnitude: ") << mag;
		
		return str;
	}
	else
	{
		if (M_nb > 0)
		{
			return QString("M %1").arg(M_nb);
		}
		else if (NGC_nb > 0)
		{
			return QString("NGC %1").arg(NGC_nb);
		}
		else if (IC_nb > 0)
		{
			return QString("IC %1").arg(IC_nb);
		}
	}
	
	// All nebula have at least an NGC or IC number
	assert(false);
	return "";
}

 float Nebula::getSelectPriority(const Navigator *nav) const
 {
	if( ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getFlagHints() )
	{
		// make very easy to select IF LABELED
		return -10;
	}
	else
	{
		if (getMagnitude(nav)>20) return 20;
		return getMagnitude(nav);
	}
 }

Vec3f Nebula::getInfoColor(void) const
{
	return ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getNamesColor();
}

double Nebula::getCloseViewFov(const Navigator*) const
{
	return angularSize * 4;
}

// Read nebula data from file and compute x,y and z;
// returns false if can't parse record
bool Nebula::readTexture(const QString& setName, const QString& record)
{
	QString tex_name;
	QString name;
	float ra;
	float de;
	float tex_angular_size;
	float tex_rotation;
	int ngc;
	
	QString buf = record;
	QTextStream istr(&buf);


	istr >> ngc >> ra >> de >> mag >> tex_angular_size >> tex_rotation >> name >> tex_name >> credit;
	if (istr.status() != QTextStream::Ok)
		return false;

	if (credit  == "none")
		credit = "";
	else
		credit = "Credit: " + credit;

	credit.replace('_', " ");

	// Only set name if not already set from NGC data
	if(englishName == "") 
	{
		name.replace('_', " ");
		englishName = name;
	}

	// Calc the RA and DE from the datas
	float RaRad = ra*M_PI/180.;
	float DecRad = de*M_PI/180.;

	// Calc the Cartesian coord with RA and DE
	StelUtils::sphe_to_rect(RaRad,DecRad,XYZ);
	XYZ*=RADIUS_NEB;

	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setMipmapsMode(true);
	
	nebTex = StelApp::getInstance().getTextureManager().createTextureThread(QString("nebulae/")+setName+"/"+tex_name);
	
	luminance = ToneReproducer::magToLuminance(mag, tex_angular_size*tex_angular_size*3600);

	float texSize = RADIUS_NEB * sin(tex_angular_size/2/60*M_PI/180);
	
	// Only in case the texture was not previously loaded from NGC cat
	if (angularSize<0)
		angularSize = tex_angular_size;

	// Precomputation of the rotation/translation matrix
	Mat4f matPrecomp = Mat4f::translation(XYZ) *
	                   Mat4f::zrotation(RaRad) *
	                   Mat4f::yrotation(-DecRad) *
	                   Mat4f::xrotation(tex_rotation*M_PI/180.);

	tex_quad_vertex[0] = matPrecomp * Vec3f(0.,-texSize,-texSize);
	tex_quad_vertex[1] = matPrecomp * Vec3f(0., texSize,-texSize);
	tex_quad_vertex[3] = matPrecomp * Vec3f(0.,-texSize, texSize);
	tex_quad_vertex[2] = matPrecomp * Vec3f(0., texSize, texSize);

#if 0
	QString outJSON;

	Vec3d vv[4];
	vv[0] = tex_quad_vertex[1];
	vv[1] = tex_quad_vertex[0];
	vv[2] = tex_quad_vertex[3];
	vv[3] = tex_quad_vertex[2];
	int w,h;
	nebTex->getDimensions(w,h);
	double resolution = MY_MIN(tex_quad_vertex[0].angle(tex_quad_vertex[1]), tex_quad_vertex[0].angle(tex_quad_vertex[2]));
	resolution*=180./M_PI;
	outJSON += "{\n\
\t\"credit\" : \""+credit+"\",\n\
\t\"imageUrl\" : \"nebulae/default/"+tex_name+"\",\n\
\t\"skyConvexPolygons\" : [[";
	double lon, lat;
	for (int i=0;i<4;++i)
	{
		StelUtils::rect_to_sphe(&lon, &lat, vv[i]);
		outJSON+=QString("[%1, %2]").arg(lon*180./M_PI, 0, 'g', 10).arg(lat*180./M_PI, 0, 'g', 8);
		if (i!=3)
			outJSON+=", ";
	}
	outJSON += QString("]],\n\
\t\"textureCoords\" : [[[0,0], [1,0], [1,1], [0,1]]],\n\
\t\"minResolution\" : %1,\n\
\t\"luminance\" : %2\n\
},\n").arg(resolution, 0, 'g', 10).arg(luminance, 0, 'g', 8);
	//std::cout << outJSON.toStdString();
#endif
					   
	return true;
}


void Nebula::draw_tex(const Projector* prj, const Navigator* nav, ToneReproducer* eye)
{
	if (!nebTex || !flagShowTexture) return;

	if (!nebTex->bind())
		return;
	
	// if start zooming in, turn up brightness to full for DSO images
	// gradual change might be better

	if(flagBright && getOnScreenSize(prj, nav)>12.)
	{
		glColor4f(1.0,1.0,1.0,1.0);
	}
	else
	{
		float ad_lum=eye->adaptLuminance(luminance);

		// TODO this should be revisited to be less ad hoc
		// 3 is a fudge factor since only about 1/3 of a texture is not black background
		float texLum=0;
		if (!nebTex->getAverageLuminance(texLum))
		{
			qWarning() << "Use undefined texture luminance";
		}
		float cmag = 3 * ad_lum / texLum;
		glColor3f(cmag,cmag,cmag);
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	Vec3d v;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(1,0);              // Bottom Right
		prj->drawVertex3v(tex_quad_vertex[0]);
        glTexCoord2i(0,0);              // Bottom Left
		prj->drawVertex3v(tex_quad_vertex[1]);
        glTexCoord2i(1,1);              // Top Right
		prj->drawVertex3v(tex_quad_vertex[3]);
        glTexCoord2i(0,1);              // Top Left
		prj->drawVertex3v(tex_quad_vertex[2]);
    glEnd();
}

void Nebula::draw_circle(const Projector* prj, const Navigator * nav)
{
	if (4.f/getOnScreenSize(prj, nav)<0.1) return;
	glBlendFunc(GL_ONE, GL_ONE);
	float lum = MY_MIN(1,4.f/getOnScreenSize(prj, nav))*0.8;
	glColor3f(circle_color[0]*lum*hints_brightness, circle_color[1]*lum*hints_brightness, circle_color[2]*lum*hints_brightness);
	Nebula::tex_circle->bind();
	prj->drawSprite2dMode(XY[0], XY[1], 8);
}

void Nebula::draw_no_tex(const Projector* prj, const Navigator * nav,ToneReproducer* eye)
{
	float d = getOnScreenSize(prj, nav);
	float cmag = 0.20 * hints_brightness;

	glColor3f(cmag,cmag,cmag);
	tex_circle->bind();
	prj->drawSprite2dMode(XY[0], XY[1], d);
}

void Nebula::draw_name(const Projector* prj)
{
	glColor4f(label_color[0], label_color[1], label_color[2], hints_brightness);
	float size = getOnScreenSize(prj);
	float shift = 4.f + size/1.8f;

	QString nebulaname = getNameI18n();

	prj->drawText(nebula_font,XY[0]+shift, XY[1]+shift, nebulaname, 0, 0, 0, false);

	// draw image credit, if it fits easily
	if(flagShowTexture && credit != "" && size > nebula_font->getStrLen(credit))
	{
		prj->drawText(nebula_font,XY[0]+shift, XY[1]-shift, credit, 0, 0, 0, false);
	}
}

bool Nebula::readNGC(char *recordstr)
{
	int rahr;
	float ramin;
	int dedeg;
	float demin;
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

	sscanf(&recordstr[47],"%f",&mag);
	if (mag < 1) mag = 99;

	// Calc the angular size in radian : TODO this should be independant of tex_angular_size
	float size;
	sscanf(&recordstr[40],"%f",&size);

	angularSize = size/60;
	if (angularSize<0)
		angularSize=0;
	
	if (size < 0)
		size = 1;

	luminance = ToneReproducer::magToLuminance(mag, size*size*3600);
	if (luminance < 0)
		luminance = .0075;

	// this is a huge performance drag if called every frame, so cache here
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

QString Nebula::getTypeString(void) const
{
	QString wsType;

	switch(nType)
	{
		case NEB_GX:
			wsType = "Galaxy";
			break;
		case NEB_OC:
			wsType = "Open cluster";
			break;
		case NEB_GC:
			wsType = "Globular cluster";
			break;
		case NEB_N:
			wsType = "Nebula";
			break;
		case NEB_PN:
			wsType = "Planetary nebula";
			break;
		case NEB_CN:
			wsType = "Cluster associated with nebulosity";
			break;
		case NEB_UNKNOWN:
			wsType = "Unknown";
			break;
		default:
			wsType = "Undocumented type";
			break;
	}
	return wsType;
}

