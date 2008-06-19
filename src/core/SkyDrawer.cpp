/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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
 
#include "SkyDrawer.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "ToneReproducer.hpp"
#include "StelTextureMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"

#include <QStringList>
#include <QSettings>
#include <QDebug>

// The 0.025 corresponds to the maximum eye resolution in degree
#define EYE_RESOLUTION (0.25)

SkyDrawer::SkyDrawer(StelCore* acore) : core(acore)
{
	prj = core->getProjection();
	eye = core->getToneReproducer();

	inScale = 1.;
	bortleScaleIndex = 3;
	limitMagnitude = -100.f;
	
	setMaxFov(180.f);
	setMinFov(0.1f);
	update(0);
	
	QSettings* conf = StelApp::getInstance().getSettings();
	initColorTableFromConfigFile(conf);
	
	setTwinkleAmount(conf->value("stars/star_twinkle_amount",0.3).toDouble());
	setFlagTwinkle(conf->value("stars/flag_star_twinkle",true).toBool());
	setFlagPointStar(conf->value("stars/flag_point_star",false).toBool());
	setMaxFov(conf->value("stars/mag_converter_max_fov",60.0).toDouble());
	setMinFov(conf->value("stars/mag_converter_min_fov",0.1).toDouble());
	
	bool ok=true;

	setBortleScale(conf->value("stars/init_bortle_scale",3).toInt(&ok));
	if (!ok)
	{
		conf->setValue("stars/init_bortle_scale",3);
		setBortleScale(3);
		ok = true;
	}
	
	setRelativeStarScale(conf->value("stars/relative_scale",1.0).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("stars/relative_scale",1.0);
		setRelativeStarScale(1.0);
		ok = true;
	}
	
	setAbsoluteStarScale(conf->value("stars/absolute_scale",1.0).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("stars/absolute_scale",1.0);
		setAbsoluteStarScale(1.0);
		ok = true;
	}
	
	// Initialize buffers for use by gl vertex array
	nbPointSources = 0;
	maxPointSources = 1000;
	verticesGrid = new Vec2f[maxPointSources*4];
	colorGrid = new Vec3f[maxPointSources*4];
	textureGrid = new Vec2f[maxPointSources*4];
	for (unsigned int i=0;i<maxPointSources; ++i)
	{
		textureGrid[i*4].set(0,0);
		textureGrid[i*4+1].set(1,0);
		textureGrid[i*4+2].set(1,1);
		textureGrid[i*4+3].set(0,1);
	}
}


SkyDrawer::~SkyDrawer()
{
}

// Init parameters from config file
void SkyDrawer::init()
{
	StelApp::getInstance().getTextureManager().setDefaultParams();
	// Load star texture no mipmap:
	texHalo = StelApp::getInstance().getTextureManager().createTexture("star16x16.png");
}

void SkyDrawer::update(double deltaTime)
{
	float fov = prj->getFov();
	if (fov > max_fov)
	{
		fov = max_fov;
	}
	else
	{
		if (fov < min_fov)
			fov = min_fov;
	}
	
	// This factor is fully arbitrary. It corresponds to the collecting area x exposure time of the instrument
	// It is based on a power law, so that it varies progressively with the FOV to smoothly switch from human 
	// vision to binocculares/telescope. Use a max of 0.7 because after that the atmosphere starts to glow too much!
	float powFactor = std::pow(60./MY_MAX(0.7,fov), 0.8);
	eye->setInputScale(inScale*powFactor);
	
	// Set the fov factor for point source luminance computation
	// the division by powFactor should in principle not be here, but it doesn't look nice if removed
	lnfov_factor = std::log(2025000.f* 60.f*60.f / (fov*fov) / (EYE_RESOLUTION*EYE_RESOLUTION)/powFactor/1.4);
	
	// Precompute
	starLinearScale = std::pow(2.1f*starAbsoluteScaleF, 1.3f/2.f*starRelativeScale);
	
	// Compute the current limit magnitude by dichotomy
	float a=-26.f;
	float b=30.f;
	float rcmag[2];
	limitMagnitude = 0.f;
	int safety=0;
	while (std::fabs(limitMagnitude-a)>0.05)
	{
		computeRCMag(limitMagnitude, rcmag);
		if (rcmag[0]<=0.f)
		{
			float tmp = limitMagnitude;
			limitMagnitude=(a+limitMagnitude)/2;
			b=tmp;
		}
		else
		{
			float tmp = limitMagnitude;
			limitMagnitude=(b+limitMagnitude)/2;
			a=tmp;
		}
		++safety;
		if (safety>20)
		{
			limitMagnitude=-99;
			break;
		}
	}
}

