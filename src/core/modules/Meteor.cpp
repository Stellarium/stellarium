/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

// This is an ad hoc meteor model
// Could use a simple ablation physics model in the future

/*
NOTE: Here the radiant is always along the ecliptic at the apex of the Earth's way.
In reality, individual meteor streams have varying velocity vectors and therefore radiants
which are generally not at the apex of the Earth's way, such as the Perseids shower.
*/

// Improved realism and efficiency 2004-12

#include <cstdlib>
#include "Meteor.hpp"
#include "StelCore.hpp"

#include "renderer/StelRenderer.hpp"
#include "StelToneReproducer.hpp"
#include "StelMovementMgr.hpp"

Meteor::Meteor(const StelCore* core, double v)
	: vertexBuffer(NULL)
{
	const StelToneReproducer* eye = core->getToneReproducer();
	
	//  velocity = 11+(double)rand()/((double)RAND_MAX+1)*v;  // abs range 11-72 km/s
	velocity=v;
	
	maxMag = 1;
	
	// determine meteor model view matrix (want z in dir of travel of earth, z=0 at center of earth)
	// meteor life is so short, no need to recalculate
	double equ_rotation; // rotation needed to align with path of earth
	Vec3d sun_dir = core->heliocentricEclipticToEquinoxEqu( Vec3d(0,0,0) );
	
	Mat4d tmat = Mat4d::xrotation(-23.45f*M_PI/180.f);  // ecliptical tilt
	sun_dir.transfo4d(tmat);  // convert to ecliptical coordinates
	sun_dir.normalize();
	equ_rotation = acos( sun_dir.dot( Vec3d(1,0,0) ) );
	if( sun_dir[1] < 0 ) equ_rotation = 2*M_PI - equ_rotation;
	
	equ_rotation -= M_PI_2;
	
	mmat = Mat4d::xrotation(23.45f*M_PI/180.f) * Mat4d::zrotation(equ_rotation) * Mat4d::yrotation(M_PI_2);
	
	
	// select random trajectory using polar coordinates in XY plane, centered on observer
	xydistance = (double)rand()/((double)RAND_MAX+1)*(VISIBLE_RADIUS);
	double angle = (double)rand()/((double)RAND_MAX+1)*2*M_PI;
	
	// find observer position in meteor coordinate system
	obs = core->altAzToEquinoxEqu(Vec3d(0,0,EARTH_RADIUS));
	obs.transfo4d(mmat.transpose());
	
	// set meteor start x,y
	posInternal[0] = posTrain[0] = position[0] = xydistance*cos(angle) +obs[0];
	posInternal[1] = posTrain[1] = position[1] = xydistance*sin(angle) +obs[1];
	
	// determine life of meteor (start and end z value based on atmosphere burn altitudes)
	
	// D is distance from center of earth
	double D = sqrt( position[0]*position[0] + position[1]*position[1] );
	
	if( D > EARTH_RADIUS+HIGH_ALTITUDE ) 
	{
		// won't be visible
		alive = 0;
		return;
	}
	
	startH = sqrt( pow(EARTH_RADIUS+HIGH_ALTITUDE,2) - D*D);
	
	// determine end of burn point, and nearest point to observer for distance mag calculation
	// mag should be max at nearest point still burning
	if( D > EARTH_RADIUS+LOW_ALTITUDE ) 
	{
		endH = -startH;  // earth grazing
		minDist = xydistance;
	} 
	else 
	{
		endH = sqrt( pow(EARTH_RADIUS+LOW_ALTITUDE,2) - D*D);
		minDist = sqrt( xydistance*xydistance + pow( endH - obs[2], 2) );
	}
	
	if(minDist > VISIBLE_RADIUS ) 
	{
		// on average, not visible (although if were zoomed ...)
		alive = 0;
		return;
	}
	  
	/* experiment
	// limit lifetime to 0.5-3.0 sec
	double tmp_h = startH - velocity * (0.5 + (double)rand()/((double)RAND_MAX+1) * 2.5);
	if( tmp_h > endH ) {
	  endH = tmp_h;
	}
	*/
	
	posTrain[2] = position[2] = startH;
	
	//  qDebug("New meteor: %f %f s:%f e:%f v:%f\n", position[0], position[1], startH, endH, velocity);
	
	alive = 1;
	train = 1;
	
	// Determine drawing color given magnitude and eye 
	// (won't be visible during daylight)
	
	// *** color varies somewhat based on velocity, plus atmosphere reddening
	
	// determine intensity
	float Mag1 = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;
	float Mag2 = (double)rand()/((double)RAND_MAX+1)*6.75f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;
	
	mag = (5. + Mag) / 256.0;
	if (mag>250) mag = mag - 256;
	
	float term1 = std::exp(-0.92103f*(mag + 12.12331f)) * 108064.73f;
	
	float cmag=1.f;
	float rmag;
	
	// Compute the equivalent star luminance for a 5 arc min circle and convert it
	// in function of the eye adaptation
	rmag = eye->adaptLuminanceScaled(term1);
	rmag = rmag/powf(core->getMovementMgr()->getCurrentFov(),0.85f)*500.f;
	
	// if size of star is too small (blink) we put its size to 1.2 --> no more blink
	// And we compensate the difference of brighteness with cmag
	if (rmag<1.2f) 
	{
		cmag=rmag*rmag/1.44f;
	}
	
	mag = cmag;  // assumes white
	
	// most visible meteors are under about 180km distant
	// scale max mag down if outside this range 
	float scale = 1;
	if(minDist!=0) scale = 180*180/(minDist*minDist);
	if( scale < 1 ) mag *= scale;
	
}

