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

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QSettings>

#include "Atmosphere.hpp"
#include "renderer/StelRenderer.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"

inline bool myisnan(double value)
{
	return value != value;
}

Atmosphere::Atmosphere() 
	: viewport(0,0,0,0)
	, averageLuminance(0.f)
	, eclipseFactor(1.f)
	, lightPollutionLuminance(0)
	, shader(NULL)
	, vertexGrid(NULL)
	, renderer(NULL)
{
	setFadeDuration(1.5f);
}

Atmosphere::~Atmosphere(void)
{
	if(NULL != shader)     {delete shader;}
	if(NULL != vertexGrid) {delete vertexGrid;}
	foreach(StelIndexBuffer* buffer, rowIndices)
	{
		delete buffer;
	}
}


void Atmosphere::computeColor
	(double JD, Vec3d sunPos, Vec3d moonPos, float moonPhase, StelCore* core, float eclipseFac,
	 float latitude, float altitude, float temperature, float relativeHumidity)
{
	// We lazily initialize vertex buffer at the first draw, 
	// so we can only call this after that.
	// We also need a renderer reference (again lazily from draw()) to 
	// construct index buffers as they might change at every call to computeColor().
	if(NULL == renderer) {return;}

	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	if (viewport != prj->getViewport())
	{
		// The viewport changed: update the number of points in the grid
		updateGrid(prj);
	}

	eclipseFactor = eclipseFac;
	if(eclipseFac < 0.0001f)
		eclipseFactor = 0.0001f;

	// No need to calculate if not visible
	if (!fader.getInterstate())
	{
		averageLuminance = 0.001f + lightPollutionLuminance;
		return;
	}

	// Calculate the atmosphere RGB for each point of the grid
	if (myisnan(sunPos.length()))
		sunPos.set(0.,0.,-1.*AU);
	if (myisnan(moonPos.length()))
		moonPos.set(0.,0.,-1.*AU);

	sunPos.normalize();
	moonPos.normalize();

	float sun_pos[3];
	sun_pos[0] = sunPos[0];
	sun_pos[1] = sunPos[1];
	sun_pos[2] = sunPos[2];

	float moon_pos[3];
	moon_pos[0] = moonPos[0];
	moon_pos[1] = moonPos[1];
	moon_pos[2] = moonPos[2];

	sky.setParamsv(sun_pos, 5.f);

	skyb.setLocation(latitude * M_PI/180., altitude, temperature, relativeHumidity);
	skyb.setSunMoon(moon_pos[2], sun_pos[2]);

	// Calculate the date from the julian day.
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	skyb.setDate(year, month, moonPhase);

	// Variables used to compute the average sky luminance
	double sum_lum = 0.;

	Vec3d point(1., 0., 0.);
	skylightStruct2 b2;
	float lumi;

	vertexGrid->unlock();
	// Compute the sky color for every point above the ground
	for (int i=0; i<(1+skyResolutionX)*(1+skyResolutionY); ++i)
	{
		const Vec2f position = vertexGrid->getVertex(i).position;
		prj->unProject(position[0], position[1], point);

		Q_ASSERT(fabs(point.lengthSquared()-1.0) < 1e-10);

		if (point[2]<=0)
		{
			point[2] = -point[2];
			// The sky below the ground is the symmetric of the one above :
			// it looks nice and gives proper values for brightness estimation
		}

		// Use the Skybright.cpp 's models for brightness which gives better results.
		lumi = skyb.getLuminance(moon_pos[0]*point[0]+moon_pos[1]*point[1]+moon_pos[2]*point[2], 
								 sun_pos[0]*point[0]+sun_pos[1]*point[1]+sun_pos[2]*point[2],
		                         point[2]);
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

		Q_ASSERT_X(NULL != vertexGrid, Q_FUNC_INFO, 
		           "Vertex buffer not initialized when setting colors");

		// Now need to compute the xy part of the color component
		// This is done in a GLSL shader if possible
		if(NULL != shader)
		{
			// Store the back projected position + luminance in the input color to the shader
			const Vec4f color = Vec4f(point[0], point[1], point[2], lumi);
			vertexGrid->setVertex(i, Vertex(position, color));
			continue;
		}

		// Shaderless fallback
		if (lumi > 0.01)
		{
			// Use the Skylight model for the color
			b2.pos[0] = point[0];
			b2.pos[1] = point[1];
			b2.pos[2] = point[2];
			sky.getxyYValuev(b2);
		}
		else
		{
			// Too dark to see atmosphere color, don't bother computing it
			b2.color[0] = 0.25;
			b2.color[1] = 0.25;
		}

		const Vec4f color = Vec4f(b2.color[0], b2.color[1], lumi, 1.0f);
		vertexGrid->setVertex(i, Vertex(position, color));
	}
	vertexGrid->lock();

	// Update average luminance
	averageLuminance = sum_lum/((1+skyResolutionX)*(1+skyResolutionY));
}