// Compute the log of the luminance for a point source with the given mag for the current FOV
float SkyDrawer::pointSourceMagToLnLuminance(float mag) const
{
	return -0.92103f*(mag + 12.12331f) + lnfov_factor;
}

// Compute the luminance for an extended source with the given surface brightness in Vmag/arcmin^2
float SkyDrawer::surfacebrightnessToLuminance(float sb)
{
	return 2025000.f*std::exp(-0.92103f*(sb + 12.12331f))/(1./60.*1./60.);
}

// Compute RMag and CMag from magnitude for a point source.
bool SkyDrawer::computeRCMag(float mag, float rc_mag[2]) const
{
	rc_mag[0] = eye->adaptLuminanceScaledLn(pointSourceMagToLnLuminance(mag), starRelativeScale*1.3f/2.f);
	rc_mag[0]*=starLinearScale;
	
	// Use now statically min_rmag = 0.5, because higher and too small values look bad
	if (rc_mag[0] < 0.5f)
	{
		rc_mag[0] = rc_mag[1] = 0.f;
		return false;
	}
	
	if (flagPointStar)
	{
		rc_mag[1] = rc_mag[0] * rc_mag[0] / 1.44f;
		if (rc_mag[1] < 0.05f)
		{
			// 0.05f
			rc_mag[0] = rc_mag[1] = 0.f;
			return false;
		}
	}
	else
	{
		// if size of star is too small (blink) we put its size to 1.2 --> no more blink
		// And we compensate the difference of brighteness with cmag
		if (rc_mag[0]<1.2f)
		{
			rc_mag[1] = rc_mag[0] * rc_mag[0] / 1.44f;
			if (rc_mag[1] < 0.07f)
			{
				rc_mag[0] = rc_mag[1] = 0.f;
				return false;
			}
			rc_mag[0] = 1.2f;
		}
		else
		{
			// cmag:
			rc_mag[1] = 1.0f;
			if (rc_mag[0]>8.f)
			{
				rc_mag[0]=8.f+2.f*std::sqrt(1.f+rc_mag[0]-8.f)-2.f;
			}
		}
	}
	return true;
}

void SkyDrawer::preDrawPointSource()
{
	assert(nbPointSources==0);
}

// Finalize the drawing of point sources
void SkyDrawer::postDrawPointSource()
{
	if (nbPointSources==0)
		return;
	
	glDisable(GL_LIGHTING);
	// Blending is really important. Otherwise faint stars in the vicinity of
	// bright star will cause tiny black squares on the bright star, e.g. see Procyon.
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	if (getFlagPointStar())
	{
		glDisable(GL_TEXTURE_2D);
		glPointSize(0.1);
	}
	else
	{
		texHalo->bind();
		glEnable(GL_TEXTURE_2D);
	}
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
	// Load the color components
	glColorPointer(3, GL_FLOAT, 0, colorGrid);
	// Load the vertex array
	glVertexPointer(2, GL_FLOAT, 0, verticesGrid);
	// Load the vertex array
	glTexCoordPointer(2, GL_FLOAT, 0, textureGrid);
	
	// And draw everything at once
	glDrawArrays(GL_QUADS, 0, nbPointSources*4);
		
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	nbPointSources = 0;	
}

