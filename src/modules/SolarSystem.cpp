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


#include <algorithm>
#include <iostream>
#include <string>
#include "SolarSystem.hpp"
#include "STexture.hpp"
#include "stellplanet.h"
#include "Orbit.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelIniParser.hpp"
#include "Planet.hpp"
#include "SFont.hpp"

#include <QTextStream>
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QMultiMap>
#include <QMapIterator>

using namespace std;

SolarSystem::SolarSystem()
	:sun(NULL),moon(NULL),earth(NULL),selected(NULL),
	moonScale(1.), fontSize(14.),
	planet_name_font(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize)),
	flagOrbits(false),flag_light_travel_time(false), lastHomePlanet(NULL)
{
	setObjectName("SolarSystem");
}

void SolarSystem::setFontSize(float newFontSize)
{
	planet_name_font = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);
}

SolarSystem::~SolarSystem()
{
	  // release selected:
	selected = NULL;
	for(vector<Planet*>::iterator iter = system_planets.begin(); iter != system_planets.end(); ++iter)
	{
		if (*iter) delete *iter;
		*iter = NULL;
	}
	for(vector<Orbit*>::iterator iter = orbits.begin(); iter != orbits.end(); ++iter)
	{
		if (*iter) delete *iter;
		*iter = NULL;
	}
	sun = NULL;
	moon = NULL;
	earth = NULL;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SolarSystem::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)+10;
	return 0;
}

// Init and load the solar system data
void SolarSystem::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	loadPlanets();	// Load planets data

	// Compute position and matrix of sun and all the satellites (ie planets)
	// for the first initialization assert that center is sun center (only impacts on light speed correction)
	computePositions(StelUtils::getJDFromSystem());
	setSelected("");	// Fix a bug on macosX! Thanks Fumio!
	setScale(conf->value("stars/star_scale",1.1).toDouble());  // if reload config
	setFlagMoonScale(conf->value("viewing/flag_moon_scaled", conf->value("viewing/flag_init_moon_scaled", "false").toBool()).toBool());  // name change
	setMoonScale(conf->value("viewing/moon_scale", 5.0).toDouble());
	setFlagPlanets(conf->value("astro/flag_planets").toBool());
	setFlagHints(conf->value("astro/flag_planets_hints").toBool());
	setFlagOrbits(conf->value("astro/flag_planets_orbits").toBool());
	setFlagLightTravelTime(conf->value("astro/flag_light_travel_time", false).toBool());
	setFlagTrails(conf->value("astro/flag_object_trails", false).toBool());
	startTrails(conf->value("astro/flag_object_trails", false).toBool());	
	
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);
	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur4.png");
}

void SolarSystem::drawPointer(const Projector* prj, const Navigator * nav)
{
	const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Planet");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getObsJ2000Pos(nav);
		Vec3d screenpos;
		prj->setCurrentFrame(Projector::FRAME_J2000);
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos)) return;
	
		glColor3f(1.0f,0.3f,0.3f);
	
		float size = obj->getOnScreenSize(prj, nav);
		size+=26.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());

		texPointer->bind();

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
        
        glPushMatrix();
        glTranslatef(screenpos[0], screenpos[1], 0.0f);
        glRotatef(StelApp::getInstance().getTotalRunTime()*10.,0,0,-1);

        glTranslatef(-size/2, -size/2,0.0f);
        glRotatef(90,0,0,1);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();

        glRotatef(-90,0,0,1);
        glTranslatef(0,size,0.0f);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();

        glRotatef(-90,0,0,1);
        glTranslatef(0, size,0.0f);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();

        glRotatef(-90,0,0,1);
        glTranslatef(0,size,0);
        glBegin(GL_QUADS );
            glTexCoord2f(0.0f,0.0f);    glVertex3f(-10,-10,0);      //Bas Gauche
            glTexCoord2f(1.0f,0.0f);    glVertex3f(10,-10,0);       //Bas Droite
            glTexCoord2f(1.0f,1.0f);    glVertex3f(10,10,0);        //Haut Droit
            glTexCoord2f(0.0f,1.0f);    glVertex3f(-10,10,0);       //Haut Gauche
        glEnd ();
		glPopMatrix();
	}
}

