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

#include "MilkyWay.h"
#include "fader.h"
#include "STexture.h"
#include "stel_utility.h"
#include "navigator.h"
#include "projector.h"
#include "tone_reproductor.h"
#include "StelApp.h"
#include "StelTextureMgr.h"

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay() : radius(1.f), color(1.f, 1.f, 1.f)
{
	tex = NULL;
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	if (tex) delete tex;
	tex = NULL;
	delete fader;
	fader = NULL;
}

void MilkyWay::init(const InitParser& conf, LoadingBar& lb)
{
	setTexture("milkyway.png");
	setFlagShow(conf.get_boolean("astro:flag_milky_way"));
	setIntensity(conf.get_double("astro","milky_way_intensity",1.));
}

void MilkyWay::setTexture(const string& tex_file)
{
	if (tex) delete tex;
	StelApp::getInstance().getTextureManager().setDefaultParams();
	tex = &StelApp::getInstance().getTextureManager().createTexture(tex_file);

	// big performance improvement to cache this
	tex_avg_luminance = tex->getAverageLuminance();
}

void MilkyWay::update(double deltaTime) {fader->update((int)(deltaTime*1000));}

void MilkyWay::setFlagShow(bool b){*fader = b;}
bool MilkyWay::getFlagShow(void) const {return *fader;}
	
double MilkyWay::draw(Projector *prj, const Navigator *nav, ToneReproductor *eye)
{
	assert(tex);	// A texture must be loaded before calling this
	// Scotopic color = 0.25, 0.25 in xyY mode. Global stars luminance ~= 0.001 cd/m^2
	Vec3f c = Vec3f(0.25f*fader->getInterstate(), 0.25f*fader->getInterstate(),
                    intensity*0.002f*fader->getInterstate()/tex_avg_luminance);
	eye->xyY_to_RGB(c);
	glColor3fv(c);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	tex->bind();

	prj->sSphere(radius,1.0,20,20,
		     nav->get_j2000_to_eye_mat()*
		     Mat4d::xrotation(M_PI/180*23)*
		     Mat4d::yrotation(M_PI/180*120)*
		     Mat4d::zrotation(M_PI/180*7), 1);

	glDisable(GL_CULL_FACE);
	return 0.;
}
