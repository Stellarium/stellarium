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
#include "SolarSystem.hpp"
#include "StelTexture.hpp"
#include "stellplanet.h"
#include "Orbit.hpp"
#include "StelNavigator.hpp"
#include "StelProjector.hpp"
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
#include "StelNavigator.hpp"
#include "StelFont.hpp"
#include "StelSkyDrawer.hpp"
#include "StelStyle.hpp"
#include "StelUtils.hpp"

#include <QTextStream>
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QMultiMap>
#include <QMapIterator>
#include <QDebug>

using namespace std;

SolarSystem::SolarSystem() :sun(NULL),moon(NULL),earth(NULL),selected(NULL), moonScale(1.), fontSize(14.),
	planetNameFont(StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage(), fontSize)),
	flagOrbits(false),flagLightTravelTime(false), lastHomePlanet(NULL)
{
	setObjectName("SolarSystem");
}

void SolarSystem::setFontSize(float newFontSize)
{
	planetNameFont = StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);
}

SolarSystem::~SolarSystem()
{
	// release selected:
	selected = NULL;
	for(std::vector<Planet*>::iterator iter = systemPlanets.begin(); iter != systemPlanets.end(); ++iter)
	{
		if (*iter) delete *iter;
		*iter = NULL;
	}
	for(std::vector<Orbit*>::iterator iter = orbits.begin(); iter != orbits.end(); ++iter)
	{
		if (*iter) delete *iter;
		*iter = NULL;
	}
	sun = NULL;
	moon = NULL;
	earth = NULL;
	Planet::hintCircleTex.reset();
	Planet::texEarthShadow.reset();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SolarSystem::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)+10;
	return 0;
}

// Init and load the solar system data
void SolarSystem::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	loadPlanets();	// Load planets data

	// Compute position and matrix of sun and all the satellites (ie planets)
	// for the first initialization Q_ASSERT that center is sun center (only impacts on light speed correction)
	computePositions(StelUtils::getJDFromSystem());
	setSelected("");	// Fix a bug on macosX! Thanks Fumio!
	setFlagMoonScale(conf->value("viewing/flag_moon_scaled", conf->value("viewing/flag_init_moon_scaled", "false").toBool()).toBool());  // name change
	setMoonScale(conf->value("viewing/moon_scale", 5.0).toDouble());
	setFlagPlanets(conf->value("astro/flag_planets").toBool());
	setFlagHints(conf->value("astro/flag_planets_hints").toBool());
	setFlagLabels(conf->value("astro/flag_planets_labels", true).toBool());
	setLabelsAmount(conf->value("astro/labels_amount", 3.).toDouble());
	setFlagOrbits(conf->value("astro/flag_planets_orbits").toBool());
	setFlagLightTravelTime(conf->value("astro/flag_light_travel_time", false).toBool());
	setFlagTrails(conf->value("astro/flag_object_trails", false).toBool());
	startTrails(conf->value("astro/flag_object_trails", false).toBool());	
	
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);
	StelApp::getInstance().getTextureManager().setDefaultParams();
	StelApp::getInstance().getTextureManager().setMinFilter(GL_LINEAR);
	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur4.png");
	Planet::hintCircleTex = StelApp::getInstance().getTextureManager().createTexture("planet-indicator.png");
}

