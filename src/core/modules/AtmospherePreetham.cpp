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

#include "AtmospherePreetham.hpp"
#include "StelUtils.hpp"
#include "Planet.hpp"
#include "StelApp.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "Dithering.hpp"
#include "Skylight.hpp"

#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QOpenGLShaderProgram>


AtmospherePreetham::AtmospherePreetham(Skylight& sky)
	: viewport(0,0,0,0)
    , sky(sky)
	, skyResolutionY(44)
	, skyResolutionX(44)
	, posGrid(Q_NULLPTR)
	, posGridBuffer(QOpenGLBuffer::VertexBuffer)
	, indicesBuffer(QOpenGLBuffer::IndexBuffer)
	, colorGrid(Q_NULLPTR)
	, colorGridBuffer(QOpenGLBuffer::VertexBuffer)
{
	setFadeDuration(1.5f);

	QOpenGLShader vShader(QOpenGLShader::Vertex);
	{
		QFile vert(":/shaders/preethamAtmosphere.vert");
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
		qWarning() << "Warnings while compiling Preetham atmosphere vertex shader: " << vShader.log();
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
		qFatal("Error while compiling Preetham atmosphere fragment shader: %s", fShader.log().toLatin1().constData());
	}
	if (!fShader.log().isEmpty())
	{
		qWarning() << "Warnings while compiling Preetham atmosphere fragment shader: " << vShader.log();
	}
	atmoShaderProgram = new QOpenGLShaderProgram();
	atmoShaderProgram->addShader(&vShader);
	atmoShaderProgram->addShader(&fShader);
	StelPainter::linkProg(atmoShaderProgram, "Preetham atmosphere");

	GL(atmoShaderProgram->bind());
	GL(shaderAttribLocations.bayerPattern = atmoShaderProgram->uniformLocation("bayerPattern"));
	GL(shaderAttribLocations.rgbMaxValue = atmoShaderProgram->uniformLocation("rgbMaxValue"));
	GL(shaderAttribLocations.alphaWaOverAlphaDa = atmoShaderProgram->uniformLocation("alphaWaOverAlphaDa"));
	GL(shaderAttribLocations.oneOverGamma = atmoShaderProgram->uniformLocation("oneOverGamma"));
	GL(shaderAttribLocations.term2TimesOneOverMaxdLpOneOverGamma = atmoShaderProgram->uniformLocation("term2TimesOneOverMaxdLpOneOverGamma"));
	GL(shaderAttribLocations.term2TimesOneOverMaxdL = atmoShaderProgram->uniformLocation("term2TimesOneOverMaxdL"));
	GL(shaderAttribLocations.flagUseTmGamma = atmoShaderProgram->uniformLocation("flagUseTmGamma"));
	GL(shaderAttribLocations.brightnessScale = atmoShaderProgram->uniformLocation("brightnessScale"));
	GL(shaderAttribLocations.sunPos = atmoShaderProgram->uniformLocation("sunPos"));
	GL(shaderAttribLocations.term_x = atmoShaderProgram->uniformLocation("term_x"));
	GL(shaderAttribLocations.Ax = atmoShaderProgram->uniformLocation("Ax"));
	GL(shaderAttribLocations.Bx = atmoShaderProgram->uniformLocation("Bx"));
	GL(shaderAttribLocations.Cx = atmoShaderProgram->uniformLocation("Cx"));
	GL(shaderAttribLocations.Dx = atmoShaderProgram->uniformLocation("Dx"));
	GL(shaderAttribLocations.Ex = atmoShaderProgram->uniformLocation("Ex"));
	GL(shaderAttribLocations.term_y = atmoShaderProgram->uniformLocation("term_y"));
	GL(shaderAttribLocations.Ay = atmoShaderProgram->uniformLocation("Ay"));
	GL(shaderAttribLocations.By = atmoShaderProgram->uniformLocation("By"));
	GL(shaderAttribLocations.Cy = atmoShaderProgram->uniformLocation("Cy"));
	GL(shaderAttribLocations.Dy = atmoShaderProgram->uniformLocation("Dy"));
	GL(shaderAttribLocations.Ey = atmoShaderProgram->uniformLocation("Ey"));
	GL(shaderAttribLocations.doSRGB = atmoShaderProgram->uniformLocation("doSRGB"));
	GL(shaderAttribLocations.projectionMatrix = atmoShaderProgram->uniformLocation("projectionMatrix"));
	GL(shaderAttribLocations.skyVertex = atmoShaderProgram->attributeLocation("skyVertex"));
	GL(shaderAttribLocations.skyColor = atmoShaderProgram->attributeLocation("skyColor"));
	GL(atmoShaderProgram->release());
}

