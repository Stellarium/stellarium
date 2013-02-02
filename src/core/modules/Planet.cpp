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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <iomanip>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QVarLengthArray>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "renderer/GenericVertexTypes.hpp"
#include "renderer/StelGeometryBuilder.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"

#include "StelProjector.hpp"
#include "sideral_time.h"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

Vec3f Planet::labelColor = Vec3f(0.4,0.4,0.8);
Vec3f Planet::orbitColor = Vec3f(1,0.6,1);

Planet::SharedPlanetGraphics::~SharedPlanetGraphics()
{
	if(!initialized){return;}
	delete texEarthShadow;
	delete texHintCircle;

	if(NULL != simplePlanetShader)
		delete simplePlanetShader;

	if(NULL != shadowPlanetShader)
		delete shadowPlanetShader;

	initialized = false;
}

void Planet::SharedPlanetGraphics::lazyInit(StelRenderer* renderer)
{
	if(initialized){return;}
	texHintCircle  = renderer->createTexture("textures/planet-indicator.png");
	texEarthShadow = renderer->createTexture("textures/earth-shadow.png");
	planetShader = simplePlanetShader = shadowPlanetShader = NULL;

	if(renderer->isGLSLSupported())
	{
		if(!loadPlanetShaders(renderer))
		{
			qWarning() << "Failed to load planet shaders, falling back to CPU implementation";
			simplePlanetShader = shadowPlanetShader = NULL;
		}
	}

	initialized = true;
}

bool Planet::SharedPlanetGraphics::loadPlanetShaders(StelRenderer* renderer)
{
	Q_ASSERT_X(renderer->isGLSLSupported(), Q_FUNC_INFO,
	           "Trying to load planet shaders but GLSL is not supported");

	simplePlanetShader = renderer->createGLSLShader();
	if(!simplePlanetShader->addVertexShader(
	  "vec4 project(in vec4 v);\n"
	  "attribute mediump vec4 vertex;\n"
	  "attribute mediump vec4 unprojectedVertex;\n"
	  "attribute mediump vec2 texCoord;\n"
	  "uniform mediump mat4 projectionMatrix;\n"
	  "uniform highp vec3 lightPos;\n"
	  "uniform highp float oneMinusOblateness;\n"
	  "uniform highp float radius;\n"
	  "uniform mediump vec4 ambientLight;\n"
	  "uniform mediump vec4 diffuseLight;\n"
	  "varying mediump vec2 texc;\n"
	  "varying mediump vec4 litColor;\n"
	  "void main(void)\n"
	  "{\n"
	  "    gl_Position = projectionMatrix * project(vertex);\n"
	  "    texc = texCoord;\n"
	  "    // Must be a separate variable due to Intel drivers\n"
	  "    vec4 normal = unprojectedVertex / radius;\n"
	  "    float c = lightPos.x * normal.x * oneMinusOblateness +\n"
	  "              lightPos.y * normal.y * oneMinusOblateness +\n"
	  "              lightPos.z * normal.z / oneMinusOblateness;\n"
	  "    litColor = clamp(c, 0.0, 0.5) * diffuseLight + ambientLight;\n"
	  "}\n"))
	{
		qWarning() << "Error adding simple planet vertex shader: " << simplePlanetShader->log();
		delete simplePlanetShader;
		return false;
	}

	if(!simplePlanetShader->addFragmentShader(
	  "varying mediump vec2 texc;\n"
	  "varying mediump vec4 litColor;\n"
	  "uniform sampler2D tex;\n"
	  "uniform mediump vec4 globalColor;\n"
	  "void main(void)\n"
	  "{\n"
	  "    gl_FragColor = texture2D(tex, texc) * litColor;\n"
	  "}\n"))
	{
		qWarning() << "Error adding simple planet fragment shader: " << simplePlanetShader->log();
		delete simplePlanetShader;
		return false;
	}
	if(!simplePlanetShader->build())
	{
		qWarning() << "Error building shader: " << simplePlanetShader->log();
		delete simplePlanetShader;
		return false;
	}
	qDebug() << "Simple planet shader build log: " << simplePlanetShader->log();

	shadowPlanetShader = renderer->createGLSLShader();
	if(!shadowPlanetShader->addVertexShader(
	  "vec4 project(in vec4 v);\n"
	  "attribute mediump vec4 vertex;\n"
	  "attribute mediump vec4 unprojectedVertex;\n"
	  "attribute mediump vec2 texCoord;\n"
	  "uniform mediump mat4 projectionMatrix;\n"
	  "uniform highp vec3 lightPos;\n"
	  "uniform highp float oneMinusOblateness;\n"
	  "uniform highp float radius;\n"
	  "varying mediump vec2 texc;\n"
	  "varying mediump float lambert;\n"
	  "varying highp vec3 P;\n"
	  "\n"
	  "void main()\n"
	  "{\n"
	  "    gl_Position = projectionMatrix * project(vertex);\n"
	  "    texc = texCoord;\n"
	  "    // Must be a separate variable due to Intel drivers\n"
	  "    vec4 normal = unprojectedVertex / radius;\n"
	  "    float c = lightPos.x * normal.x * oneMinusOblateness +\n"
	  "              lightPos.y * normal.y * oneMinusOblateness +\n"
	  "              lightPos.z * normal.z / oneMinusOblateness;\n"
	  "    lambert = clamp(c, 0.0, 0.5);\n"
	  "\n"
	  "    P = vec3(unprojectedVertex);\n"
	  "}\n"
	  "\n"))
	{
		qWarning() << "Error adding shadow planet vertex shader: " << shadowPlanetShader->log();
		delete shadowPlanetShader;
		return false;
	}

	if(!shadowPlanetShader->addFragmentShader(
	  "varying mediump vec2 texc;\n"
	  "varying mediump float lambert;\n"
	  "uniform sampler2D tex;\n"
	  "uniform mediump vec4 globalColor;\n"
	  "uniform mediump vec4 ambientLight;\n"
	  "uniform mediump vec4 diffuseLight;\n"
	  "\n"
	  "varying highp vec3 P;\n"
	  "\n"
	  "uniform sampler2D info;\n"
	  "uniform int current;\n"
	  "uniform int infoCount;\n"
	  "uniform float infoSize;\n"
	  "\n"
	  "uniform bool ring;\n"
	  "uniform highp float outerRadius;\n"
	  "uniform highp float innerRadius;\n"
	  "uniform sampler2D ringS;\n"
	  "uniform bool isRing;\n"
	  "\n"
	  "uniform bool isMoon;\n"
	  "uniform sampler2D earthShadow;\n"
	  "\n"
	  "bool visible(vec3 normal, vec3 light)\n"
	  "{\n"
	  "    return (dot(light, normal) > 0.0);\n"
	  "}\n"
	  "\n"
	  "void main()\n"
	  "{\n"
	  "    float final_illumination = 1.0;\n"
	  "    vec4 diffuse = diffuseLight;\n"
	  "    vec4 data = texture2D(info, vec2(0.0, current) / infoSize);\n"
	  "    float RS = data.w;\n"
	  "    vec3 Lp = data.xyz;\n"
	  "\n"
	  "    vec3 P3;\n"
	  "\n"
	  "    if(isRing)\n"
	  "        P3 = P;\n"
	  "    else\n"
	  "    {\n"
	  "        data = texture2D(info, vec2(current, current) / infoSize);\n"
	  "        P3 = normalize(P) * data.w;\n"
	  "    }\n"
	  "\n"
	  "    if((lambert > 0.0) || isRing)\n"
	  "    {\n"
	  "        if(ring && !isRing)\n"
	  "        {\n"
	  "            vec3 ray = normalize(Lp);\n"
	  "            vec3 normal = normalize(vec3(0.0, 0.0, 1.0));\n"
	  "            float u = - dot(P3, normal) / dot(ray, normal);\n"
	  "\n"
	  "            if(u > 0.0 && u < 1e10)\n"
	  "            {\n"
	  "                float ring_radius = length(P3 + u * ray);\n"
	  "\n"
	  "                if(ring_radius > innerRadius && ring_radius < outerRadius)\n"
	  "                {\n"
	  "                    ring_radius = (ring_radius - innerRadius) / (outerRadius - innerRadius);\n"
	  "                    data = texture2D(ringS, vec2(ring_radius, 0.5));\n"
	  "\n"
	  "                    final_illumination = 1.0 - data.w;\n"
	  "                }\n"
	  "            }\n"
	  "        }\n"
	  "\n"
	  "        for(int i = 1; i < infoCount; i++)\n"
	  "        {\n"
	  "            if(current == i && !isRing)\n"
	  "                continue;\n"
	  "\n"
	  "            data = texture2D(info, vec2(i, current) / infoSize);\n"
	  "            vec3 C = data.rgb;\n"
	  "            float radius = data.a;\n"
	  "\n"
	  "            float L = length(Lp - P3);\n"
	  "            float l = length(C - P3);\n"
	  "\n"
	  "            float R = RS / L;\n"
	  "            float r = radius / l;\n"
	  "            float d = length( (Lp - P3) / L - (C - P3) / l );\n"
	  "\n"
	  "            float illumination = 1.0;\n"
	  "\n"
	  "            // distance too far\n"
	  "            if(d >= R + r)\n"
	  "            {\n"
	  "                illumination = 1.0;\n"
	  "            }\n"
	  "            // umbra\n"
	  "            else if(r >= R + d)\n"
	  "            {\n"
	  "                if(isMoon)\n"
	  "                    illumination = d / (r - R) * 0.6;\n"
	  "                else"
	  "                    illumination = 0.0;\n"
	  "            }\n"
	  "            // penumbra completely inside\n"
	  "            else if(d + r <= R)\n"
	  "            {\n"
	  "                illumination = 1.0 - r * r / (R * R);\n"
	  "            }\n"
	  "            // penumbra partially inside\n"
	  "            else\n"
	  "            {\n"
	  "                if(isMoon)\n"
	  "                    illumination = ((d - abs(R-r)) / (R + r - abs(R-r))) * 0.4 + 0.6;\n"
	  "                else\n"
	  "                {\n"
	  "                    float x = (R * R + d * d - r * r) / (2.0 * d);\n"
	  "\n"
	  "                    float alpha = acos(x / R);\n"
	  "                    float beta = acos((d - x) / r);\n"
	  "\n"
	  "                    float AR = R * R * (alpha - 0.5 * sin(2.0 * alpha));\n"
	  "                    float Ar = r * r * (beta - 0.5 * sin(2.0 * beta));\n"
	  "                    float AS = R * R * 2.0 * asin(1.0);\n"
	  "\n"
	  "                    illumination = 1.0 - (AR + Ar) / AS;\n"
	  "                }\n"
	  "            }\n"
	  "\n"
	  "            if(illumination < final_illumination)\n"
	  "                final_illumination = illumination;\n"
	  "        }\n"
	  "    }\n"
	  "\n"
	  "    vec4 litColor = (isRing ? 1.0 : lambert) * final_illumination * diffuse + ambientLight;\n"
	  "    if(isMoon && final_illumination < 1.0)\n"
	  "    {\n"
	  "        vec4 shadowColor = texture2D(earthShadow, vec2(final_illumination, 0.5));\n"
	  "        gl_FragColor = mix(texture2D(tex, texc) * litColor, shadowColor, shadowColor.a);\n"
	  "    }\n"
	  "    else\n"
	  "        gl_FragColor = texture2D(tex, texc) * litColor;\n"
	  "}\n"
	  "\n"))
	{
		qWarning() << "Error adding shadow planet fragment shader: " << shadowPlanetShader->log();
		delete shadowPlanetShader;
		return false;
	}
	if(!shadowPlanetShader->build())
	{
		qWarning() << "Error building shader: " << shadowPlanetShader->log();
		delete shadowPlanetShader;
		return false;
	}
	qDebug() << "Shadow planet shader build log: " << shadowPlanetShader->log();

	return true;
}

