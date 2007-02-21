/*
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

#ifndef ASTROIMAGE_H_
#define ASTROIMAGE_H_

#include "STextureTypes.hpp"
#include "SphereGeometry.hpp"

class Projector;
class Navigator;
class ToneReproducer;

//! Base class for any astro image with a fixed position
class AstroImage
{
public:
	AstroImage();
	AstroImage(STextureSP tex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3);
	AstroImage(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3);
	virtual ~AstroImage();
	
	//! Draw the image on the screen. Assume that we are in Orthographic projection mode.
	void draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	
	//! Return the matching ConvexPolygon
	const ConvexPolygon& getPolygon(void) const {return poly;}
	
	const STextureSP getTexture() const {return tex;}
	void setTexture(STextureSP atex) {tex=atex;}

	bool getLoadStatus() const {return loadStatus;}

	const std::string& getFullPath() const {return fullPath;}
private:
	friend class FitsAstroImage;
	
	// The texture matching the positions
	STextureSP tex;
	
	// Position of the 4 corners of the texture in sky coordinates
	const ConvexPolygon poly;
	
	// Is set to true once the texture loading is over. If at this point tex==NULL, there has been an error
	bool loadStatus;
	
	// Full path to the image if a file exists
	std::string fullPath;
};

#endif /*ASTROIMAGE_H_*/
