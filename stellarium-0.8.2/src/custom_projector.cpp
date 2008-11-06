/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#include <cstdio>
#include <iostream>
#include "custom_projector.h"

//kornyakov: for 3d-objects
#include <GL\glut.h>
#include "texture.h"
#include "3dsloader.h"


#define texturepath "F:\\MyStudy\\3dsmodels\\textures\\"

char datapath[1024];


CustomProjector::CustomProjector(const Vec4i& viewport, double _fov)
                :Projector(viewport, _fov)
{
	mat_projection.set(1., 0., 0., 0.,
							0., 1., 0., 0.,
							0., 0., -1, 0.,
							0., 0., 0., 1.);				
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of glOrtho
void CustomProjector::init_project_matrix(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
	glMatrixMode(GL_MODELVIEW);
}

// Override glVertex3f
// Here is the main trick for texturing in fisheye mode : The trick is to compute the
// new coordinate in orthographic projection which will simulate the fisheye projection.
void CustomProjector::sVertex3(double x, double y, double z, const Mat4d& mat) const
{
	Vec3d win;
	Vec3d v(x,y,z);
	project_custom(v, win, mat);

	// Can be optimized by avoiding matrix inversion if it's always the same
	gluUnProject(win[0],win[1],win[2],mat,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
	glVertex3dv(v);
}

void CustomProjector::sSphere(GLdouble radius, GLdouble one_minus_oblateness,
                              GLint slices, GLint stacks,
                              const Mat4d& mat, int orient_inside,bool bump) const
{
    glPushMatrix();
    glLoadMatrixd(mat);

      // It is really good for performance to have Vec4f,Vec3f objects
      // static rather than on the stack. But why?
      // Is the constructor/destructor so expensive?
    static Vec4f lightPos4;
    static Vec3f lightPos3;
    GLboolean isLightOn;
    static Vec3f transNorm;
    float c;

    static Vec4f ambientLight;
    static Vec4f diffuseLight;

    glGetBooleanv(GL_LIGHTING, &isLightOn);

    if (isLightOn)
    {
        glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos4);
        lightPos3 = lightPos4;
        lightPos3 -= mat * Vec3d(0.,0.,0.); // -posCenterEye
        lightPos3.normalize();
        glGetLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
        glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
        glDisable(GL_LIGHTING);
    }

    GLfloat x, y, z;
    GLfloat s, t;
    GLint i, j;
    GLfloat nsign;

    if (orient_inside) {
      nsign = -1.0;
      t=0.0; // from inside texture is reversed
    } else {
      nsign = 1.0;
      t=1.0;
    }

    const GLfloat drho = M_PI / (GLfloat) stacks;
/***** MSVC Stellarium Build *****/
//Original == #if defined(__sun) || defined(__sun__)
#if defined(__sun) || defined(__sun__) || defined(__MSVC__)
/***** MSVC Stellarium Build *****/
	// in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
    double *cos_sin_rho = new double[2*(stacks+1)];
#else
    double cos_sin_rho[2*(stacks+1)];
#endif
    double *cos_sin_rho_p = cos_sin_rho;
    for (i = 0; i <= stacks; i++) {
      double rho = i * drho;
      *cos_sin_rho_p++ = cos(rho);
      *cos_sin_rho_p++ = sin(rho);
    }

    const GLfloat dtheta = 2.0 * M_PI / (GLfloat) slices;

/***** MSVC Stellarium Build *****/
//Original == double cos_sin_theta[2*(slices+1)];
#if defined(__sun) || defined(__sun__) || defined(__MSVC__)
/***** MSVC Stellarium Build *****/
	double *cos_sin_theta = new double[2*(slices+1)];
#else
    double cos_sin_theta[2*(slices+1)];
#endif

    double *cos_sin_theta_p = cos_sin_theta;
    for (i = 0; i <= slices; i++) {
      double theta = (i == slices) ? 0.0 : i * dtheta;
      *cos_sin_theta_p++ = cos(theta);
      *cos_sin_theta_p++ = sin(theta);
    }

    // texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
    // t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
    // cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
    const GLfloat ds = 1.0 / slices;
    const GLfloat dt = nsign / stacks; // from inside texture is reversed

    //kornyakov: black planets
    float intensity = GetIntensity();

    // draw intermediate as quad strips
    for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks;
         i++,cos_sin_rho_p+=2)
    {
        glBegin(GL_QUAD_STRIP);
        s = 0.0;
        for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;
             j++,cos_sin_theta_p+=2)
        {
            x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
            y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
            z = nsign * cos_sin_rho_p[0];
            glTexCoord2f(s, t);
			if(bump) glNormal3f(x * one_minus_oblateness * nsign,
                                y * one_minus_oblateness * nsign,
                                z * nsign);
            if (isLightOn)
            {
                transNorm = mat.multiplyWithoutTranslation(
                                  Vec3d(x * one_minus_oblateness * nsign,
                                        y * one_minus_oblateness * nsign,
                                        z * nsign));
                c = lightPos3.dot(transNorm);
                if (c<0) c=0;
                //kornyakov: planet fading
				//Ljubov: something for bump-mapping
			    if (bump) glColor3f(x * radius,y * radius,z * one_minus_oblateness * radius);
				else
				glColor3f(intensity*(c*diffuseLight[0] + ambientLight[0]),
                               intensity*(c*diffuseLight[1] + ambientLight[1]),
                               intensity*(c*diffuseLight[2] + ambientLight[2])); 
            }
            sVertex3(x * radius, y * radius, z * one_minus_oblateness * radius, mat);
            x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
            y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
            z = nsign * cos_sin_rho_p[2];
            glTexCoord2f(s, t - dt);
		    if(bump) glNormal3f(x * one_minus_oblateness * nsign,
                                y * one_minus_oblateness * nsign,
                                z * nsign);
            if (isLightOn)
            {
                transNorm = mat.multiplyWithoutTranslation(
                                  Vec3d(x * one_minus_oblateness * nsign,
                                        y * one_minus_oblateness * nsign,
                                        z * nsign));
                c = lightPos3.dot(transNorm);
                if (c<0) c=0;
                //kornyakov: planet fading
                //Ljubov: something for bump-mapping
			    if (bump) glColor3f(x * radius,y * radius,z * one_minus_oblateness * radius);
				else
				glColor3f(intensity*(c*diffuseLight[0] + ambientLight[0]),
                               intensity*(c*diffuseLight[1] + ambientLight[1]),
                               intensity*(c*diffuseLight[2] + ambientLight[2])); 
            }
            sVertex3(x * radius, y * radius, z * one_minus_oblateness * radius, mat);
            s += ds;
        }
        glEnd();
        t -= dt;
    }
    glPopMatrix();
    if (isLightOn) glEnable(GL_LIGHTING);

	/***** MSVC Stellarium Build *****/
	//Original == #if defined(__sun) || defined(__sun__)
	#if defined(__sun) || defined(__sun__) || defined(__MSVC__)
		delete[] cos_sin_theta;
	/***** MSVC Stellarium Build *****/
		delete[] cos_sin_rho;
	#endif
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void CustomProjector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	static GLdouble da, r, dz;
	static GLfloat z, nsign;
	static GLint i, j;

	nsign = 1.0;
	if (orient_inside) glCullFace(GL_FRONT);
	//nsign = -1.0;
	//else nsign = 1.0;

	da = 2.0 * M_PI / slices;
	dz = height / stacks;

	GLfloat ds = 1.0 / slices;
	GLfloat dt = 1.0 / stacks;
	GLfloat t = 0.0;
	z = 0.0;
	r = radius;
	for (j = 0; j < stacks; j++)
	{
	GLfloat s = 0.0;
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= slices; i++)
	{
		GLfloat x, y;
		if (i == slices)
		{
			x = sinf(0.0);
			y = cosf(0.0);
		}
		else
		{
			x = sinf(i * da);
			y = cosf(i * da);
		}
		glNormal3f(x * nsign, y * nsign, 0);
		glTexCoord2f(s, t);
		sVertex3(x * r, y * r, z, mat);
		glNormal3f(x * nsign, y * nsign, 0);
		glTexCoord2f(s, t + dt);
		sVertex3(x * r, y * r, z + dz, mat);
		s += ds;
	}			/* for slices */
	glEnd();
	t += dt;
	z += dz;
	}				/* for stacks */

	glPopMatrix();
	if (orient_inside) glCullFace(GL_BACK);
}