Meteor::~Meteor()
{   
	if(NULL != vertexBuffer)
	{
		delete vertexBuffer;
	}
}

// returns true if alive
bool Meteor::update(double deltaTime)
{
	if(!alive) return(0);

	if(position[2] < endH) 
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out

		mag -= maxMag * deltaTime/500.0f;
		if(mag < 0) alive=0;  // no longer visible
	}

	// *** would need time direction multiplier to allow reverse time replay
	position[2] = position[2] - velocity*deltaTime/1000.0f;

	// train doesn't extend beyond start of burn
	if(position[2] + velocity*0.5f > startH) 
	{
		posTrain[2] = startH ;
	} 
	else 
	{
		posTrain[2] -= velocity*deltaTime/1000.0f;
	}

	//qDebug("meteor position: %f delta_t %d\n", position[2], deltaTime);

	// determine visual magnitude based on distance to observer
	double dist = sqrt(xydistance*xydistance + pow( position[2]-obs[2], 2));

	if(dist == 0) dist = .01;  // just to be cautious (meteor hits observer!)

	distMultiplier = minDist*minDist / (dist*dist);

	return(alive);
}

// Assumes that we are in local frame
void Meteor::draw(const StelCore* core, StelProjectorP projector, StelRenderer* renderer)
{
	if (!alive) {return;}

	Vec3d spos = position;
	Vec3d epos = posTrain;

	// convert to equ
	spos.transfo4d(mmat);
	epos.transfo4d(mmat);

	// convert to local and correct for earth radius [since equ and local coordinates in stellarium use same 0 point!] 
	spos = core->equinoxEquToAltAz(spos);
	epos = core->equinoxEquToAltAz(epos);
	spos[2] -= EARTH_RADIUS;
	epos[2] -= EARTH_RADIUS;
	// 1216 is to scale down under 1 for desktop version
	spos /= 1216;
	epos /= 1216;

	//  qDebug("[%f %f %f] (%d, %d) (%d, %d)\n", position[0], position[1], position[2], (int)start[0], (int)start[1], (int)end[0], (int)end[1]);
	
	if(NULL == vertexBuffer)
	{
		// This well need to be rewritten if the train data member gets changed after construction.
		vertexBuffer = 
			renderer->createVertexBuffer<Vertex>(train ? PrimitiveType_LineStrip : PrimitiveType_Points);
	}

	if (train)
	{
		// connect this point with last drawn point
		const double tmag = mag * distMultiplier;

		// compute an intermediate point so can curve slightly along projection distortions
		Vec3d posi = posInternal; 
		posi[2]    = position[2] + (posTrain[2] - position[2]) / 2;
		posi.transfo4d(mmat);
		posi       = core->equinoxEquToAltAz( posi );
		posi[2]   -= EARTH_RADIUS;
		posi      /= 1216;

		vertexBuffer->unlock();
		vertexBuffer->clear();
		vertexBuffer->addVertex(Vertex(epos, Vec4f(0.0f, 0.0f, 0.0f, 0.0f)));
		vertexBuffer->addVertex(Vertex(posi, Vec4f(1.0f, 1.0f, 1.0f, tmag * 0.5)));
		vertexBuffer->addVertex(Vertex(spos, Vec4f(1.0f, 1.0f, 1.0f, tmag)));
		vertexBuffer->lock();
		renderer->drawVertexBuffer(vertexBuffer, NULL, projector);
	}
	else
	{
		vertexBuffer->unlock();
		vertexBuffer->clear();
		// A line with two identical points ends up being a point.
		vertexBuffer->addVertex(Vertex(spos, Vec4f(1.0f, 1.0f, 1.0f, 1.0f)));
		vertexBuffer->lock();
		renderer->drawVertexBuffer(vertexBuffer, NULL, projector);
	}
}

bool Meteor::isAlive(void)
{
	return(alive);
}
