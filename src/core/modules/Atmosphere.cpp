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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "Atmosphere.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelFileMgr.hpp"
#include "Dithering.hpp"

#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QOpenGLShaderProgram>


Atmosphere::Atmosphere(void)
	: viewport(0,0,0,0)
	, skyResolutionY(44)
	, skyResolutionX(44)
	, posGrid(Q_NULLPTR)
	, posGridBuffer(QOpenGLBuffer::VertexBuffer)
	, indicesBuffer(QOpenGLBuffer::IndexBuffer)
	, colorGrid(Q_NULLPTR)
	, colorGridBuffer(QOpenGLBuffer::VertexBuffer)
	, averageLuminance(0.f)
	, overrideAverageLuminance(false)
	, eclipseFactor(1.f)
	, lightPollutionLuminance(0)
{
	setFadeDuration(1.5f);

	QOpenGLShader vShader(QOpenGLShader::Vertex);
	{
		QFile vert(":/shaders/atmosphere.vert");
		if(!vert.open(QFile::ReadOnly))
			qFatal("Failed to open atmosphere vertex shader source");
		QFile toneRepro(":/shaders/xyYToRGB.glsl");
		if(!toneRepro.open(QFile::ReadOnly))
			qFatal("Failed to open ToneReproducer shader source");
		if (!vShader.compileSourceCode(vert.readAll()+toneRepro.readAll()))
			qFatal("Error while compiling atmosphere vertex shader: %s", vShader.log().toLatin1().constData());
	}
	if (!vShader.log().isEmpty())
	{
		qWarning() << "Warnings while compiling atmosphere vertex shader: " << vShader.log();
	}
	QOpenGLShader fShader(QOpenGLShader::Fragment);
	if (!fShader.compileSourceCode(
					makeDitheringShader()+
					"varying mediump vec3 resultSkyColor;\n"
					"void main()\n"
					"{\n"
					 "   gl_FragColor = vec4(dither(resultSkyColor), 1.);\n"
					 "}"))
	{
		qFatal("Error while compiling atmosphere fragment shader: %s", fShader.log().toLatin1().constData());
	}
	if (!fShader.log().isEmpty())
	{
		qWarning() << "Warnings while compiling atmosphere fragment shader: " << vShader.log();
	}
	atmoShaderProgram = new QOpenGLShaderProgram();
	atmoShaderProgram->addShader(&vShader);
	atmoShaderProgram->addShader(&fShader);
	StelPainter::linkProg(atmoShaderProgram, "atmosphere");

	atmoShaderProgram->bind();
	shaderAttribLocations.bayerPattern = atmoShaderProgram->uniformLocation("bayerPattern");
	shaderAttribLocations.rgbMaxValue = atmoShaderProgram->uniformLocation("rgbMaxValue");
	shaderAttribLocations.alphaWaOverAlphaDa = atmoShaderProgram->uniformLocation("alphaWaOverAlphaDa");
	shaderAttribLocations.oneOverGamma = atmoShaderProgram->uniformLocation("oneOverGamma");
	shaderAttribLocations.term2TimesOneOverMaxdLpOneOverGamma = atmoShaderProgram->uniformLocation("term2TimesOneOverMaxdLpOneOverGamma");
	shaderAttribLocations.brightnessScale = atmoShaderProgram->uniformLocation("brightnessScale");
	shaderAttribLocations.sunPos = atmoShaderProgram->uniformLocation("sunPos");
	shaderAttribLocations.term_x = atmoShaderProgram->uniformLocation("term_x");
	shaderAttribLocations.Ax = atmoShaderProgram->uniformLocation("Ax");
	shaderAttribLocations.Bx = atmoShaderProgram->uniformLocation("Bx");
	shaderAttribLocations.Cx = atmoShaderProgram->uniformLocation("Cx");
	shaderAttribLocations.Dx = atmoShaderProgram->uniformLocation("Dx");
	shaderAttribLocations.Ex = atmoShaderProgram->uniformLocation("Ex");
	shaderAttribLocations.term_y = atmoShaderProgram->uniformLocation("term_y");
	shaderAttribLocations.Ay = atmoShaderProgram->uniformLocation("Ay");
	shaderAttribLocations.By = atmoShaderProgram->uniformLocation("By");
	shaderAttribLocations.Cy = atmoShaderProgram->uniformLocation("Cy");
	shaderAttribLocations.Dy = atmoShaderProgram->uniformLocation("Dy");
	shaderAttribLocations.Ey = atmoShaderProgram->uniformLocation("Ey");
	shaderAttribLocations.projectionMatrix = atmoShaderProgram->uniformLocation("projectionMatrix");
	shaderAttribLocations.skyVertex = atmoShaderProgram->attributeLocation("skyVertex");
	shaderAttribLocations.skyColor = atmoShaderProgram->attributeLocation("skyColor");
	atmoShaderProgram->release();
}

