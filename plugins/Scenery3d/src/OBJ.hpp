/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza
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

/** OBJ loader based on dhpoware's glObjViewer (http://www.dhpoware.com/demos/glObjViewer.html) See license below **/

//-----------------------------------------------------------------------------
// Copyright (c) 2007 dhpoware. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _OBJ_HPP_
#define _OBJ_HPP_

#include <QFile>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include "StelTexture.hpp"
#include "VecMath.hpp"
#include "AABB.hpp"

class Heightmap;

//! A basic Wavefront .OBJ format model loader.
//!
//! FS: The internal loader still is not very robust, uses many C IO functions (fopen,fscanf...) 
//! and will have serious problems handling a malformed file including many potential buffer overflows. 
//! Meaning: **Do NOT use for untrusted, downloaded files!**
class OBJ
{
public:
	//! OBJ files can have vertices encoded in different order.
	//! Only XYZ and XZY may occur in real life, but we can cope with all...
	enum vertexOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };

	//! Encapsulates all information that the describes the surface appearance of a StelModel
	struct Material
	{
		//! MTL Illumination models, do not use externally anymore. See the developer doc for info.
		//! @deprecated This is from the old illum model, do not consider this outside this class!
		enum Illum { I_NONE=-1, I_DIFFUSE=0, I_DIFFUSE_AND_AMBIENT=1, I_SPECULAR=2, I_TRANSLUCENT=9 };
		Illum illum;

		//! Creates a material with the default values
		Material() {
			ambient[0] = -1.f;
			ambient[1] = -1.f;
			ambient[2] = -1.f;
			diffuse[0] = 0.8f;
			diffuse[1] = 0.8f;
			diffuse[2] = 0.8f;
			specular[0] = 0.0f;
			specular[1] = 0.0f;
			specular[2] = 0.0f;
			emission[0] = 0.0f;
			emission[1] = 0.0f;
			emission[2] = 0.0f;
			shininess = 8.0f;
			name = "<invalid>";
			alpha = -1.0f;
			alphatest = false;
			backfacecull = true;
			illum = I_NONE;
			hasSpecularity = false;
			hasTransparency = false;
		}

		//! Needs to be called after everything is loaded
		void finalize();

		//! Material name
		QString name;
		//! Ka, Kd, Ks, Ke
		QVector3D ambient;
		QVector3D diffuse;
		QVector3D specular;
		QVector3D emission;
		//! Shininess [0..128]
		float shininess;
		//! Transparency [0..1]
		float alpha;
		//! If to perform binary alpha testing. Default off.
		bool alphatest;
		//! If to perform backface culling. Default on.
		bool backfacecull;
		//! Quick check if this material has specularity
		bool hasSpecularity;
		//! Quick check if this material has "real" transparency (i.e. needs blending)
		bool hasTransparency;

		//! Texture name
		QString textureName;
		//!< Shared pointer to texture of the model. This can be null.
		StelTextureSP texture;
		//! Bump map name
		QString bumpMapName;
		//!< Shared pointer to bump map texture of the model. This can be null.
		StelTextureSP bump_texture;
		//! Height map name
		QString heightMapName;
		//!< Shared pointer to height map texture of the model. This can be null.
		StelTextureSP height_texture;
		//! Name of emissive texture
		QString emissiveMapName;
		//!< Shared pointer to emissive texture of the model. This can be null.
		StelTextureSP emissive_texture;
	};


	//! A vertex struct holds the vertex itself (position), corresponding texture coordinates, normals, tangents and bitangents
	//! It does not use Vec3f etc. to be POD compliant (needed for offsetof in some compilers)
	struct Vertex
	{
		GLfloat position[3];
		GLfloat texCoord[2];
		GLfloat normal[3];
		GLfloat tangent[4];
		GLfloat bitangent[3];

		static const Vertex EmptyVertex;
	};

	//! Structure for a Mesh, will be used with Stellarium to render
	//! Holds the starting index, the number of triangles and a pointer to the MTL
	struct StelModel
	{
		int startIndex, triangleCount;
		//materials are managed by OBJ
		const Material* pMaterial;
		//AABB is managed by this
		AABB bbox;
		//The centroid location of all vertices of this model
		Vec3f centroid;
	};

	//! Initializes values
	OBJ();
	//! Destructor
	~OBJ();

	//! Cleanup, will be called inside the destructor
	void clean();
	//! Loads the given obj file and, if specified rebuilds normals
	bool load(const QString& filename, const enum vertexOrder order, bool rebuildNormals = false);
	//! Transform all the vertices through multiplication with a 4x4 matrix.
	//! @param mat Matrix to multiply vertices with.
	void transform(QMatrix4x4 mat);
	//! Returns a Material
	Material &getMaterial(int i);
	//! Returns a StelModel
	const StelModel& getStelModel(int i) const;

	//! This should be called after textures are loaded, and will re-order the StelModels to be grouped by their material.
	//! Furthermore, this is a prerequisite for transparencyDepthSort.
	void finalizeForRendering();

	//! Sorts the transparent StelModels according to their distance to the specified position.
	//! They are sorted so that they can be drawn back-to-front.
	void transparencyDepthSort(const Vec3f& position);

	//! Getters for various datastructures
	int getNumberOfIndices() const;
	int getNumberOfStelModels() const;
	int getNumberOfTriangles() const;
	int getNumberOfVertices() const;
	int getNumberOfMaterials() const;

	//! Returns a vertex reference
	const Vertex& getVertex(int i) const;
	//! Returns the vertex array
	const Vertex* getVertexArray() const;
	//! Returns the vertex size
	int getVertexSize() const;

	//! Returns flags
	bool isLoaded() const;
	bool hasPositions() const;
	bool hasTextureCoords() const;
	bool hasNormals() const;
	bool hasTangents() const;
	bool hasStelModels() const;

	//! Returns the bounding box for this OBJ
	//const BoundingBox* getBoundingBox() const;
	const AABB& getBoundingBox();

	void renderAABBs();

	//! Returns an estimate of the memory usage of this instance (not fully accurate, but good enough)
	size_t memoryUsage();

	//! Uploads the textures to GL (requires valid context)
	void uploadTexturesGL();
	//! Uploads the vertex and index data to GL buffers (requires valid context)
	void uploadBuffersGL();

	//! Binds the necessary GL objects, making the OBJ ready for drawing. Uses a VAO if the platform supports it.
	void bindGL();
	//! Unbinds this object's GL objects
	void unbindGL();

	//! Set up some stuff that requires a valid OpenGL context.
	static void setupGL();
	//! Returns the OpenGL index buffer type supported on this hardware.
	//! OpenGL ES may not support integer indices, so this is necessary.
	static inline GLenum getIndexBufferType() { return indexBufferType; }
	static inline size_t getIndexBufferTypeSize() { return indexBufferTypeSize; }

	//! Copy assignment operator. No deep copies are performed, but QVectors have copy-on-write semantics, so this is no problem. Does not copy GL objects.
	OBJ& operator=(const OBJ& other);
