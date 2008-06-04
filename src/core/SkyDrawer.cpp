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

SkyDrawer::SkyDrawer(Projector* aprj, ToneReproducer* aeye) : prj(aprj), eye(aeye)
{
	// DEBUG
	outScale = 112500.;
	inScale = 1.9;
	
	bortleScaleIndex = 3;
			
	setMaxFov(180.f);
	setMinFov(0.1f);
	setMagShift(0.f);
	setMaxMag(30.f);
	update(0);
	
	QSettings* conf = StelApp::getInstance().getSettings();
	initColorTableFromConfigFile(conf);
	
	setScale(conf->value("stars/star_scale",1.1).toDouble());
	setMagScale(conf->value("stars/star_mag_scale",1.3).toDouble());
	setTwinkleAmount(conf->value("stars/star_twinkle_amount",0.3).toDouble());
	setFlagTwinkle(conf->value("stars/flag_star_twinkle",true).toBool());
	setFlagPointStar(conf->value("stars/flag_point_star",false).toBool());
	setMaxFov(conf->value("stars/mag_converter_max_fov",60.0).toDouble());
	setMinFov(conf->value("stars/mag_converter_min_fov",0.1).toDouble());
	
	bool ok = true;
	setMagShift(conf->value("stars/mag_converter_mag_shift",0.0).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("stars/mag_converter_mag_shift",0.0);
		setMagShift(0.0);
		ok = true;
	}
	setMaxMag(conf->value("stars/mag_converter_max_mag",30.0).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("stars/mag_converter_max_mag",30.0);
		setMaxMag(30.0);
		ok = true;
	}
	
	setBortleScale(conf->value("stars/init_bortle_scale",3).toInt(&ok));
	if (!ok)
	{
		conf->setValue("stars/init_bortle_scale",3);
		setBortleScale(3);
		ok = true;
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
	
	// I am not quite sure how to interpret this one..
	eye->setOutputScale(outScale);
	
	// This factor is fully arbitrary. It corresponds to the collecting area x exposure time of the instrument
	// It is based on a power law, so that it varies progressively with the FOV to smoothly switch from human 
	// vision to binocculares/telescope. Use a max of 0.7 because after that the atmosphere starts to glow too much!
	float powFactor = std::pow(60./MY_MAX(0.7,fov), 0.8);
	eye->setInputScale(inScale*powFactor); // *
	
	// Set the fov factor for point source luminance computation
	// the division by powFactor should in principle not be here, but it doesn't look nice if removed
	lnfov_factor = std::log(60.f*60.f / (fov*fov) / (EYE_RESOLUTION*EYE_RESOLUTION)/powFactor/1.4);
}

// Compute the log of the luminance for a point source with the given mag for the current FOV
float SkyDrawer::pointSourceMagToLnLuminance(float mag) const
{
	return -0.92103f*(mag + mag_shift + 12.12331f) + lnfov_factor;
}

// Compute the luminance for an extended source with the given surface brightness in Vmag/arcmin^2
float SkyDrawer::surfacebrightnessToLuminance(float sb) const
{
	return std::exp(-0.92103f*(sb + mag_shift + 12.12331f))/(1./60.*1./60.);
}

// Compute RMag and CMag from magnitude for a point source.
int SkyDrawer::computeRCMag(float mag, float rc_mag[2]) const
{
	// Real world magnitude limit (independent of FOV)
	if (mag > max_mag)
	{
		rc_mag[0] = rc_mag[1] = 0.f;
		return -1;
	}
	
	if (mag<-10)
	{
		// Avoid computing for too bright objects
		//assert(0);
		rc_mag[0] = rc_mag[1] = 0.f;
		return -1;
	}

    // rmag:
	//rc_mag[0] = std::sqrt(eye->adaptLuminanceScaled(std::exp(-0.92103f*(mag + mag_shift + 12.12331f)) * fov_factor)) * 300.f;
	rc_mag[0] = eye->adaptLuminanceScaledLn(pointSourceMagToLnLuminance(mag), 1.3f/2.);
	const static float fact = std::pow(2.1, 1.3/2.);
	rc_mag[0]*=fact;
	
	// Use now statically min_rmag = 0.6, because higher and too small values look bad
	if (rc_mag[0] < 0.6f)
	{
		rc_mag[0] = rc_mag[1] = 0.f;
		return -1;
	}
	
	if (flagPointStar)
	{
		if (rc_mag[0] * starScale < 0.1f)
		{
			// 0.05f
			rc_mag[0] = rc_mag[1] = 0.f;
			return -1;
		}
		rc_mag[1] = rc_mag[0] * rc_mag[0] / 1.44f;
		if (rc_mag[1] * starMagScale < 0.1f)
		{
			// 0.05f
			rc_mag[0] = rc_mag[1] = 0.f;
			return -1;
		}
    	// Global scaling
		rc_mag[1] *= starMagScale;
	}
	else
	{
		// if size of star is too small (blink) we put its size to 1.2 --> no more blink
		// And we compensate the difference of brighteness with cmag
		if (rc_mag[0]<1.2f)
		{
			if ((rc_mag[0] * starScale) < 0.1f)
			{
				rc_mag[0] = rc_mag[1] = 0.f;
				return -1;
			}
			rc_mag[1] = rc_mag[0] * rc_mag[0] / 1.44f;
			if (rc_mag[1] * starMagScale < 0.1f)
			{
				rc_mag[0] = rc_mag[1] = 0.f;
				return -1;
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
		// Global scaling
		rc_mag[0] *= starScale;
		rc_mag[1] *= starMagScale;
	}
	return 0;
}

void SkyDrawer::prepareDraw()
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
	
}

// Draw a disk source halo.
bool SkyDrawer::drawDiskSource(double x, double y, double r, float mag, const Vec3f& color)
{
	float rc_mag[2];
	if (computeRCMag(mag,rc_mag) < 0)
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

// Draw a point source halo.
int SkyDrawer::drawPointSource(double x, double y, const float rc_mag[2], unsigned int b_v)
{	
	// qDebug() << "StarMgr::drawStar: " << XY[0] << '/' << XY[1] << ", " << rmag;
	// assert(rc_mag[1]>= 0.f);
	if (rc_mag[0]<=0.f || rc_mag[1]<=0.f)
		return -1;
	
	const Vec3f& color = colorTable[b_v];
	
	// Random coef for star twinkling
	const float tw = flagStarTwinkle ? (1.-twinkleAmount*rand()/RAND_MAX) : 1.0;
	glColor3fv(color*(rc_mag[1]*tw));
	
	if (flagPointStar)
	{
		// Draw the star rendered as GLpoint. This may be faster but it is not so nice
		prj->drawPoint2d(x, y);
	}
	else
	{
		prj->drawSprite2dMode(x, y, 2.f*rc_mag[0]);
	}
	return 0;
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
	setInScale(bortleToInScale[bIndex-1]);
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
