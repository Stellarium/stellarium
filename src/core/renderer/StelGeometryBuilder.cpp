/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#include "StelGeometryBuilder.hpp"

#include "StelRenderer.hpp"


//TODO These lookup tables and their generation function need documentation

static const int MAX_SLICES = 4096;
static float COS_SIN_THETA[2 * (MAX_SLICES + 1)];
static const int MAX_STACKS = 4096;
static float COS_SIN_RHO[2 * (MAX_STACKS + 1)];

static void computeCosSinTheta(const float phi, const int segments)
{
	float *cosSin        = COS_SIN_THETA;
	float *cosSinReverse = cosSin + 2 * (segments + 1);

	const float c = std::cos(phi);
	const float s = std::sin(phi);

	*cosSin++ = 1.0f;
	*cosSin++ = 0.0f;
	*--cosSinReverse = -cosSin[-1];
	*--cosSinReverse =  cosSin[-2];
	*cosSin++ = c;
	*cosSin++ = s;
	*--cosSinReverse = -cosSin[-1];
	*--cosSinReverse =  cosSin[-2];

	while (cosSin < cosSinReverse)
	{
		cosSin[0] = cosSin[-2] * c - cosSin[-1] * s;
		cosSin[1] = cosSin[-2] * s + cosSin[-1] * c;
		cosSin += 2;
		*--cosSinReverse = -cosSin[-1];
		*--cosSinReverse =  cosSin[-2];
	}
}

static void computeCosSinRho(const float phi, const int segments)
{
	float *cosSin        = COS_SIN_RHO;
	float *cosSinReverse = cosSin + 2 * (segments + 1);

	const float c = std::cos(phi);
	const float s = std::sin(phi);

	*cosSin++ = 1.0f;
	*cosSin++ = 0.0f;

	*--cosSinReverse =  cosSin[-1];
	*--cosSinReverse = -cosSin[-2];

	*cosSin++ = c;
	*cosSin++ = s;

	*--cosSinReverse =  cosSin[-1];
	*--cosSinReverse = -cosSin[-2];

	while (cosSin < cosSinReverse)
	{
		cosSin[0] = cosSin[-2] * c - cosSin[-1] * s;
		cosSin[1] = cosSin[-2] * s + cosSin[-1] * c;
		cosSin += 2;
		*--cosSinReverse =  cosSin[-1];
		*--cosSinReverse = -cosSin[-2];
	}
}

//! Prepare a vertex buffer for generation.
//!
//! If the buffer is NULL, it is constructed with specified primitive type.
//! Otherwise, it is unlocked and cleared.
template<class V>
static void prepareVertexBuffer
	(StelRenderer* renderer, StelVertexBuffer<V>*& vertices, const PrimitiveType type)
{
	if(NULL == vertices)
	{
		vertices = renderer->createVertexBuffer<V>(type);
	}
	else
	{
		vertices->unlock();
		vertices->clear();
	}
}

void StelGeometrySphere::draw(StelRenderer* renderer, StelProjectorP projector)
{
	// Lit spheres must update at every projector as projector is used in lighting.
	if(updated || type == SphereType_Lit)
	{
		regenerate(renderer, projector);
	}

	switch(type)
	{
		case SphereType_Fisheye:
		case SphereType_Unlit:
			for(int row = 0; row < rowIndices.size(); ++row)
			{
				renderer->drawVertexBuffer(unlitVertices, rowIndices[row], projector);
			}
			break;
		case SphereType_Lit:
			for(int row = 0; row < rowIndices.size(); ++row)
			{
				renderer->drawVertexBuffer(litVertices, rowIndices[row], projector);
			}
			break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}
} 