void SolarSystem::drawPointer(const StelCore* core)
{
	const StelNavigator* nav = core->getNavigator();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	
	const QList<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Planet");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(nav);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos)) return;
	
		glColor3f(1.0f,0.3f,0.3f);
	
		float size = obj->getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter()*2.;
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
	qDebug() << "Loading Solar System data ...";
	QString iniFile;
	try
	{
		iniFile = StelApp::getInstance().getFileMgr().findFile("data/ssystem.ini");
	}
	catch(std::runtime_error& e)
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
	int readOk=0;
	int totalPlanets=0;
	for (int i = 0;i<orderedSections.size();++i)
	{
		totalPlanets++;
		const QString secname = orderedSections.at(i);
		const QString englishName = pd.value(secname+"/name").toString();
		const QString strParent = pd.value(secname+"/parent").toString();
		Planet *parent = NULL;
 		if (strParent!="none")
		{
			// Look in the other planets the one named with strParent
			vector<Planet*>::iterator iter = systemPlanets.begin();
			while (iter != systemPlanets.end())
			{
				if ((*iter)->getEnglishName()==strParent)
				{
					parent = (*iter);
				}
				iter++;
			}
			if (parent == NULL)
			{
				qWarning() << "ERROR : can't find parent solar system body for " << englishName;
				abort();
				continue;
			}
		}

		const QString funcName = pd.value(secname+"/coord_func").toString();
		posFuncType posfunc;
		OsulatingFunctType *osculatingFunc = 0;
		bool closeOrbit = pd.value(secname+"/closeOrbit", true).toBool();

		if (funcName=="ell_orbit")
		{
			// Read the orbital elements
			const double epoch = pd.value(secname+"/orbit_Epoch",J2000).toDouble();
			const double eccentricity = pd.value(secname+"/orbit_Eccentricity").toDouble();
			if (eccentricity >= 1.0) closeOrbit = false;
			double pericenterDistance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			double semi_major_axis;
			if (pericenterDistance <= 0.0) {
				semi_major_axis = pd.value(secname+"/orbit_SemiMajorAxis",-1e100).toDouble();
				if (semi_major_axis <= -1e100) {
					qDebug() << "ERROR: " << englishName
						<< ": you must provide orbit_PericenterDistance or orbit_SemiMajorAxis";
					abort();
				} else {
					semi_major_axis /= AU;
					Q_ASSERT(eccentricity != 1.0); // parabolic orbits have no semi_major_axis
					pericenterDistance = semi_major_axis * (1.0-eccentricity);
				}
			} else {
				pericenterDistance /= AU;
				semi_major_axis = (eccentricity == 1.0)
				                ? 0.0 // parabolic orbits have no semi_major_axis
				                : pericenterDistance / (1.0-eccentricity);
			}
			double meanMotion = pd.value(secname+"/orbit_MeanMotion",-1e100).toDouble();
			double period;
			if (meanMotion <= -1e100) {
				period = pd.value(secname+"/orbit_Period",-1e100).toDouble();
				if (period <= -1e100) {
					meanMotion = (eccentricity == 1.0)
					            ? 0.01720209895 * (1.5/pericenterDistance)
					                            * sqrt(0.5/pericenterDistance)
					            : (semi_major_axis > 0.0)
					            ? 0.01720209895 / (semi_major_axis*sqrt(semi_major_axis))
					            : 0.01720209895 / (-semi_major_axis*sqrt(-semi_major_axis));
					period = 2.0*M_PI/meanMotion;
				} else {
					meanMotion = 2.0*M_PI/period;
				}
			} else {
				period = 2.0*M_PI/meanMotion;
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
			const double parentRotObliquity = parent->getParent()
			                                  ? parent->getRotObliquity()
			                                  : 0.0;
			const double parent_rot_asc_node = parent->getParent()
			                                  ? parent->getRotAscendingnode()
			                                  : 0.0;
			double parent_rot_j2000_longitude = 0.0;
			if (parent->getParent()) {
				const double c_obl = cos(parentRotObliquity);
				const double s_obl = sin(parentRotObliquity);
				const double c_nod = cos(parent_rot_asc_node);
				const double s_nod = sin(parent_rot_asc_node);
				const Vec3d OrbitAxis0( c_nod,       s_nod,        0.0);
				const Vec3d OrbitAxis1(-s_nod*c_obl, c_nod*c_obl,s_obl);
				const Vec3d OrbitPole(  s_nod*s_obl,-c_nod*s_obl,c_obl);
				const Vec3d J2000Pole(StelNavigator::matJ2000ToVsop87.multiplyWithoutTranslation(Vec3d(0,0,1)));
				Vec3d J2000NodeOrigin(J2000Pole^OrbitPole);
				J2000NodeOrigin.normalize();
				parent_rot_j2000_longitude = atan2(J2000NodeOrigin*OrbitAxis1,J2000NodeOrigin*OrbitAxis0);
			}

			// Create an elliptical orbit
			EllipticalOrbit *orb = new EllipticalOrbit(pericenterDistance,
			                                           eccentricity,
			                                           inclination,
			                                           ascending_node,
			                                           arg_of_pericenter,
			                                           mean_anomaly,
			                                           period,
			                                           epoch,
			                                           parentRotObliquity,
			                                           parent_rot_asc_node,
													   parent_rot_j2000_longitude);
			orbits.push_back(orb);

			posfunc = posFuncType(orb, &EllipticalOrbit::positionAtTimevInVSOP87Coordinates);
		} 
		else if (funcName=="comet_orbit")
		{
			// Read the orbital elements
			// orbit_PericenterDistance,orbit_SemiMajorAxis: given in AU
			// orbit_MeanMotion: given in degrees/day
			// orbit_Period: given in days
			// orbit_TimeAtPericenter,orbit_Epoch: JD
			// orbit_MeanAnomaly,orbit_Inclination,orbit_ArgOfPericenter,orbit_AscendingNode: given in degrees
			const double eccentricity = pd.value(secname+"/orbit_Eccentricity",0.0).toDouble();
			if (eccentricity >= 1.0) closeOrbit = false;
			double pericenterDistance = pd.value(secname+"/orbit_PericenterDistance",-1e100).toDouble();
			double semi_major_axis;
			if (pericenterDistance <= 0.0) {
				semi_major_axis = pd.value(secname+"/orbit_SemiMajorAxis",-1e100).toDouble();
				if (semi_major_axis <= -1e100) {
					qWarning() << "ERROR: " << englishName
						<< ": you must provide orbit_PericenterDistance or orbit_SemiMajorAxis";
					abort();
				} else {
					Q_ASSERT(eccentricity != 1.0); // parabolic orbits have no semi_major_axis
					pericenterDistance = semi_major_axis * (1.0-eccentricity);
				}
			} else {
				semi_major_axis = (eccentricity == 1.0)
				                ? 0.0 // parabolic orbits have no semi_major_axis
				                : pericenterDistance / (1.0-eccentricity);
			}
			double meanMotion = pd.value(secname+"/orbit_MeanMotion",-1e100).toDouble();
			if (meanMotion <= -1e100) {
				const double period = pd.value(secname+"/orbit_Period",-1e100).toDouble();
				if (period <= -1e100) {
					if (parent->getParent()) {
						qWarning() << "ERROR: " << englishName
							<< ": when the parent body is not the sun, you must provide "
							<< "either orbit_MeanMotion or orbit_Period";
					} else {
						// in case of parent=sun: use Gaussian gravitational constant
						// for calculating meanMotion:
						meanMotion = (eccentricity == 1.0)
						            ? 0.01720209895 * (1.5/pericenterDistance)
						                            * sqrt(0.5/pericenterDistance)
						            : (semi_major_axis > 0.0)
						            ? 0.01720209895 / (semi_major_axis*sqrt(semi_major_axis))
						            : 0.01720209895 / (-semi_major_axis*sqrt(-semi_major_axis));
					}
				} else {
					meanMotion = 2.0*M_PI/period;
				}
			} else {
				meanMotion *= (M_PI/180.0);
			}
			double time_at_pericenter = pd.value(secname+"/orbit_TimeAtPericenter",-1e100).toDouble();
			if (time_at_pericenter <= -1e100) {
				const double epoch = pd.value(secname+"/orbit_Epoch",-1e100).toDouble();
				double mean_anomaly = pd.value(secname+"/orbit_MeanAnomaly",-1e100).toDouble();
				if (epoch <= -1e100 || mean_anomaly <= -1e100) {
					qWarning() << "ERROR: " << englishName
						<< ": when you do not provide orbit_TimeAtPericenter, you must provide both "
						<< "orbit_Epoch and orbit_MeanAnomaly";
					abort();
				} else {
					mean_anomaly *= (M_PI/180.0);
					time_at_pericenter = epoch - mean_anomaly / meanMotion;
				}
			}
			const double inclination = pd.value(secname+"/orbit_Inclination").toDouble()*(M_PI/180.0);
			const double arg_of_pericenter = pd.value(secname+"/orbit_ArgOfPericenter").toDouble()*(M_PI/180.0);
			const double ascending_node = pd.value(secname+"/orbit_AscendingNode").toDouble()*(M_PI/180.0);
			const double parentRotObliquity = parent->getParent()
			                                  ? parent->getRotObliquity()
			                                  : 0.0;
			const double parent_rot_asc_node = parent->getParent()
			                                  ? parent->getRotAscendingnode()
			                                  : 0.0;
			double parent_rot_j2000_longitude = 0.0;
                        if (parent->getParent()) {
                           const double c_obl = cos(parentRotObliquity);
                           const double s_obl = sin(parentRotObliquity);
                           const double c_nod = cos(parent_rot_asc_node);
                           const double s_nod = sin(parent_rot_asc_node);
                           const Vec3d OrbitAxis0( c_nod,       s_nod,        0.0);
                           const Vec3d OrbitAxis1(-s_nod*c_obl, c_nod*c_obl,s_obl);
                           const Vec3d OrbitPole(  s_nod*s_obl,-c_nod*s_obl,c_obl);
						   const Vec3d J2000Pole(StelNavigator::matJ2000ToVsop87.multiplyWithoutTranslation(Vec3d(0,0,1)));
                           Vec3d J2000NodeOrigin(J2000Pole^OrbitPole);
                           J2000NodeOrigin.normalize();
                           parent_rot_j2000_longitude = atan2(J2000NodeOrigin*OrbitAxis1,J2000NodeOrigin*OrbitAxis0);
                        }
			CometOrbit *orb = new CometOrbit(pericenterDistance,
			                                 eccentricity,
			                                 inclination,
			                                 ascending_node,
			                                 arg_of_pericenter,
			                                 time_at_pericenter,
			                                 meanMotion,
			                                 parentRotObliquity,
			                                 parent_rot_asc_node,
											 parent_rot_j2000_longitude);
			orbits.push_back(orb);

			posfunc = posFuncType(orb,&CometOrbit::positionAtTimevInVSOP87Coordinates);
		}

		if (funcName=="sun_special")
			posfunc = posFuncType(get_sun_helio_coordsv);

		if (funcName=="mercury_special") {
			posfunc = posFuncType(get_mercury_helio_coordsv);
			osculatingFunc = &get_mercury_helio_osculating_coords;
		}
        
		if (funcName=="venus_special") {
			posfunc = posFuncType(get_venus_helio_coordsv);
			osculatingFunc = &get_venus_helio_osculating_coords;
		}

		if (funcName=="earth_special") {
			posfunc = posFuncType(get_earth_helio_coordsv);
			osculatingFunc = &get_earth_helio_osculating_coords;
		}

		if (funcName=="lunar_special")
			posfunc = posFuncType(get_lunar_parent_coordsv);

		if (funcName=="mars_special") {
			posfunc = posFuncType(get_mars_helio_coordsv);
			osculatingFunc = &get_mars_helio_osculating_coords;
		}

		if (funcName=="phobos_special")
			posfunc = posFuncType(get_phobos_parent_coordsv);

		if (funcName=="deimos_special")
			posfunc = posFuncType(get_deimos_parent_coordsv);

		if (funcName=="jupiter_special") {
			posfunc = posFuncType(get_jupiter_helio_coordsv);
			osculatingFunc = &get_jupiter_helio_osculating_coords;
		}

		if (funcName=="europa_special")
			posfunc = posFuncType(get_europa_parent_coordsv);

		if (funcName=="calisto_special")
			posfunc = posFuncType(get_callisto_parent_coordsv);

		if (funcName=="io_special")
			posfunc = posFuncType(get_io_parent_coordsv);

		if (funcName=="ganymede_special")
			posfunc = posFuncType(get_ganymede_parent_coordsv);

		if (funcName=="saturn_special") {
			posfunc = posFuncType(get_saturn_helio_coordsv);
			osculatingFunc = &get_saturn_helio_osculating_coords;
		}

		if (funcName=="mimas_special")
			posfunc = posFuncType(get_mimas_parent_coordsv);

		if (funcName=="enceladus_special")
			posfunc = posFuncType(get_enceladus_parent_coordsv);

		if (funcName=="tethys_special")
			posfunc = posFuncType(get_tethys_parent_coordsv);

		if (funcName=="dione_special")
			posfunc = posFuncType(get_dione_parent_coordsv);

		if (funcName=="rhea_special")
			posfunc = posFuncType(get_rhea_parent_coordsv);

		if (funcName=="titan_special")
			posfunc = posFuncType(get_titan_parent_coordsv);

		if (funcName=="iapetus_special")
			posfunc = posFuncType(get_iapetus_parent_coordsv);

		if (funcName=="hyperion_special")
			posfunc = posFuncType(get_hyperion_parent_coordsv);

		if (funcName=="uranus_special") {
			posfunc = posFuncType(get_uranus_helio_coordsv);
			osculatingFunc = &get_uranus_helio_osculating_coords;
		}

		if (funcName=="miranda_special")
			posfunc = posFuncType(get_miranda_parent_coordsv);

		if (funcName=="ariel_special")
			posfunc = posFuncType(get_ariel_parent_coordsv);

		if (funcName=="umbriel_special")
			posfunc = posFuncType(get_umbriel_parent_coordsv);

		if (funcName=="titania_special")
			posfunc = posFuncType(get_titania_parent_coordsv);

		if (funcName=="oberon_special")
			posfunc = posFuncType(get_oberon_parent_coordsv);

		if (funcName=="neptune_special") {
			posfunc = posFuncType(get_neptune_helio_coordsv);
			osculatingFunc = &get_neptune_helio_osculating_coords;
		}

		if (funcName=="pluto_special")
			posfunc = posFuncType(get_pluto_helio_coordsv);


		if (posfunc.empty())
		{
			qWarning() << "ERROR : can't find posfunc " << funcName << " for " << englishName;
			exit(-1);
		}

		// Create the Planet and add it to the list
		Planet* p = new Planet(parent,
					englishName,
					pd.value(secname+"/lighting").toBool(),
					pd.value(secname+"/radius").toDouble()/AU,
					pd.value(secname+"/oblateness", 0.0).toDouble(),
					StelUtils::strToVec3f(pd.value(secname+"/color").toString()),
					pd.value(secname+"/albedo").toDouble(),
					pd.value(secname+"/tex_map").toString(),
					pd.value(secname+"/tex_halo").toString(),
					posfunc,
					osculatingFunc,
					closeOrbit,
					pd.value(secname+"/hidden", 0).toBool(),
					pd.value(secname+"/atmosphere", false).toBool());

		if (secname=="earth") earth = p;
		if (secname=="sun") sun = p;
		if (secname=="moon") moon = p;

		double rotObliquity = pd.value(secname+"/rot_obliquity",0.).toDouble()*(M_PI/180.0);
		double rotAscNode = pd.value(secname+"/rot_equator_ascending_node",0.).toDouble()*(M_PI/180.0);

		// Use more common planet North pole data if available
		// NB: N pole as defined by IAU (NOT right hand rotation rule)
		// NB: J2000 epoch
		double J2000NPoleRA = pd.value(secname+"/rot_pole_ra", 0.).toDouble()*M_PI/180.;
		double J2000NPoleDE = pd.value(secname+"/rot_pole_de", 0.).toDouble()*M_PI/180.;

		if(J2000NPoleRA || J2000NPoleDE)
		{
			Vec3d J2000NPole;
			StelUtils::spheToRect(J2000NPoleRA,J2000NPoleDE,J2000NPole);
		  
			Vec3d vsop87Pole(StelNavigator::matJ2000ToVsop87.multiplyWithoutTranslation(J2000NPole));
		  
			double ra, de;
			StelUtils::rectToSphe(&ra, &de, vsop87Pole);
		  
			rotObliquity = (M_PI_2 - de);
			rotAscNode = (ra + M_PI_2);
		  
			// qDebug() << "\tCalculated rotational obliquity: " << rotObliquity*180./M_PI << endl;
			// qDebug() << "\tCalculated rotational ascending node: " << rotAscNode*180./M_PI << endl;
		}

		p->setRotationElements(
		    pd.value(secname+"/rot_periode", pd.value(secname+"/orbit_Period", 24.).toDouble()).toDouble()/24.,
		    pd.value(secname+"/rot_rotation_offset",0.).toDouble(),
		    pd.value(secname+"/rot_epoch", J2000).toDouble(),
		    rotObliquity,
		    rotAscNode,
		    pd.value(secname+"/rot_precession_rate",0.).toDouble()*M_PI/(180*36525),
		    pd.value(secname+"/orbit_visualization_period",0.).toDouble());


		if (pd.value(secname+"/rings", 0).toBool()) {
			const double rMin = pd.value(secname+"/ring_inner_size").toDouble()/AU;
			const double rMax = pd.value(secname+"/ring_outer_size").toDouble()/AU;
			Ring *r = new Ring(rMin,rMax,pd.value(secname+"/tex_ring").toString());
			p->setRings(r);
		}

		systemPlanets.push_back(p);
		readOk++;
	}

	// special case: load earth shadow texture
	StelApp::getInstance().getTextureManager().setDefaultParams();
	Planet::texEarthShadow = StelApp::getInstance().getTextureManager().createTexture("earth-shadow.png");
	
	qDebug() << "Loaded" << readOk << "/" << totalPlanets << "planet orbits";
}

// Compute the position for every elements of the solar system.
// The order is not important since the position is computed relatively to the mother body
void SolarSystem::computePositions(double date, const Vec3d& observerPos)
{
	if (flagLightTravelTime)
	{
		for (std::vector<Planet*>::const_iterator iter(systemPlanets.begin());iter!=systemPlanets.end();iter++)
		{
			(*iter)->computePositionWithoutOrbits(date);
		}
		for (std::vector<Planet*>::const_iterator iter(systemPlanets.begin());iter!=systemPlanets.end();iter++)
		{
			const double light_speed_correction = ((*iter)->getHeliocentricEclipticPos()-observerPos).length() * (AU / (SPEED_OF_LIGHT * 86400));
			(*iter)->computePosition(date-light_speed_correction);
		}
	}
	else
	{
		for (std::vector<Planet*>::const_iterator iter(systemPlanets.begin());iter!=systemPlanets.end();iter++)
		{
			(*iter)->computePosition(date);
		}
	}  
	computeTransMatrices(date, observerPos);
}

// Compute the transformation matrix for every elements of the solar system.
// The elements have to be ordered hierarchically, eg. it's important to compute earth before moon.
void SolarSystem::computeTransMatrices(double date, const Vec3d& observerPos)
{
	if (flagLightTravelTime)
	{
		for (std::vector<Planet*>::const_iterator iter(systemPlanets.begin());iter!=systemPlanets.end();iter++)
		{
			const double light_speed_correction = ((*iter)->getHeliocentricEclipticPos()-observerPos).length() * (AU / (SPEED_OF_LIGHT * 86400));
			(*iter)->computeTransMatrix(date-light_speed_correction);
		}
  	}
	else
	{
		for (std::vector<Planet*>::const_iterator iter(systemPlanets.begin());iter!=systemPlanets.end();iter++)
		{
			(*iter)->computeTransMatrix(date);
		}
	}
}

// Draw all the elements of the solar system
// We are supposed to be in heliocentric coordinate
void SolarSystem::draw(StelCore* core)
{
	if (!flagShow)
		return;
	
	StelNavigator* nav = core->getNavigator();
	Planet::setFont(&planetNameFont);
	
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
	glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(0.f,0.f,0.f,1.f));
	glEnable(GL_LIGHT0);

	// Compute each Planet distance to the observer
	Vec3d obsHelioPos = nav->getObserverHeliocentricEclipticPos();
	
	vector<Planet*>::iterator iter;
	iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		(*iter)->computeDistance(obsHelioPos);
		++iter;
	}

	// And sort them from the furthest to the closest
	sort(systemPlanets.begin(),systemPlanets.end(),biggerDistance());

	// Draw the elements
	float maxMagLabel=core->getSkyDrawer()->getLimitMagnitude()*0.80+(labelsAmount*1.2f)-2.f;
	iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		(*iter)->draw(core, maxMagLabel);
		++iter;
	}

	glDisable(GL_LIGHT0);

	drawPointer(core);
}

