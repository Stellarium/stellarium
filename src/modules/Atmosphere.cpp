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

#ifdef USE_OPENGL_ES2
 #include "GLES2/gl2.h"
#endif

#if QT_VERSION>=0x040600
 #include <QGLShaderProgram>
 #include <QtOpenGL>
#else
 #include "GLee.h"
 #include "fixx11h.h"
#endif

#include "Atmosphere.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelFileMgr.hpp"

#include <QDebug>

inline bool myisnan(double value)
{
	return value != value;
}

Atmosphere::Atmosphere(void) :viewport(0,0,0,0),skyResolutionY(44), posGrid(NULL), colorGrid(NULL), indices(NULL),
					   averageLuminance(0.f), eclipseFactor(1.), lightPollutionLuminance(0)
{
	setFadeDuration(3.f);
	useShader = StelApp::getInstance().getUseGLShaders();
#if QT_VERSION>=0x040600
	useShader = useShader && QGLShaderProgram::hasShaderPrograms();
	if (useShader)
	{
		QString filePath;
		try
		{
			filePath = StelApp::getInstance().getFileMgr().findFile("data/shaders/xyYToRGB.glsl");
		}
		catch (std::runtime_error& e)
		{
			qFatal("Can't find data/shaders/xyYToRGB.glsl shader file to load");
		}
		qDebug() << "Use vertex shader for atmosphere rendering: " << filePath;
		QGLShader* vShader = new QGLShader(filePath, QGLShader::VertexShader);
		atmoShaderProgram = new QGLShaderProgram();
		if (!vShader->isCompiled())
		{
			qWarning() << "Error while compiling shader: " << vShader->log();
			useShader = false;
		}
		if (!vShader->log().isEmpty())
		{
			qWarning() << "Warnings while compiling vertex shader: " << vShader->log();
		}
		QGLShader* fShader = new QGLShader(QGLShader::FragmentShader);
		if (!fShader->compile(
						"varying mediump vec4 resultSkyColor;\n"
						"void main()\n"
						"{\n"
						 "   gl_FragColor = resultSkyColor;\n"
						 "}"))
		{
			qWarning() << "Error while compiling fragment shader: " << fShader->log();
			useShader = false;
		}
		if (!fShader->log().isEmpty())
		{
			qWarning() << "Warnings while compiling fragment shader: " << vShader->log();
		}
		atmoShaderProgram->addShader(vShader);
		atmoShaderProgram->addShader(fShader);
		if (!atmoShaderProgram->link())
		{
			qWarning() << "Error while linking shader program: " << atmoShaderProgram->log();
			useShader = false;
		}
		if (!atmoShaderProgram->log().isEmpty())
		{
			qWarning() << "Warnings while linking shader: " << atmoShaderProgram->log();
		}
	}
#else
	if (!QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_Version_2_0) && !QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_ES_Version_2_0))
	{
		useShader=false;
	}

	if (useShader)
	{
		qDebug() << "Use vertex shader for atmosphere rendering.";
		QString filePath;
		GLuint shaderXyYToRGB = glCreateShader(GL_VERTEX_SHADER);
		try
		{
			filePath = StelApp::getInstance().getFileMgr().findFile("data/shaders/xyYToRGB.glsl");
		}
		catch (std::runtime_error& e)
		{
			qFatal("Can't find data/shaders/xyYToRGB.glsl shader file to load");
		}

		QFile fic(filePath);
		fic.open(QIODevice::ReadOnly);
		QByteArray qba = fic.readAll();
		const char* data = qba.constData();
		fic.close();
		glShaderSource(shaderXyYToRGB, 1, &data, NULL);
		glCompileShader(shaderXyYToRGB);
		char msg[4048];
		msg[0]='\0';
		int l;
		glGetShaderInfoLog(shaderXyYToRGB, 4048, &l, msg);
		if (msg[0]!='\0')
			qWarning() << msg;
		int val;
		glGetShaderiv(shaderXyYToRGB, GL_COMPILE_STATUS, &val);
		if (val==GL_FALSE)
		{
			useShader = false;
			qWarning() << "Shader compilation error, fall back to standard rendering.";
		}
		else
		{
			GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
			const char* fshaderSrc = "varying vec4 resultSkyColor;\n"
			"void main()\n"
			"{\n"
			 "   gl_FragColor = resultSkyColor;\n"
			 "}";
			glShaderSource(fshader, 1, &fshaderSrc, NULL);
			glCompileShader(fshader);
			atmoShaderProgram = glCreateProgram();
			glAttachShader(atmoShaderProgram,shaderXyYToRGB);
			glAttachShader(atmoShaderProgram,fshader);
			glLinkProgram(atmoShaderProgram);
		}
	}