// Init and load the solar system data
void SolarSystem::loadPlanets()
{
	cout << "Loading Solar System data...";
	QString iniFile;
	try
	{
		iniFile = StelApp::getInstance().getFileMgr().findFile("data/ssystem.ini");
	}
	catch(exception& e)
	{
		qWarning() << "ERROR while loading ssysyem.ini (unable to find data/ssystem.ini): " << e.what() << endl;
		return;
	}
	QSettings pd(iniFile, StelIniFormat);
	if (pd.status() != QSettings::NoError)
	{
		qWarning() << "ERROR while parsing ssysyem.ini file";
		return;
	}

	// QSettings does not allow us to say that the sections of the file
	// will be listed in the same order  as in the file like the old 
	// InitParser used to so we can no longer assume that.  
	//
	// This means we must first decide what order to read the sections
	// of the file in (each section contains one planet) to avoid setting
	// the parent Planet* to one which has not yet been created.
	//
	// Stage 1: Make a map of body names back to the section names 
	// which they come from. Also make a map of body name to parent body 
	// name. These two maps can be made in a single pass through the 
	// sections of the file.
	//
	// Stage 2: Make an ordered list of section names such that each
	// item is only ever dependent on items which appear earlier in the
	// list.
	// 2a: Make a QMultiMap relating the number of levels of dependency
	//     to the body name, i.e. 
	//     0 -> Sun
	//     1 -> Mercury
	//     1 -> Venus
	//     1 -> Earth
	//     2 -> Moon
	//     etc.
	// 2b: Populate an ordered list of section names by iterating over
	//     the QMultiMap.  This type of contains is always sorted on the
	//     key, so it's easy.
	//     i.e. [sol, earth, moon] is fine, but not [sol, moon, earth]
	//
	// Stage 3: iterate over the ordered sections decided in stage 2,
	// creating the planet objects from the QSettings data.
	
	// Stage 1 (as described above).
	QMap<QString, QString> secNameMap;
	QMap<QString, QString> parentMap;
	QStringList sections = pd.childGroups();
	for (int i=0; i<sections.size(); ++i)
	{
		const QString secname = sections.at(i);
		const QString englishName = pd.value(secname+"/name").toString();
		const QString strParent = pd.value(secname+"/parent").toString();
		secNameMap[englishName] = secname;
		if (strParent!="none" && !strParent.isEmpty() && !englishName.isEmpty())
			parentMap[englishName] = strParent;
	}

	// Stage 2a (as described above).
	QMultiMap<int, QString> depLevelMap;
	for (int i=0; i<sections.size(); ++i)
	{
		const QString englishName = pd.value(sections.at(i)+"/name").toString();

		// follow dependencies, incrementing level when we have one 
		// till we run out.
		QString p=englishName;
		int level = 0;
		while(parentMap.contains(p) && parentMap[p]!="none")
		{
			level++;
			p = parentMap[p];
		}

		depLevelMap.insert(level, secNameMap[englishName]);
	}

	// Stage 2b (as described above).
	QStringList orderedSections;
	QMapIterator<int, QString> levelMapIt(depLevelMap);
	while(levelMapIt.hasNext())
	{
		levelMapIt.next();
		orderedSections << levelMapIt.value();
	}

	// Stage 3 (as described above).
	for (int i = 0;i<orderedSections.size();++i)
	{
		const QString secname = orderedSections.at(i);
		const QString englishName = pd.value(secname+"/name").toString();
		const QString strParent = pd.value(secname+"/parent").toString();
		Planet *parent = NULL;
 		if (strParent!="none")
		{
			// Look in the other planets the one named with strParent
			vector<Planet*>::iterator iter = system_planets.begin();
			while (iter != system_planets.end())
			{
				if ((*iter)->getEnglishName()==strParent.toStdString())
				{
					parent = (*iter);
				}
				iter++;
			}
			if (parent == NULL)
			{
				qWarning() << "ERROR : can't find parent solar system body for " << englishName;
				assert(0);
			}
		}

		const QString funcName = pd.value(secname+"/coord_func").toString();
		pos_func_type posfunc;
		OsulatingFunctType *osculating_func = 0;
		bool close_orbit = pd.value(secname+"/close_orbit", true).toBool();

		if (funcName=="ell_orbit")
		{
			// Read the orbital elements
			const double epoch = pd.value(secname+"/orbit_Epoch",J2000).toDouble();
			const double eccentricity = pd.value(secname+"/orbit_Eccentricity").toDouble();
			if (eccentricity >= 1.0) close_orbit = false;
			double pericenter_distance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			double semi_major_axis;
			if (pericenter_distance <= 0.0) {
				semi_major_axis = pd.value(secname+"/orbit_SemiMajorAxis",-1e100).toDouble();
				if (semi_major_axis <= -1e100) {
					qDebug() << "ERROR: " << englishName
						<< ": you must provide orbit_PericenterDistance or orbit_SemiMajorAxis";
					assert(0);
				} else {
					semi_major_axis /= AU;
					assert(eccentricity != 1.0); // parabolic orbits have no semi_major_axis
					pericenter_distance = semi_major_axis * (1.0-eccentricity);
				}
			} else {
				pericenter_distance /= AU;
				semi_major_axis = (eccentricity == 1.0)
				                ? 0.0 // parabolic orbits have no semi_major_axis
				                : pericenter_distance / (1.0-eccentricity);
			}
			double mean_motion = pd.value(secname+"/orbit_MeanMotion",-1e100).toDouble();
			double period;
			if (mean_motion <= -1e100) {
				period = pd.value(secname+"/orbit_Period",-1e100).toDouble();
				if (period <= -1e100) {
					mean_motion = (eccentricity == 1.0)
					            ? 0.01720209895 * (1.5/pericenter_distance)
					                            * sqrt(0.5/pericenter_distance)
					            : (semi_major_axis > 0.0)
					            ? 0.01720209895 / (semi_major_axis*sqrt(semi_major_axis))
					            : 0.01720209895 / (-semi_major_axis*sqrt(-semi_major_axis));
					period = 2.0*M_PI/mean_motion;
				} else {
					mean_motion = 2.0*M_PI/period;
				}
			} else {
				period = 2.0*M_PI/mean_motion;
			}
			const double inclination = pd.value(secname+"/orbit_Inclination").toDouble()*(M_PI/180.0);
			const double ascending_node = pd.value(secname+"/orbit_AscendingNode").toDouble()*(M_PI/180.0);
			double arg_of_pericenter = pd.value(secname+"/orbit_ArgOfPericenter",-1e100).toDouble();
			double long_of_pericenter;
			if (arg_of_pericenter <= -1e100) {
				long_of_pericenter = pd.value(secname+"/orbit_LongOfPericenter").toDouble()*(M_PI/180.0);
				arg_of_pericenter = long_of_pericenter - ascending_node;
			} else {
				arg_of_pericenter *= (M_PI/180.0);
				long_of_pericenter = arg_of_pericenter + ascending_node;
			}
			double mean_anomaly = pd.value(secname+"/orbit_MeanAnomaly",-1e100).toDouble();
			double mean_longitude;
			if (mean_anomaly <= -1e100) {
				mean_longitude = pd.value(secname+"/orbit_MeanLongitude").toDouble()*(M_PI/180.0);
				mean_anomaly = mean_longitude - long_of_pericenter;
			} else {
				mean_anomaly *= (M_PI/180.0);
				mean_longitude = mean_anomaly + long_of_pericenter;
			}

			// when the parent is the sun use ecliptic rathe than sun equator:
			const double parent_rot_obliquity = parent->get_parent()
			                                  ? parent->getRotObliquity()
			                                  : 0.0;
			const double parent_rot_asc_node = parent->get_parent()
			                                  ? parent->getRotAscendingnode()
			                                  : 0.0;
			// Create an elliptical orbit
			EllipticalOrbit *orb = new EllipticalOrbit(pericenter_distance,
			                                           eccentricity,
			                                           inclination,
			                                           ascending_node,
			                                           arg_of_pericenter,
			                                           mean_anomaly,
			                                           period,
			                                           epoch,
			                                           parent_rot_obliquity,
			                                           parent_rot_asc_node);
			orbits.push_back(orb);

			posfunc = pos_func_type(orb, &EllipticalOrbit::positionAtTimevInVSOP87Coordinates);
		} else
		if (funcName=="comet_orbit")
		{
			// Read the orbital elements
			const double eccentricity = pd.value(secname+"/orbit_Eccentricity",0.0).toDouble();
			if (eccentricity >= 1.0) close_orbit = false;
			double pericenter_distance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			double semi_major_axis;
			if (pericenter_distance <= 0.0) {
				semi_major_axis = pd.value(secname+"/orbit_SemiMajorAxis",-1e100).toDouble();
				if (semi_major_axis <= -1e100) {
					qWarning() << "ERROR: " << englishName
						<< ": you must provide orbit_PericenterDistance or orbit_SemiMajorAxis";
					assert(0);
				} else {
					assert(eccentricity != 1.0); // parabolic orbits have no semi_major_axis
					pericenter_distance = semi_major_axis * (1.0-eccentricity);
				}
			} else {
				semi_major_axis = (eccentricity == 1.0)
				                ? 0.0 // parabolic orbits have no semi_major_axis
				                : pericenter_distance / (1.0-eccentricity);
			}
			double mean_motion = pd.value(secname+"/orbit_MeanMotion",-1e100).toDouble();
			double period;
			if (mean_motion <= -1e100) {
				period = pd.value(secname+"/orbit_Period",-1e100).toDouble();
				if (period <= -1e100) {
					mean_motion = (eccentricity == 1.0)
					            ? 0.01720209895 * (1.5/pericenter_distance)
					                            * sqrt(0.5/pericenter_distance)
					            : (semi_major_axis > 0.0)
					            ? 0.01720209895 / (semi_major_axis*sqrt(semi_major_axis))
					            : 0.01720209895 / (-semi_major_axis*sqrt(-semi_major_axis));
					period = 2.0*M_PI/mean_motion;
				} else {
					mean_motion = 2.0*M_PI/period;
				}
			} else {
				period = 2.0*M_PI/mean_motion;
			}
			double time_at_pericenter = pd.value(secname+"/orbit_TimeAtPericenter",-1e100).toDouble();
			if (time_at_pericenter <= -1e100) {
				const double epoch = pd.value(secname+"/orbit_Epoch",-1e100).toDouble();
				double mean_anomaly = pd.value(secname+"/orbit_MeanAnomaly",-1e100).toDouble();
				if (epoch <= -1e100 || mean_anomaly <= -1e100) {
					qWarning() << "ERROR: " << englishName
						<< ": when you do not provide orbit_TimeAtPericenter, you must provide both "
						<< "orbit_Epoch and orbit_MeanAnomaly";
					assert(0);
				} else {
					mean_anomaly *= (M_PI/180.0);
					const double mean_motion = 0.01720209895 / (semi_major_axis*sqrt(semi_major_axis));
					time_at_pericenter = epoch - mean_anomaly / mean_motion;
				}
			}
			const double inclination = pd.value(secname+"/orbit_Inclination").toDouble()*(M_PI/180.0);
			const double ascending_node = pd.value(secname+"/orbit_AscendingNode").toDouble()*(M_PI/180.0);
			const double arg_of_pericenter = pd.value(secname+"/orbit_ArgOfPericenter").toDouble()*(M_PI/180.0);
			CometOrbit *orb = new CometOrbit(pericenter_distance,
			                                 eccentricity,
			                                 inclination,
			                                 ascending_node,
			                                 arg_of_pericenter,
			                                 time_at_pericenter,
			                                 mean_motion);
			orbits.push_back(orb);

			posfunc = pos_func_type(orb,&CometOrbit::positionAtTimevInVSOP87Coordinates);
		}

		if (funcName=="sun_special")
			posfunc = pos_func_type(get_sun_helio_coordsv);

		if (funcName=="mercury_special") {
			posfunc = pos_func_type(get_mercury_helio_coordsv);
			osculating_func = &get_mercury_helio_osculating_coords;
		}
        
		if (funcName=="venus_special") {
			posfunc = pos_func_type(get_venus_helio_coordsv);
			osculating_func = &get_venus_helio_osculating_coords;
		}

		if (funcName=="earth_special") {
			posfunc = pos_func_type(get_earth_helio_coordsv);
			osculating_func = &get_earth_helio_osculating_coords;
		}

		if (funcName=="lunar_special")
			posfunc = pos_func_type(get_lunar_parent_coordsv);

		if (funcName=="mars_special") {
			posfunc = pos_func_type(get_mars_helio_coordsv);
			osculating_func = &get_mars_helio_osculating_coords;
		}

		if (funcName=="phobos_special")
			posfunc = pos_func_type(get_phobos_parent_coordsv);

		if (funcName=="deimos_special")
			posfunc = pos_func_type(get_deimos_parent_coordsv);

		if (funcName=="jupiter_special") {
			posfunc = pos_func_type(get_jupiter_helio_coordsv);
			osculating_func = &get_jupiter_helio_osculating_coords;
		}

		if (funcName=="europa_special")
			posfunc = pos_func_type(get_europa_parent_coordsv);

		if (funcName=="calisto_special")
			posfunc = pos_func_type(get_callisto_parent_coordsv);

		if (funcName=="io_special")
			posfunc = pos_func_type(get_io_parent_coordsv);

		if (funcName=="ganymede_special")
			posfunc = pos_func_type(get_ganymede_parent_coordsv);

		if (funcName=="saturn_special") {
			posfunc = pos_func_type(get_saturn_helio_coordsv);
			osculating_func = &get_saturn_helio_osculating_coords;
		}

		if (funcName=="mimas_special")
			posfunc = pos_func_type(get_mimas_parent_coordsv);

		if (funcName=="enceladus_special")
			posfunc = pos_func_type(get_enceladus_parent_coordsv);

		if (funcName=="tethys_special")
			posfunc = pos_func_type(get_tethys_parent_coordsv);

		if (funcName=="dione_special")
			posfunc = pos_func_type(get_dione_parent_coordsv);

		if (funcName=="rhea_special")
			posfunc = pos_func_type(get_rhea_parent_coordsv);

		if (funcName=="titan_special")
			posfunc = pos_func_type(get_titan_parent_coordsv);

		if (funcName=="iapetus_special")
			posfunc = pos_func_type(get_iapetus_parent_coordsv);

		if (funcName=="hyperion_special")
			posfunc = pos_func_type(get_hyperion_parent_coordsv);

		if (funcName=="uranus_special") {
			posfunc = pos_func_type(get_uranus_helio_coordsv);
			osculating_func = &get_uranus_helio_osculating_coords;
		}

		if (funcName=="miranda_special")
			posfunc = pos_func_type(get_miranda_parent_coordsv);

		if (funcName=="ariel_special")
			posfunc = pos_func_type(get_ariel_parent_coordsv);

		if (funcName=="umbriel_special")
			posfunc = pos_func_type(get_umbriel_parent_coordsv);

		if (funcName=="titania_special")
			posfunc = pos_func_type(get_titania_parent_coordsv);

		if (funcName=="oberon_special")
			posfunc = pos_func_type(get_oberon_parent_coordsv);

		if (funcName=="neptune_special") {
			posfunc = pos_func_type(get_neptune_helio_coordsv);
			osculating_func = &get_neptune_helio_osculating_coords;
		}

		if (funcName=="pluto_special")
			posfunc = pos_func_type(get_pluto_helio_coordsv);


		if (posfunc.empty())
		{
			qWarning() << "ERROR : can't find posfunc " << funcName << " for " << englishName;
			exit(-1);
		}

		// Create the Planet and add it to the list
		Planet* p = new Planet(parent,
					englishName.toStdString(),
					pd.value(secname+"/halo").toBool(),
					pd.value(secname+"/lighting").toBool(),
					pd.value(secname+"/radius").toDouble()/AU,
					pd.value(secname+"/oblateness", 0.0).toDouble(),
					StelUtils::str_to_vec3f(pd.value(secname+"/color").toString()),
					pd.value(secname+"/albedo").toDouble(),
					pd.value(secname+"/tex_map").toString(),
					pd.value(secname+"/tex_halo").toString(),
					posfunc,
					osculating_func,
					close_orbit,
					pd.value(secname+"/hidden", 0).toBool());

		if (secname=="earth") earth = p;
		if (secname=="sun") sun = p;
		if (secname=="moon") moon = p;

		p->set_rotation_elements(
		    pd.value(secname+"/rot_periode", pd.value(secname+"/orbit_Period", 24.).toDouble()).toDouble()/24.,
		    pd.value(secname+"/rot_rotation_offset",0.).toDouble(),
		    pd.value(secname+"/rot_epoch", J2000).toDouble(),
		    pd.value(secname+"/rot_obliquity",0.).toDouble()*(M_PI/180.0),
		    pd.value(secname+"/rot_equator_ascending_node",0.).toDouble()*(M_PI/180.0),
		    pd.value(secname+"/rot_precession_rate",0.).toDouble()*M_PI/(180*36525),
		    pd.value(secname+"/sidereal_period",0.).toDouble());


		if (pd.value(secname+"/rings", 0).toBool()) {
			const double r_min = pd.value(secname+"/ring_inner_size").toDouble()/AU;
			const double r_max = pd.value(secname+"/ring_outer_size").toDouble()/AU;
			Ring *r = new Ring(r_min,r_max,pd.value(secname+"/tex_ring").toString());
			p->set_rings(r);
		}

		QString bighalotexfile = pd.value(secname+"/tex_big_halo", "").toString();
		if (!bighalotexfile.isEmpty())
		{
			p->set_big_halo(bighalotexfile);
			p->set_halo_size(pd.value(secname+"/big_halo_size", 50.f).toDouble());
		}

		system_planets.push_back(p);
	}

	// special case: load earth shadow texture
	StelApp::getInstance().getTextureManager().setDefaultParams();
	tex_earth_shadow = StelApp::getInstance().getTextureManager().createTexture("earth-shadow.png");
	
	cout << "(loaded)" << endl;
}