void SolarSystem::setStelStyle(const StelStyle& style)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();
	QString section = style.confSectionName;
	
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelsColor(StelUtils::strToVec3f(conf->value(section+"/planet_names_color", defaultColor).toString()));
	setOrbitsColor(StelUtils::strToVec3f(conf->value(section+"/planet_orbits_color", defaultColor).toString()));
	setTrailsColor(StelUtils::strToVec3f(conf->value(section+"/object_trails_color", defaultColor).toString()));
}

Planet* SolarSystem::searchByEnglishName(QString planetEnglishName) const
{
	vector<Planet*>::const_iterator iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		if((*iter)->getEnglishName() == planetEnglishName) return (*iter);  // also check standard ini file names
		++iter;
	}

	return NULL;
}

StelObjectP SolarSystem::searchByNameI18n(const QString& planetNameI18) const
{
	vector<Planet*>::const_iterator iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		if( (*iter)->getNameI18n() == planetNameI18 ) return (*iter);  // also check standard ini file names
		++iter;
	}
	return NULL;
}


StelObjectP SolarSystem::searchByName(const QString& name) const
{
	vector<Planet*>::const_iterator iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
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

	vector<Planet*>::const_iterator iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		equPos = (*iter)->getEquinoxEquatorialPos(core->getNavigator());
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

// Return a stl vector containing the planets located inside the limFov circle around position v
QList<StelObjectP> SolarSystem::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!getFlagPlanets())
		return result;
		
	Vec3d v = core->getNavigator()->j2000ToEquinoxEqu(vv);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	static Vec3d equPos;

	vector<Planet*>::const_iterator iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		equPos = (*iter)->getEquinoxEquatorialPos(core->getNavigator());
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
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
	StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	vector<Planet*>::iterator iter;
	for( iter = systemPlanets.begin(); iter < systemPlanets.end(); iter++ )
	{
		(*iter)->translateName(trans);
	}
	planetNameFont = StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}