#endif
}

Atmosphere::~Atmosphere(void)
{
	if (posGrid)
	{
		delete[] posGrid;
		posGrid = NULL;
	}
	if (colorGrid)
	{
		delete[] colorGrid;
		colorGrid = NULL;
	}
	if (indices)
	{
		delete[] indices;
		indices = NULL;
	}
	if (useShader)
	{
#if QT_VERSION>=0x040600
		delete atmoShaderProgram;
#else
		// Cleanup shader stuff
		glDeleteProgram(atmoShaderProgram);
#endif
	}
}

void Atmosphere::computeColor(double JD, Vec3d _sunPos, Vec3d moonPos, float moonPhase,
							   StelCore* core, float latitude, float altitude, float temperature, float relativeHumidity)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	if (viewport != prj->getViewport())
	{
		// The viewport changed: update the number of point of the grid
		viewport = prj->getViewport();
		if (posGrid)
			delete[] posGrid;
		if (colorGrid)
			delete[] colorGrid;
		if (indices)
			delete[] indices;
		skyResolutionX = (int)floor(0.5+skyResolutionY*(0.5*sqrt(3.0))*prj->getViewportWidth()/prj->getViewportHeight());
		posGrid = new Vec2f[(1+skyResolutionX)*(1+skyResolutionY)];
		colorGrid = new Vec4f[(1+skyResolutionX)*(1+skyResolutionY)];
		float stepX = (float)prj->getViewportWidth() / (skyResolutionX-0.5);
		float stepY = (float)prj->getViewportHeight() / skyResolutionY;
		float viewport_left = (float)prj->getViewportPosX();
		float viewport_bottom = (float)prj->getViewportPosY();
		for (int x=0; x<=skyResolutionX; ++x)
		{
			for(int y=0; y<=skyResolutionY; ++y)
			{
				Vec2f &v(posGrid[y*(1+skyResolutionX)+x]);
				v[0] = viewport_left + ((x == 0) ? 0.f :
						(x == skyResolutionX) ? (float)prj->getViewportWidth() : (x-0.5*(y&1))*stepX);
				v[1] = viewport_bottom+y*stepY;
			}
		}

		// Generate the indices used to draw the quads
		indices = new GLushort[(skyResolutionX+1)*skyResolutionY*2];
		int i=0;
		for (int y2=0; y2<skyResolutionY; ++y2)
		{
			GLushort g0 = y2*(1+skyResolutionX);
			GLushort g1 = (y2+1)*(1+skyResolutionX);
			for (int x2=0; x2<=skyResolutionX; ++x2)
			{
				indices[i++]=g0++;
				indices[i++]=g1++;
			}
		}
	}
	
	if (myisnan(_sunPos.length()))
		_sunPos.set(0.,0.,-1.*AU);
	if (myisnan(moonPos.length()))
		moonPos.set(0.,0.,-1.*AU);

	// Update the eclipse intensity factor to apply on atmosphere model
	// these are for radii
	const double sun_angular_size = atan(696000./AU/_sunPos.length());
	const double moon_angular_size = atan(1738./AU/moonPos.length());
	const double touch_angle = sun_angular_size + moon_angular_size;

	// determine luminance falloff during solar eclipses
	_sunPos.normalize();
	moonPos.normalize();
	double separation_angle = std::acos(_sunPos.dot(moonPos));  // angle between them
	// qDebug("touch at %f\tnow at %f (%f)\n", touch_angle, separation_angle, separation_angle/touch_angle);
	// bright stars should be visible at total eclipse
	// TODO: correct for atmospheric diffusion
	// TODO: use better coverage function (non-linear)
	// because of above issues, this algorithm darkens more quickly than reality
	if( separation_angle < touch_angle)
	{
		double dark_angle = moon_angular_size - sun_angular_size;
		float min;
		if(dark_angle < 0)
		{
			// annular eclipse
			float asun = sun_angular_size*sun_angular_size;
			min = (asun - moon_angular_size*moon_angular_size)/asun;  // minimum proportion of sun uncovered
			dark_angle *= -1;
		}
		else min = 0.0001;  // so bright stars show up at total eclipse

		if (separation_angle < dark_angle)
			eclipseFactor = min;
		else
			eclipseFactor = min + (1.-min)*(separation_angle-dark_angle)/(touch_angle-dark_angle);
	}
	else
		eclipseFactor = 1.;


	// No need to calculate if not visible
	if (!fader.getInterstate())
	{
		averageLuminance = 0.001 + lightPollutionLuminance;
		return;
	}

	// Calculate the atmosphere RGB for each point of the grid

	float sunPos[3];
	sunPos[0] = _sunPos[0];
	sunPos[1] = _sunPos[1];
	sunPos[2] = _sunPos[2];

	float moon_pos[3];
	moon_pos[0] = moonPos[0];
	moon_pos[1] = moonPos[1];
	moon_pos[2] = moonPos[2];

	sky.setParamsv(sunPos, 5.f);

	skyb.setLocation(latitude * M_PI/180., altitude, temperature, relativeHumidity);
	skyb.setSunMoon(moon_pos[2], sunPos[2]);

	// Calculate the date from the julian day.
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	skyb.setDate(year, month, moonPhase);

	// Variables used to compute the average sky luminance
	double sum_lum = 0.;

	Vec3d point(1., 0., 0.);
	skylightStruct2 b2;
	float lumi;

	// Compute the sky color for every point above the ground
	for (int i=0; i<(1+skyResolutionX)*(1+skyResolutionY); ++i)
	{
		Vec2f &v(posGrid[i]);
		prj->unProject(v[0],v[1],point);

		Q_ASSERT(fabs(point.lengthSquared()-1.0) < 1e-10);

		if (point[2]<=0)
		{
			point[2] = -point[2];
			// The sky below the ground is the symmetric of the one above :
			// it looks nice and gives proper values for brightness estimation
		}

		// Use the Skybright.cpp 's models for brightness which gives better results.
		lumi = skyb.getLuminance(moon_pos[0]*point[0]+moon_pos[1]*point[1]+
				moon_pos[2]*point[2], sunPos[0]*point[0]+sunPos[1]*point[1]+
				sunPos[2]*point[2], point[2]);
		lumi *= eclipseFactor;
		// Add star background luminance
		lumi += 0.0001;
		// Multiply by the input scale of the ToneConverter (is not done automatically by the xyYtoRGB method called later)
		//lumi*=eye->getInputScale();

		// Add the light pollution luminance AFTER the scaling to avoid scaling it because it is the cause
		// of the scaling itself
		lumi += lightPollutionLuminance;

		// Store for later statistics
		sum_lum+=lumi;

		// Now need to compute the xy part of the color component
		// This can be done in the openGL shader if possible
		if (useShader)
		{
			// Store the back projected position + luminance in the input color to the shader
			colorGrid[i].set(point[0], point[1], point[2], lumi);
		}
		else
		{
			if (lumi>0.01)
			{
				b2.pos[0] = point[0];
				b2.pos[1] = point[1];
				b2.pos[2] = point[2];
				// Use the Skylight model for the color
				sky.getxyYValuev(b2);
			}
			else
			{
				// Too dark to see atmosphere color, don't bother computing it
				b2.color[0]=0.25;
				b2.color[1]=0.25;
			}
			colorGrid[i].set(b2.color[0], b2.color[1], lumi, 1.f);
		}
	}

	// Update average luminance
	averageLuminance = sum_lum/((1+skyResolutionX)*(1+skyResolutionY));
}