private:
	struct FaceAttributes
	{
		int materialIndex;
		int objectIndex;
	};

	typedef QVector<FaceAttributes> AttributeVector;
	typedef QVector<Vec3f> VF3Vector;
	typedef QVector<Vec2f> VF2Vector;
	typedef Vec3f VPos;
	typedef QVector<Vec3f> PosVector;
	typedef QMap<QString,int> MatCacheT;
	typedef QMap<int, QVector<int> > VertCacheT;

	void addFaceAttrib(AttributeVector& attributeArray, uint index, int material, int object);
	void addTrianglePos(const PosVector& vertexCoords, VertCacheT &vertexCache, unsigned int index, int v0, int v1, int v2);
	void addTrianglePosNormal(const PosVector &vertexCoords, const VF3Vector& normals, VertCacheT &vertexCache, unsigned int index,
				  int v0, int v1, int v2,
				  int vn0, int vn1, int vn2);
	void addTrianglePosTexCoord(const PosVector &vertexCoords, const VF2Vector &textureCoords, VertCacheT &vertexCache, unsigned int index,
				    int v0, int v1, int v2,
				    int vt0, int vt1, int vt2);
	void addTrianglePosTexCoordNormal(PosVector &vertexCoords, const VF2Vector &textureCoords, const VF3Vector &normals, VertCacheT &vertexCache, unsigned int index,
					  int v0, int v1, int v2,
					  int vt0, int vt1, int vt2,
					  int vn0, int vn1, int vn2);
	int addVertex(VertCacheT &vertexCache, int hash, const Vertex* pVertex);
	//! Builds the StelModels based on material
	void buildStelModels(const AttributeVector &attributeArray);
	//! Generates normals in case they aren't specified/need rebuild
	void generateNormals();
	//! Generates tangents (and bitangents/binormals) (useful for NormalMapping, Parallax Mapping, ...)
	void generateTangents();
	//! First pass - scans the file for memory allocation
	void importFirstPass(QFile& pFile, MatCacheT &materialCache);
	//! Second pass - actual parsing step
	void importSecondPass(FILE *pFile, const enum vertexOrder order, const MatCacheT &materialCache);
	//! Imports material file and fills the material datastructure
	bool importMaterials(const QString& filename, MatCacheT& materialCache);
	QString absolutePath(QString path);
	//! Determine the bounding box extrema
	void findBounds();
	//! Binds the GL buffers to the vertex attributes
	void bindBuffersGL();
	//! Releases vertex attribute bindings and buffers
	void unbindBuffersGL();

	//! Returns the file for this filename, handling decompression if necessary
	QFile* getFile(const QString& filename);

	//! Used for parsing a texture string
	QString parseTextureString(const char *buffer) const;

	//! Flags
	bool m_loaded;
	bool m_hasPositions;
	bool m_hasTextureCoords;
	bool m_hasNormals;
	bool m_hasTangents;
	bool m_hasStelModels;
	static bool vertexArraysSupported;

	//! The type of the index buffer, may be GL_UNSIGNED_INT or GL_UNSIGNED_SHORT (on ES where GL_OES_element_index_uint is unsupported)
	static GLenum indexBufferType;
	//! The sizeof() of the indexBufferType
	static size_t indexBufferTypeSize;
	int m_firstTransparentIndex;

	//! Structure sizes
	unsigned int m_numberOfVertexCoords;
	unsigned int m_numberOfTextureCoords;
	unsigned int m_numberOfNormals;
	unsigned int m_numberOfTriangles;
	unsigned int m_numberOfMaterials;
	unsigned int m_numberOfStelModels;

	//! Bounding box for the entire scene
	AABB pBoundingBox;

	//! Base path to this file
	QString m_basePath;

	//! Datastructures
	QVector<StelModel> m_stelModels;
	QVector<Material> m_materials;
	QVector<Vertex> m_vertexArray;
	QVector<unsigned int> m_indexArray;

	//! OpenGL objects
	QOpenGLBuffer m_vertexBuffer;
	QOpenGLBuffer m_indexBuffer;
	QOpenGLVertexArrayObject* m_vertexArrayObject;

	//! Heightmap
	friend class Heightmap;
};