// Compute the position for every elements of the solar system.
// The order is not important since the position is computed relatively to the mother body
void SolarSystem::computePositions(double date, const Vec3d& observerPos) {
  if (flag_light_travel_time) {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      (*iter)->computePositionWithoutOrbits(date);
    }
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      const double light_speed_correction =
        ((*iter)->get_heliocentric_ecliptic_pos()-observerPos).length()
        * (AU / (SPEED_OF_LIGHT * 86400));
      (*iter)->compute_position(date-light_speed_correction);
    }
  } else {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      (*iter)->compute_position(date);
    }
  }
  
  computeTransMatrices(date, observerPos);
}

// Compute the transformation matrix for every elements of the solar system.
// The elements have to be ordered hierarchically, eg. it's important to compute earth before moon.
void SolarSystem::computeTransMatrices(double date, const Vec3d& observerPos) {
  if (flag_light_travel_time) {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      const double light_speed_correction =
        ((*iter)->get_heliocentric_ecliptic_pos()-observerPos).length()
        * (AU / (SPEED_OF_LIGHT * 86400));
      (*iter)->compute_trans_matrix(date-light_speed_correction);
    }
  } else {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      (*iter)->compute_trans_matrix(date);
    }
  }
}

// Draw all the elements of the solar system
// We are supposed to be in heliocentric coordinate
double SolarSystem::draw(StelCore* core)
{
	if(!Planet::getflagShow())
		return 0.0;
	
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	ToneReproducer* eye = core->getToneReproducer();
	
	Planet::set_font(&planet_name_font);
	
	// Set the light parameters taking sun as the light source
	const float zero[4] = {0,0,0,0};
	const float ambient[4] = {0.02,0.02,0.02,0.02};
	const float diffuse[4] = {1,1,1,1};
	glLightfv(GL_LIGHT0,GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,zero);

	glMaterialfv(GL_FRONT,GL_AMBIENT,  ambient);
	glMaterialfv(GL_FRONT,GL_DIFFUSE,  diffuse);
	glMaterialfv(GL_FRONT,GL_EMISSION, zero);
	glMaterialfv(GL_FRONT,GL_SHININESS,zero);
	glMaterialfv(GL_FRONT,GL_SPECULAR, zero);

	// Light pos in zero (sun)
	//nav->switchToHeliocentric();
	glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(0.f,0.f,0.f,1.f));
	glEnable(GL_LIGHT0);

	// Compute each Planet distance to the observer
	Vec3d obs_helio_pos = nav->getObserverHelioPos();
	
	vector<Planet*>::iterator iter;
	iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		(*iter)->compute_distance(obs_helio_pos);
		++iter;
	}

	// And sort them from the furthest to the closest
	sort(system_planets.begin(),system_planets.end(),bigger_distance());

	// Draw the elements
	double maxSquaredDistance = 0;
	iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		double squaredDistance = 0;
		if(*iter==moon && near_lunar_eclipse(nav, prj))
		{
			// TODO: moon magnitude label during eclipse isn't accurate...

			// special case to update stencil buffer for drawing lunar eclipses
			glClear(GL_STENCIL_BUFFER_BIT);
			glClearStencil(0x0);

			glStencilFunc(GL_ALWAYS, 0x1, 0x1);
			glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

			squaredDistance = (*iter)->draw(prj, nav, eye, 1);
		}
		else
		{
			squaredDistance = (*iter)->draw(prj, nav, eye, 0);
		}
		if (squaredDistance > maxSquaredDistance)
			maxSquaredDistance = squaredDistance;

		++iter;
	}

	glDisable(GL_LIGHT0);


	// special case: draw earth shadow over moon if appropriate
	// stencil buffer is set up in moon drawing above
	// This effect curently only looks right from earth viewpoint
	if(nav->getHomePlanet()->getEnglishName() == "Earth") 
		draw_earth_shadow(nav, prj);

	drawPointer(prj, nav);
	
	return maxSquaredDistance;
}