QString SolarSystem::getPlanetHashString(void)
{
	QString str;
	QTextStream oss(&str);
	
	vector <Planet *>::iterator iter;
	for (iter = systemPlanets.begin(); iter != systemPlanets.end(); ++iter)
	{
		if((*iter)->getParent() != NULL && (*iter)->getParent()->getEnglishName() != "Sun")
		{
			oss << (*iter)->getParent()->getEnglishName() << " : ";
		}
		
		oss << (*iter)->getEnglishName() << endl;
		oss << (*iter)->getEnglishName() << endl;
	}
	return str;
}

void SolarSystem::startTrails(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = systemPlanets.begin(); iter < systemPlanets.end(); iter++ )
	{
		(*iter)->startTrail(b);
	}
}

void SolarSystem::setFlagTrails(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = systemPlanets.begin(); iter < systemPlanets.end(); iter++ )
	{
		(*iter)->setFlagTrail(b);
	}
}

bool SolarSystem::getFlagTrails(void) const
{
	for (std::vector<Planet*>::const_iterator iter = systemPlanets.begin();
	     iter != systemPlanets.end(); iter++ ) {
		if ((*iter)->getFlagTrail()) return true;
	}
	return false;
}

void SolarSystem::setFlagHints(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = systemPlanets.begin(); iter < systemPlanets.end(); iter++ )
	{
		(*iter)->setFlagHints(b);
	}
}