void *convert_to_RGB_Surface(SDL_Surface *bitmap)
{
  unsigned char *pixel = (unsigned char *)malloc(sizeof(char) * 4 * bitmap->h * bitmap->w); 
  int soff = 0;   
  int doff = 0;   
  int x, y;
  unsigned char *spixels = (unsigned char *)bitmap->pixels;
  SDL_Palette *pal = bitmap->format->palette; 

  for (y = 0; y < bitmap->h; y++)
    for (x = 0; x < bitmap->w; x++)
    {
      SDL_Color* col = &pal->colors[spixels[soff]];

      pixel[doff] = col->r; 
      pixel[doff+1] = col->g; 
      pixel[doff+2] = col->b; 
      pixel[doff+3] = 255; 
      doff += 4; 
      soff++;
    }

    return (void *)pixel; 
}

void CustomProjector::render_node(Lib3dsNode *node, Lib3dsFile* file, const Mat4d& matrix, double scale, char* tp)
{
	Lib3dsNode *p;
	Player_texture *pt;
    for (p=node->childs; p!=0; p=p->next) {
      render_node(p, file, matrix, scale, tp);
    }
	if (node->type==LIB3DS_OBJECT_NODE) {
		Lib3dsMesh *mesh;
    if (strcmp(node->name,"$$$DUMMY")==0) { //Serkin: what is it?
      return;
    }

    mesh = lib3ds_file_mesh_by_name(file, node->data.object.morph);
    if( mesh == NULL )
      mesh = lib3ds_file_mesh_by_name(file, node->name);

    if (!mesh->user.d) 
	{
      ASSERT(mesh);
      if (!mesh) 
	  {
        return;
      }
        unsigned p;
        Lib3dsVector *normalL=(Lib3dsVector *)malloc(3*sizeof(Lib3dsVector)*mesh->faces);
        Lib3dsMaterial *oldmat = (Lib3dsMaterial *)-1;
        {
          Lib3dsMatrix M;
          lib3ds_matrix_copy(M, mesh->matrix);
          lib3ds_matrix_inv(M);
          glMultMatrixf(&M[0][0]);
        }
        lib3ds_mesh_calculate_normals(mesh, normalL);

        for (p=0; p<mesh->faces; ++p) {
          Lib3dsFace *f=&mesh->faceL[p];
          Lib3dsMaterial *mat=0;

          Player_texture *pt = NULL;
          int tex_mode = 0;


          if (f->material[0]) {
            mat=lib3ds_file_material_by_name(file, f->material);
          }

          if( mat != oldmat ) {
            if (mat) {
				if( mat->two_sided )
                glDisable(GL_CULL_FACE);
              else
                glEnable(GL_CULL_FACE);

              //glDisable(GL_CULL_FACE);

              /* Texturing added by Gernot < gz@lysator.liu.se > */
			  
              if (mat->texture1_map.name[0]) 
			  {		/* texture map? */
                Lib3dsTextureMap *tex = &mat->texture1_map;
                if (!tex->user.p) 
				{		/* no player texture yet? */
                  char texname[1024];
                  pt = (Player_texture *)malloc(sizeof(*pt));
                  tex->user.p = pt;
                  strcpy(texname, datapath);
                  strcat(texname, tp);
				  strcat(texname, "\\");
                  strcat(texname, tex->name);
                  pt->bitmap = IMG_Load(texname);
                  if (pt->bitmap) 
				  {	/* could image be loaded ? */
                    /* this OpenGL texupload code is incomplete format-wise!
                    * to make it complete, examine SDL_surface->format and
                    * tell us @lib3ds.sf.net about your improvements :-)
                    */
                    int upload_format = GL_RED; /* safe choice, shows errors */

                    int bytespp = pt->bitmap->format->BytesPerPixel;
                    void *pixel = NULL;
                    glGenTextures(1, &pt->tex_id);
                    if (pt->bitmap->format->palette) 
					{
                      pixel = convert_to_RGB_Surface(pt->bitmap);
                      upload_format = GL_RGBA;
                    }
                    else 
					{
                      pixel = pt->bitmap->pixels;
                      /* e.g. this could also be a color palette */
                      if (bytespp == 1) 
						  upload_format = GL_LUMINANCE;
                      else if (bytespp == 3) 
						  upload_format = GL_RGB;
                      else if (bytespp == 4) 
						  upload_format = GL_RGBA;
                    }
                    glBindTexture(GL_TEXTURE_2D, pt->tex_id);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_XSIZE, TEX_YSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pt->bitmap->w, pt->bitmap->h, upload_format, GL_UNSIGNED_BYTE, pixel);
                    pt->scale_x = (float)pt->bitmap->w/(float)TEX_XSIZE;
                    pt->scale_y = (float)pt->bitmap->h/(float)TEX_YSIZE;
                    pt->valid = 1;
                  }
                  else 
				  {
                    fprintf(stderr, "Load of texture %s did not succeed (format not supported !)\n", texname);
                    pt->valid = 0;
                  }
                }
                else 
				{
                  pt = (Player_texture *)tex->user.p;
                }
                tex_mode = pt->valid;
              }
              else {
                tex_mode = 0;
              }
			  if (tex_mode)
			  {
				static const Lib3dsRgba a={1.0, 1.0, 1.0, 1.0};
				static const Lib3dsRgba d={1.0, 1.0, 1.0, 0.0};
				static const Lib3dsRgba s={0.0, 0.0, 0.0, 1.0};
				glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
				glMaterialfv(GL_FRONT, GL_SPECULAR, s);
				glMaterialfv(GL_FRONT, GL_AMBIENT,a);
				glMaterialf(GL_FRONT, GL_SHININESS, 128);
			  }
			  else
			  {
				glMaterialfv(GL_FRONT, GL_AMBIENT, mat->ambient);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse);
				glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular);
				glMaterialf(GL_FRONT, GL_SHININESS, 64);
			  }
            }
            else {
              static const Lib3dsRgba a={0.5, 0.5, 0.5, 1.0};
              static const Lib3dsRgba d={0.7, 0.7, 0.7, 1.0};
              static const Lib3dsRgba s={1.0, 1.0, 1.0, 1.0};
              glMaterialfv(GL_FRONT, GL_AMBIENT, a);
              glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
              glMaterialfv(GL_FRONT, GL_SPECULAR, s);
              glMaterialf(GL_FRONT, GL_SHININESS, 128);
            }
            oldmat = mat;
          }

          else if (mat != NULL && mat->texture1_map.name[0]) {
            Lib3dsTextureMap *tex = &mat->texture1_map;
            if (tex != NULL && tex->user.p != NULL) 
			{
              pt = (Player_texture *)tex->user.p;
              tex_mode = pt->valid;
            }
          }
          {
			int i;
			
			if (tex_mode)
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, pt->tex_id);
				glNormal3fv(f->normal);
				glBegin(GL_TRIANGLES);
				for (i=0; i<3; ++i)
				{
					glNormal3fv(normalL[3*p+i]);
					if (mesh->texelL)
						glTexCoord2f(mesh->texelL[f->points[i]][1]*pt->scale_x, pt->scale_y - mesh->texelL[f->points[i]][0]*pt->scale_y);
					sVertex3((double)mesh->pointL[f->points[i]].pos[0]*scale, (double)mesh->pointL[f->points[i]].pos[1]*scale, (double)mesh->pointL[f->points[i]].pos[2]*scale, matrix);
				} 
				glEnd();
			}
			else
			{
				glNormal3fv(f->normal);
				glBegin(GL_TRIANGLES);
				for (i=0; i<3; ++i)
				{
					glNormal3fv(normalL[3*p+i]);
					float x = mesh->pointL[f->points[i]].pos[0]*scale;
					float y = mesh->pointL[f->points[i]].pos[1]*scale;
					float z = mesh->pointL[f->points[i]].pos[2]*scale;
					sVertex3(x, y, z, matrix);
				}
				glEnd();
			}

            if (tex_mode)
              glDisable(GL_TEXTURE_2D);
          }
		}
        free(normalL);
      }
  }
}

