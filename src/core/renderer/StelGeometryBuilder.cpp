#include "StelGeometryBuilder.hpp"


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

//! Checks preconditions for sphere building.
//!
//! In release builds, this function does nothing, and should be optimized away.
//!
//! @param vertices        Vertex buffer to store sphere vertices.
//! @param rowIndexBuffers Index buffers to store triangle strips forming sphere rows.
//! @param stacks          Number of rows of the sphere grid.
//! @param slices          Number of columns of the sphere grid.
static void spherePreconditions(StelVertexBuffer<VertexP3T2>* const vertices,
                                QVector<StelIndexBuffer*> rowIndexBuffers,
                                const int stacks, const int slices)
{
	Q_ASSERT_X(stacks <= MAX_STACKS, Q_FUNC_INFO, "Too many stacks");
	Q_ASSERT_X(stacks > 3, Q_FUNC_INFO,
	           "Need at least 3 row index buffers to build a sphere");
	Q_ASSERT_X(slices <= MAX_SLICES, Q_FUNC_INFO, "Too many slices");
	Q_ASSERT_X(slices > 3, Q_FUNC_INFO, "Need at least 3 columns to build a sphere");
	Q_ASSERT_X(vertices->length() == 0, Q_FUNC_INFO, 
	           "Need an empty vertex buffer to build a sphere");
	Q_ASSERT_X(vertices->primitiveType() == PrimitiveType_TriangleStrip, 
	           Q_FUNC_INFO, "Need a triangle strip vertex buffer to build a sphere");
	foreach(const StelIndexBuffer* buffer, rowIndexBuffers)
	{
		Q_ASSERT_X(buffer->length() == 0, Q_FUNC_INFO, 
		           "Need empty index buffers to build a sphere");
	}
}

// A lot of code in the below two member functions is identical, but 
// there is no simple way to refactor it out without using a single function 
// calling many subfunctions. Perhaps a Sphere class with generation variables 
// as data members could solve this.

void StelGeometryBuilder::buildSphereFisheye
	(StelVertexBuffer<VertexP3T2>* const vertices, 
	 QVector<StelIndexBuffer*>& rowIndexBuffers, const float radius,
	 const int slices, const float textureFov, const bool orientInside)
{
	const int stacks = rowIndexBuffers.size();
	spherePreconditions(vertices, rowIndexBuffers, stacks, slices);

	const float stackAngle = M_PI / stacks;
	const float dtheta     = 2.0f * M_PI / slices;
	const float drho       = stackAngle / textureFov;
	computeCosSinRho(stackAngle, stacks);
	computeCosSinTheta(dtheta, slices);

	const Vec2f texOffset(0.5f, 0.5f);
	float yTexMult = orientInside ? -1.0f : 1.0f;

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
		for (int j = 0; j <= slices; ++j)
		{
			const Vec3f v(-cosSinTheta[1] * cosSinRho1, cosSinTheta[0] * cosSinRho1, cosSinRho0);
			const Vec2f t(cosSinTheta[0], yTexMult * cosSinTheta[1]);
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
		StelIndexBuffer* const indices = rowIndexBuffers[i];
		indices->unlock();

		for (int j = 0; j <= slices; ++j)
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

void StelGeometryBuilder::buildSphere 
	(StelVertexBuffer<VertexP3T2>* const vertices, 
	 QVector<StelIndexBuffer*>& rowIndexBuffers, const float radius,
	 const float oneMinusOblateness, const int slices, 
	 const bool orientInside, const bool flipTexture)
{
	const int stacks = rowIndexBuffers.size();

	spherePreconditions(vertices, rowIndexBuffers, stacks, slices);

	vertices->unlock();
	const float drho   = M_PI / stacks;
	const float dtheta = 2.0f * M_PI / slices;
	computeCosSinRho(drho, stacks);
	computeCosSinTheta(dtheta, slices);

	const float nsign = orientInside ? -1.0f : 1.0f;
	// from inside texture is reversed 
	float t           = orientInside ?  0.0f : 1.0f;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
	// If the texture is flipped, we iterate the coordinates backward.
	const float ds = (flipTexture ? -1.0f : 1.0f) / slices;
	const float dt = nsign / stacks; // from inside texture is reversed

	const float* cosSinRho = COS_SIN_RHO;
	for (int i = 0; i <= stacks; ++i)
	{
		const float cosSinRho0 = cosSinRho[0];
		const float cosSinRho1 = cosSinRho[1];
		
		float s = !flipTexture ? 0.0f : 1.0f;
		const float* cosSinTheta = COS_SIN_THETA;
		for (int j = 0; j <= slices; ++j)
		{
			const Vec3f v(-cosSinTheta[1] * cosSinRho1,
			              cosSinTheta[0] * cosSinRho1, 
			              nsign * cosSinRho0 * oneMinusOblateness);
			vertices->addVertex(VertexP3T2(v * radius, Vec2f(s, t)));
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
		StelIndexBuffer* const indices = rowIndexBuffers[i];
		indices->unlock();

		for (int j = 0; j <= slices; ++j)
		{
			indices->addIndex(index);
			indices->addIndex(index + slices + 1);
			index += 1;
		}

		indices->lock();
	}

	vertices->lock();
}