void StelGeometrySphere::regenerate(StelRenderer* renderer, StelProjectorP projector)
{
	Q_ASSERT_X(type == SphereType_Fisheye || type == SphereType_Unlit || 
	           type == SphereType_Lit, Q_FUNC_INFO, 
	           "New sphere type - need to update StelGeometrySphere::regenerate()");

	// Prepare the vertex buffer
	if(type == SphereType_Fisheye || type == SphereType_Unlit)
	{
		Q_ASSERT_X(NULL == litVertices, Q_FUNC_INFO,
		           "Lit vertex buffer is used for unlit sphere");
		prepareVertexBuffer(renderer, unlitVertices, PrimitiveType_TriangleStrip);
	}
	else if(type == SphereType_Lit)
	{
		Q_ASSERT_X(NULL == unlitVertices, Q_FUNC_INFO,
		           "Unlit vertex buffer is used for lit sphere");
		prepareVertexBuffer(renderer, litVertices, PrimitiveType_TriangleStrip);
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}

	// Prepare index buffers for rows.
	// Add/remove rows based on loops. Clear reused rows.
	const int rowsKept = std::min(stacks, rowIndices.size());
	for(int row = 0; row < rowsKept; ++row)
	{
		rowIndices[row]->unlock();
		rowIndices[row]->clear();
	}

	if(stacks > rowIndices.size())
	{
		for(int row = rowIndices.size(); row < stacks; ++row)
		{
			rowIndices.append(renderer->createIndexBuffer(IndexType_U16));
		}
	}
	else if(stacks < rowIndices.size())
	{
		for(int row = stacks; row < rowIndices.size(); ++row)
		{
			delete rowIndices[row];
		}
		rowIndices.resize(stacks);
	}


	// Now actually build the sphere

	const float dtheta     = 2.0f * M_PI / slices;
	computeCosSinTheta(dtheta, slices);

	if(type == SphereType_Fisheye)
	{
		StelVertexBuffer<VertexP3T2>* vertices = unlitVertices;
		const float stackAngle = M_PI / stacks;
		const float drho       = stackAngle / fisheyeTextureFov;
		computeCosSinRho(stackAngle, stacks);

		const Vec2f texOffset(0.5f, 0.5f);
		float yTexMult = orientInside ? -1.0f : 1.0f;
		float xTexMult = flipTexture ? -1.0f : 1.0f;

		// Build intermediate stacks as triangle strips
		const float* cosSinRho = COS_SIN_RHO;
		float rho = 0.0f;

		// Generate all vertices of the sphere.
		vertices->unlock();
		for (int i = 0; i <= stacks; ++i, rho += drho)
		{
			const float rhoClamped = std::min(rho, 0.5f);
			const float cosSinRho0 = cosSinRho[0];
			const float cosSinRho1 = cosSinRho[1];
			
			const float* cosSinTheta = COS_SIN_THETA;
			for (int slice = 0; slice <= slices; ++slice)
			{
				const Vec3f v(-cosSinTheta[1] * cosSinRho1,
								  cosSinTheta[0] * cosSinRho1, 
								  cosSinRho0 * oneMinusOblateness);
				const Vec2f t(xTexMult * cosSinTheta[0], yTexMult * cosSinTheta[1]);
				vertices->addVertex(VertexP3T2(v * radius, texOffset + rhoClamped * t));
				cosSinTheta += 2;
			}

			cosSinRho += 2;
		}
		vertices->lock();

		// Generate index buffers for strips (rows) forming the sphere.
		uint index = 0;
		for (int i = 0; i < stacks; ++i)
		{
			StelIndexBuffer* const indices = rowIndices[i];
			indices->unlock();

			for (int slice = 0; slice <= slices; ++slice)
			{
				const uint i1 = orientInside ? index + slices + 1 : index;
				const uint i2 = orientInside ? index : index + slices + 1;
				indices->addIndex(i1);
				indices->addIndex(i2);
				index += 1;
			}

			indices->lock();
		}
	}
	else if(type == SphereType_Unlit || type == SphereType_Lit)
	{
		const bool lit = type == SphereType_Lit;
		Vec3d lightPos;
		Vec4f ambientLight, diffuseLight;
		if(lit)
		{
			// Set up the light.
			lightPos = Vec3d(light.position[0], light.position[1], light.position[2]);
			projector->getModelViewTransform()
			         ->getApproximateLinearTransfo()
			         .transpose()
			         .multiplyWithoutTranslation(Vec3d(lightPos[0], lightPos[1], lightPos[2]));
			projector->getModelViewTransform()->backward(lightPos);
			lightPos.normalize();
			ambientLight = light.ambient;
			diffuseLight = light.diffuse; 
		}
		// Set up vertex generation.
		(lit ? litVertices->unlock() : unlitVertices->unlock());
		const float drho   = M_PI / stacks;
		computeCosSinRho(drho, stacks);

		const float nsign = orientInside ? -1.0f : 1.0f;
		// from inside texture is reversed 
		float t           = orientInside ?  0.0f : 1.0f;

		// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
		// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
		// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
		const float ds = (flipTexture ? -1.0f : 1.0f) / slices;
		const float dt = nsign / stacks; // from inside texture is reversed

		// Generate sphere vertices.
		const float* cosSinRho = COS_SIN_RHO;
		for (int i = 0; i <= stacks; ++i)
		{
			const float cosSinRho0 = cosSinRho[0];
			const float cosSinRho1 = cosSinRho[1];
			
			float s = flipTexture ? 1.0f : 0.0f;
			const float* cosSinTheta = COS_SIN_THETA;
			// Used to bake light colors into lit spheres' vertices
			Vec4f color;
			for (int slice = 0; slice <= slices; ++slice)
			{
				const Vec3f v(-cosSinTheta[1] * cosSinRho1,
				              cosSinTheta[0] * cosSinRho1, 
				              nsign * cosSinRho0 * oneMinusOblateness);
				if(lit)
				{
					const float c = 
						std::max(0.0, nsign * (lightPos[0] * v[0] * oneMinusOblateness +
						                       lightPos[1] * v[1] * oneMinusOblateness +
						                       lightPos[2] * v[2] / oneMinusOblateness));
					const Vec4f color = std::min(c, 0.5f) * diffuseLight + ambientLight;
					litVertices->addVertex(VertexP3T2C4(v * radius, Vec2f(s, t), color));
				}
				else
				{
					unlitVertices->addVertex(VertexP3T2(v * radius, Vec2f(s, t)));
				}
				s += ds;
				cosSinTheta += 2;
			}

			cosSinRho += 2;
			t -= dt;
		}
		// Generate index buffers for strips (rows) forming the sphere.
		uint index = 0;
		for (int i = 0; i < stacks; ++i)
		{
			StelIndexBuffer* const indices = rowIndices[i];
			indices->unlock();

			for (int slice = 0; slice <= slices; ++slice)
			{
				indices->addIndex(index);
				indices->addIndex(index + slices + 1);
				index += 1;
			}

			indices->lock();
		}

		(lit ? litVertices->lock() : unlitVertices->lock());
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}

	updated = false;
}

