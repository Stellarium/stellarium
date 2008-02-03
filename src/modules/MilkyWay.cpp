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

#include "MilkyWay.hpp"
#include "Fader.hpp"
#include "STexture.hpp"
#include "StelUtils.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "ToneReproducer.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"
#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay() : radius(1.f), color(1.f, 1.f, 1.f)
{
	setObjectName("MilkyWay");
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	delete fader;
	fader = NULL;
}

void MilkyWay::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	setTexture("milkyway.png");
	setFlagShow(conf->value("astro/flag_milky_way").toBool());
	setIntensity(conf->value("astro/milky_way_intensity",1.).toDouble());
}

void MilkyWay::setTexture(const QString& texFile)
{
	StelApp::getInstance().getTextureManager().setDefaultParams();
	tex = StelApp::getInstance().getTextureManager().createTexture(texFile);

	// big performance improvement to cache this
	if (!tex->getAverageLuminance(tex_avg_luminance))
		qWarning() << "Luminance used from unloaded image";
}

void MilkyWay::update(double deltaTime) {fader->update((int)(deltaTime*1000));}

void MilkyWay::setFlagShow(bool b){*fader = b;}
bool MilkyWay::getFlagShow(void) const {return *fader;}
	
double MilkyWay::draw(StelCore* core)
{
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	ToneReproducer* eye = core->getToneReproducer();
	
	assert(tex);	// A texture must be loaded before calling this
	
	// Scotopic color = 0.25, 0.25 in xyY mode. Global stars luminance ~= 0.001 cd/m^2
	Vec3f c = Vec3f(0.25f, 0.25f, 0.0001f*fader->getInterstate());
	//cerr << eye->adaptLuminance(c[2]) << endl;
	eye->xyYToRGB(c);
	//cerr << tex_avg_luminance << endl;
	c = Vec3f(c[0]*c[0], c[1]*c[1], c[2]*c[2]);
	c*=intensity/tex_avg_luminance*6;
	//c-=Vec3f(0.3,0.3,0.3);
	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;
	glColor3fv(c);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	tex->bind();

	prj->setCustomFrame(nav->get_j2000_to_eye_mat()*
		     Mat4d::xrotation(M_PI/180*23)*
		     Mat4d::yrotation(M_PI/180*120)*
		     Mat4d::zrotation(M_PI/180*7));
	prj->sSphere(radius,1.0,20,20,1);

	glDisable(GL_CULL_FACE);
	return 0.;
}