Planet::Planet(const QString& englishName,
               int flagLighting,        
               double radius,
               double oblateness,
               Vec3f color,
               float albedo,
               const QString& atexMapName,
               posFuncType coordFunc,
               void* auserDataPtr,
               OsculatingFunctType *osculatingFunc,
               bool acloseOrbit,
               bool hidden,
	       bool hasAtmosphere,
	       const QString& pType)
	: englishName(englishName),
	  flagLighting(flagLighting),
	  radius(radius), oneMinusOblateness(1.0-oblateness),
	  color(color), albedo(albedo), axisRotation(0.),
	  texture(NULL),
	  rings(NULL),
	  sphereScale(1.f),
	  lastJD(J2000),
	  coordFunc(coordFunc),
	  userDataPtr(auserDataPtr),
	  osculatingFunc(osculatingFunc),
	  parent(NULL),
	  hidden(hidden),
	  atmosphere(hasAtmosphere),
	  pType(pType),
	  unlitSphere(NULL),
	  litSphere(NULL)
{
	texMapName = atexMapName;
	lastOrbitJD =0;
	deltaJD = StelCore::JD_SECOND;
	orbitCached = 0;
	orbitVertices = NULL;
	closeOrbit = acloseOrbit;

	eclipticPos=Vec3d(0.,0.,0.);
	rotLocalToParent = Mat4d::identity();

	nameI18 = englishName;
	if (englishName!="Pluto")
	{
		deltaJD = 0.001*StelCore::JD_SECOND;
	}
	flagLabels = true;
}

Planet::~Planet()
{
	if(NULL != rings)
	{
		delete rings;
		rings = NULL;
	}

	if(NULL != orbitVertices)
	{
		delete orbitVertices;
		orbitVertices = NULL;
	}

	if(NULL != litSphere)
	{
		Q_ASSERT_X(NULL == unlitSphere, Q_FUNC_INFO,
		           "Both lit and unlit spheres have been generated");
		delete litSphere;
		litSphere = NULL;
	}
	if(NULL != unlitSphere)
	{
		Q_ASSERT_X(NULL == litSphere, Q_FUNC_INFO,
		           "Both lit and unlit spheres have been generated");
		delete unlitSphere;
		unlitSphere = NULL;
	}
	if(NULL != texture)
	{
		delete texture;
		texture = NULL;
	}
}

void Planet::translateName(StelTranslator& trans)
{
	nameI18 = trans.qtranslate(englishName);
}