AtmospherePreetham::~AtmospherePreetham(void)
{
	delete [] posGrid;
	posGrid = Q_NULLPTR;
	delete[] colorGrid;
	colorGrid = Q_NULLPTR;
	delete atmoShaderProgram;
	atmoShaderProgram = Q_NULLPTR;
}

void AtmospherePreetham::computeColor(StelCore* core, const double JD, const Planet& currentPlanet, const Planet& sun, const Planet*const moon,
									  const StelLocation& location, const float temperature, const float relativeHumidity,
									  const float extinctionCoefficient, const bool noScatter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	if (viewport != prj->getViewport())
	{
		// The viewport changed: update the number of point of the grid
		viewport = prj->getViewport();
		delete[] colorGrid;
		delete [] posGrid;
		skyResolutionY = StelApp::getInstance().getSettings()->value("landscape/atmosphereybin", 44).toUInt();
		skyResolutionX = static_cast<unsigned int>(floorf(0.5f+static_cast<float>(skyResolutionY)*(0.5f*sqrtf(3.0f))*static_cast<float>(prj->getViewportWidth())/static_cast<float>(prj->getViewportHeight())));
		posGrid = new Vec2f[static_cast<size_t>((1+skyResolutionX)*(1+skyResolutionY))];
		colorGrid = new Vec4f[static_cast<size_t>((1+skyResolutionX)*(1+skyResolutionY))];
		float stepX = static_cast<float>(prj->getViewportWidth()) / (static_cast<float>(skyResolutionX)-0.5f);
		float stepY = static_cast<float>(prj->getViewportHeight()) / static_cast<float>(skyResolutionY);
		float viewport_left = static_cast<float>(prj->getViewportPosX());
		float viewport_bottom = static_cast<float>(prj->getViewportPosY());
		for (unsigned int x=0; x<=skyResolutionX; ++x)
		{
			for(unsigned int y=0; y<=skyResolutionY; ++y)
			{
				Vec2f &v(posGrid[y*(1+skyResolutionX)+x]);
				v[0] = viewport_left + ((x == 0) ? 0.f :
						(x == skyResolutionX) ? static_cast<float>(prj->getViewportWidth()) : (static_cast<float>(x)-0.5f*(y&1))*stepX);
				v[1] = viewport_bottom+static_cast<float>(y)*stepY;
			}
		}
		posGridBuffer.destroy();
		//posGridBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		Q_ASSERT(posGridBuffer.type()==QOpenGLBuffer::VertexBuffer);
		posGridBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		posGridBuffer.create();
		posGridBuffer.bind();
		posGridBuffer.allocate(posGrid, static_cast<int>((1+skyResolutionX)*(1+skyResolutionY))*8);
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

	auto sunPos  =  sun.getAltAzPosAuto(core);
	if (qIsNaN(sunPos.length()))
		sunPos.set(0.,0.,-1.*AU);

	Vec3d moonPos = sunPos;
	// Calculate the atmosphere RGB for each point of the grid. We can use abbreviated numbers here.

	// qDebug("touch at %f\tnow at %f (%f)\n", touch_angle, separation_angle, separation_angle/touch_angle);
	// bright stars should be visible at total eclipse
	// TODO: correct for atmospheric diffusion
	// TODO: use better coverage function (non-linear)
	// because of above issues, this algorithm darkens more quickly than reality
	if (moon)
	{
		moonPos = moon->getAltAzPosAuto(core);
		if (qIsNaN(moonPos.length()))
			moonPos.set(0.,0.,-1.*AU);

		// Update the eclipse intensity factor to apply on atmosphere model
		// these are for radii
		const double sun_angular_radius = atan(sun.getEquatorialRadius()/sunPos.length());
		const double moon_angular_radius = atan(moon->getEquatorialRadius()/moonPos.length());
		const double touch_angle = sun_angular_radius + moon_angular_radius;

		// determine luminance falloff during solar eclipses
		sunPos.normalize();
		moonPos.normalize();
		double separation_angle = std::acos(sunPos.dot(moonPos));  // angle between them

		if(separation_angle < touch_angle)
		{
			double dark_angle = moon_angular_radius - sun_angular_radius;
			double min = 0.0025; // 0.005f; // 0.0001f;  // so bright stars show up at total eclipse
			if (dark_angle < 0.)
			{
				// annular eclipse
				double asun = sun_angular_radius*sun_angular_radius;
				min = (asun - moon_angular_radius*moon_angular_radius)/asun;  // minimum proportion of sun uncovered
				dark_angle *= -1;
			}

			if (separation_angle < dark_angle)
				eclipseFactor = static_cast<float>(min);
			else
				eclipseFactor = static_cast<float>(min + (1.0-min)*(separation_angle-dark_angle)/(touch_angle-dark_angle));
		}
		else
			eclipseFactor = 1.0f;
	}
	else
	{
		eclipseFactor = 1.f;
		sunPos.normalize();
	}
	// TODO: compute eclipse factor also for Lunar eclipses! (lp:#1471546)

	// No need to calculate if not visible
	if (!fader.getInterstate())
	{
		// GZ 20180114: Why did we add light pollution if atmosphere was not visible?????
		// And what is the meaning of 0.001? Approximate contribution of stellar background? Then why is it 0.0001 below???
		averageLuminance = 0.001f;
		return;
	}

	// Calculate the atmosphere RGB for each point of the grid

	// GZ: This used a constant Preetham Turbidity of 5 which is already quite hazy.
	// We can work with the k value set in the atmosphere/extinction settings.
	// Given that T=1 = Pure Air where k=0.16, and other values assumed in parallel, we come to an
	// ad-hoc mapping function which should not look to bad: T=25(k-0.16)+1
	//sky.setParamsv(sunPos, 5.f); // To reach the 5 we have k=0.32
	StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();

	float turbidity= ( (skyDrawer->getFlagTfromK()) ?  25.f*(extinctionCoefficient-0.16f)+1.f   : static_cast<float>(skyDrawer->getT()));
	// Note that Preetham's model has some quirks for too low turbidities.
	// A hard limit is some assert later, so must be technically at least 1.203.
	// Also, Preetham has optimized to T in[2..6], which translates now to k in [0.2-0.36].
	// In Schaefer-Krisciunas Moon brightness paper, k=0.172 for Mauna Kea. Preetham will likely be too bright here.
	//sky.setParamsv(sunPos, qBound(2.f, turbidity, 6.f));
	Vec3f sunPosF=sunPos.toVec3f();
	Vec3f moonPosF=moonPos.toVec3f();
	sky.setParamsv(sunPosF, qBound(2.f, turbidity, 16.f));  // GZ-AT allow more turbidity for testing

	skyb.setLocation(location.latitude * M_PI_180f, static_cast<float>(location.altitude), temperature, relativeHumidity);
	skyb.setSunMoon(moonPosF[2], sunPosF[2]);

	// Calculate the date from the julian day.
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	const auto moonPhase = moon ? moon->getPhaseAngle(currentPlanet.getHeliocentricEclipticPos()) : 0.;
	const auto moonMagnitude = moon ? moon->getVMagnitudeWithExtinction(core) : 100.f;
	skyb.setDate(year, month, static_cast<float>(moonPhase), moonMagnitude);

	// Variables used to compute the average sky luminance
	float sum_lum = 0.f;

	Vec3d point(1., 0., 0.);
	float lumi=0.f;

	skylightStruct2 skylight;
	// Compute the sky color for every point above the ground
	for (unsigned int i=0; i<(1+skyResolutionX)*(1+skyResolutionY); ++i)
	{
		const Vec2f &v(posGrid[i]);
		prj->unProject(static_cast<double>(v[0]),static_cast<double>(v[1]),point);

		Q_ASSERT(fabs(point.lengthSquared()-1.0) < 1e-10);

		Vec3f pointF=point.toVec3f();
		if (!noScatter)
		{
			// Use mirroring for sun only
			if (pointF[2]<=0.f)
			{
				pointF[2] *= -1.f;
				// The sky below the ground is the symmetric of the one above :
				// it looks nice and gives proper values for brightness estimation
				// Use the Skybright.cpp 's models for brightness which gives better results.
			}
			if (sky.getFlagSchaefer())
			{
				lumi = skyb.getLuminance(moonPosF[0]*pointF[0]+moonPosF[1]*pointF[1]+moonPosF[2]*pointF[2],
							 sunPosF[0] *pointF[0]+sunPosF[1] *pointF[1]+sunPosF[2] *pointF[2],
							 pointF[2]);
			}
			else // Experimental: Re-allow CIE/Preetham brightness instead.
			{
				skylight.pos[0]=pointF.v[0];
				skylight.pos[1]=pointF.v[1];
				skylight.pos[2]=pointF.v[2];
				sky.getxyYValuev(skylight);
				lumi=skylight.color[2];
			}
		}

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

		// No need to compute the xy part of the color component
		// This is done in the openGL shader
		// Store the back projected position + luminance in the input color to the shader
		colorGrid[i].set(pointF[0], pointF[1], pointF[2], lumi);
	}
	
	colorGridBuffer.bind();
	colorGridBuffer.write(0, colorGrid, static_cast<int>((1+skyResolutionX)*(1+skyResolutionY)*4*4));
	colorGridBuffer.release();
	
	// Update average luminance
	if (!overrideAverageLuminance)
		averageLuminance = sum_lum/static_cast<float>((1+skyResolutionX)*(1+skyResolutionY));
}

// Draw the atmosphere using the precalc values stored in tab_sky
void AtmospherePreetham::draw(StelCore* core)
{
	if (StelApp::getInstance().getVisionModeNight())
		return;

	const float atm_intensity = fader.getInterstate();
	if (atm_intensity==0.f)
	    return;

	StelToneReproducer* eye = core->getToneReproducer();

	StelPainter sPainter(core->getProjection2d());
	sPainter.setBlending(true, GL_ONE, GL_ONE);

	GL(atmoShaderProgram->bind());
	float a, b, c, d;
	bool useTmGamma, sRGB;
	eye->getShadersParams(a, b, c, d, useTmGamma, sRGB);

	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.alphaWaOverAlphaDa, a));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.oneOverGamma, b));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.term2TimesOneOverMaxdLpOneOverGamma, c));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.term2TimesOneOverMaxdL, d));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.flagUseTmGamma, useTmGamma));

	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.brightnessScale, atm_intensity));
	Vec3f sunPos;
	float term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey;
	sky.getShadersParams(sunPos, term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey);
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.sunPos, sunPos[0], sunPos[1], sunPos[2]));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.term_x, term_x));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Ax, Ax));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Bx, Bx));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Cx, Cx));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Dx, Dx));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Ex, Ex));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.term_y, term_y));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Ay, Ay));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.By, By));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Cy, Cy));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Dy, Dy));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.Ey, Ey));
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.doSRGB, sRGB));
	const Mat4f& m = sPainter.getProjector()->getProjectionMatrix();
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.projectionMatrix,
					      QMatrix4x4(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15])));

	const auto rgbMaxValue=calcRGBMaxValue(sPainter.getDitheringMode());
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.rgbMaxValue, rgbMaxValue[0], rgbMaxValue[1], rgbMaxValue[2]));
	auto& gl=*sPainter.glFuncs();
	gl.glActiveTexture(GL_TEXTURE1);
	if(!bayerPatternTex)
		bayerPatternTex=makeBayerPatternTexture(*sPainter.glFuncs());
	gl.glBindTexture(GL_TEXTURE_2D, bayerPatternTex);
	GL(atmoShaderProgram->setUniformValue(shaderAttribLocations.bayerPattern, 1));
	
	GL(colorGridBuffer.bind());
	GL(atmoShaderProgram->setAttributeBuffer(shaderAttribLocations.skyColor, GL_FLOAT, 0, 4, 0));
	GL(colorGridBuffer.release());
	GL(atmoShaderProgram->enableAttributeArray(shaderAttribLocations.skyColor));
	GL(posGridBuffer.bind());
	GL(atmoShaderProgram->setAttributeBuffer(shaderAttribLocations.skyVertex, GL_FLOAT, 0, 2, 0));
	GL(posGridBuffer.release());
	GL(atmoShaderProgram->enableAttributeArray(shaderAttribLocations.skyVertex));

	// And draw everything at once
	GL(indicesBuffer.bind());
	std::size_t shift=0;
	for (unsigned int y=0;y<skyResolutionY;++y)
	{
		sPainter.glFuncs()->glDrawElements(GL_TRIANGLE_STRIP, static_cast<int>((skyResolutionX+1)*2), GL_UNSIGNED_SHORT, reinterpret_cast<void*>(shift));
		shift += static_cast<size_t>((skyResolutionX+1)*2*2);
	}
	GL(indicesBuffer.release());
	
	GL(atmoShaderProgram->disableAttributeArray(shaderAttribLocations.skyVertex));
	GL(atmoShaderProgram->disableAttributeArray(shaderAttribLocations.skyColor));
	GL(atmoShaderProgram->release());
	// debug output
	// sPainter.setColor(0.7f, 0.7f, 0.7f);
	// sPainter.drawText(83, 108, QString("Tonemapper::worldAdaptationLuminance(): %1" ).arg(eye->getWorldAdaptationLuminance()));
	// sPainter.drawText(83, 120, QString("AtmospherePreetham::getAverageLuminance(): %1" ).arg(getAverageLuminance()));
}