void StelGeometryRing::draw(StelRenderer* renderer, StelProjectorP projector)
{
	if(updated)
	{
		regenerate(renderer);
	}

	switch(type)
	{
		case RingType_Textured:
			for(int loop = 0; loop < loops; ++loop)
			{
				renderer->drawVertexBuffer(texturedVertices, loopIndices[loop], projector);
			}
			break;
		case RingType_Plain2D:
			for(int loop = 0; loop < loops; ++loop)
			{
				renderer->drawVertexBuffer(plain2DVertices, loopIndices[loop]);
			}
			break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown sphere type");
	}
} 

void StelGeometryRing::regenerate(StelRenderer* renderer)
{
	Q_ASSERT_X(type == RingType_Plain2D || type == RingType_Textured, Q_FUNC_INFO, 
	           "New ring type - need to update StelGeometryRing::regenerate()");

	// Prepare the vertex buffer
	if(type == RingType_Textured)
	{
		Q_ASSERT_X(NULL == plain2DVertices, Q_FUNC_INFO,
		           "Plain 2D vertex buffer is used for textured 3D ring");
		prepareVertexBuffer(renderer, texturedVertices, PrimitiveType_TriangleStrip);
	}
	else if(type == RingType_Plain2D)
	{
		Q_ASSERT_X(NULL == texturedVertices, Q_FUNC_INFO,
		           "Textured vertex buffer is used for a plain 2D ring");
		prepareVertexBuffer(renderer, plain2DVertices, PrimitiveType_TriangleStrip);
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown ring type");
	}

	// Prepare index buffers for loops.
	// Add/remove loops based on stacks. Clear reused loops.
	const int loopsKept = std::min(loops, loopIndices.size());
	for(int loop = 0; loop < loopsKept; ++loop)
	{
		loopIndices[loop]->unlock();
		loopIndices[loop]->clear();
	}

	if(loops > loopIndices.size())
	{
		for(int loop = loopIndices.size(); loop < loops; ++loop)
		{
			loopIndices.append(renderer->createIndexBuffer(IndexType_U16));
		}
	}
	else if(loops < loopIndices.size())
	{
		for(int loop = loops; loop < loopIndices.size(); ++loop)
		{
			delete loopIndices[loop];
		}
		loopIndices.resize(loops);
	}

	const float dr     = (outerRadius - innerRadius) / loops;
	const float dtheta = (flipFaces ? -1.0f : 1.0f) * 2.0f * M_PI / slices;
	computeCosSinTheta(dtheta, slices);

	// Generate vertices of the ring.
	float r = innerRadius;
	if(type == RingType_Textured)
	{
		StelVertexBuffer<VertexP3T2>* const vertices = texturedVertices;
		for(int loop = 0; loop <= loops; ++loop, r += dr)
		{
			const float texR = (r - innerRadius) / (outerRadius - innerRadius);
			const float* cosSinTheta = COS_SIN_THETA;
			for (int slice = 0; slice <= slices; ++slice, cosSinTheta += 2)
			{
				vertices->addVertex(
					VertexP3T2(offset + Vec3f(r * cosSinTheta[0], r * cosSinTheta[1], 0.0f),
					           Vec2f(texR, 0.5f)));
			}
		}
		vertices->lock();
	}
	else if(type == RingType_Plain2D)
	{
		const Vec2f offset2D(offset[0], offset[1]);
		StelVertexBuffer<VertexP2>* const vertices = plain2DVertices;
		for(int loop = 0; loop <= loops; ++loop, r += dr)
		{
			const float* cosSinTheta = COS_SIN_THETA;
			for (int slice = 0; slice <= slices; ++slice, cosSinTheta += 2)
			{
				vertices->addVertex(
					VertexP2(offset2D + Vec2f(r * cosSinTheta[0], r * cosSinTheta[1])));
			}
		}
		vertices->lock();
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown ring type");
	}

	// Generate a triangle strip index buffer for each loop.
	uint index = 0;
	for(int loop = 0; loop < loops; ++loop)
	{
		StelIndexBuffer* indices = loopIndices[loop];
		indices->unlock();
		for (int slice = 0; slice <= slices; ++slice)
		{
			indices->addIndex(index);
			indices->addIndex(index + slices + 1);
			++index;
		}
		indices->lock();
	} 
	updated = false;
}