// Draw a point source halo.
bool SkyDrawer::drawPointSource(double x, double y, const float rc_mag[2], const Vec3f& color)
{	
	if (rc_mag[0]<=0.f || rc_mag[1]<=0.f)
		return false;
	
	// Random coef for star twinkling
	const float tw = flagStarTwinkle ? (1.f-twinkleAmount*rand()/RAND_MAX) : 1.f;
	
	if (flagPointStar)
	{
		// Draw the star rendered as GLpoint. This may be faster but it is not so nice
		glColor3fv(color*(rc_mag[1]*tw));
		prj->drawPoint2d(x, y);
	}
	else
	{
		// Store the drawing instructions in the vertex arrays
		colorGrid[nbPointSources*4+3] = color*(rc_mag[1]*tw);		
		const double radius = rc_mag[0];
		Vec2f* v = &(verticesGrid[nbPointSources*4]);
		v->set(x-radius,y-radius); ++v;
		v->set(x+radius,y-radius); ++v;
		v->set(x+radius,y+radius); ++v;
		v->set(x-radius,y+radius); ++v;
		++nbPointSources;
		if (nbPointSources>=maxPointSources)
		{
			// Flush the buffer (draw all buffered stars)
			postDrawPointSource();
		}
	}
	return true;
}


// Draw a disk source halo.
bool SkyDrawer::drawDiskSource(double x, double y, double r, float mag, const Vec3f& color)
{
	glDisable(GL_LIGHTING);
	// Blending is really important. Otherwise faint stars in the vicinity of
	// bright star will cause tiny black squares on the bright star, e.g. see Procyon.
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	if (getFlagPointStar())
	{
		glDisable(GL_TEXTURE_2D);
		glPointSize(0.1);
	}
	else
	{
		texHalo->bind();
		glEnable(GL_TEXTURE_2D);
	}
	
	float rc_mag[2];
	if (computeRCMag(mag,rc_mag)==false)
		return false;

	if (getFlagPointStar())
	{
		if (r<=1.f)
		{
			glColor3fv(color*rc_mag[1]);
			prj->drawPoint2d(x, y);
		}
	}
	else
	{
		if (r<1.f) r=1.f;
		rc_mag[1] *= 0.5*rc_mag[0]/(r*r*r);
		if (rc_mag[1]>1.f) rc_mag[1] = 1.f;

		if (rc_mag[0]<r)
		{
			rc_mag[1]*= (rc_mag[0]/r);
			rc_mag[0] = r;
		}
		glColor3fv(color*rc_mag[1]);
		prj->drawSprite2dMode(x, y, rc_mag[0]*2);
	}
	return true;
}


void SkyDrawer::preDrawSky3dModel(double illuminatedArea, float mag, bool lighting)
{
	// Set the main source of light to be the sun
	const Vec3d sun_pos = core->getNavigation()->get_helio_to_eye_mat()*Vec3d(0,0,0);
	glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(sun_pos[0],sun_pos[1],sun_pos[2],1.f));
	
	glEnable(GL_CULL_FACE);
	
	float surfLuminance = surfacebrightnessToLuminance(mag + std::log10(illuminatedArea)/2.5f);
	float aLum = eye->adaptLuminanceScaled(surfLuminance/1000);
	//reportLuminanceInFov(surfLuminance/1000, illuminatedArea/(60.*60.)*(core->getProjection()->getPixelPerRadAtCenter()* core->getProjection()->getPixelPerRadAtCenter()*M_PI/180.*M_PI/180.));
	
	if (aLum<=1.f)
	{
		tempRCMag[0]=0.f;
		tempRCMag[1]=0.f;
	}
	else
	{
		tempRCMag[0] = std::sqrt(aLum-1.f);
		tempRCMag[1] = 1.f;
	}
	
	if (lighting)
	{
		glEnable(GL_LIGHTING);
		const float diffuse[4] = {aLum,aLum,aLum,1};
		glLightfv(GL_LIGHT0,GL_DIFFUSE, diffuse);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glColor3fv(Vec3f(1.f,1.f,1.f));
	}
}