Atmosphere::~Atmosphere(void)
{
	delete [] posGrid;
	posGrid = Q_NULLPTR;
	delete[] colorGrid;
	colorGrid = Q_NULLPTR;
	delete atmoShaderProgram;
	atmoShaderProgram = Q_NULLPTR;
}

void Atmosphere::computeColor(double JD, Vec3d _sunPos, Vec3d moonPos, float moonPhase, float moonMagnitude,
							   StelCore* core, float latitude, float altitude, float temperature, float relativeHumidity)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	if (viewport != prj->getViewport())
	{
		// The viewport changed: update the number of point of the grid
		viewport = prj->getViewport();
		delete[] colorGrid;
		delete [] posGrid;
		skyResolutionY = StelApp::getInstance().getSettings()->value("landscape/atmosphereybin", 44).toUInt();
		skyResolutionX = static_cast<unsigned int>(floorf(0.5f+skyResolutionY*(0.5f*sqrtf(3.0f))*prj->getViewportWidth()/prj->getViewportHeight()));
		posGrid = new Vec2f[static_cast<size_t>((1+skyResolutionX)*(1+skyResolutionY))];
		colorGrid = new Vec4f[static_cast<size_t>((1+skyResolutionX)*(1+skyResolutionY))];
		float stepX = static_cast<float>(prj->getViewportWidth()) / static_cast<float>(skyResolutionX-0.5f);
		float stepY = static_cast<float>(prj->getViewportHeight()) / skyResolutionY;
		float viewport_left = prj->getViewportPosX();
		float viewport_bottom = prj->getViewportPosY();
		for (unsigned int x=0; x<=skyResolutionX; ++x)
		{
			for(unsigned int y=0; y<=skyResolutionY; ++y)
			{
				Vec2f &v(posGrid[y*(1+skyResolutionX)+x]);
				v[0] = viewport_left + ((x == 0) ? 0.f :
						(x == skyResolutionX) ? prj->getViewportWidth() : (x-0.5f*(y&1))*stepX);
				v[1] = viewport_bottom+y*stepY;
			}
		}
		posGridBuffer.destroy();
		//posGridBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		Q_ASSERT(posGridBuffer.type()==QOpenGLBuffer::VertexBuffer);
		posGridBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		posGridBuffer.create();
		posGridBuffer.bind();
		posGridBuffer.allocate(posGrid, (1+skyResolutionX)*(1+skyResolutionY)*8);
		posGridBuffer.release();
		
		// Generate the indices used to draw the quads
		unsigned short* indices = new unsigned short[static_cast<size_t>((skyResolutionX+1)*skyResolutionY*2)];
		int i=0;
		for (unsigned int y2=0; y2<skyResolutionY; ++y2)
		{
			unsigned short g0 = static_cast<unsigned short>(y2*(1+skyResolutionX));
			unsigned short g1 = static_cast<unsigned short>((y2+1)*(1+skyResolutionX));
			for (unsigned int x2=0; x2<=skyResolutionX; ++x2)
			{
				indices[i++]=g0++;
				indices[i++]=g1++;
			}
		}
		indicesBuffer.destroy();
		//indicesBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
		Q_ASSERT(indicesBuffer.type()==QOpenGLBuffer::IndexBuffer);
		indicesBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		indicesBuffer.create();
		indicesBuffer.bind();
		indicesBuffer.allocate(indices, static_cast<int>((skyResolutionX+1)*skyResolutionY*2*2));
		indicesBuffer.release();
		delete[] indices;
		indices=Q_NULLPTR;
		
		colorGridBuffer.destroy();
		colorGridBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
		colorGridBuffer.create();
		colorGridBuffer.bind();
		colorGridBuffer.allocate(colorGrid, static_cast<int>((1+skyResolutionX)*(1+skyResolutionY)*4*4));
		colorGridBuffer.release();
	}

	if (qIsNaN(_sunPos.length()))
		_sunPos.set(0.,0.,-1.*AU);
	if (qIsNaN(moonPos.length()))
		moonPos.set(0.,0.,-1.*AU);

	// Update the eclipse intensity factor to apply on atmosphere model
	// these are for radii
	const double sun_angular_size = atan(696000./AU/_sunPos.length());
	const double moon_angular_size = atan(1738./AU/moonPos.length());
	const double touch_angle = sun_angular_size + moon_angular_size;

	// determine luminance falloff during solar eclipses
	_sunPos.normalize();
	moonPos.normalize();
	// Calculate the atmosphere RGB for each point of the grid. We can use abbreviated numbers here.
	Vec3f sunPosF=_sunPos.toVec3f();
	Vec3f moonPosF=moonPos.toVec3f();

	double separation_angle = std::acos(_sunPos.dot(moonPos));  // angle between them
	// qDebug("touch at %f\tnow at %f (%f)\n", touch_angle, separation_angle, separation_angle/touch_angle);
	// bright stars should be visible at total eclipse
	// TODO: correct for atmospheric diffusion
	// TODO: use better coverage function (non-linear)
	// because of above issues, this algorithm darkens more quickly than reality
	// Note: On Earth only, else moon would brighten other planets' atmospheres (LP:1673283)
	if ((core->getCurrentLocation().planetName=="Earth") && (separation_angle < touch_angle))
	{
		double dark_angle = moon_angular_size - sun_angular_size;
		double min = 0.0025; // 0.005f; // 0.0001f;  // so bright stars show up at total eclipse
		if (dark_angle < 0.)
		{
			// annular eclipse
			double asun = sun_angular_size*sun_angular_size;
			min = (asun - moon_angular_size*moon_angular_size)/asun;  // minimum proportion of sun uncovered
			dark_angle *= -1;
		}

		if (separation_angle < dark_angle)
			eclipseFactor = static_cast<float>(min);
		else
			eclipseFactor = static_cast<float>(min + (1.0-min)*(separation_angle-dark_angle)/(touch_angle-dark_angle));
	}
	else
		eclipseFactor = 1.f;
	// TODO: compute eclipse factor also for Lunar eclipses! (lp:#1471546)

	// No need to calculate if not visible
	if (!fader.getInterstate())
	{
		// GZ 20180114: Why did we add light pollution if atmosphere was not visible?????
		// And what is the meaning of 0.001? Approximate contribution of stellar background? Then why is it 0.0001 below???
		averageLuminance = 0.001f;
		return;
	}

	sky.setParamsv(sunPosF, 5.f);
	skyb.setLocation(latitude * M_PI_180f, altitude, temperature, relativeHumidity);
	skyb.setSunMoon(moonPosF[2], sunPosF[2]);

	// Calculate the date from the julian day.
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	skyb.setDate(year, month, moonPhase, moonMagnitude);

	// Variables used to compute the average sky luminance
	float sum_lum = 0.f;

	Vec3d point(1., 0., 0.);
	float lumi;

	// Compute the sky color for every point above the ground
	for (unsigned int i=0; i<(1+skyResolutionX)*(1+skyResolutionY); ++i)
	{
		const Vec2f &v(posGrid[i]);
		prj->unProject(static_cast<double>(v[0]),static_cast<double>(v[1]),point);

		Q_ASSERT(fabs(point.lengthSquared()-1.0) < 1e-10);

		Vec3f pointF=point.toVec3f();
		// Use mirroring for sun only
		if (pointF[2]<=0.f)
		{
			pointF[2] *= -1.f;
			// The sky below the ground is the symmetric of the one above :
			// it looks nice and gives proper values for brightness estimation
			// Use the Skybright.cpp 's models for brightness which gives better results.
		}
		lumi = skyb.getLuminance(moonPosF[0]*pointF[0]+moonPosF[1]*pointF[1]+
				moonPosF[2]*pointF[2], sunPosF[0]*pointF[0]+sunPosF[1]*pointF[1]+
				sunPosF[2]*pointF[2], pointF[2]);
		lumi *= eclipseFactor;
		// Add star background luminance
		lumi += 0.0001f;
		// Multiply by the input scale of the ToneConverter (is not done automatically by the xyYtoRGB method called later)
		//lumi*=eye->getInputScale();

		// Add the light pollution luminance AFTER the scaling to avoid scaling it because it is the cause
		// of the scaling itself
		lumi += fader.getInterstate()*lightPollutionLuminance;

		// Store for later statistics
		sum_lum+=lumi;

		// Now need to compute the xy part of the color component
		// This is done in the openGL shader
		// Store the back projected position + luminance in the input color to the shader
		colorGrid[i].set(pointF[0], pointF[1], pointF[2], lumi);
	}
	
	colorGridBuffer.bind();
	colorGridBuffer.write(0, colorGrid, static_cast<int>((1+skyResolutionX)*(1+skyResolutionY)*4*4));
	colorGridBuffer.release();
	
	// Update average luminance
	if (!overrideAverageLuminance)
		averageLuminance = sum_lum/((1+skyResolutionX)*(1+skyResolutionY));
}

