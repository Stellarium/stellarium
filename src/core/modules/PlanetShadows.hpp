/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _PLANETSHADOWS_HPP_
#define _PLANETSHADOWS_HPP_

#include "StelTextureTypes.hpp"
#include "VecMath.hpp"

class QOpenGLShaderProgram;
class StelPainter;

class PlanetShadows
{
public:
	struct ShaderVars {
		int projectionMatrix;
		int texCoord;
		int unprojectedVertex;
		int vertex;
		int texture;

		int lightPos;
		int diffuseLight;
		int ambientLight;
		int radius;
		int oneMinusOblateness;
		int info;
		int infoCount;
		int infoSize;
		int current;
		int isRing;
		int ring;
		int outerRadius;
		int innerRadius;
		int ringS;
		int isMoon;
		int earthShadow;
	};

	static PlanetShadows* getInstance();
	static void cleanup();

	bool isSupported();
	bool isActive();

	void setData(const char* data, int size, int count);
	void setCurrent(int current);
	void setRings(StelTextureSP rings_texture, float min, float max);
	void setMoon(StelTextureSP moon_texture);

	void setupShading(StelPainter *sPainter, float radius, float oneMinusOblateness, bool isRing);
	QOpenGLShaderProgram* setupGeneralUniforms(const QMatrix4x4& projection);

	~PlanetShadows();

private:
	friend class StelPainter;
	static PlanetShadows* instance;

	bool supported;

	QOpenGLShaderProgram* shaderProgram;
	ShaderVars shaderVars;

	float rings_min;
	float rings_max;

	StelTextureSP texture;
	StelTextureSP rings_texture;
	StelTextureSP moon_texture;

	// int info; what's this for again? :D should be set to 1 in the beginning of rendering?!
	int current;
	int infoCount;
	float infoSize;

	PlanetShadows();
	void initShader();
};

#endif // _PLANETSHADOWS_HPP_