// Terminate drawing of a 3D model, draw the halo
void SkyDrawer::postDrawSky3dModel(double x, double y, float mag, const Vec3f& color)
{
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	
	// Now draw the halo according the object brightness
// 	preDrawPointSource();
// 	bool save = getFlagTwinkle();
// 	setFlagTwinkle(false);
	//drawPointSource(x,y,tempRCMag,color);
// 	setFlagTwinkle(save);
// 	postDrawPointSource();
}

// Report that an object of luminance lum with an on-screen area of area pixels is currently displayed
void SkyDrawer::reportLuminanceInFov(double lum, float area)
{
	float fact = (area>=100.f) ? 1.f : area/100.f;
	fact*=fact*fact;
	if ((fact*lum) > maxLum)
		maxLum = fact*lum;
}

void SkyDrawer::preDraw()
{
	eye->setWorldAdaptationLuminance(maxLum);
	// Re-initialize for next stage
	maxLum = 0;
}


// Set the parameters so that the stars disapear at about the limit given by the bortle scale
// See http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
void SkyDrawer::setBortleScale(int bIndex)
{
	// Associate the Bortle index (1 to 9) to inScale value
	if (bIndex<1)
	{
		qWarning() << "WARING: Bortle scale index range is [1;9], given" << bIndex;
		bIndex = 1;
	}
	if (bIndex>9)
	{
		qWarning() << "WARING: Bortle scale index range is [1;9], given" << bIndex;
		bIndex = 19;
	}
	
	bortleScaleIndex = bIndex;
	
	// These value have been calibrated by hand, looking at the faintest star in stellarium at 60 deg FOV
	// They should roughly match the scale described at http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
	static const float bortleToInScale[9] = {6.40, 4.90, 3.28, 2.12, 1.51, 1.08, 0.91, 0.67, 0.52};
	setInputScale(bortleToInScale[bIndex-1]);
}

