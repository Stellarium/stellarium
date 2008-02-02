#include "AstroImage.hpp"
#include "Projector.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"

AstroImage::AstroImage()
{
}

AstroImage::AstroImage(STextureSP atex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : tex(atex), poly(StelGeom::ConvexPolygon(v3, v2, v1, v0))
{
}

AstroImage::AstroImage(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : poly(StelGeom::ConvexPolygon(v3, v2, v1, v0))
{
}

AstroImage::~AstroImage()
{
}

void AstroImage::draw(StelCore* core)
{
	if (!tex) return;
	
	if (!tex->bind())
		return;
	
	Projector* prj = core->getProjection();
	
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(1.0,1.0,1.0,1.0);
	
	prj->setCurrentFrame(Projector::FRAME_J2000);
	
	Vec3d win;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2dv(tex->getCoordinates()[0]);
		prj->project(poly[2],win); glVertex3dv(win);
		glTexCoord2dv(tex->getCoordinates()[1]);
		prj->project(poly[3],win); glVertex3dv(win);
		glTexCoord2dv(tex->getCoordinates()[2]);
		prj->project(poly[1],win); glVertex3dv(win);
		glTexCoord2dv(tex->getCoordinates()[3]);
		prj->project(poly[0],win); glVertex3dv(win);
    glEnd();
}
