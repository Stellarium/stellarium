/*
 * Stellarium
 * Copyright (C) Fabien Chereau
 * Author 2006 Johannes Gajdosik
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

#include <cmath>
#include <QSettings>
#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDebug>

#include "GLee.h"

#include "fixx11h.h"
#include "StelViewportDistorter.hpp"
#include "SphericMirrorCalculator.hpp"
#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"

class StelViewportDistorterDummy : public StelViewportDistorter
{
private:
	friend class StelViewportDistorter;
	QString getType(void) const { return "none"; }
	void prepare(void) const {}
	void distort(void) const {}
	bool distortXY(int &x,int &y) const { return true; }

};


class StelViewportDistorterFisheyeToSphericMirror : public StelViewportDistorter
{
private:
	friend class StelViewportDistorter;
	StelViewportDistorterFisheyeToSphericMirror(int screen_w,int screen_h);
	~StelViewportDistorterFisheyeToSphericMirror(void);
	QString getType(void) const
	{
		return "fisheye_to_spheric_mirror";
	}
	void cleanup(void);
	void prepare(void) const;
	void distort(void) const;
	bool distortXY(int &x,int &y) const;

private:
	bool flag_use_ext_framebuffer_object;
	const int screen_w;
	const int screen_h;
	const StelProjector::StelProjectorParams originalProjectorParams;
	StelProjector::StelProjectorParams newProjectorParams;
	int viewport_texture_offset[2];
	int texture_wh;

	Vec2f *texture_point_array;
	int max_x,max_y;
	double step_x,step_y;

	QVector<Vec2f> displayVertexList;
	QVector<Vec4f> displayColorList;
	QVector<Vec2f> displayTexCoordList;
	GLuint mirror_texture;
	GLuint fbo;             // frame buffer object
	GLuint depth_buffer;    // depth render buffer
};


struct VertexPoint
{
	Vec2f ver_xy;
	Vec4f color;
	double h;
};



StelViewportDistorterFisheyeToSphericMirror::StelViewportDistorterFisheyeToSphericMirror(int screen_w,int screen_h)
	:	screen_w(screen_w), screen_h(screen_h),
		originalProjectorParams(StelApp::getInstance().getCore()->getCurrentStelProjectorParams()),
		texture_point_array(0)
{
	QSettings& conf = *StelApp::getInstance().getSettings();
	StelCore* core = StelApp::getInstance().getCore();
	
	flag_use_ext_framebuffer_object = false;
#if 0
	flag_use_ext_framebuffer_object = GLEE_EXT_framebuffer_object;
	if (flag_use_ext_framebuffer_object)
	{
		flag_use_ext_framebuffer_object = conf.value("spheric_mirror/flag_use_ext_framebuffer_object",false).toBool();
	}
	qDebug() << "INFO: flag_use_ext_framebuffer_object = " << flag_use_ext_framebuffer_object;
	if (flag_use_ext_framebuffer_object && !GLEE_EXT_packed_depth_stencil)
	{
		qWarning() << "WARNING: "
		"using EXT_framebuffer_object, but EXT_packed_depth_stencil "
		"not available: no stencil buffer support";
	}
#endif
															 
	// initialize viewport parameters and texture size:
	
	// maximum FOV value of the not yet distorted image
	double distorter_max_fov = conf.value("spheric_mirror/distorter_max_fov",175.0).toDouble();
	if (distorter_max_fov > 240.0)
		distorter_max_fov = 240.0;
	else if (distorter_max_fov < 120.0)
		distorter_max_fov = 120.0;
	if (distorter_max_fov > core->getMovementMgr()->getMaxFov())
		distorter_max_fov = core->getMovementMgr()->getMaxFov();
	
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	core->getMovementMgr()->setMaxFov(distorter_max_fov);

	// width of the not yet distorted image
	newProjectorParams.viewportXywh[2] = conf.value("spheric_mirror/newProjectorParams.viewportXywh[2]idth", originalProjectorParams.viewportXywh[2]).toInt();
	if (newProjectorParams.viewportXywh[2] <= 0)
	{
		newProjectorParams.viewportXywh[2] = originalProjectorParams.viewportXywh[2];
	}
	else if (!flag_use_ext_framebuffer_object && newProjectorParams.viewportXywh[2] > screen_w)
	{
		newProjectorParams.viewportXywh[2] = screen_w;
	}
	
	// height of the not yet distorted image
	newProjectorParams.viewportXywh[3] = conf.value("spheric_mirror/newProjectorParams.viewportXywh[3]eight", originalProjectorParams.viewportXywh[3]).toInt();
	if (newProjectorParams.viewportXywh[3] <= 0)
	{
		newProjectorParams.viewportXywh[3] = originalProjectorParams.viewportXywh[3];
	}
	else if (!flag_use_ext_framebuffer_object && newProjectorParams.viewportXywh[3] > screen_h)
	{
		newProjectorParams.viewportXywh[3] = screen_h;
	}
	
	// center of the FOV-disk in the not yet distorted image
	newProjectorParams.viewportCenter[0] = conf.value("spheric_mirror/viewportCenterX", 0.5*newProjectorParams.viewportXywh[2]).toDouble();
	newProjectorParams.viewportCenter[1] = conf.value("spheric_mirror/viewportCenterY", 0.5*newProjectorParams.viewportXywh[3]).toDouble();
	
	// diameter of the FOV-disk in pixels
	newProjectorParams.viewportFovDiameter = conf.value("spheric_mirror/viewport_fov_diameter", qMin(newProjectorParams.viewportXywh[2],newProjectorParams.viewportXywh[3])).toDouble();

	texture_wh = 1;
	while (texture_wh < newProjectorParams.viewportXywh[2] || texture_wh < newProjectorParams.viewportXywh[3])
		texture_wh <<= 1;
	viewport_texture_offset[0] = (texture_wh-newProjectorParams.viewportXywh[2])>>1;
	viewport_texture_offset[1] = (texture_wh-newProjectorParams.viewportXywh[3])>>1;

	if (flag_use_ext_framebuffer_object)
	{
		newProjectorParams.viewportXywh[0] = viewport_texture_offset[0];
		newProjectorParams.viewportXywh[1] = viewport_texture_offset[1];
	}
	else
	{
		newProjectorParams.viewportXywh[0] = (screen_w-newProjectorParams.viewportXywh[2]) >> 1;
		newProjectorParams.viewportXywh[1] = (screen_h-newProjectorParams.viewportXywh[3]) >> 1;
	}
	//qDebug() << "texture_wh: " << texture_wh;
	//qDebug() << "viewportFovDiameter: " << viewportFovDiameter;
	//qDebug() << "screen: " << screen_w << ", " << screen_h;
	//qDebug() << "viewport: " << viewport[0] << ", " << viewport[1] << ", "
	//         << newProjectorParams.viewportXywh[2] << ", " << newProjectorParams.viewportXywh[3];
	//qDebug() << "viewport_texture_offset: "
	//         << viewport_texture_offset[0] << ", "
	//         << viewport_texture_offset[1];
	
	StelApp::getInstance().getCore()->setCurrentStelProjectorParams(newProjectorParams);

	// initialize mirror_texture:
	glGenTextures(1, &mirror_texture);
	glBindTexture(GL_TEXTURE_2D, mirror_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (flag_use_ext_framebuffer_object)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		             texture_wh,texture_wh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		// create the depth buffer (which also includes the stencil buffer
		// in case of EXT_packed_depth_stencil):
		glGenRenderbuffersEXT(1, &depth_buffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_buffer);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
		                         GLEE_EXT_packed_depth_stencil
		                         ? GL_DEPTH24_STENCIL8_EXT
		                         : GL_DEPTH_COMPONENT,
		                         texture_wh,texture_wh);
		// Setup our FBO
		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		// Attach the texture to the FBO so we can render to it
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		                          GL_TEXTURE_2D, mirror_texture, 0);
		// Attach the depth (and stencil) buffer to the FBO as it's
		// depth (and stencil) attachment
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
		                             GL_RENDERBUFFER_EXT, depth_buffer);
		if (GLEE_EXT_packed_depth_stencil)
		{
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
			                             GL_STENCIL_ATTACHMENT_EXT,
			                             GL_RENDERBUFFER_EXT, depth_buffer);
		}
		const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			qDebug() << "could not initialize GL_FRAMEBUFFER_EXT";
			Q_ASSERT(0);
		}
		// clear the texture
		glClear(GL_COLOR_BUFFER_BIT);
		// Unbind the FBO
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	else
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		const int size = 3*texture_wh*texture_wh;
		unsigned char *pixel_data = new unsigned char[size];
		Q_ASSERT(pixel_data);
		memset(pixel_data,0,size);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		             texture_wh, texture_wh, 0, GL_RGB, GL_UNSIGNED_BYTE,
		             pixel_data);
		delete[] pixel_data;
	}

	// init transformation
	VertexPoint *vertex_point_array = 0;
	const QString custom_distortion_file = conf.value("spheric_mirror/custom_distortion_file","").toString();
	if (custom_distortion_file.isEmpty())
	{
		double texture_triangle_base_length = conf.value("spheric_mirror/texture_triangle_base_length",16.0).toDouble();
		if (texture_triangle_base_length > 256.0)
			texture_triangle_base_length = 256.0;
		else if (texture_triangle_base_length < 2.0)
			texture_triangle_base_length = 2.0;
		max_x = (int)trunc(0.5 + screen_w/texture_triangle_base_length);
		step_x = screen_w / (double)(max_x-0.5);
		max_y = (int)trunc(screen_h/(texture_triangle_base_length*0.5*sqrt(3.0)));
		step_y = screen_h/ (double)max_y;
		//qDebug() << "max_x: " << max_x << ", max_y: " << max_y
		//         << ", step_x: " << step_x << ", step_y: " << step_y;

		double gamma = conf.value("spheric_mirror/projector_gamma",0.45).toDouble();
		if (gamma < 0.0) gamma = 0.0;
		
		const float view_scaling_factor = 0.5 * newProjectorParams.viewportFovDiameter / prj->fovToViewScalingFactor(distorter_max_fov*(M_PI/360.0));
		texture_point_array = new Vec2f[(max_x+1)*(max_y+1)];
		vertex_point_array = new VertexPoint[(max_x+1)*(max_y+1)];
		double max_h = 0;
		SphericMirrorCalculator calc(conf);
		for (int j=0;j<=max_y;j++)
		{
			for (int i=0;i<=max_x;i++)
			{
				VertexPoint &vertex_point(vertex_point_array[(j*(max_x+1)+i)]);
				Vec2f &texture_point(texture_point_array[(j*(max_x+1)+i)]);
				vertex_point.ver_xy[0] = ((i == 0) ? 0.f :
				                          (i == max_x) ? screen_w :
				                          (i-0.5f*(j&1))*step_x);
				vertex_point.ver_xy[1] = j*step_y;
				Vec3d v,vX,vY;
				bool rc = calc.retransform(
				              (vertex_point.ver_xy[0]-0.5f*screen_w) / screen_h,
				              (vertex_point.ver_xy[1]-0.5f*screen_h) / screen_h,
				              v,vX,vY);
				rc &= prj->forward(v);
				const float x = newProjectorParams.viewportCenter[0] + v[0] * view_scaling_factor;
				const float y = newProjectorParams.viewportCenter[1] + v[1] * view_scaling_factor;
				vertex_point.h = rc ? (vX^vY).length() : 0.0;

				// sharp image up to the border of the fisheye image, at the cost of
				// accepting clamping artefacts. You can get rid of the clamping
				// artefacts by specifying a viewport size a little less then
				// (1<<n)*(1<<n), for instance 1022*1022. With a viewport size
				// of 512*512 and viewportFovDiameter=512 you will get clamping
				// artefacts in the 3 otherwise black hills on the bottom of the image.

				//      if (x < 0.f) {x=0.f;vertex_point.h=0;}
				//      else if (x > newProjectorParams.viewportXywh[2]) {x=newProjectorParams.viewportXywh[2];vertex_point.h=0;}
				//      if (y < 0.f) {y=0.f;vertex_point.h=0;}
				//      else if (y > newProjectorParams.viewportXywh[3]) {y=newProjectorParams.viewportXywh[3];vertex_point.h=0;}

				texture_point[0] = (viewport_texture_offset[0]+x)/texture_wh;
				texture_point[1] = (viewport_texture_offset[1]+y)/texture_wh;

				if (vertex_point.h > max_h) max_h = vertex_point.h;
			}
		}
		for (int j=0;j<=max_y;j++)
		{
			for (int i=0;i<=max_x;i++)
			{
				VertexPoint &vertex_point(vertex_point_array[(j*(max_x+1)+i)]);
				vertex_point.color[0] = vertex_point.color[1] = vertex_point.color[2] =
				                            (vertex_point.h<=0.0) ? 0.0 : exp(gamma*log(vertex_point.h/max_h));
				vertex_point.color[3] = 1.0f;
			}
		}
	}
	else
	{
		QFile file;
		QDataStream in;
		try
		{
			file.setFileName(StelApp::getInstance().getFileMgr().findFile(custom_distortion_file));
			file.open(QIODevice::ReadOnly);
			if (file.error() != QFile::NoError)
				throw("failed to open file");

			in.setDevice(&file);
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "WARNING: could not open custom_distortion_file:" << custom_distortion_file << e.what();
		}
		Q_ASSERT(file.error()!=QFile::NoError);
		in >> max_x >> max_y;
		Q_ASSERT(in.status()==QDataStream::Ok && max_x>0 && max_y>0);
		step_x = screen_w / (double)(max_x-0.5);
		step_y = screen_h/ (double)max_y;
		//qDebug() << "max_x: " << max_x << ", max_y: " << max_y
		//         << ", step_x: " << step_x << ", step_y: " << step_y;
		texture_point_array = new Vec2f[(max_x+1)*(max_y+1)];
		vertex_point_array = new VertexPoint[(max_x+1)*(max_y+1)];
		for (int j=0;j<=max_y;j++)
		{
			for (int i=0;i<=max_x;i++)
			{
				VertexPoint &vertex_point(vertex_point_array[(j*(max_x+1)+i)]);
				Vec2f &texture_point(texture_point_array[(j*(max_x+1)+i)]);
				vertex_point.ver_xy[0] = ((i == 0) ? 0.f :
				                          (i == max_x) ? screen_w :
				                          (i-0.5f*(j&1))*step_x);
				vertex_point.ver_xy[1] = j*step_y;
				float x,y;
				in >> x >> y
				>> vertex_point.color[0]
				>> vertex_point.color[1]
				>> vertex_point.color[2];
				vertex_point.color[3] = 1.0f;
				Q_ASSERT(in.status()!=QDataStream::Ok);
				//      if (x < 0.f) {x=0.f;vertex_point.h=0;}
				//      else if (x > newProjectorParams.viewportXywh[2]) {x=newProjectorParams.viewportXywh[2];vertex_point.h=0;}
				//      if (y < 0.f) {y=0.f;vertex_point.h=0;}
				//      else if (y > newProjectorParams.viewportXywh[3]) {y=newProjectorParams.viewportXywh[3];vertex_point.h=0;}

				texture_point[0] = (viewport_texture_offset[0]+x)/texture_wh;
				texture_point[1] = (viewport_texture_offset[1]+y)/texture_wh;

			}
		}
	}

	// initialize the display list
	displayVertexList.clear();
	for (int j=0;j<max_y;j++)
	{
		const Vec2f *t0 = texture_point_array + j*(max_x+1);
		const Vec2f *t1 = t0;
		const VertexPoint *v0 = vertex_point_array + j*(max_x+1);
		const VertexPoint *v1 = v0;
		if (j&1)
		{
			t1 += (max_x+1);
			v1 += (max_x+1);
		}
		else
		{
			t0 += (max_x+1);
			v0 += (max_x+1);
		}
		for (int i=0;i<=max_x;i++,t0++,t1++,v0++,v1++)
		{
			displayColorList << v0->color << v1->color;
			displayTexCoordList << *t0 << *t1;
			displayVertexList << v0->ver_xy << v1->ver_xy;
		}
	}
	delete[] vertex_point_array;
}



StelViewportDistorterFisheyeToSphericMirror::~StelViewportDistorterFisheyeToSphericMirror(void)
{
	if (texture_point_array) delete[] texture_point_array;
	if (flag_use_ext_framebuffer_object)
	{
		glDeleteFramebuffersEXT(1, &fbo);
		glDeleteRenderbuffersEXT(1, &depth_buffer);
	}
	glDeleteTextures(1,&mirror_texture);
	
	// TODO repair
	// prj->setMaxFov(original_max_fov);
//	prj->setViewport(original_viewport[0],original_viewport[1],
// 	                 original_viewport[2],original_viewport[3],
// 	                 original_viewportCenter[0],original_viewportCenter[1],
// 	                 original_viewportFovDiameter);
}




bool StelViewportDistorterFisheyeToSphericMirror::distortXY(int &x,int &y) const
{
	float texture_x,texture_y;

	// find the triangle and interpolate accordingly:
	float dy = y / step_y;
	const int j = (int)floorf(dy);
	dy -= j;
	if (j&1)
	{
		float dx = x / step_x + 0.5f*(1.f-dy);
		const int i = (int)floorf(dx);
		dx -= i;
		const Vec2f *const t = texture_point_array + (j*(max_x+1)+i);
		if (dx + dy <= 1.f)
		{
			if (i == 0)
			{
				dx -= 0.5f*(1.f-dy);
				dx *= 2.f;
			}
			texture_x = t[0][0]
			            + dx * (t[1][0]-t[0][0])
			            + dy * (t[max_x+1][0]-t[0][0]);
			texture_y = t[0][1]
			            + dx * (t[1][1]-t[0][1])
			            + dy * (t[max_x+1][1]-t[0][1]);
		}
		else
		{
			if (i == max_x-1)
			{
				dx -= 0.5f*(1.f-dy);
				dx *= 2.f;
			}
			texture_x = t[max_x+2][0]
			            + (1.f-dy) * (t[1][0]-t[max_x+2][0])
			            + (1.f-dx) * (t[max_x+1][0]-t[max_x+2][0]);
			texture_y = t[max_x+2][1]
			            + (1.f-dy) * (t[1][1]-t[max_x+2][1])
			            + (1.f-dx) * (t[max_x+1][1]-t[max_x+2][1]);
		}
	}
	else
	{
		float dx = x / step_x + 0.5f*dy;
		const int i = (int)floorf(dx);
		dx -= i;
		const Vec2f *const t = texture_point_array + (j*(max_x+1)+i);
		if (dx >= dy)
		{
			if (i == max_x-1)
			{
				dx -= 0.5f*dy;
				dx *= 2.f;
			}
			texture_x = t[1][0]
			            + (1.f-dx) * (t[0][0]-t[1][0])
			            + dy * (t[max_x+2][0]-t[1][0]);
			texture_y = t[1][1]
			            + (1.f-dx) * (t[0][1]-t[1][1])
			            + dy * (t[max_x+2][1]-t[1][1]);
		}
		else
		{
			if (i == 0)
			{
				dx -= 0.5f*dy;
				dx *= 2.f;
			}
			texture_x = t[max_x+1][0]
			            + (1.f-dy) * (t[0][0]-t[max_x+1][0])
			            + dx * (t[max_x+2][0]-t[max_x+1][0]);
			texture_y = t[max_x+1][1]
			            + (1.f-dy) * (t[0][1]-t[max_x+1][1])
			            + dx * (t[max_x+2][1]-t[max_x+1][1]);
		}
	}


	x = (int)floorf(0.5+texture_wh*texture_x) - viewport_texture_offset[0] + newProjectorParams.viewportXywh[0];
	y = (int)floorf(0.5+texture_wh*texture_y) - viewport_texture_offset[1] + newProjectorParams.viewportXywh[1];
	return true;
}

void StelViewportDistorterFisheyeToSphericMirror::prepare(void) const
{
	// First we bind the FBO so we can render to it
	if (flag_use_ext_framebuffer_object)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	}
}

void StelViewportDistorterFisheyeToSphericMirror::distort(void) const
{	
	// set rendering back to default frame buffer
	if (flag_use_ext_framebuffer_object)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	StelPainter sPainter(StelApp::getInstance().getCore()->getProjection2d());
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, screen_w, screen_h);
	glMatrixMode(GL_PROJECTION);        // projection matrix mode
	glLoadIdentity();
	glOrtho(0,screen_w,0,screen_h, -1, 1); // set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);         // modelview matrix mode
	glLoadIdentity();
	
	glBindTexture(GL_TEXTURE_2D, mirror_texture);
	if (!flag_use_ext_framebuffer_object)
	{
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
		                    viewport_texture_offset[0],
		                    viewport_texture_offset[1],
			 			    newProjectorParams.viewportXywh[0],
	   						newProjectorParams.viewportXywh[1],
		                    newProjectorParams.viewportXywh[2],
					  		newProjectorParams.viewportXywh[3]);
	}


	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, mirror_texture);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, displayColorList.constData());
	glVertexPointer(2, GL_FLOAT, 0, displayVertexList.constData());
	glTexCoordPointer(2, GL_FLOAT, 0, displayTexCoordList.constData());
	for (int j=0;j<max_y;j++)
	{
		glDrawArrays(GL_TRIANGLE_STRIP, j*(max_x+1)*2, (max_x+1)*2);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glViewport(newProjectorParams.viewportXywh[0],newProjectorParams.viewportXywh[1],newProjectorParams.viewportXywh[2],newProjectorParams.viewportXywh[3]);
}


StelViewportDistorter *StelViewportDistorter::create(const QString &type, int width,int height, StelProjectorP prj)
{
	if (type == "fisheye_to_spheric_mirror")
	{
		return new StelViewportDistorterFisheyeToSphericMirror(width,height);
	}
	return new StelViewportDistorterDummy;
}