void SolarSystem::setColorScheme(const QSettings* conf, const QString& section)
{
	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setNamesColor(StelUtils::str_to_vec3f(conf->value(section+"/planet_names_color", defaultColor).toString()));
	setOrbitsColor(StelUtils::str_to_vec3f(conf->value(section+"/planet_orbits_color", defaultColor).toString()));
	setTrailsColor(StelUtils::str_to_vec3f(conf->value(section+"/object_trails_color", defaultColor).toString()));
}

Planet* SolarSystem::searchByEnglishName(string planetEnglishName) const
{
//printf("SolarSystem::searchByEnglishName(\"%s\"): start\n",
//       planetEnglishName.c_str());
	// side effect - bad?
//	transform(planetEnglishName.begin(), planetEnglishName.end(), planetEnglishName.begin(), ::tolower);

	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
//printf("SolarSystem::searchByEnglishName(\"%s\"): %s\n",
//       planetEnglishName.c_str(),
//       (*iter)->getEnglishName().c_str());
		if( (*iter)->getEnglishName() == planetEnglishName ) return (*iter);  // also check standard ini file names
		++iter;
	}
//printf("SolarSystem::searchByEnglishName(\"%s\"): not found\n",
//       planetEnglishName.c_str());
	return NULL;
}

StelObjectP SolarSystem::searchByNameI18n(const wstring& planetNameI18) const
{
	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		if( (*iter)->getNameI18n() == planetNameI18 ) return (*iter);  // also check standard ini file names
		++iter;
	}
	return NULL;
}