void StelGeometryBuilder::buildFanDisk
	(StelVertexBuffer<VertexP3T2>* const vertexBuffer, StelIndexBuffer* const indexBuffer,
	 const float radius, const int innerFanSlices, const int level)
{
	Q_ASSERT_X(vertexBuffer->length() == 0, Q_FUNC_INFO, 
	           "Need an empty vertex buffer to start building a fan disk");
	Q_ASSERT_X(indexBuffer->length() == 0, Q_FUNC_INFO, 
	           "Need an empty index buffer to start building a fan disk");
	Q_ASSERT_X(vertexBuffer->primitiveType() == PrimitiveType_Triangles, 
	           Q_FUNC_INFO, "Need a triangles vertex buffer to build a fan disk");
	Q_ASSERT_X(innerFanSlices >= 3, Q_FUNC_INFO, 
	           "Can't build a fan disk with less than 3 slices");
	Q_ASSERT_X(level < 32, Q_FUNC_INFO, 
	           "Can't build a fan disk with more than 31 levels "
	           "(to prevent excessive vertex counts - this limit can be increased)");

	vertexBuffer->unlock();
	indexBuffer->unlock();
	float radii[32];
	radii[level] = radius;
	for (int l = level - 1; l >= 0; --l)
	{
		radii[l] = radii[l + 1] * 
		           (1.0f - M_PI / (innerFanSlices << (l + 1))) * 2.0f / 3.0f;
	}

	const int slices = innerFanSlices << level;
	const float dtheta = 2.0f * M_PI / slices;
	Q_ASSERT_X(slices <= MAX_SLICES, Q_FUNC_INFO, "Too many slices");
	computeCosSinTheta(dtheta, slices);

	int slicesStep = 2;
	
	// Texcoords at the center are 0.5, 0.5, and vary between 0.0 to 1.0 for 
	// opposite sides in distance of radius from center.
	const float texMult = 0.5 / radius;
	const Vec2f texOffset(0.5f, 0.5f);

	// Current index in the index buffer.
	uint index = 0;

	for (int l = level; l > 0; --l, slicesStep <<= 1)
	{
		const float* cosSinTheta = COS_SIN_THETA;
		for (int s = 0; s < slices - 1; s += slicesStep)
		{
			const Vec2f v0(radii[l] * cosSinTheta[slicesStep],
			               radii[l] * cosSinTheta[slicesStep + 1]);
			const Vec2f v1(radii[l] * cosSinTheta[2 * slicesStep],
			               radii[l] * cosSinTheta[2 * slicesStep +1]);
			const Vec2f v2(radii[l - 1] * cosSinTheta[2 * slicesStep],
			               radii[l - 1] * cosSinTheta[2 * slicesStep + 1]);
			const Vec2f v3(radii[l - 1] * cosSinTheta[0], radii[l - 1] * cosSinTheta[1]);
			const Vec2f v4(radii[l] * cosSinTheta[0], radii[l] * cosSinTheta[1]);

			vertexBuffer->addVertex(VertexP3T2(v0, texOffset + v0 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v1, texOffset + v1 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v2, texOffset + v2 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v3, texOffset + v3 * texMult));
			vertexBuffer->addVertex(VertexP3T2(v4, texOffset + v4 * texMult));

			indexBuffer->addIndex(index);
			indexBuffer->addIndex(index + 1);
			indexBuffer->addIndex(index + 2);
			indexBuffer->addIndex(index);
			indexBuffer->addIndex(index + 2);
			indexBuffer->addIndex(index + 3);
			indexBuffer->addIndex(index);
			indexBuffer->addIndex(index + 3);
			indexBuffer->addIndex(index + 4);

			cosSinTheta += 2 * slicesStep;
			index += 5;
		}
	}

	// draw the inner polygon
	slicesStep >>= 1;

	const float* cosSinTheta = COS_SIN_THETA;
	const float rad = radii[0];
	if (slices == 1)
	{
		const Vec2f v0(rad * cosSinTheta[0], rad * cosSinTheta[1]);
		cosSinTheta += 2 * slicesStep;
		const Vec2f v1(rad * cosSinTheta[0], rad * cosSinTheta[1]);
		cosSinTheta += 2 * slicesStep;
		const Vec2f v2(rad * cosSinTheta[0], rad * cosSinTheta[1]);

		vertexBuffer->addVertex(VertexP3T2(v0, texOffset + v0 * texMult));
		vertexBuffer->addVertex(VertexP3T2(v1, texOffset + v1 * texMult));
		vertexBuffer->addVertex(VertexP3T2(v2, texOffset + v2 * texMult));

		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);

		vertexBuffer->lock();
		indexBuffer->lock();
		return;
	}

	int s = 0;
	while (s < slices)
	{
		const Vec2f v1(rad * cosSinTheta[0], rad * cosSinTheta[1]);
		cosSinTheta += 2 * slicesStep;
		const Vec2f v2(rad * cosSinTheta[0], rad * cosSinTheta[1]);

		vertexBuffer->addVertex(VertexP3T2(Vec3f(0.0f, 0.0f, 0.0f), texOffset));
		vertexBuffer->addVertex(VertexP3T2(v1, texOffset + v1 * texMult));
		vertexBuffer->addVertex(VertexP3T2(v2, texOffset + v2 * texMult));

		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);
		indexBuffer->addIndex(index++);

		s += slicesStep;
	}

	vertexBuffer->lock();
	indexBuffer->lock();
}