inline OBJ::Material& OBJ::getMaterial(int i) { return m_materials[i]; }

inline const OBJ::StelModel& OBJ::getStelModel(int i) const { return m_stelModels[i]; }

inline int OBJ::getNumberOfIndices() const { return m_numberOfTriangles * 3; }

inline int OBJ::getNumberOfStelModels() const { return m_numberOfStelModels; }

inline int OBJ::getNumberOfTriangles() const { return m_numberOfTriangles; }

inline int OBJ::getNumberOfVertices() const { return static_cast<int>(m_vertexArray.size()); }

inline int OBJ::getNumberOfMaterials() const {return m_numberOfMaterials; }

inline const OBJ::Vertex& OBJ::getVertex(int i) const { return m_vertexArray[i]; }

inline const OBJ::Vertex* OBJ::getVertexArray() const { return &m_vertexArray[0]; }

inline int OBJ::getVertexSize() const { return static_cast<int>(sizeof(Vertex)); }

inline bool OBJ::isLoaded() const{ return m_loaded; }

inline bool OBJ::hasNormals() const{ return m_hasNormals; }

inline bool OBJ::hasPositions() const { return m_hasPositions; }

inline bool OBJ::hasTangents() const { return m_hasTangents; }

inline bool OBJ::hasTextureCoords() const { return m_hasTextureCoords; }

inline bool OBJ::hasStelModels() const { return m_hasStelModels; }

//inline const OBJ::BoundingBox* OBJ::getBoundingBox() const { return pBoundingBox; }
inline const AABB &OBJ::getBoundingBox() {return pBoundingBox; }

inline QString OBJ::absolutePath(QString path) { return m_basePath + path; }

#endif