StelObjectP SolarSystem::searchByName(const string& name) const
{
	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		if( (*iter)->getEnglishName() == name ) return (*iter); 
		++iter;
	}
	return NULL;
}

// Search if any Planet is close to position given in earth equatorial position and return the distance
StelObject* SolarSystem::search(Vec3d pos, const StelCore* core) const
{
	pos.normalize();
	Planet * closest = NULL;
	double cos_angle_closest = 0.;
	static Vec3d equPos;

	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		equPos = (*iter)->getEarthEquatorialPos(core->getNavigation());
		equPos.normalize();
		double cos_ang_dist = equPos[0]*pos[0] + equPos[1]*pos[1] + equPos[2]*pos[2];
		if (cos_ang_dist>cos_angle_closest)
		{
			closest = *iter;
			cos_angle_closest = cos_ang_dist;
		}
		iter++;
	}

	if (cos_angle_closest>0.999)
	{
		return closest;
	}
	else return NULL;
}

// Return a stl vector containing the planets located inside the lim_fov circle around position v
vector<StelObjectP> SolarSystem::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
	vector<StelObjectP> result;
	if (!getFlagPlanets())
		return result;
		
	Vec3d v = core->getNavigation()->j2000_to_earth_equ(vv);
	v.normalize();
	double cos_lim_fov = cos(limitFov * M_PI/180.);
	static Vec3d equPos;

	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		equPos = (*iter)->getEarthEquatorialPos(core->getNavigation());
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cos_lim_fov)
		{
			result.push_back(*iter);
		}
		iter++;
	}
	return result;
}