void Atmosphere::draw(StelCore* core, StelRenderer* renderer)
{
	// Renderer is NULL at first draw call.
	// We load the shader, initialize vertex buffer and set the renderer.
	//
	// We don't actually draw anything at the first call - computeColor 
	// must be called to fill the vertex buffer with drawable data.
	if(NULL == this->renderer)
	{
		Q_ASSERT_X(NULL == shader && NULL == vertexGrid && rowIndices.size() == 0, 
		           Q_FUNC_INFO, "Shader and/or vertex/index buffer initialized at first draw call");
		if(renderer->isGLSLSupported())
		{
			if(!lazyLoadShader(renderer))
			{
				qWarning() << "Failed loading atmosphere shader. Falling back to CPU code path";
			}
		}

		vertexGrid = renderer->createVertexBuffer<Vertex>(PrimitiveType_TriangleStrip);

		this->renderer = renderer;
		return;
	}

	Q_ASSERT_X(NULL != vertexGrid && rowIndices.size() > 0, Q_FUNC_INFO, 
	           "Vertex and/or index buffer not initialized at first draw call");
	Q_ASSERT_X(vertexGrid->length() == (skyResolutionY + 1) * (skyResolutionX + 1),
	           Q_FUNC_INFO, "Vertex grid size doesn't match atmosphere resolution");
	Q_ASSERT_X(rowIndices.size() == skyResolutionY, Q_FUNC_INFO,
	           "Number of row index buffers doesn't match atmosphere resolution");

	if (StelApp::getInstance().getVisionModeNight())
	{
		return;
	}

	StelToneReproducer* eye = core->getToneReproducer();

	if (!fader.getInterstate())
	{
		return;
	}

	renderer->setBlendMode(BlendMode_Add);

	const float atm_intensity = fader.getInterstate();
	if(NULL != shader)
	{
		shader->bind();
		float a, b, c;
		eye->getShadersParams(a, b, c);

		shader->setUniformValue("alphaWaOverAlphaDa"                  , a);
		shader->setUniformValue("oneOverGamma"                        , b);
		shader->setUniformValue("term2TimesOneOverMaxdLpOneOverGamma" , c);
		shader->setUniformValue("brightnessScale"                     , atm_intensity);

		Vec3f sunPos;
		float term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey;
		sky.getShadersParams(sunPos, term_x, Ax, Bx, Cx, Dx, Ex, term_y, Ay, By, Cy, Dy, Ey);

		shader->setUniformValue("sunPos" , sunPos);
		shader->setUniformValue("term_x" , term_x);
		shader->setUniformValue("Ax"     , Ax);
		shader->setUniformValue("Bx"     , Bx);
		shader->setUniformValue("Cx"     , Cx);
		shader->setUniformValue("Dx"     , Dx);
		shader->setUniformValue("Ex"     , Ex);
		shader->setUniformValue("term_y" , term_y);
		shader->setUniformValue("Ay"     , Ay);
		shader->setUniformValue("By"     , By);
		shader->setUniformValue("Cy"     , Cy);
		shader->setUniformValue("Dy"     , Dy);
		shader->setUniformValue("Ey"     , Ey);

		const Mat4f& projectionMatrix = core->getProjection2d()->getProjectionMatrix();
		shader->setUniformValue("projectionMatrix", projectionMatrix);

		for(int row = 0; row < skyResolutionY; ++row)
		{
			renderer->drawVertexBuffer(vertexGrid, rowIndices[row]);
		}

		shader->release();
	}
	else
	{
		// No shader is available on this graphics card, compute colors on the CPU.
		// Adapt luminance at this point to avoid a mismatch with the adaptation value
		vertexGrid->unlock();
		for (int i = 0; i  < (1 + skyResolutionX) * (1 + skyResolutionY); ++i)
		{
			Vertex vertex = vertexGrid->getVertex(i);
			eye->xyYToRGB(vertex.color);
			vertex.color *= atm_intensity;
			vertexGrid->setVertex(i, vertex);
		}
		vertexGrid->lock();

		for(int row = 0; row < skyResolutionY; ++row)
		{
			renderer->drawVertexBuffer(vertexGrid, rowIndices[row]);
		}
	}
}