Vec3f SkyDrawer::colorTable[128] = {
	Vec3f(0.587877,0.755546,1.000000),
	Vec3f(0.609856,0.750638,1.000000),
	Vec3f(0.624467,0.760192,1.000000),
	Vec3f(0.639299,0.769855,1.000000),
	Vec3f(0.654376,0.779633,1.000000),
	Vec3f(0.669710,0.789527,1.000000),
	Vec3f(0.685325,0.799546,1.000000),
	Vec3f(0.701229,0.809688,1.000000),
	Vec3f(0.717450,0.819968,1.000000),
	Vec3f(0.733991,0.830383,1.000000),
	Vec3f(0.750857,0.840932,1.000000),
	Vec3f(0.768091,0.851637,1.000000),
	Vec3f(0.785664,0.862478,1.000000),
	Vec3f(0.803625,0.873482,1.000000),
	Vec3f(0.821969,0.884643,1.000000),
	Vec3f(0.840709,0.895965,1.000000),
	Vec3f(0.859873,0.907464,1.000000),
	Vec3f(0.879449,0.919128,1.000000),
	Vec3f(0.899436,0.930956,1.000000),
	Vec3f(0.919907,0.942988,1.000000),
	Vec3f(0.940830,0.955203,1.000000),
	Vec3f(0.962231,0.967612,1.000000),
	Vec3f(0.984110,0.980215,1.000000),
	Vec3f(1.000000,0.986617,0.993561),
	Vec3f(1.000000,0.977266,0.971387),
	Vec3f(1.000000,0.967997,0.949602),
	Vec3f(1.000000,0.958816,0.928210),
	Vec3f(1.000000,0.949714,0.907186),
	Vec3f(1.000000,0.940708,0.886561),
	Vec3f(1.000000,0.931787,0.866303),
	Vec3f(1.000000,0.922929,0.846357),
	Vec3f(1.000000,0.914163,0.826784),
	Vec3f(1.000000,0.905497,0.807593),
	Vec3f(1.000000,0.896884,0.788676),
	Vec3f(1.000000,0.888389,0.770168),
	Vec3f(1.000000,0.879953,0.751936),
	Vec3f(1.000000,0.871582,0.733989),
	Vec3f(1.000000,0.863309,0.716392),
	Vec3f(1.000000,0.855110,0.699088),
	Vec3f(1.000000,0.846985,0.682070),
	Vec3f(1.000000,0.838928,0.665326),
	Vec3f(1.000000,0.830965,0.648902),
	Vec3f(1.000000,0.823056,0.632710),
	Vec3f(1.000000,0.815254,0.616856),
	Vec3f(1.000000,0.807515,0.601243),
	Vec3f(1.000000,0.799820,0.585831),
	Vec3f(1.000000,0.792222,0.570724),
	Vec3f(1.000000,0.784675,0.555822),
	Vec3f(1.000000,0.777212,0.541190),
	Vec3f(1.000000,0.769821,0.526797),
	Vec3f(1.000000,0.762496,0.512628),
	Vec3f(1.000000,0.755229,0.498664),
	Vec3f(1.000000,0.748032,0.484926),
	Vec3f(1.000000,0.740897,0.471392),
	Vec3f(1.000000,0.733811,0.458036),
	Vec3f(1.000000,0.726810,0.444919),
	Vec3f(1.000000,0.719856,0.431970),
	Vec3f(1.000000,0.712983,0.419247),
	Vec3f(1.000000,0.706154,0.406675),
	Vec3f(1.000000,0.699375,0.394265),
	Vec3f(1.000000,0.692681,0.382075),
	Vec3f(1.000000,0.686003,0.369976),
	Vec3f(1.000000,0.679428,0.358120),
	Vec3f(1.000000,0.672882,0.346373),
	Vec3f(1.000000,0.666372,0.334740),
	Vec3f(1.000000,0.659933,0.323281),
	Vec3f(1.000000,0.653572,0.312004),
	Vec3f(1.000000,0.647237,0.300812),
	Vec3f(1.000000,0.640934,0.289709),
	Vec3f(1.000000,0.634698,0.278755),
	Vec3f(1.000000,0.628536,0.267954),
	Vec3f(1.000000,0.622390,0.257200),
	Vec3f(1.000000,0.616298,0.246551),
	Vec3f(1.000000,0.610230,0.235952),
	Vec3f(1.000000,0.604259,0.225522),
	Vec3f(1.000000,0.598288,0.215083),
	Vec3f(1.000000,0.592391,0.204756),
	Vec3f(1.000000,0.586501,0.194416),
	Vec3f(1.000000,0.580657,0.184120),
	Vec3f(1.000000,0.574901,0.173930),
	Vec3f(1.000000,0.569127,0.163645),
	Vec3f(1.000000,0.563449,0.153455),
	Vec3f(1.000000,0.557758,0.143147),
	Vec3f(1.000000,0.552134,0.132843),
	Vec3f(1.000000,0.546541,0.122458),
	Vec3f(1.000000,0.540984,0.111966),
	Vec3f(1.000000,0.535464,0.101340),
	Vec3f(1.000000,0.529985,0.090543),
	Vec3f(1.000000,0.524551,0.079292),
	Vec3f(1.000000,0.519122,0.068489),
	Vec3f(1.000000,0.513743,0.058236),
	Vec3f(1.000000,0.508417,0.048515),
	Vec3f(1.000000,0.503104,0.039232),
	Vec3f(1.000000,0.497805,0.030373),
	Vec3f(1.000000,0.492557,0.021982),
	Vec3f(1.000000,0.487338,0.014007),
	Vec3f(1.000000,0.482141,0.006417),
	Vec3f(1.000000,0.477114,0.000000),
	Vec3f(1.000000,0.473268,0.000000),
	Vec3f(1.000000,0.469419,0.000000),
	Vec3f(1.000000,0.465552,0.000000),
	Vec3f(1.000000,0.461707,0.000000),
	Vec3f(1.000000,0.457846,0.000000),
	Vec3f(1.000000,0.453993,0.000000),
	Vec3f(1.000000,0.450129,0.000000),
	Vec3f(1.000000,0.446276,0.000000),
	Vec3f(1.000000,0.442415,0.000000),
	Vec3f(1.000000,0.438549,0.000000),
	Vec3f(1.000000,0.434702,0.000000),
	Vec3f(1.000000,0.430853,0.000000),
	Vec3f(1.000000,0.426981,0.000000),
	Vec3f(1.000000,0.423134,0.000000),
	Vec3f(1.000000,0.419268,0.000000),
	Vec3f(1.000000,0.415431,0.000000),
	Vec3f(1.000000,0.411577,0.000000),
	Vec3f(1.000000,0.407733,0.000000),
	Vec3f(1.000000,0.403874,0.000000),
	Vec3f(1.000000,0.400029,0.000000),
	Vec3f(1.000000,0.396172,0.000000),
	Vec3f(1.000000,0.392331,0.000000),
	Vec3f(1.000000,0.388509,0.000000),
	Vec3f(1.000000,0.384653,0.000000),
	Vec3f(1.000000,0.380818,0.000000),
	Vec3f(1.000000,0.376979,0.000000),
	Vec3f(1.000000,0.373166,0.000000),
	Vec3f(1.000000,0.369322,0.000000),
	Vec3f(1.000000,0.365506,0.000000),
	Vec3f(1.000000,0.361692,0.000000),
};