// Update i18 names from english names according to passed translator
void SolarSystem::updateI18n()
{
	Translator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->translateName(trans);
	}
	planet_name_font = StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}

wstring SolarSystem::getPlanetHashString(void)
{
	QString str;
	QTextStream oss(&str);
	
	vector <Planet *>::iterator iter;
	for (iter = system_planets.begin(); iter != system_planets.end(); ++iter)
	{
		if((*iter)->get_parent() != NULL && (*iter)->get_parent()->getEnglishName() != "Sun")
		{
			oss << q_((*iter)->get_parent()->getEnglishName().c_str()) << " : ";
		}
		
		oss << q_((*iter)->getEnglishName().c_str()) << endl;
		oss << (*iter)->getEnglishName().c_str() << endl;
	}
	return str.toStdWString();
}

void SolarSystem::startTrails(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->startTrail(b);
	}
}

void SolarSystem::setFlagTrails(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->setFlagTrail(b);
	}
}

bool SolarSystem::getFlagTrails(void) const
{
	for (vector<Planet*>::const_iterator iter = system_planets.begin();
	     iter != system_planets.end(); iter++ ) {
		if ((*iter)->getFlagTrail()) return true;
	}
	return false;
}

void SolarSystem::setFlagHints(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->setFlagHints(b);
	}
}