bool Atmosphere::lazyLoadShader(StelRenderer* renderer)
{
	qDebug() << "Using vertex shader for atmosphere rendering.";

	QFile vertexShaderFile(":/shaders/xyYToRGB.glsl");
	if(!vertexShaderFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Error opening shader file: ':/shaders/xyYToRGB.glsl'";
		return false;
	}
	shader = renderer->createGLSLShader();
	if(!shader->addVertexShader(QTextStream(&vertexShaderFile).readAll()))
	{
		qWarning() << "Error adding atmosphere vertex shader: " << shader->log();
		delete shader;
		shader = NULL;
		return false;
	}
	if(!shader->addFragmentShader("varying mediump vec4 resultSkyColor;\n"
	                              "void main()\n"
	                              "{\n"
	                              "   gl_FragColor = resultSkyColor;\n"
	                              "}"))
	{
		qWarning() << "Error adding atmosphere fragment shader: " << shader->log();
		delete shader;
		shader = NULL;
		return false;
	}

	if(!shader->build())
	{
		qWarning() << "Error building atmosphere shader: " << shader->log();
		delete shader;
		shader = NULL;
		return false;
	}

	if(!shader->log().isEmpty())
	{
		qDebug() << "Atmosphere shader build log: " << shader->log();
	}

	return true;
}

void Atmosphere::updateGrid(const StelProjectorP projector)
{
	viewport                   = projector->getViewport();
	const float viewportWidth  = projector->getViewportWidth();
	const float viewportHeight = projector->getViewportHeight();
	const float aspectRatio    = viewportWidth / viewportHeight;
	skyResolutionY             = StelApp::getInstance()
	                                     .getSettings()
	                                    ->value("landscape/atmosphereybin", 44)
	                                     .toInt();
	const float resolutionX    = skyResolutionY * 0.5 * sqrt(3.0) * aspectRatio;
	skyResolutionX             = static_cast<int>(floor(0.5 + resolutionX));
	const float stepX          = viewportWidth  / (skyResolutionX - 0.5);
	const float stepY          = viewportHeight / skyResolutionY;
	const float viewportLeft   = projector->getViewportPosX();
	const float viewportBottom = projector->getViewportPosY();

	vertexGrid->unlock();
	vertexGrid->clear();

	// Construct the vertex grid.
	for(int y = 0; y <= skyResolutionY; ++y)
	{
		const float yPos = viewportBottom + y * stepY;
		for (int x = 0; x <= skyResolutionX; ++x)
		{
			const float offset = (x == 0)              ? 0.0f :
			                     (x == skyResolutionX) ? viewportWidth
			                                           : (x - 0.5 * (y & 1)) * stepX;
			const float xPos = viewportLeft + offset;
			vertexGrid->addVertex(Vertex(Vec2f(xPos, yPos), Vec4f()));
		}
	}
	vertexGrid->lock();

	// The grid is (resolutionX + 1) * (resolutionY + 1),
	// so the rows are for 0 to resolutionY-1
	// The last row includes vertices in row resolutionY

	// Construct an index buffer for each row in the grid.
	for(int row = 0; row < skyResolutionY; ++row)
	{
		StelIndexBuffer* buffer; 
		// Reuse previously used row index buffer.
		if(rowIndices.size() > row)
		{
			buffer = rowIndices[row];
			buffer->unlock();
			buffer->clear();
		}
		// Add new row index buffer.
		else
		{
			buffer = renderer->createIndexBuffer(IndexType_U16);
			rowIndices.append(buffer);
		}

		uint g0 = row       * (1 + skyResolutionX);
		uint g1 = (row + 1) * (1 + skyResolutionX);
		for (int col = 0; col <= skyResolutionX; ++col)
		{
			buffer->addIndex(g0++);
			buffer->addIndex(g1++);
		}
		buffer->lock();

		Q_ASSERT_X(buffer->length() == (skyResolutionX + 1) * 2, Q_FUNC_INFO,
		           "Unexpected grid row index buffer size");
	}

	Q_ASSERT_X(rowIndices.size() >= skyResolutionY, Q_FUNC_INFO,
	           "Not enough row index buffers");
}