static double Gamma(double gamma,double x)
{
	return ((x<=0.0) ? 0.0 : exp(gamma*log(x)));
}

static Vec3f Gamma(double gamma,const Vec3f &x)
{
	return Vec3f(Gamma(gamma,x[0]),Gamma(gamma,x[1]),Gamma(gamma,x[2]));
}

// Load B-V conversion parameters from config file
void SkyDrawer::initColorTableFromConfigFile(QSettings* conf) 
{
	std::map<float,Vec3f> color_map;
	for (float b_v=-0.5f;b_v<=4.0f;b_v+=0.01) 
	{
		char entry[256];
		sprintf(entry,"bv_color_%+5.2f",b_v);
		const QStringList s(conf->value(QString("stars/") + entry).toStringList());
		if (!s.isEmpty())
		{
			const Vec3f c(StelUtils::str_to_vec3f(s));
			color_map[b_v] = Gamma(1/0.45,c);
		}
	}

	if (color_map.size() > 1) 
	{
		for (int i=0;i<128;i++) 
		{
			const float b_v = SkyDrawer::indexToBV(i);
			std::map<float,Vec3f>::const_iterator greater(color_map.upper_bound(b_v));
			if (greater == color_map.begin()) 
			{
				colorTable[i] = greater->second;
			} 
			else 
			{
				std::map<float,Vec3f>::const_iterator less(greater);--less;
				if (greater == color_map.end()) 
				{
					colorTable[i] = less->second;
				} 
				else 
				{
					colorTable[i] = Gamma(0.45, ((b_v-less->first)*greater->second + (greater->first-b_v)*less->second) *(1.f/(greater->first-less->first)));
				}
			}
		}
	}
	
	// because the star texture is not fully white we need to add a factor here to avoid to dark colors == too saturated
	for (int i=0;i<128;i++) 
	{
		colorTable[i] *= 1.4;
		colorTable[i][0] *=1./1.3;
		colorTable[i][1] *=1./1.2;
	}
}
