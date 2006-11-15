#include "AstroImage.h"
#include "projector.h"

AstroImage::AstroImage() : tex(NULL)
{
}

AstroImage::AstroImage(STexture* tex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : tex(tex)
{
	vertex[0] = v0;
	vertex[1] = v1;
	vertex[2] = v2;
	vertex[3] = v3;
}

AstroImage::~AstroImage()
{
	if (tex) delete tex;
	tex = NULL;
}

void AstroImage::draw(Projector *prj, const Navigator *nav, ToneReproductor *eye)
{
	if (!tex) return;
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glColor4f(1.0,1.0,1.0,1.0);

	tex->bind();

	Vec3d win;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2dv(tex->texCoordinates[0]);
		prj->project_j2000(vertex[1],win); glVertex3dv(win);
		glTexCoord2dv(tex->texCoordinates[1]);
		prj->project_j2000(vertex[0],win); glVertex3dv(win);
        glTexCoord2dv(tex->texCoordinates[2]);
		prj->project_j2000(vertex[2],win); glVertex3dv(win);
        glTexCoord2dv(tex->texCoordinates[3]);
		prj->project_j2000(vertex[3],win); glVertex3dv(win);
    glEnd();
}