bool SolarSystem::getFlagHints(void) const
{
	for (std::vector<Planet*>::const_iterator iter = systemPlanets.begin(); iter != systemPlanets.end(); iter++)
	{
		if ((*iter)->getFlagHints()) return true;
	}
	return false;
}

void SolarSystem::setFlagLabels(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = systemPlanets.begin(); iter < systemPlanets.end(); iter++ )
	{
		(*iter)->setFlagLabels(b);
	}
}

bool SolarSystem::getFlagLabels() const
{
	for (std::vector<Planet*>::const_iterator iter = systemPlanets.begin(); iter != systemPlanets.end(); iter++)
	{
		if ((*iter)->getFlagLabels()) return true;
	}
	return false;
}

void SolarSystem::setFlagOrbits(bool b)
{
	flagOrbits = b;
	if (!b || !selected || selected == sun)
	{
		vector<Planet*>::iterator iter;
		for( iter = systemPlanets.begin(); iter < systemPlanets.end(); iter++ )
		{
			(*iter)->setFlagOrbits(b);
		}
	}
	else
	{
		// if a Planet is selected and orbits are on,
		// fade out non-selected ones
		vector<Planet*>::iterator iter;
		for (iter = systemPlanets.begin();
		     iter != systemPlanets.end(); iter++ )
		{
			if (selected == (*iter)) (*iter)->setFlagOrbits(b);
			else (*iter)->setFlagOrbits(false);
		}		
	}
}