void CustomProjector::drawObject(Object3DS* obj, const Mat4d& mat)
{
	bool flag = glIsEnabled(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
	Lib3dsNode *p;
	double scale = obj->scale;
	for (p=obj->file->nodes; p!=0; p=p->next) {
		render_node(p, obj->file, mat, scale, obj->texpath);
    }
	if (!flag) glDisable(GL_DEPTH_TEST); // We enable the depth test (also called z buffer)

/*#define f 0.000001f
	glBegin(GL_QUADS);			// Start Drawing Quads
		// Front Face
		glNormal3f( 0.0f, 0.0f, 1.0f);		// Normal Facing Forward
		glTexCoord2f(0.0f, 0.0f); sVertex3(-f, f,  f, mat);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); sVertex3( f, -f,  f, mat);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); sVertex3( f,  f,  f, mat);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); sVertex3(-f,  f,  f, mat);	// Top Left Of The Texture and Quad
		// Back Face
		glNormal3f( 0.0f, 0.0f,-1.0f);		// Normal Facing Away
		glTexCoord2f(1.0f, 0.0f); sVertex3(-f, -f, -f, mat);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); sVertex3(-f,  f, -f, mat);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); sVertex3( f,  f, -f, mat);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); sVertex3( f, -f, -f, mat);	// Bottom Left Of The Texture and Quad
		// Top Face
		glNormal3f( 0.0f, 1.0f, 0.0f);		// Normal Facing Up
		glTexCoord2f(0.0f, 1.0f); sVertex3(-f,  f, -f, mat);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); sVertex3(-f,  f,  f, mat);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); sVertex3( f,  f,  f, mat);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); sVertex3( f,  f, -f, mat);	// Top Right Of The Texture and Quad
		// Bottom Face
		glNormal3f( 0.0f,-1.0f, 0.0f);		// Normal Facing Down
		glTexCoord2f(1.0f, 1.0f); sVertex3(-f, -f, -f, mat);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); sVertex3( f, -f, -f, mat);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); sVertex3( f, -f,  f, mat);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); sVertex3(-f, -f,  f, mat);	// Bottom Right Of The Texture and Quad
		// Right face
		glNormal3f( 1.0f, 0.0f, 0.0f);		// Normal Facing Right
		glTexCoord2f(1.0f, 0.0f); sVertex3( f, -f, -f, mat);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); sVertex3( f,  f, -f, mat);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); sVertex3( f,  f,  f, mat);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); sVertex3( f, -f,  f, mat);	// Bottom Left Of The Texture and Quad
		// Left Face
		glNormal3f(-1.0f, 0.0f, 0.0f);		// Normal Facing Left
		glTexCoord2f(0.0f, 0.0f); sVertex3(-f, -f, -f, mat);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); sVertex3(-f, -f,  f, mat);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(1.0f, 1.0f); sVertex3(-f,  f,  f, mat);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); sVertex3(-f,  f, -f, mat);	// Top Left Of The Texture and Quad
	glEnd();	*/
}

void CustomProjector::ms3dsObject(Object3DS* obj, const Mat4d& mat, int orient_inside)
{
	glPushMatrix();
	glLoadMatrixd(mat);
	CustomProjector::drawObject(obj, mat);
	glPopMatrix();
}