// override computable luminance. This is for special operations only, e.g. for scripting of brightness-balanced image export.
// To return to auto-computed values, set any negative value.
void Atmosphere::setAverageLuminance(float overrideLum)
{
	if (overrideLum<0.f)
	{
		overrideAverageLuminance=false;
		averageLuminance=0.f;
	}
	else
	{
		overrideAverageLuminance=true;
		averageLuminance=overrideLum;
	}
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
	sPainter.setBlending(true, GL_ONE, GL_ONE);

	const float atm_intensity = fader.getInterstate();

	atmoShaderProgram->bind();
	float a, b, c;
	eye->getShadersParams(a, b, c);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.alphaWaOverAlphaDa, a);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.oneOverGamma, b);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.term2TimesOneOverMaxdLpOneOverGamma, c);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.brightnessScale, atm_intensity);
	Vec3f sunPos;
	float term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey;
	sky.getShadersParams(sunPos, term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.sunPos, sunPos[0], sunPos[1], sunPos[2]);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.term_x, term_x);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Ax, Ax);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Bx, Bx);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Cx, Cx);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Dx, Dx);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Ex, Ex);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.term_y, term_y);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Ay, Ay);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.By, By);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Cy, Cy);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Dy, Dy);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.Ey, Ey);
	const Mat4f& m = sPainter.getProjector()->getProjectionMatrix();
	atmoShaderProgram->setUniformValue(shaderAttribLocations.projectionMatrix,
		QMatrix4x4(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]));

	const auto rgbMaxValue=calcRGBMaxValue(sPainter.getDitheringMode());
	atmoShaderProgram->setUniformValue(shaderAttribLocations.rgbMaxValue, rgbMaxValue[0], rgbMaxValue[1], rgbMaxValue[2]);
	auto& gl=*sPainter.glFuncs();
	gl.glActiveTexture(GL_TEXTURE1);
	if(!bayerPatternTex)
		bayerPatternTex=makeBayerPatternTexture(*sPainter.glFuncs());
	gl.glBindTexture(GL_TEXTURE_2D, bayerPatternTex);
	atmoShaderProgram->setUniformValue(shaderAttribLocations.bayerPattern, 1);
	
	colorGridBuffer.bind();
	atmoShaderProgram->setAttributeBuffer(shaderAttribLocations.skyColor, GL_FLOAT, 0, 4, 0);
	colorGridBuffer.release();
	atmoShaderProgram->enableAttributeArray(shaderAttribLocations.skyColor);
	posGridBuffer.bind();
	atmoShaderProgram->setAttributeBuffer(shaderAttribLocations.skyVertex, GL_FLOAT, 0, 2, 0);
	posGridBuffer.release();
	atmoShaderProgram->enableAttributeArray(shaderAttribLocations.skyVertex);

	// And draw everything at once
	indicesBuffer.bind();
	std::size_t shift=0;
	for (unsigned int y=0;y<skyResolutionY;++y)
	{
		sPainter.glFuncs()->glDrawElements(GL_TRIANGLE_STRIP, static_cast<int>((skyResolutionX+1)*2), GL_UNSIGNED_SHORT, reinterpret_cast<void*>(shift));
		shift += static_cast<size_t>((skyResolutionX+1)*2*2);
	}
	indicesBuffer.release();
	
	atmoShaderProgram->disableAttributeArray(shaderAttribLocations.skyVertex);
	atmoShaderProgram->disableAttributeArray(shaderAttribLocations.skyColor);
	atmoShaderProgram->release();
	// GZ: debug output
	//const StelProjectorP prj = core->getProjection(StelCore::FrameEquinoxEqu);
	//StelPainter painter(prj);
	//painter.setFont(font);
	//sPainter.setColor(0.7, 0.7, 0.7);
	//sPainter.drawText(83, 120, QString("Atmosphere::getAverageLuminance(): %1" ).arg(getAverageLuminance()));
	//qDebug() << atmosphere->getAverageLuminance();
}