// Draw the atmosphere using the precalc values stored in tab_sky
void Atmosphere::draw(StelCore* core)
{
	if (StelApp::getInstance().getVisionModeNight())
		return;

	StelToneReproducer* eye = core->getToneReproducer();

	if (!fader.getInterstate())
		return;

	StelPainter sPainter(core->getProjection2d());

	const float atm_intensity = fader.getInterstate();
	if (useShader)
	{
#if QT_VERSION>=0x040600
		atmoShaderProgram->enable();
		float a, b, c;
		eye->getShadersParams(a, b, c);
		atmoShaderProgram->setUniformValue("alphaWaOverAlphaDa", a);
		atmoShaderProgram->setUniformValue("oneOverGamma", b);
		atmoShaderProgram->setUniformValue("term2TimesOneOverMaxdLpOneOverGamma", c);
		atmoShaderProgram->setUniformValue("brightnessScale", atm_intensity);
		Vec3f sunPos;
		float term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey;
		sky.getShadersParams(sunPos, term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey);
		atmoShaderProgram->setUniformValue("sunPos", sunPos[0], sunPos[1], sunPos[2]);
		atmoShaderProgram->setUniformValue("term_x", term_x);
		atmoShaderProgram->setUniformValue("Ax", Ax);
		atmoShaderProgram->setUniformValue("Bx", Bx);
		atmoShaderProgram->setUniformValue("Cx", Cx);
		atmoShaderProgram->setUniformValue("Dx", Dx);
		atmoShaderProgram->setUniformValue("Ex", Ex);
		atmoShaderProgram->setUniformValue("term_y", term_y);
		atmoShaderProgram->setUniformValue("Ay", Ay);
		atmoShaderProgram->setUniformValue("By", By);
		atmoShaderProgram->setUniformValue("Cy", Cy);
		atmoShaderProgram->setUniformValue("Dy", Dy);
		atmoShaderProgram->setUniformValue("Ey", Ey);
		const Mat4f m = sPainter.getProjector()->getProjectionMatrix();
		atmoShaderProgram->setUniformValue("projectionMatrix",
			QMatrix4x4(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]));
		atmoShaderProgram->setAttributeArray("skyVertex", (const GLfloat*)posGrid, 2, 0);
		atmoShaderProgram->setAttributeArray("skyColor", (const GLfloat*)colorGrid, 4, 0);
#else
		glUseProgram(atmoShaderProgram);
		float a, b, c;
		eye->getShadersParams(a, b, c);
		GLint loc = glGetUniformLocation(atmoShaderProgram, "alphaWaOverAlphaDa");
		glUniform1f(loc, a);
		loc = glGetUniformLocation(atmoShaderProgram, "oneOverGamma");
		glUniform1f(loc, b);
		loc = glGetUniformLocation(atmoShaderProgram, "term2TimesOneOverMaxdLpOneOverGamma");
		glUniform1f(loc, c);
		loc = glGetUniformLocation(atmoShaderProgram, "brightnessScale");
		glUniform1f(loc, atm_intensity);

		Vec3f sunPos;
		float term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey;
		sky.getShadersParams(sunPos, term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey);
		loc = glGetUniformLocation(atmoShaderProgram, "sunPos");
		glUniform3f(loc, sunPos[0], sunPos[1], sunPos[2]);
		loc = glGetUniformLocation(atmoShaderProgram, "term_x");
		glUniform1f(loc, term_x);
		loc = glGetUniformLocation(atmoShaderProgram, "Ax");
		glUniform1f(loc, Ax);
		loc = glGetUniformLocation(atmoShaderProgram, "Bx");
		glUniform1f(loc, Bx);
		loc = glGetUniformLocation(atmoShaderProgram, "Cx");
		glUniform1f(loc, Cx);
		loc = glGetUniformLocation(atmoShaderProgram, "Dx");
		glUniform1f(loc, Dx);
		loc = glGetUniformLocation(atmoShaderProgram, "Ex");
		glUniform1f(loc, Ex);
		loc = glGetUniformLocation(atmoShaderProgram, "term_y");
		glUniform1f(loc, term_y);
		loc = glGetUniformLocation(atmoShaderProgram, "Ay");
		glUniform1f(loc, Ay);
		loc = glGetUniformLocation(atmoShaderProgram, "By");
		glUniform1f(loc, By);
		loc = glGetUniformLocation(atmoShaderProgram, "Cy");
		glUniform1f(loc, Cy);
		loc = glGetUniformLocation(atmoShaderProgram, "Dy");
		glUniform1f(loc, Dy);
		loc = glGetUniformLocation(atmoShaderProgram, "Ey");
		glUniform1f(loc, Ey);

		loc = glGetUniformLocation(atmoShaderProgram, "projectionMatrix");
		glUniformMatrix4fv(loc, 1, false, (float*)sPainter.getProjector()->getProjectionMatrix());

		// Load the vertex array
		loc = glGetAttribLocation(atmoShaderProgram, "skyVertex");
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 2, GL_FLOAT, false, 0, posGrid);

		// Load the color components
		loc = glGetAttribLocation(atmoShaderProgram, "skyColor");
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 4, GL_FLOAT, false, 0, colorGrid);
#endif
	}
	else
	{
		// No shader is available on this graphics card, compute colors with the CPU
		// Adapt luminance at this point to avoid a mismatch with the adaptation value
		for (int i=0;i<(1+skyResolutionX)*(1+skyResolutionY);++i)
		{
			Vec4f& c = colorGrid[i];
			eye->xyYToRGB(c);
			c*=atm_intensity;
		}
#ifndef USE_OPENGL_ES2
		glShadeModel(GL_SMOOTH);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, colorGrid);
		glVertexPointer(2, GL_FLOAT, 0, posGrid);
#endif
	}

	// And draw everything at once
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	GLushort* shift=indices;
	for (int y=0;y<skyResolutionY;++y)
	{
		glDrawElements(GL_TRIANGLE_STRIP, (skyResolutionX+1)*2, GL_UNSIGNED_SHORT, shift);
		shift += (skyResolutionX+1)*2;
	}

	if (useShader)
	{
#if QT_VERSION>=0x040600
		atmoShaderProgram->disable();
#else
		glUseProgram(0);
#endif
	}
	else
	{
#ifndef USE_OPENGL_ES2
		glShadeModel(GL_FLAT);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
#endif
	}
}