void SolarSystem::setFlagLightTravelTime(bool b)
{
	flagLightTravelTime = b;
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


void SolarSystem::update(double deltaTime)
{
	bool restartTrails = false;
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();

	// Determine if home planet has changed, and restart planet trails
	// since the data is no longer useful
	if (nav->getHomePlanet() != lastHomePlanet)
	{
		lastHomePlanet = nav->getHomePlanet();
		restartTrails = true;
	}

	vector<Planet*>::iterator iter = systemPlanets.begin();
	while (iter != systemPlanets.end())
	{
		if(restartTrails) (*iter)->startTrail(true);
		(*iter)->updateTrail(nav);
		(*iter)->update((int)(deltaTime*1000));
		iter++;
	}
}


// is a lunar eclipse close at hand?
bool SolarSystem::nearLunarEclipse()
{
	// TODO: could replace with simpler test

	Vec3d e = getEarth()->getEclipticPos();
	Vec3d m = getMoon()->getEclipticPos();  // relative to earth
	Vec3d mh = getMoon()->getHeliocentricEclipticPos();  // relative to sun

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
QStringList SolarSystem::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;
	
	QString objw = objPrefix.toUpper();
	
	vector <Planet*>::const_iterator iter;
	for (iter=systemPlanets.begin(); iter!=systemPlanets.end(); ++iter)
	{
		QString constw = (*iter)->getNameI18n().mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << (*iter)->getNameI18n();
			if (result.size()==maxNbItem)
				return result;
		}
	}
	return result;
}

