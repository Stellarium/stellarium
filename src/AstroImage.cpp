#include "AstroImage.hpp"
#include "Projector.hpp"
#include "STexture.hpp"

AstroImage::AstroImage()
{
}

AstroImage::AstroImage(STextureSP atex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : tex(atex), poly(ConvexPolygon(v3, v2, v1, v0))
{
}

AstroImage::AstroImage(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : poly(ConvexPolygon(v3, v2, v1, v0))
{
}

AstroImage::~AstroImage()
{
}

void AstroImage::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye)
{
	if (!tex) return;
	
	glEnable(GL_TEXTURE_2D);

	glColor4f(1.0,1.0,1.0,1.0);

	tex->bind();
	
	prj->setCurrentFrame(Projector::FRAME_J2000);
	
	Vec3d win;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2dv(tex->texCoordinates[0]);
		prj->project(poly.getVertex(2),win); glVertex3dv(win);
		glTexCoord2dv(tex->texCoordinates[1]);
		prj->project(poly.getVertex(3),win); glVertex3dv(win);
        glTexCoord2dv(tex->texCoordinates[2]);
		prj->project(poly.getVertex(1),win); glVertex3dv(win);
        glTexCoord2dv(tex->texCoordinates[3]);
		prj->project(poly.getVertex(0),win); glVertex3dv(win);
    glEnd();
}
