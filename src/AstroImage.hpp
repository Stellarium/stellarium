#ifndef ASTROIMAGE_H_
#define ASTROIMAGE_H_

#include "STexture.hpp"
#include "SphereGeometry.h"

class Projector;
class Navigator;
class ToneReproducer;

//! Base class for any astro image with a fixed position
class AstroImage
{
public:
	AstroImage();
	AstroImage(STexture* tex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3);
	AstroImage(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3);
	virtual ~AstroImage();
	
	//! Draw the image on the screen. Assume that we are in Orthographic projection mode.
	void draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	
	//! Return the matching ConvexPolygon
	const ConvexPolygon& getPolygon(void) const {return poly;}
	
	const STexture* getTexture() const {return tex;}
	void setTexture(STexture* atex) {if (tex) delete tex; tex=atex;}

	bool getLoadStatus() const {return loadStatus;}

	const std::string& getFullPath() const {return fullPath;}
private:
	friend class FitsAstroImage;
	
	// The texture matching the positions
	STexture* tex;
	
	// Position of the 4 corners of the texture in sky coordinates
	const ConvexPolygon poly;
	
	// Is set to true once the texture loading is over. If at this point tex==NULL, there has been an error
	bool loadStatus;
	
	// Full path to the image if a file exists
	std::string fullPath;
};

#endif /*ASTROIMAGE_H_*/