void SolarSystem::selectedObjectChangeCallBack(StelModuleSelectAction action)
{
	const QList<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Planet");
	if (!newSelected.empty())
		setSelected(newSelected[0].get());
}

// Activate/Deactivate planets display
void SolarSystem::setFlagPlanets(bool b) 
{
	flagShow=b;
}

bool SolarSystem::getFlagPlanets(void) const {return flagShow;}

// Set/Get planets names color
void SolarSystem::setLabelsColor(const Vec3f& c) {Planet::setLabelColor(c);}
const Vec3f& SolarSystem::getLabelsColor(void) const {return Planet::getLabelColor();}

// Set/Get orbits lines color
void SolarSystem::setOrbitsColor(const Vec3f& c) {Planet::setOrbitColor(c);}
Vec3f SolarSystem::getOrbitsColor(void) const {return Planet::getOrbitColor();}

// Set/Get planets trails color
void SolarSystem::setTrailsColor(const Vec3f& c)  {Planet::setTrailColor(c);}
Vec3f SolarSystem::getTrailsColor(void) const {return Planet::getTrailColor();}

// Set/Get if Moon display is scaled
void SolarSystem::setFlagMoonScale(bool b)
{
	if (!b) getMoon()->setSphereScale(1);
	else getMoon()->setSphereScale(moonScale);
	flagMoonScale = b;
}

// Set/Get Moon display scaling factor 
void SolarSystem::setMoonScale(float f)
{
	moonScale = f;
	if (flagMoonScale)
		getMoon()->setSphereScale(moonScale);
}

// Set selected planets by englishName
void SolarSystem::setSelected(const QString& englishName)
{
	setSelected(searchByEnglishName(englishName));
}

bool SolarSystem::biggerDistance::operator()(Planet* p1, Planet* p2)
{
	return p1->getDistance() > p2->getDistance();
}

// Get the list of all the planet english names
QStringList SolarSystem::getAllPlanetEnglishNames() const
{
	QStringList res;
	for (std::vector<Planet*>::const_iterator iter(systemPlanets.begin());iter!=systemPlanets.end();iter++)
	{
		res.append((*iter)->englishName);
	}
	return res;
}
