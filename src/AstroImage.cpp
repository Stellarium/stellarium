#include "AstroImage.hpp"
#include "Projector.hpp"

AstroImage::AstroImage() : tex(NULL)
{
}

AstroImage::AstroImage(STexture* tex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : tex(tex), poly(ConvexPolygon(v3, v2, v1, v0))
{
}

AstroImage::AstroImage(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : tex(NULL), poly(ConvexPolygon(v3, v2, v1, v0))
{
}

AstroImage::~AstroImage()
{
	if (tex) delete tex;
	tex = NULL;
}

void AstroImage::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye)
{
	if (!tex) return;
	
	glEnable(GL_TEXTURE_2D);

	glColor4f(1.0,1.0,1.0,1.0);

	tex->bind();

	Vec3d win;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2dv(tex->texCoordinates[0]);
		prj->project_j2000(poly.getVertex(2),win); glVertex3dv(win);
		glTexCoord2dv(tex->texCoordinates[1]);
		prj->project_j2000(poly.getVertex(3),win); glVertex3dv(win);
        glTexCoord2dv(tex->texCoordinates[2]);
		prj->project_j2000(poly.getVertex(1),win); glVertex3dv(win);
        glTexCoord2dv(tex->texCoordinates[3]);
		prj->project_j2000(poly.getVertex(0),win); glVertex3dv(win);
    glEnd();
}