// Return the information string "ready to print" :)
QString Planet::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if (flags&Name)
	{
		oss << "<h2>" << q_(englishName);  // UI translation can differ from sky translation
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
	}

	if (flags&Extra1)
	{
		if (pType.length()>0)
			oss << q_("Type: <b>%1</b>").arg(q_(pType)) << "<br />";
	}

	if (flags&Magnitude)
	{
		if (core->getSkyDrawer()->getFlagHasAtmosphere())
		    oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
										    QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
		else
		    oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core, false), 0, 'f', 2) << "<br>";
	}
	if (flags&AbsoluteMagnitude)
		oss << q_("Absolute Magnitude: %1").arg(getVMagnitude(core, false)-5.*(std::log10(getJ2000EquatorialPos(core).length()*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if ((flags&Extra1) && (core->getCurrentLocation().planetName=="Earth"))
	{
		//static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
		//double ecl= -(ssystem->getEarth()->getRotObliquity()); // BUG DETECTED! Earth's obliquity is apparently reported constant.
		double ra_equ, dec_equ, lambda, beta;
		double ecl= get_mean_ecliptical_obliquity(core->getJDay()) *M_PI/180.0;
		StelUtils::rectToSphe(&ra_equ,&dec_equ,getEquinoxEquatorialPos(core));
		StelUtils::ctRadec2Ecl(ra_equ, dec_equ, ecl, &lambda, &beta);
		if (lambda<0) lambda+=2.0*M_PI;
		oss << q_("Ecliptic Topocentric (of date): %1/%2").arg(StelUtils::radToDmsStr(lambda, true), StelUtils::radToDmsStr(beta, true)) << "<br>";
		oss << q_("Obliquity (of date, for Earth): %1").arg(StelUtils::radToDmsStr(ecl, true)) << "<br>";
	}

	if (flags&Distance)
	{
		double distanceAu = getJ2000EquatorialPos(core).length();
		if (distanceAu < 0.1)
		{
			double distanceKm = AU * distanceAu;
			// xgettext:no-c-format
			oss << QString(q_("Distance: %1AU (%2 km)"))
			       .arg(distanceAu, 0, 'f', 8)
			       .arg(distanceKm, 0, 'f', 0);
		}
		else
		{
			// xgettext:no-c-format
			oss << q_("Distance: %1AU").arg(distanceAu, 0, 'f', 8);
		}
		oss << "<br>";
	}

	if (flags&Size)
	{
		double angularSize = 2.*getAngularSize(core)*M_PI/180.;
		if (rings)
		{
			double withoutRings = 2.*getSpheroidAngularSize(core)*M_PI/180.;
			oss << q_("Apparent diameter: %1, with rings: %2")
			       .arg(StelUtils::radToDmsStr(withoutRings, true),
			            StelUtils::radToDmsStr(angularSize, true));
		}
		else
		{
			oss << q_("Apparent diameter: %1").arg(StelUtils::radToDmsStr(angularSize, true));
		}
		oss << "<br>";
	}

	double siderealPeriod = getSiderealPeriod();
	double siderealDay = getSiderealDay();
	if ((flags&Extra1) && (siderealPeriod>0))
	{
		// TRANSLATORS: Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
		oss << q_("Sidereal period: %1 days (%2 a)").arg(QString::number(siderealPeriod, 'f', 2)).arg(QString::number(siderealPeriod/365.25, 'f', 3)) << "<br>";
		if (std::abs(siderealDay)>0)
		{			
			oss << q_("Sidereal day: %1").arg(StelUtils::hoursToHmsStr(std::abs(siderealDay*24))) << "<br>";
			if (englishName == "Venus" || englishName == "Uranus" || englishName == "Pluto")
				oss << q_("Mean solar day: %1").arg(StelUtils::hoursToHmsStr(std::abs(24*StelUtils::calculateSolarDay(siderealPeriod, siderealDay, false)))) << "<br>";
			else
				oss << q_("Mean solar day: %1").arg(StelUtils::hoursToHmsStr(std::abs(siderealDay/((siderealPeriod - 1)/siderealPeriod)*24))) << "<br>";
		}
	}

	if ((flags&Extra2) && (englishName.compare("Sun")!=0))
	{
		const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();		
		oss << QString(q_("Phase Angle: %1")).arg(StelUtils::radToDmsStr(getPhaseAngle(observerHelioPos))) << "<br>";
		oss << QString(q_("Elongation: %1")).arg(StelUtils::radToDmsStr(getElongation(observerHelioPos))) << "<br>";
		oss << QString(q_("Phase: %1")).arg(getPhase(observerHelioPos), 0, 'f', 2) << "<br>";
		oss << QString(q_("Illuminated: %1%")).arg(getPhase(observerHelioPos) * 100, 0, 'f', 1) << "<br>";
	}

	postProcessInfoString(str, flags);

	return str;
}

//! Get sky label (sky translation)
QString Planet::getSkyLabel(const StelCore*) const
{
	QString str;
	QTextStream oss(&str);
	oss.setRealNumberPrecision(2);
	oss << nameI18;

	if (sphereScale != 1.f)
	{
		oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
	}
	return str;
}

float Planet::getSelectPriority(const StelCore* core) const
{
	if( ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getFlagHints() )
	{
	// easy to select, especially pluto
		return getVMagnitude(core, false)-15.f;
	}
	else
	{
		return getVMagnitude(core, false) - 8.f;
	}
}

Vec3f Planet::getInfoColor(void) const
{
	Vec3f col = ((SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem"))->getLabelsColor();
	if (StelApp::getInstance().getVisionModeNight())
		col = StelUtils::getNightColor(col);
	return col;
}


double Planet::getCloseViewFov(const StelCore* core) const
{
	return std::atan(radius*sphereScale*2.f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
}

double Planet::getSatellitesFov(const StelCore* core) const
{
	// TODO: calculate from satellite orbits rather than hard code
	if (englishName=="Jupiter") return std::atan(0.005f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	if (englishName=="Saturn") return std::atan(0.005f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	if (englishName=="Mars") return std::atan(0.0001f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	if (englishName=="Uranus") return std::atan(0.002f/getEquinoxEquatorialPos(core).length())*180./M_PI * 4;
	return -1.;
}

double Planet::getParentSatellitesFov(const StelCore* core) const
{
	if (parent && parent->parent) return parent->getSatellitesFov(core);
	return -1.0;
}

// Set the orbital elements
void Planet::setRotationElements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate, double _siderealPeriod )
{
	re.period = _period;
	re.offset = _offset;
	re.epoch = _epoch;
	re.obliquity = _obliquity;
	re.ascendingNode = _ascendingNode;
	re.precessionRate = _precessionRate;
	re.siderealPeriod = _siderealPeriod;  // used for drawing orbit lines

	deltaOrbitJD = re.siderealPeriod/ORBIT_SEGMENTS;
}

Vec3d Planet::getJ2000EquatorialPos(const StelCore *core) const
{
	return StelCore::matVsop87ToJ2000.multiplyWithoutTranslation(getHeliocentricEclipticPos() - core->getObserverHeliocentricEclipticPos());
}

// Compute the position in the parent Planet coordinate system
// Actually call the provided function to compute the ecliptical position
void Planet::computePositionWithoutOrbits(const double date)
{
	if (fabs(lastJD-date)>deltaJD)
	{
		coordFunc(date, eclipticPos, userDataPtr);
		lastJD = date;
	}
}

void Planet::computePosition(const double date)
{

	if (orbitFader.getInterstate()>0.000001 && deltaOrbitJD > 0 && (fabs(lastOrbitJD-date)>deltaOrbitJD || !orbitCached))
	{

		// calculate orbit first (for line drawing)
		double date_increment = re.siderealPeriod/ORBIT_SEGMENTS;
		double calc_date;
		// int delta_points = (int)(0.5 + (date - lastOrbitJD)/date_increment);
		int delta_points;

		if( date > lastOrbitJD )
		{
			delta_points = (int)(0.5 + (date - lastOrbitJD)/date_increment);
		}
		else
		{
			delta_points = (int)(-0.5 + (date - lastOrbitJD)/date_increment);
		}
		double new_date = lastOrbitJD + delta_points*date_increment;

		// qDebug( "Updating orbit coordinates for %s (delta %f) (%d points)\n", name.c_str(), deltaOrbitJD, delta_points);

		if( delta_points > 0 && delta_points < ORBIT_SEGMENTS && orbitCached)
		{

			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				if(d + delta_points >= ORBIT_SEGMENTS )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;

					// date increments between points will not be completely constant though
					computeTransMatrix(calc_date);
					if (osculatingFunc)
					{
						(*osculatingFunc)(date,calc_date,eclipticPos);
					}
					else
					{
						coordFunc(calc_date, eclipticPos, userDataPtr);
					}
					orbitP[d] = eclipticPos;
					orbit[d] = getHeliocentricEclipticPos();
				}
				else
				{
					orbitP[d] = orbitP[d+delta_points];
					orbit[d] = getHeliocentricPos(orbitP[d]);
				}
			}

			lastOrbitJD = new_date;
		}
		else if( delta_points < 0 && abs(delta_points) < ORBIT_SEGMENTS  && orbitCached)
		{

			for( int d=ORBIT_SEGMENTS-1; d>=0; d-- )
			{
				if(d + delta_points < 0 )
				{
					// calculate new points
					calc_date = new_date + (d-ORBIT_SEGMENTS/2)*date_increment;

					computeTransMatrix(calc_date);
					if (osculatingFunc) {
						(*osculatingFunc)(date,calc_date,eclipticPos);
					}
					else
					{
						coordFunc(calc_date, eclipticPos, userDataPtr);
					}
					orbitP[d] = eclipticPos;
					orbit[d] = getHeliocentricEclipticPos();
				}
				else
				{
					orbitP[d] = orbitP[d+delta_points];
					orbit[d] = getHeliocentricPos(orbitP[d]);
				}
			}

			lastOrbitJD = new_date;

		}
		else if( delta_points || !orbitCached)
		{

			// update all points (less efficient)
			for( int d=0; d<ORBIT_SEGMENTS; d++ )
			{
				calc_date = date + (d-ORBIT_SEGMENTS/2)*date_increment;
				computeTransMatrix(calc_date);
				if (osculatingFunc)
				{
					(*osculatingFunc)(date,calc_date,eclipticPos);
				}
				else
				{
					coordFunc(calc_date, eclipticPos, userDataPtr);
				}
				orbitP[d] = eclipticPos;
				orbit[d] = getHeliocentricEclipticPos();
			}

			lastOrbitJD = date;
			if (!osculatingFunc) orbitCached = 1;
		}


		// calculate actual Planet position
		coordFunc(date, eclipticPos, userDataPtr);

		lastJD = date;

	}
	else if (fabs(lastJD-date)>deltaJD)
	{
		// calculate actual Planet position
		coordFunc(date, eclipticPos, userDataPtr);
		for( int d=0; d<ORBIT_SEGMENTS; d++ )
			orbit[d]=getHeliocentricPos(orbitP[d]);
		lastJD = date;
	}

}

// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate
void Planet::computeTransMatrix(double jd)
{
	axisRotation = getSiderealTime(jd);

	// Special case - heliocentric coordinates are on ecliptic,
	// not solar equator...
	if (parent)
	{
		rotLocalToParent = Mat4d::zrotation(re.ascendingNode - re.precessionRate*(jd-re.epoch)) * Mat4d::xrotation(re.obliquity);
	}
}

void Planet::computeModelMatrix(Mat4d &result) const
{
	result = Mat4d::translation(eclipticPos) * rotLocalToParent * Mat4d::zrotation(M_PI/180*(axisRotation + 90.));
	PlanetP p = parent;
	while (p && p->parent)
	{
		result = Mat4d::translation(p->eclipticPos) * result * p->rotLocalToParent;
		p = p->parent;
	}
}

#ifdef ANDROID
/* This is kind of ugly, but I'm hitting a GCC bug
  in this code if it tries to optimize it. This bug
  _may_ be fixed in the GCC included in newer versions
  of the NDK. */
#pragma GCC optimize ("O0")
#endif

Mat4d Planet::getRotEquatorialToVsop87(void) const
{
	Mat4d rval = rotLocalToParent;
	if (parent)
	{
		for (PlanetP p=parent;p->parent;p=p->parent)
			rval = p->rotLocalToParent * rval;
	}
	return rval;
}

void Planet::setRotEquatorialToVsop87(const Mat4d &m)
{
	Mat4d a = Mat4d::identity();
	if (parent)
	{
		for (PlanetP p=parent;p->parent;p=p->parent)
			a = p->rotLocalToParent * a;
	}
	rotLocalToParent = a.transpose() * m;
}

#ifdef ANDROID
#pragma GCC reset_options
#endif


// Compute the z rotation to use from equatorial to geographic coordinates
double Planet::getSiderealTime(double jd) const
{
	if (englishName=="Earth")
	{
		return get_apparent_sidereal_time(jd);
	}

	double t = jd - re.epoch;
	double rotations = t / (double) re.period;
	double wholeRotations = floor(rotations);
	double remainder = rotations - wholeRotations;

	if (englishName=="Jupiter")
	{
		// use semi-empirical coefficient for GRS drift
		// TODO: need improved
		return remainder * 360. + re.offset - 0.2483 * std::abs(StelApp::getInstance().getCore()->getJDay() - 2456172);
	}
	else
		return remainder * 360. + re.offset;
}

// Get the Planet position in the parent Planet ecliptic coordinate in AU
Vec3d Planet::getEclipticPos() const
{
	return eclipticPos;
}

// Return the heliocentric ecliptical position (Vsop87)
Vec3d Planet::getHeliocentricEclipticPos() const
{
	Vec3d pos = eclipticPos;
	PlanetP pp = parent;
	if (pp)
	{
		while (pp->parent)
		{
			pos += pp->eclipticPos;
			pp = pp->parent;
		}
	}
	return pos;
}

// Return heliocentric coordinate of p
Vec3d Planet::getHeliocentricPos(Vec3d p) const
{
	// Optimization:
	//
	// This code used about 8% of runtime before,
	// this is an optimized version - avoiding smart pointer checks 
	// (this function doesn't own any of the parents - planets 
	// and solar system do, so we're OK)
	//
	// This is the equivalent (previous) unoptimized code:
	// (update this if you make any functionality changes)
	// PlanetP pp = parent;
	// if (pp)
	// {
	// 	while (pp->parent)
	// 	{
	// 		pos += pp->eclipticPos;
	// 		pp = pp->parent;
	// 	}
	// }
	Vec3d pos = p;
	const Planet* ourParent = &(*parent);
	const Planet* parentsParent;
	// int i = 0;
	if (NULL != ourParent)
	{
		// const Planet* const parentsParent = &(*(ourParent->parent));
		while (NULL != (parentsParent = &(*(ourParent->parent))))
		{
			pos += ourParent->eclipticPos;
			ourParent = parentsParent;
		}
	}
	return pos;
}

void Planet::setHeliocentricEclipticPos(const Vec3d &pos)
{
	eclipticPos = pos;
	PlanetP p = parent;
	if (p)
	{
		while (p->parent)
		{
			eclipticPos -= p->eclipticPos;
			p = p->parent;
		}
	}
}

// Compute the distance to the given position in heliocentric coordinate (in AU)
double Planet::computeDistance(const Vec3d& obsHelioPos)
{
	distance = (obsHelioPos-getHeliocentricEclipticPos()).length();
	return distance;
}

// Get the phase angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getPhaseAngle(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	//return std::acos(observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	return std::acos((observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq)));
}

// Get the planet phase for an observer at pos obsPos in heliocentric coordinates (in AU)
float Planet::getPhase(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	return 0.5f * std::abs(1.f + cos_chi);
}

// Get the elongation angle (radians) for an observer at pos obsPos in heliocentric coordinates (dist in AU)
double Planet::getElongation(const Vec3d& obsPos) const
{
	const double observerRq = obsPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (obsPos - planetHelioPos).lengthSquared();
	return std::acos((observerPlanetRq  + observerRq - planetRq)/(2.0*sqrt(observerPlanetRq*observerRq)));
}

// Computation of the visual magnitude (V band) of the planet.
float Planet::getVMagnitude(const StelCore* core, bool withExtinction) const
{
	float extinctionMag=0.0; // track magnitude loss
	if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
	{
	    Vec3d altAz=getAltAzPosApparent(core);
	    altAz.normalize();
	    core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
	}


	if (parent == 0)
	{
		// sun, compute the apparent magnitude for the absolute mag (4.83) and observer's distance
		const double distParsec = std::sqrt(core->getObserverHeliocentricEclipticPos().lengthSquared())*AU/PARSEC;

		// check how much of it is visible
		const SolarSystem* ssm = GETSTELMODULE(SolarSystem);
		double shadowFactor = ssm->getEclipseFactor(core);
		if(shadowFactor < 1e-11)
			shadowFactor = 1e-11;

		return 4.83 + 5.*(std::log10(distParsec)-1.) - 2.5*(std::log10(shadowFactor)) + extinctionMag;
	}

	// Compute the angular phase
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const double observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const double planetRq = planetHelioPos.lengthSquared();
	const double observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const double cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*sqrt(observerPlanetRq*planetRq));
	double phase = std::acos(cos_chi);

	double shadowFactor = 1.;
	// Check if the satellite is inside the inner shadow of the parent planet:
	if (parent->parent != 0)
	{
		const Vec3d& parentHeliopos = parent->getHeliocentricEclipticPos();
		const double parent_Rq = parentHeliopos.lengthSquared();
		const double pos_times_parent_pos = planetHelioPos * parentHeliopos;
		if (pos_times_parent_pos > parent_Rq)
		{
			// The satellite is farther away from the sun than the parent planet.
			const double sun_radius = parent->parent->radius;
			const double sun_minus_parent_radius = sun_radius - parent->radius;
			const double quot = pos_times_parent_pos/parent_Rq;

			// Compute d = distance from satellite center to border of inner shadow.
			// d>0 means inside the shadow cone.
			double d = sun_radius - sun_minus_parent_radius*quot - std::sqrt((1.-sun_minus_parent_radius/sqrt(parent_Rq)) * (planetRq-pos_times_parent_pos*quot));
			if (d>=radius)
			{
				// The satellite is totally inside the inner shadow.
				shadowFactor = 1e-9;
			}
			else if (d>-radius)
			{
				// The satellite is partly inside the inner shadow,
				// compute a fantasy value for the magnitude:
				d /= radius;
				shadowFactor = (0.5 - (std::asin(d)+d*std::sqrt(1.0-d*d))/M_PI);
			}
		}
	}

	// Use empirical formulae for main planets when seen from earth
	if (core->getCurrentLocation().planetName=="Earth")
	{
		const double phaseDeg=phase*180./M_PI;
		const double d = 5. * log10(sqrt(observerPlanetRq*planetRq));
		//double f1 = phaseDeg/100.;

		/*
		// Algorithm provided by Pere Planesas (Observatorio Astronomico Nacional)
		if (englishName=="Mercury")
		{
			if ( phaseDeg > 150. ) f1 = 1.5;
			return -0.36 + d + 3.8*f1 - 2.73*f1*f1 + 2*f1*f1*f1 + extinctionMag;
		}
		if (englishName=="Venus")
			return -4.29 + d + 0.09*f1 + 2.39*f1*f1 - 0.65*f1*f1*f1 + extinctionMag;
		if (englishName=="Mars")
			return -1.52 + d + 0.016*phaseDeg + extinctionMag;
		if (englishName=="Jupiter")
			return -9.25 + d + 0.005*phaseDeg + extinctionMag;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
			// double rings = -2.6*sinx + 1.25*sinx*sinx;
			return -8.88 + d + 0.044*phaseDeg + extinctionMag;// + rings;
		}
		if (englishName=="Uranus")
			return -7.19 + d + 0.0028*phaseDeg + extinctionMag;
		if (englishName=="Neptune")
			return -6.87 + d + extinctionMag;
		if (englishName=="Pluto")
			return -1.01 + d + 0.041*phaseDeg + extinctionMag;
		*/
		// GZ: I prefer the values given by Meeus, Astronomical Algorithms (1992).
		// There are two solutions:
		// (1) G. Mller, based on visual observations 1877-91. [Expl.Suppl.1961]
		// (2) Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes.
		// The structure is almost identical, just the numbers are different!
		// I activate (1) for now, because we want to simulate the eye's impression. (Esp. Venus!)
		// (1)
		if (englishName=="Mercury")
		    {
			double ph50=phaseDeg-50.0;
			return 1.16 + d + 0.02838*ph50 + 0.0001023*ph50*ph50 + extinctionMag;
		    }
		if (englishName=="Venus")
			return -4.0 + d + 0.01322*phaseDeg + 0.0000004247*phaseDeg*phaseDeg*phaseDeg + extinctionMag;
		if (englishName=="Mars")
			return -1.3 + d + 0.01486*phaseDeg + extinctionMag;
		if (englishName=="Jupiter")
			return -8.93 + d + extinctionMag;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
			// GZ: implemented from Meeus, Astr.Alg.1992
			const double jd=core->getJDay();
			const double T=(jd-2451545.0)/36525.0;
			const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
			const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
			static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
			const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
			double lambda=atan2(saturnEarth[1], saturnEarth[0]);
			double beta=atan2(saturnEarth[2], sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
			const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
			double rings = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
			return -8.68 + d + 0.044*phaseDeg + rings + extinctionMag;
		}
		if (englishName=="Uranus")
			return -6.85 + d + extinctionMag;
		if (englishName=="Neptune")
			return -7.05 + d + extinctionMag;
		if (englishName=="Pluto")
			return -1.0 + d + extinctionMag;
		/*
		// (2)
		if (englishName=="Mercury")
			return 0.42 + d + .038*phaseDeg - 0.000273*phaseDeg*phaseDeg + 0.000002*phaseDeg*phaseDeg*phaseDeg + extinctionMag;
		if (englishName=="Venus")
			return -4.40 + d + 0.0009*phaseDeg + 0.000239*phaseDeg*phaseDeg - 0.00000065*phaseDeg*phaseDeg*phaseDeg + extinctionMag;
		if (englishName=="Mars")
			return -1.52 + d + 0.016*phaseDeg + extinctionMag;
		if (englishName=="Jupiter")
			return -9.40 + d + 0.005*phaseDeg + extinctionMag;
		if (englishName=="Saturn")
		{
			// TODO re-add rings computation
			// GZ: implemented from Meeus, Astr.Alg.1992
			const double jd=core->getJDay();
			const double T=(jd-2451545.0)/36525.0;
			const double i=((0.000004*T-0.012998)*T+28.075216)*M_PI/180.0;
			const double Omega=((0.000412*T+1.394681)*T+169.508470)*M_PI/180.0;
			static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
			const Vec3d saturnEarth=getHeliocentricEclipticPos() - ssystem->getEarth()->getHeliocentricEclipticPos();
			double lambda=atan2(saturnEarth[1], saturnEarth[0]);
			double beta=atan2(saturnEarth[2], sqrt(saturnEarth[0]*saturnEarth[0]+saturnEarth[1]*saturnEarth[1]));
			const double sinB=sin(i)*cos(beta)*sin(lambda-Omega)-cos(i)*sin(beta);
			double rings = -2.6*fabs(sinB) + 1.25*sinB*sinB; // sinx=sinB, saturnicentric latitude of earth. longish, see Meeus.
			return -8.88 + d + 0.044*phaseDeg + rings + extinctionMag;
		}
		if (englishName=="Uranus")
			return -7.19f + d + extinctionMag;
		if (englishName=="Neptune")
			return -6.87f + d + extinctionMag;
		if (englishName=="Pluto")
			return -1.00f + d + extinctionMag;
	*/
	// TODO: decide which set of formulae is best?
	}

	// This formula seems to give wrong results
	const double p = (1.0 - phase/M_PI) * cos_chi + std::sqrt(1.0 - cos_chi*cos_chi) / M_PI;
	double F = 2.0 * albedo * radius * radius * p / (3.0*observerPlanetRq*planetRq) * shadowFactor;
	return -26.73 - 2.5 * std::log10(F) + extinctionMag;
}

double Planet::getAngularSize(const StelCore* core) const
{
	double rad = radius;
	if (rings)
		rad = rings->getSize();
	return std::atan2(rad*sphereScale,getJ2000EquatorialPos(core).length()) * 180./M_PI;
}


double Planet::getSpheroidAngularSize(const StelCore* core) const
{
	return std::atan2(radius*sphereScale,getJ2000EquatorialPos(core).length()) * 180./M_PI;
}

// Draw the Planet and all the related infos : name, circle etc..
void Planet::draw(StelCore* core, StelRenderer* renderer, float maxMagLabels, 
                  const QFont& planetNameFont,
                  SharedPlanetGraphics& planetGraphics)
{
	if (hidden)
		return;

	Mat4d mat = Mat4d::translation(eclipticPos) * rotLocalToParent;
	PlanetP p = parent;
	while (p && p->parent)
	{
		mat = Mat4d::translation(p->eclipticPos) * mat * p->rotLocalToParent;
		p = p->parent;
	}

	// This removed totally the Planet shaking bug!!!
	StelProjector::ModelViewTranformP transfo = core->getHeliocentricEclipticModelViewTransform();
	transfo->combine(mat);
	if (getEnglishName() == core->getCurrentLocation().planetName)
	{
		// Draw the rings if we are located on a planet with rings, but not the planet itself.
		if (rings)
		{
			rings->draw(core->getProjection(transfo), renderer, transfo, planetGraphics.planetShader, 1000.0,
						planetGraphics.planetShader == planetGraphics.shadowPlanetShader ? &planetGraphics.info : NULL);
		}
		return;
	}

	// Compute the 2D position and check if in the screen
	const StelProjectorP prj = core->getProjection(transfo);
	float screenSz = getAngularSize(core)*M_PI/180.*prj->getPixelPerRadAtCenter();
	float viewport_left = prj->getViewportPosX();
	float viewport_bottom = prj->getViewportPosY();
	if (prj->project(Vec3d(0), screenPos)
	    && screenPos[1]>viewport_bottom - screenSz && screenPos[1] < viewport_bottom + prj->getViewportHeight()+screenSz
	    && screenPos[0]>viewport_left - screenSz && screenPos[0] < viewport_left + prj->getViewportWidth() + screenSz)
	{
		// Draw the name, and the circle if it's not too close from the body it's turning around
		// this prevents name overlaping (ie for jupiter satellites)
		float ang_dist = 300.f*atan(getEclipticPos().length()/getEquinoxEquatorialPos(core).length())/core->getMovementMgr()->getCurrentFov();
		if (ang_dist==0.f)
			ang_dist = 1.f; // if ang_dist == 0, the Planet is sun..

		// by putting here, only draw orbit if Planet is visible for clarity
		drawOrbit(core, renderer);  // TODO - fade in here also...

		if (flagLabels && ang_dist>0.25 && maxMagLabels>getVMagnitude(core))
		{
			labelsFader=true;
		}
		else
		{
			labelsFader=false;
		}
		drawHints(core, renderer, planetNameFont, planetGraphics);

		draw3dModel(core, renderer, planetGraphics, transfo, screenSz);
	}
	return;
}

void Planet::draw3dModel(StelCore* core, StelRenderer* renderer,
                         SharedPlanetGraphics& planetGraphics,
                         StelProjector::ModelViewTranformP transfo, float screenSz)
{
	// This is the main method drawing a planet 3d model
	// Some work has to be done on this method to make the rendering nicer

	if (screenSz > 1.0f)
	{
		StelProjector::ModelViewTranformP transfo2 = transfo->clone();
		transfo2->combine(Mat4d::zrotation(M_PI/180*(axisRotation + 90.)));
		StelProjectorP projector = core->getProjection(transfo2);
		StelLight light;

		if (flagLighting)
		{
			// Set the main source of light to be the sun
			Vec3d sunPos(0);
			core->getHeliocentricEclipticModelViewTransform()->forward(sunPos);
			// Set the light parameters taking sun as the light source
			light.position = Vec3f(sunPos[0], sunPos[1], sunPos[2]);
			light.ambient  = Vec4f(0.02f, 0.02f, 0.02f, 0.02f);
			light.diffuse  = Vec4f(2.0f, 2.0f, 2.0f, 1.0f);
			if (StelApp::getInstance().getVisionModeNight())
			{
				light.diffuse[1] = 0.; light.diffuse[2] = 0.;
				light.ambient[1] = 0.; light.ambient[2] = 0.;
			}
		}

		if (rings)
		{
			const double dist = getEquinoxEquatorialPos(core).length();
			double z_near = 0.9*(dist - rings->getSize());
			double z_far  = 1.1*(dist + rings->getSize());
			if (z_near < 0.0) z_near = 0.0;
			double n,f;
			core->getClippingPlanes(&n,&f); // Save clipping planes
			core->setClippingPlanes(z_near,z_far);

			renderer->clearDepthBuffer();
			renderer->setDepthTest(DepthTest_ReadWrite);
			drawSphere(renderer, projector, flagLighting ? &light : NULL, 
					   planetGraphics, screenSz);
			renderer->setDepthTest(DepthTest_ReadOnly);
			rings->draw(projector, renderer, transfo, planetGraphics.planetShader, screenSz,
						planetGraphics.planetShader == planetGraphics.shadowPlanetShader ? &planetGraphics.info : NULL);
			renderer->setDepthTest(DepthTest_Disabled);
			core->setClippingPlanes(n,f);  // Restore old clipping planes
		}
		else
		{
			SolarSystem* ssm = GETSTELMODULE(SolarSystem);
			if (this==ssm->getMoon() && core->getCurrentLocation().planetName=="Earth" && ssm->nearLunarEclipse())
			{
				// Draw earth shadow over moon using stencil buffer if appropriate
				// This effect curently only looks right from earth viewpoint
				// TODO: moon magnitude label during eclipse isn't accurate...
				renderer->clearStencilBuffer();
				renderer->setStencilTest(StencilTest_Write_1);
				drawSphere(renderer, projector, flagLighting ? &light : NULL, 
						   planetGraphics, screenSz);
				renderer->setStencilTest(StencilTest_Disabled);
				if(planetGraphics.planetShader == planetGraphics.simplePlanetShader)
					drawEarthShadow(core, renderer, planetGraphics);
			}
			else
			{
				// Normal planet
				drawSphere(renderer, projector, flagLighting ? &light : NULL, 
						   planetGraphics, screenSz);
			}
		}
	}

	// Draw the halo

	float surfArcMin2 = getSpheroidAngularSize(core)*60;
	surfArcMin2 = surfArcMin2*surfArcMin2*M_PI; // the total illuminated area in arcmin^2

	const Vec3d tmp = getJ2000EquatorialPos(core);
	float mag = getVMagnitude(core, true);

	SolarSystem* ssm = GETSTELMODULE(SolarSystem);
	if(this != ssm->getSun() || mag < -15.0f)
		core->getSkyDrawer()
			->postDrawSky3dModel(core->getProjection(StelCore::FrameJ2000),
								 tmp, surfArcMin2, mag, color);
	if(this == ssm->getSun() && core->getCurrentLocation().planetName == "Earth")
	{
		float eclipseFactor = ssm->getEclipseFactor(core);
		if(eclipseFactor < 0.001)
		{
			core->getSkyDrawer()->drawSunCorona(core->getProjection(StelCore::FrameJ2000), tmp, screenSz, 1.0 - eclipseFactor * 1000);
		}
	}
}

void Planet::drawUnlitSphere(StelRenderer* renderer, StelProjectorP projector)
{
	if(NULL == unlitSphere)
	{
		const SphereParams params = SphereParams(radius * sphereScale).resolution(40, 40)
		                            .oneMinusOblateness(oneMinusOblateness);
		unlitSphere = StelGeometryBuilder().buildSphereUnlit(params);
	}

	unlitSphere->draw(renderer, projector);
}

void Planet::drawSphere(StelRenderer* renderer, StelProjectorP projector,
						const StelLight* light, SharedPlanetGraphics& planetGraphics, float screenSz)
{
	if(texMapName == "") {return;}
	if(NULL == texture)
	{
		const TextureParams textureParams = 
			TextureParams().generateMipmaps().wrap(TextureWrap_Repeat);
		texture = renderer->createTexture("textures/" + texMapName, textureParams,
		                                  TextureLoadingMode_LazyAsynchronous);
	}
	texture->bind();

	renderer->setBlendMode(BlendMode_None);
	renderer->setCulledFaces(CullFace_Back);

	// Now draw the sphere
	// Lighting is disabled, so just draw the plain sphere.
	if(NULL == light)
	{
		const Vec4f color = StelApp::getInstance().getVisionModeNight()
		                  ? Vec4f(1.f, 0.f, 0.f, 1.0f) : Vec4f(1.f, 1.f, 1.f, 1.0f);
		renderer->setGlobalColor(color);
		drawUnlitSphere(renderer, projector);
	}
	// Do the lighting on shader, avoiding the need to regenerate the sphere.
	// Shader is NULL if it failed to load, in that case we need to do lighting on the CPU.
	else if(renderer->isGLSLSupported() && NULL != planetGraphics.planetShader)
	{
		StelGLSLShader* shader = planetGraphics.planetShader;

		shader->bind();
		// provides the unprojectedVertex attribute to the shader.
		shader->useUnprojectedPositionAttribute();

		Vec3d lightPos = Vec3d(light->position[0], light->position[1], light->position[2]);
		projector->getModelViewTransform()
		         ->getApproximateLinearTransfo()
		         .transpose()
		         .multiplyWithoutTranslation(Vec3d(lightPos[0], lightPos[1], lightPos[2]));
		projector->getModelViewTransform()->backward(lightPos);
		lightPos.normalize();

		shader->setUniformValue("lightPos"           , Vec3f(lightPos[0], lightPos[1] , lightPos[2]));
		shader->setUniformValue("diffuseLight"       , light->diffuse);
		shader->setUniformValue("ambientLight"       , light->ambient);
		shader->setUniformValue("radius"             , static_cast<float>(radius * sphereScale));
		shader->setUniformValue("oneMinusOblateness" , static_cast<float>(oneMinusOblateness));

		if(shader == planetGraphics.shadowPlanetShader)
		{
			shader->setUniformValue("info", planetGraphics.info.info);
			shader->setUniformValue("infoCount", planetGraphics.info.infoCount);
			shader->setUniformValue("infoSize", planetGraphics.info.infoSize);
			shader->setUniformValue("current", planetGraphics.info.current);

			shader->setUniformValue("isRing", false);

			const bool ring = (rings != NULL) && rings->texture;
			if(ring)
				rings->texture->bind(2);
			shader->setUniformValue("ring", ring);
			shader->setUniformValue("outerRadius", ring ? static_cast<float>(rings->radiusMax) : 0.0f);
			shader->setUniformValue("innerRadius", ring ? static_cast<float>(rings->radiusMin) : 0.0f);
			shader->setUniformValue("ringS",       ring ? 2 : 0);

			const bool moon = this == GETSTELMODULE(SolarSystem)->getMoon();
			if(moon)
				planetGraphics.texEarthShadow->bind(3);
			shader->setUniformValue("isMoon", moon);
			shader->setUniformValue("earthShadow", moon ? 3: 0);
		}

		drawUnlitSphere(renderer, projector);

		shader->release();
	}
	// If shaders are not supported and we need lighting, we generate the sphere 
	// with lighting baked into vertex colors.
	else
	{
		// Adapt the number of facets according with the size of the sphere for optimization
		// 40 facets for 1024 pixels diameter on screen
		const int resolution = std::min(40, std::max(7, static_cast<int>(screenSz * 40.0 / 50.0)));

		// Lazily construct the lit sphere.
		if(NULL == litSphere)
		{
			const SphereParams params = SphereParams(radius * sphereScale)
			                            .resolution(resolution, resolution)
			                            .oneMinusOblateness(oneMinusOblateness);
			litSphere = StelGeometryBuilder().buildSphereLit(params, *light);
		}
		else
		{
			litSphere->setResolution(resolution, resolution);
			litSphere->setLight(*light);
		}
		litSphere->draw(renderer, projector);
	}

	renderer->setCulledFaces(CullFace_None);
}

// draws earth shadow overlapping the moon using stencil buffer
// umbra and penumbra are sized separately for accuracy
void Planet::drawEarthShadow(StelCore* core, StelRenderer* renderer, 
                             SharedPlanetGraphics& planetGraphics)
{
	SolarSystem* ssm = GETSTELMODULE(SolarSystem);
	Vec3d e = ssm->getEarth()->getEclipticPos();
	Vec3d m = ssm->getMoon()->getEclipticPos();  // relative to earth
	Vec3d mh = ssm->getMoon()->getHeliocentricEclipticPos();  // relative to sun
	float mscale = ssm->getMoon()->getSphereScale();

	// shadow location at earth + moon distance along earth vector from sun
	Vec3d en = e;
	en.normalize();
	Vec3d shadow = en * (e.length() + m.length());

	// find shadow radii in AU
	double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000./AU;
	double r_umbra = 6378.1/AU - m.length()*(689621.9/AU/e.length());

	// find vector orthogonal to sun-earth vector using cross product with
	// a non-parallel vector
	Vec3d rpt = shadow^Vec3d(0,0,1);
	rpt.normalize();
	Vec3d upt = rpt*r_umbra*mscale*1.02;  // point on umbra edge
	rpt *= r_penumbra*mscale;  // point on penumbra edge

	// modify shadow location for scaled moon
	Vec3d mdist = shadow - mh;
	if (mdist.length() > r_penumbra + 2000./AU)
		return;   // not visible so don't bother drawing

	shadow = mh + mdist*mscale;

	StelProjectorP projector = core->getProjection(StelCore::FrameHeliocentricEcliptic);

	renderer->setBlendMode(BlendMode_Alpha);
	renderer->setGlobalColor(1.0f, 1.0f, 1.0f);
	// We draw only where the stencil buffer is at 1, i.e. where the moon was drawn
	renderer->setStencilTest(StencilTest_DrawIf_1);

	// shadow radial texture
	planetGraphics.texEarthShadow->bind();

	// Draw umbra first
	StelVertexBuffer<VertexP3T2>* umbra =
		renderer->createVertexBuffer<VertexP3T2>(PrimitiveType_TriangleFan);

	// johannes: work-around for nasty ATI rendering bug: use y-texture coordinate of 0.5 instead of 0.0
	const Mat4d& rotMat = Mat4d::rotation(shadow, 2.*M_PI/100.);
	Vec3f r(upt[0], upt[1], upt[2]);
	const Vec3f shadowF(shadow[0], shadow[1], shadow[2]);

	umbra->addVertex(VertexP3T2(shadowF, Vec2f(0.0f, 0.5f)));
	for (int i=1; i<=101; ++i)
	{
		r.transfo4d(rotMat);
		// position in texture of umbra edge
		umbra->addVertex(VertexP3T2(shadowF + r, Vec2f(0.6f, 0.5f)));
	}
	umbra->lock();
	renderer->drawVertexBuffer(umbra, NULL, projector);
	delete umbra;

	// now penumbra
	StelVertexBuffer<VertexP3T2>* penumbra =
		renderer->createVertexBuffer<VertexP3T2>(PrimitiveType_TriangleStrip);
	Vec3f u = r;
	r = Vec3f(rpt[0], rpt[1], rpt[2]);
	for (int i=0; i<=200; i+=2)
	{
		r.transfo4d(rotMat);
		u.transfo4d(rotMat);

		// position in texture of umbra edge
		penumbra->addVertex(VertexP3T2(shadowF + u, Vec2f(0.6f, 0.5f)));
		penumbra->addVertex(VertexP3T2(shadowF + r, Vec2f(1.0f, 0.5f)));
	}
	penumbra->lock();
	renderer->drawVertexBuffer(penumbra, NULL, projector);
	delete penumbra;

	renderer->setStencilTest(StencilTest_Disabled);
	renderer->clearStencilBuffer();
}

void Planet::drawHints(const StelCore* core, StelRenderer* renderer, 
                       const QFont& planetNameFont, SharedPlanetGraphics& planetGraphics)
{
	if (labelsFader.getInterstate()<=0.f)
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	renderer->setFont(planetNameFont);
	// Draw nameI18 + scaling if it's not == 1.
	float tmp = (hintFader.getInterstate()<=0 ? 7.f : 10.f) + getAngularSize(core)*M_PI/180.f*prj->getPixelPerRadAtCenter()/1.44f; // Shift for nameI18 printing

	Vec4f color(labelColor[0], labelColor[1], labelColor[2], labelsFader.getInterstate());
	renderer->setGlobalColor(color);
	renderer->drawText(TextParams(screenPos[0], screenPos[1], getSkyLabel(core))
	                   .shift(tmp, tmp).useGravity());

	// hint disapears smoothly on close view
	if (hintFader.getInterstate()<=0)
		return;
	tmp -= 10.f;
	if (tmp<1) tmp=1;
	color[3] *= hintFader.getInterstate() / tmp * 0.7f;
	renderer->setGlobalColor(color);

	// Draw the 2D small circle
	renderer->setBlendMode(BlendMode_Alpha);
	planetGraphics.texHintCircle->bind();
	renderer->drawTexturedRect(screenPos[0] - 11, screenPos[1] - 11, 22, 22);
}

Ring::Ring(double radiusMin,double radiusMax,const QString &texname)
	 : radiusMin(radiusMin)
	 , radiusMax(radiusMax)
	 , texName(texname)
	 , texture(NULL)
	 , ring(NULL)
{
}

Ring::~Ring(void)
{
	if(NULL != ring)
	{
		delete ring;
		ring = NULL;
	}
	if(NULL != texture)
	{
		delete texture;
		texture = NULL;
	}
}

void Ring::draw(StelProjectorP projector, StelRenderer* renderer, 
				StelProjector::ModelViewTranformP transfo, class StelGLSLShader* shader, double screenSz, ShadowPlanetShaderInfo* info)
{
	if(NULL == texture)
	{
		texture = renderer->createTexture("textures/" + texName);
	}
	screenSz -= 50;
	screenSz /= 250.0;
	if (screenSz < 0.0) screenSz = 0.0;
	else if (screenSz > 1.0) screenSz = 1.0;
	const int slices = 128+(int)((256-128)*screenSz);
	const int stacks = 8+(int)((32-8)*screenSz);

	renderer->setBlendMode(BlendMode_Alpha);
	renderer->setCulledFaces(CullFace_Back);
	const Vec4f color = StelApp::getInstance().getVisionModeNight()
	                  ? Vec4f(1.f, 0.f, 0.f, 1.0f) : Vec4f(1.f, 1.f, 1.f, 1.0f);
	renderer->setGlobalColor(color);

	texture->bind();

	Mat4d mat = transfo->getApproximateLinearTransfo();
	// solve the ring wraparound by culling:
	// decide if we are above or below the ring plane
	const double h = mat.r[ 8] * mat.r[12]
	               + mat.r[ 9] * mat.r[13]
	               + mat.r[10] * mat.r[14];

	if(NULL == ring)
	{
		ring = StelGeometryBuilder()
			.buildRingTextured(RingParams(radiusMin, radiusMax).resolution(slices, stacks));
	}
	else
	{
		ring->setResolution(slices, stacks);
	}
	ring->setFlipFaces(h >= 0);

	if(info && renderer->isGLSLSupported())
	{
		shader->bind();
		// provides the unprojectedVertex attribute to the shader.
		shader->useUnprojectedPositionAttribute();

		shader->setUniformValue("lightPos"           , Vec3f(0, 0, 0));
		shader->setUniformValue("diffuseLight"       , Vec4f(1, 1, 1, 1));
		shader->setUniformValue("ambientLight"       , Vec4f(0, 0, 0, 1));
		shader->setUniformValue("radius"             , 1.0f);
		shader->setUniformValue("oneMinusOblateness" , 1.0f);

		shader->setUniformValue("info",      info->info);
		shader->setUniformValue("infoCount", info->infoCount);
		shader->setUniformValue("infoSize",  info->infoSize);
		shader->setUniformValue("current",   info->current);

		shader->setUniformValue("isRing", true);

		shader->setUniformValue("ring",        true);
		shader->setUniformValue("outerRadius", static_cast<float>(radiusMax));
		shader->setUniformValue("innerRadius", static_cast<float>(radiusMin));
		shader->setUniformValue("ringS",       0);
	}

	ring->draw(renderer, projector);

	if(info && renderer->isGLSLSupported())
	{
		shader->release();
	}

	renderer->setCulledFaces(CullFace_None);
}

// draw orbital path of Planet
void Planet::drawOrbit(const StelCore* core, StelRenderer* renderer)
{
	if (!orbitFader.getInterstate())
		return;
	if (!re.siderealPeriod)
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameHeliocentricEcliptic);

	renderer->setBlendMode(BlendMode_Alpha);
	renderer->setGlobalColor(orbitColor[0], orbitColor[1], 
	                         orbitColor[2], orbitFader.getInterstate());

	Vec3d onscreen;
	// special case - use current Planet position as center vertex so that draws
	// on it's orbit all the time (since segmented rather than smooth curve)
	Vec3d savePos = orbit[ORBIT_SEGMENTS/2];
	orbit[ORBIT_SEGMENTS/2]=getHeliocentricEclipticPos();
	orbit[ORBIT_SEGMENTS]=orbit[0];
	int nbIter = closeOrbit ? ORBIT_SEGMENTS : ORBIT_SEGMENTS-1;

	if(NULL == orbitVertices)
	{
		orbitVertices = 
			renderer->createVertexBuffer<Vertex2D>(PrimitiveType_LineStrip);
	}

	for (int n=0; n<=nbIter; ++n)
	{
		if (prj->project(orbit[n],onscreen) && (orbitVertices->length()==0 || !prj->intersectViewportDiscontinuity(orbit[n-1], orbit[n])))
		{
			orbitVertices->addVertex(Vertex2D(onscreen[0], onscreen[1]));
		}
		else if(orbitVertices->length() > 0)
		{
			orbitVertices->lock();
			renderer->drawVertexBuffer(orbitVertices);
			orbitVertices->unlock();
			orbitVertices->clear();
		}
	}
	orbit[ORBIT_SEGMENTS/2]=savePos;

	if(orbitVertices->length() > 0)
	{
		orbitVertices->lock();
		renderer->drawVertexBuffer(orbitVertices);
		orbitVertices->unlock();
		orbitVertices->clear();
	}
}

void Planet::update(int deltaTime)
{
	hintFader.update(deltaTime);
	labelsFader.update(deltaTime);
	orbitFader.update(deltaTime);
}

void Planet::setSphereScale(float s)
{
	sphereScale = s;
	if(NULL != unlitSphere) {unlitSphere->setRadius(radius * s);}
	if(NULL != litSphere)   {litSphere->setRadius(radius * s);}
}