bool SolarSystem::getFlagHints(void) const
{
	for (vector<Planet*>::const_iterator iter = system_planets.begin();
        iter != system_planets.end(); iter++ ) {
		if ((*iter)->getFlagHints()) return true;
	}
	return false;
}

void SolarSystem::setFlagOrbits(bool b)
{
	flagOrbits = b;
	if (!b || !selected || selected == sun)
	{
		vector<Planet*>::iterator iter;
		for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
		{
			(*iter)->setFlagOrbits(b);
		}
	}
	else
	{
		// if a Planet is selected and orbits are on,
        // fade out non-selected ones
		vector<Planet*>::iterator iter;
		for (iter = system_planets.begin();
             iter != system_planets.end(); iter++ )
		{
            if (selected == (*iter)) (*iter)->setFlagOrbits(b);
            else (*iter)->setFlagOrbits(false);
		}		
	}
}


void SolarSystem::setSelected(StelObject* obj)
{
    if (obj && obj->getType() == "Planet")
    	selected = obj;
    else
    	selected = NULL;
	// Undraw other objects hints, orbit, trails etc..
	setFlagHints(getFlagHints());
	setFlagOrbits(getFlagOrbits());
	setFlagTrails(getFlagTrails());
}

// draws earth shadow overlapping the moon using stencil buffer
// umbra and penumbra are sized separately for accuracy
void SolarSystem::draw_earth_shadow(const Navigator * nav, Projector * prj)
{

	Vec3d e = getEarth()->get_ecliptic_pos();
	Vec3d m = getMoon()->get_ecliptic_pos();  // relative to earth
	Vec3d mh = getMoon()->get_heliocentric_ecliptic_pos();  // relative to sun
	float mscale = getMoon()->get_sphere_scale();

	// shadow location at earth + moon distance along earth vector from sun
	Vec3d en = e;
	en.normalize();
	Vec3d shadow = en * (e.length() + m.length());

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3f(1,1,1);

	// find shadow radii in AU
	double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000/AU;
	double r_umbra = 6378.1/AU - m.length()*(689621.9/AU/e.length());

	// find vector orthogonal to sun-earth vector using cross product with
	// a non-parallel vector
	Vec3d rpt = shadow^Vec3d(0,0,1);
	rpt.normalize();
	Vec3d upt = rpt*r_umbra*mscale*1.02;  // point on umbra edge
	rpt *= r_penumbra*mscale;  // point on penumbra edge

	// modify shadow location for scaled moon
	Vec3d mdist = shadow - mh;
	if(mdist.length() > r_penumbra + 2000/AU) return;   // not visible so don't bother drawing

	shadow = mh + mdist*mscale;
	r_penumbra *= mscale;

	//nav->switchToHeliocentric();
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	prj->setCurrentFrame(Projector::FRAME_HELIO);
	// shadow radial texture
	tex_earth_shadow->bind();

	Vec3d r, s;

	// umbra first
	glBegin(GL_TRIANGLE_FAN);
      // johannes: work-around for nasty ATI rendering bug:
      // use y-texture coordinate of 0.5 instead of 0.0
	glTexCoord2f(0.f,0.5f);
	prj->drawVertex3v(shadow);

	for (int i=0; i<=100; i++)
	{
		r = Mat4d::rotation(shadow, 2*M_PI*i/100.) * upt;
		s = shadow + r;

		glTexCoord2f(0.6f,0.5f);  // position in texture of umbra edge
		prj->drawVertex3v(s);
	}
	glEnd();


	// now penumbra
	Vec3d u, sp;
	glBegin(GL_TRIANGLE_STRIP);
	for (int i=0; i<=100; i++)
	{
		r = Mat4d::rotation(shadow, 2*M_PI*i/100.) * rpt;
		u = Mat4d::rotation(shadow, 2*M_PI*i/100.) * upt;
		s = shadow + r;
		sp = shadow + u;

		glTexCoord2f(0.6f,0.5f);
		prj->drawVertex3v(sp);

		glTexCoord2f(1.f,0.5f);  // position in texture of umbra edge
		prj->drawVertex3v(s);
	}
	glEnd();

	glDisable(GL_STENCIL_TEST);

}


void SolarSystem::update(double delta_time)
{
	bool restartTrails = false;
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();

	// Determine if home planet has changed, and restart planet trails
	// since the data is no longer useful
	if(nav->getHomePlanet() != lastHomePlanet) {
		lastHomePlanet = nav->getHomePlanet();
		restartTrails = true;
	}

	vector<Planet*>::iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		if(restartTrails) (*iter)->startTrail(true);
		(*iter)->update_trail(nav);
		(*iter)->update((int)(delta_time*1000));
		iter++;
	}
}


// is a lunar eclipse close at hand?
bool SolarSystem::near_lunar_eclipse(const Navigator * nav, Projector *prj)
{
	// TODO: could replace with simpler test

	Vec3d e = getEarth()->get_ecliptic_pos();
	Vec3d m = getMoon()->get_ecliptic_pos();  // relative to earth
	Vec3d mh = getMoon()->get_heliocentric_ecliptic_pos();  // relative to sun

	// shadow location at earth + moon distance along earth vector from sun
	Vec3d en = e;
	en.normalize();
	Vec3d shadow = en * (e.length() + m.length());

	// find shadow radii in AU
	double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000/AU;

	// modify shadow location for scaled moon
	Vec3d mdist = shadow - mh;
	if(mdist.length() > r_penumbra + 2000/AU) return 0;   // not visible so don't bother drawing

	return 1;
}

//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
vector<wstring> SolarSystem::listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem) const
{
	vector<wstring> result;
	if (maxNbItem==0) return result;
	
	wstring objw = objPrefix;
	transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
	
	vector < Planet * >::const_iterator iter;
	for (iter = system_planets.begin(); iter != system_planets.end(); ++iter)
	{
		wstring constw = (*iter)->getNameI18n().substr(0, objw.size());
		transform(constw.begin(), constw.end(), constw.begin(), ::toupper);
		if (constw==objw)
		{
			result.push_back((*iter)->getNameI18n());
			if (result.size()==maxNbItem)
				return result;
		}
	}
	return result;
}

void SolarSystem::selectedObjectChangeCallBack(StelModuleSelectAction action)
{
	const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Planet");
	if (!newSelected.empty())
		setSelected(newSelected[0].get());
//		// potentially record this action
//		if (!recordActionCallback.empty())
//			recordActionCallback("select planet " + selected_object.getEnglishName());
}

// Activate/Deactivate planets display
void SolarSystem::setFlagPlanets(bool b) {Planet::setflagShow(b);}
bool SolarSystem::getFlagPlanets(void) const {return Planet::getflagShow();}

// Set/Get planets names color
void SolarSystem::setNamesColor(const Vec3f& c) {Planet::set_label_color(c);}
const Vec3f& SolarSystem::getNamesColor(void) const {return Planet::getLabelColor();}

// Set/Get orbits lines color
void SolarSystem::setOrbitsColor(const Vec3f& c) {Planet::set_orbit_color(c);}
Vec3f SolarSystem::getOrbitsColor(void) const {return Planet::getOrbitColor();}

// Set/Get planets trails color
void SolarSystem::setTrailsColor(const Vec3f& c)  {Planet::set_trail_color(c);}
Vec3f SolarSystem::getTrailsColor(void) const {return Planet::getTrailColor();}

// Set/Get base planets display scaling factor 
void SolarSystem::setScale(float scale) {Planet::setScale(scale);}
float SolarSystem::getScale(void) const {return Planet::getScale();}

// Set/Get if Moon display is scaled
void SolarSystem::setFlagMoonScale(bool b)
{
	if (!b) getMoon()->set_sphere_scale(1);
	else getMoon()->set_sphere_scale(moonScale);
	flagMoonScale = b;
}

// Set/Get Moon display scaling factor 
void SolarSystem::setMoonScale(float f)
{
	moonScale = f;
	if (flagMoonScale)
		getMoon()->set_sphere_scale(moonScale);
}

// Set selected planets by englishName
void SolarSystem::setSelected(const string& englishName)
{
	setSelected(searchByEnglishName(englishName));
}

bool SolarSystem::bigger_distance::operator()(Planet* p1, Planet* p2)
{
	return p1->get_distance() > p2->get_distance();
}
