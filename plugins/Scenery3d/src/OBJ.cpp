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

#include "StelOpenGL.hpp"
#include "OBJ.hpp"
#include "ShaderManager.hpp"
#include "StelFileMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelUtils.hpp"

#include <QDir>
#include <QElapsedTimer>
#include <QOpenGLVertexArrayObject>
#include <QTemporaryFile>
#include <QDebug>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iostream>
#include <limits>

bool StelModelCompFunc(const OBJ::StelModel& lhs, const OBJ::StelModel& rhs)
{
	const OBJ::Material* lMat = lhs.pMaterial;
	const OBJ::Material* rMat = rhs.pMaterial;

	//same material, should be in the same batch
	if(lMat == rMat)
		return false;
	else
	{
		//transparent should be drawn at the end
		if(lMat->hasTransparency != rMat->hasTransparency)
		{
			return lMat->hasTransparency < rMat->hasTransparency;
		}
		else if(lMat->hasTransparency && lMat->alpha != rMat->alpha)
		{
			//order by alpha - larger values first
			return lMat->alpha > rMat->alpha;
		}
		else if(lMat->hasSpecularity!=rMat->hasSpecularity)
		{
			//no transparency, order by specularity
			return lMat->hasSpecularity < rMat->hasSpecularity;
		}
		else
		{
			//order by pointer address, this should induce some order in the materials
			//because they are all stored successively in a vector!
			//making sure that material switches are minimized
			return lMat < rMat;
		}
	}
}

bool OBJ::vertexArraysSupported=false;
GLenum OBJ::indexBufferType=GL_UNSIGNED_SHORT;
size_t OBJ::indexBufferTypeSize=0;

//static function
void OBJ::setupGL()
{
	//disable VAOs on Intel because of serious bugs in their implemenation...
	QString vendor(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	if(vendor.contains("Intel",Qt::CaseInsensitive))
	{
		OBJ::vertexArraysSupported = false;
		qWarning()<<"[OBJ] Disabling VAO usage because of Intel bugs";
	}
	else
	{
		//find out if VAOs are supported, simplest way is just trying to create one
		//Qt has the necessary checks inside the create method
		QOpenGLVertexArrayObject testObj;
		OBJ::vertexArraysSupported = testObj.create();
		testObj.destroy();
	}

	if( OBJ::vertexArraysSupported )
	{
		qDebug()<<"[OBJ] Vertex Array Objects are supported";
	}
	else
	{
		qWarning()<<"[OBJ] Vertex Array Objects are not supported on your hardware";
	}

	//check if we can enable int index buffers
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	if(ctx->isOpenGLES())
	{
		//query for extension
		if(ctx->hasExtension("GL_OES_element_index_uint"))
		{
			OBJ::indexBufferType = GL_UNSIGNED_INT;
		}
	}
	else
	{
		//we are on Desktop, so int is always supported
		OBJ::indexBufferType = GL_UNSIGNED_INT;
	}

	if(OBJ::indexBufferType==GL_UNSIGNED_SHORT)
	{
		OBJ::indexBufferTypeSize = sizeof(unsigned short);
		qWarning()<<"[OBJ] Your hardware does not support integer indices. Large models will not load.";
	}
	else
	{
		Q_ASSERT(OBJ::indexBufferType == GL_UNSIGNED_INT);
		OBJ::indexBufferTypeSize = sizeof(unsigned int);
	}
}

OBJ::OBJ() : m_vertexBuffer(QOpenGLBuffer::VertexBuffer), m_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
	//Iinitialize this OBJ
	m_loaded = false;
	m_hasPositions = false;
	m_hasNormals = false;
	m_hasTextureCoords = false;
	m_hasTangents = false;
	m_hasStelModels = false;

	m_numberOfVertexCoords = 0;
	m_numberOfTextureCoords = 0;
	m_numberOfNormals = 0;
	m_numberOfTriangles = 0;
	m_numberOfMaterials = 0;
	m_numberOfStelModels = 0;

	m_firstTransparentIndex = -1;

	pBoundingBox = AABB(Vec3f(0.0f), Vec3f(0.0f));
	//the constructor does no GL stuff, so it is always safe even if VAOs are not supported
	m_vertexArrayObject = new QOpenGLVertexArrayObject();
}

OBJ::~OBJ()
{
	clean();
	delete m_vertexArrayObject;
}

void OBJ::clean()
{
	m_loaded = false;
	m_hasPositions = false;
	m_hasNormals = false;
	m_hasTextureCoords = false;
	m_hasTangents = false;

	m_numberOfVertexCoords = 0;
	m_numberOfTextureCoords = 0;
	m_numberOfNormals = 0;
	m_numberOfTriangles = 0;
	m_numberOfMaterials = 0;
	m_numberOfStelModels = 0;

	pBoundingBox.min = Vec3f(0.0f);
	pBoundingBox.max = Vec3f(0.0f);

	//note that the wrappers still remain usable, the opengl objs are recreated automatically
	m_vertexBuffer.destroy();
	m_indexBuffer.destroy();

	if(vertexArraysSupported)
		m_vertexArrayObject->destroy();

	m_stelModels.clear();
	m_materials.clear();
	m_vertexArray.clear();
	m_indexArray.clear();
}

QFile* OBJ::getFile(const QString &filename)
{
	QFile* file = new QFile(filename);
	if(!file->open(QIODevice::ReadOnly))
	{
		qWarning()<<"[OBJ] Could not open file "<<filename;
		delete file;
		return NULL;
	}

	//check if this is a compressed file
	if(filename.endsWith(".gz"))
	{
		//TODO because the current loader depends on C IO, we have to save the decompressed data as a temporary file...
		QTemporaryFile* tmpFile = new QTemporaryFile(QDir::tempPath() + "/scenery3d_XXXXXX.obj.tmp");
		if (tmpFile->open())
		{
			qDebug()<<"[OBJ] Storing decompressed file in "<<tmpFile->fileName();
		}
		else
		{
			qDebug()<<"[OBJ] Cannot write temp file";
			return NULL;

		}
		//perform decompression of original file
		QByteArray data = StelUtils::uncompress(*file);
		tmpFile->write(data);
		tmpFile->flush();
		tmpFile->close();
		delete file;

		file = tmpFile;

		//reopen file read only
		if (!file->open(QIODevice::ReadOnly))
		{
			qWarning() << "[Scenery3d] OBJ::getFile(): Cannot open decompressed file! This comes unexpected.";
		}
	}

	return file;
}

bool OBJ::load(const QString& filename, const enum vertexOrder order, bool rebuildNormals)
{
	QElapsedTimer timer;
	timer.start();

	QFile* qtFile = getFile(filename);

	qint64 decTime = timer.restart();
	qDebug()<<"[OBJ] File opened/decompressed in "<<decTime<<"ms";

	if(!qtFile)
		return false;

	//Extract the base path, will be used to load the MTL file later on
	m_basePath.clear();
	m_basePath = StelFileMgr::dirName(filename) + "/";

	MatCacheT materialCache;

	//Parse the file
	importFirstPass(*qtFile,materialCache);
	qtFile->close();

	qint64 firstPassTime = timer.restart();

	//TODO make the second pass support Qt IO (unicode file handling, etc.)
	QByteArray ba = qtFile->fileName().toLocal8Bit();
	FILE* pFile = fopen(ba.constData(), "r");
	importSecondPass(pFile, order, materialCache);

	//Done parsing, close file
	fclose(pFile);

	//check if we support rendering the number of vertices loaded
	if(indexBufferType == GL_UNSIGNED_SHORT)
	{
		if((m_vertexArray.size() - 1) > std::numeric_limits<unsigned short>::max())
		{
			qCritical()<<"[OBJ] This scene is too complex to be rendered on your hardware. Vertices:"<<m_vertexArray.size()<<", hardware maximum:"<<std::numeric_limits<unsigned short>::max()+1;
			delete qtFile;
			return false;
		}
	}

	qint64 secondPassTime = timer.restart();

	//Find bounding extrema
	findBounds();

	qint64 boundTime = timer.restart();

	//Create vertex normals if specified or required
	if(rebuildNormals || !hasNormals())
	{
		generateNormals();
	}

	//Create tangents
	generateTangents();

	qint64 normalTime = timer.elapsed();

	//Loaded
	qDebug() << "[OBJ] Loaded OBJ successfully: " << qtFile->fileName();
	qDebug() << "[OBJ] Triangles#: " << m_numberOfTriangles;
	qDebug() << "[OBJ] Vertices#: " << m_numberOfVertexCoords<<" unique / "<< m_vertexArray.size()<<" total";
	qDebug() << "[OBJ] Normals#: " << m_numberOfNormals;
	qDebug() << "[OBJ] StelModels#: " << m_numberOfStelModels;
	qDebug() << "[OBJ] Bounding Box";
	qDebug() << "[OBJ] X: [" << pBoundingBox.min[0] << ", " << pBoundingBox.max[0] << "] ";
	qDebug() << "[OBJ] Y: [" << pBoundingBox.min[1] << ", " << pBoundingBox.max[1] << "] ";
	qDebug() << "[OBJ] Z: [" << pBoundingBox.min[2] << ", " << pBoundingBox.max[2] << "] ";
	qint64 total = firstPassTime + secondPassTime + normalTime + boundTime;
	qDebug() << "[OBJ] Required Time: Total-"<<total<<"ms ("<< (total / 1000.0f) <<"s) FP-" << firstPassTime << "ms, SP-" << secondPassTime
		 << "ms, BB-"<<boundTime<<"ms, N-"<<normalTime<<"ms";
#ifndef NDEBUG
	qDebug() << "[OBJ] memory usage: " << memoryUsage();
#endif
	m_loaded = true;

	delete qtFile; //this will also delete the temp file if one was used
	return true;
}

void OBJ::addFaceAttrib(AttributeVector &attributeArray, uint index, int material, int object)
{
	attributeArray[index].materialIndex = material;
	attributeArray[index].objectIndex = object;
}

// GCC g++ allows empty or one-zero initialisation. Other compilers in the buildbot require explicit initialisation.
const OBJ::Vertex OBJ::Vertex::EmptyVertex = {{0,0,0}, {0,0}, {0,0,0}, {0,0,0,0}, {0,0,0}};

void OBJ::addTrianglePos(const PosVector &vertexCoords, VertCacheT& vertexCache, unsigned int index, int v0, int v1, int v2)
{
	Vertex vertex = Vertex::EmptyVertex;

	//Add v0
	std::copy(vertexCoords.at(v0).v, vertexCoords.at(v0).v + 3,vertex.position);
	m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

	//Add v1
	std::copy(vertexCoords.at(v1).v, vertexCoords.at(v1).v + 3,vertex.position);
	m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

	//Add v2
	std::copy(vertexCoords.at(v2).v, vertexCoords.at(v2).v + 3,vertex.position);
	m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

void OBJ::addTrianglePosNormal(const PosVector &vertexCoords, const VF3Vector &normals, VertCacheT& vertexCache,
			       unsigned int index, int v0, int v1, int v2, int vn0, int vn1, int vn2)
{
	Vertex vertex = Vertex::EmptyVertex;

	//Add v0//vn0
	std::copy(vertexCoords.at(v0).v, vertexCoords.at(v0).v + 3,vertex.position);
	std::copy(normals.at(vn0).v, normals.at(vn0).v + 3,vertex.normal);
	m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

	//Add v1//n1
	std::copy(vertexCoords.at(v1).v, vertexCoords.at(v1).v + 3,vertex.position);
	std::copy(normals.at(vn1).v, normals.at(vn1).v + 3,vertex.normal);
	m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

	//Add v2//n2
	std::copy(vertexCoords.at(v2).v, vertexCoords.at(v2).v + 3,vertex.position);
	std::copy(normals.at(vn2).v, normals.at(vn2).v + 3,vertex.normal);
	m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

void OBJ::addTrianglePosTexCoord(const PosVector &vertexCoords, const VF2Vector &textureCoords, VertCacheT& vertexCache,
				 unsigned int index, int v0, int v1, int v2, int vt0, int vt1, int vt2)
{
	Vertex vertex = Vertex::EmptyVertex;

	//Add v0/vt0
	std::copy(vertexCoords.at(v0).v, vertexCoords.at(v0).v + 3,vertex.position);
	std::copy(textureCoords.at(vt0).v, textureCoords.at(vt0).v + 2,vertex.texCoord);
	m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

	//Add v1/vt1
	std::copy(vertexCoords.at(v1).v, vertexCoords.at(v1).v + 3,vertex.position);
	std::copy(textureCoords.at(vt1).v, textureCoords.at(vt1).v + 2,vertex.texCoord);
	m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

	//Add v2/vt2
	std::copy(vertexCoords.at(v2).v, vertexCoords.at(v2).v + 3,vertex.position);
	std::copy(textureCoords.at(vt2).v, textureCoords.at(vt2).v + 2,vertex.texCoord);
	m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

void OBJ::addTrianglePosTexCoordNormal(PosVector& vertexCoords, const VF2Vector &textureCoords, const VF3Vector &normals, VertCacheT& vertexCache,
				       unsigned int index, int v0, int v1, int v2, int vt0, int vt1, int vt2, int vn0, int vn1, int vn2)
{
	Vertex vertex = Vertex::EmptyVertex;

	//Add v0/vt0/vn0
	std::copy(vertexCoords.at(v0).v, vertexCoords.at(v0).v + 3,vertex.position);
	std::copy(textureCoords.at(vt0).v, textureCoords.at(vt0).v + 2,vertex.texCoord);
	std::copy(normals.at(vn0).v, normals.at(vn0).v + 3,vertex.normal);
	m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

	//Add v1/vt1/vn1
	std::copy(vertexCoords.at(v1).v, vertexCoords.at(v1).v + 3,vertex.position);
	std::copy(textureCoords.at(vt1).v, textureCoords.at(vt1).v + 2,vertex.texCoord);
	std::copy(normals.at(vn1).v, normals.at(vn1).v + 3,vertex.normal);
	m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

	//Add v2/vt2/vn2
	std::copy(vertexCoords.at(v2).v, vertexCoords.at(v2).v + 3,vertex.position);
	std::copy(textureCoords.at(vt2).v, textureCoords.at(vt2).v + 2,vertex.texCoord);
	std::copy(normals.at(vn2).v, normals.at(vn2).v + 3,vertex.normal);
	m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

int OBJ::addVertex(VertCacheT& vertexCache, int hash, const Vertex *pVertex)
{
	unsigned int index = -1;
	QMap<int,QVector<int> >::const_iterator iter = vertexCache.find(hash);

	if (iter == vertexCache.end())
	{
		// Vertex hash doesn't exist in the cache.
		index = static_cast<int>(m_vertexArray.size());
		m_vertexArray.push_back(*pVertex);
		vertexCache.insert(hash, QVector<int>(1, index));
	}
	else
	{
		// One or more vertices have been hashed to this entry in the cache.
		const QVector<int> &vertices = iter.value();
		const Vertex *pCachedVertex = 0;
		bool found = false;

		for (QVector<int>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
		{
			index = *i;
			pCachedVertex = &m_vertexArray.at(index);

			if (memcmp(pCachedVertex, pVertex, sizeof(Vertex)) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			index = static_cast<int>(m_vertexArray.size());
			m_vertexArray.push_back(*pVertex);
			vertexCache[hash].push_back(index);
		}
	}

	return index;
}

void OBJ::buildStelModels(const AttributeVector &attributeArray)
{
	//TODO FS this can be further optimized!
	//Imagine the case where 2 objects with the same model are not next to each other in the OBJ
	//then the current approach creates 2 StelModels
	//One could recombine them afterwards, or sort the vertices by their material before calling this
	//This would reduce material switching during rendering.

	//Group model's triangles based on material
	StelModel* pStelModel = 0;
	int materialId = -1;
	int objId = -1;
	int numStelModels = 0;

	// Count the number of meshes.
	for (int i=0; i<static_cast<int>(attributeArray.size()); ++i)
	{
		if (attributeArray[i].materialIndex != materialId)
		{
			//a change of material always requires a new model
			objId = attributeArray[i].objectIndex;
			materialId = attributeArray[i].materialIndex;
			++numStelModels;
		}
		else if (attributeArray[i].objectIndex != objId)
		{
			if(materialId>=0 && m_materials.at(materialId).hasTransparency)
			{
				//if the obj index changes, we need a new stel model if the material is potentially transparent, otherwise it is ignored
				//note that the stelmodel may get flagged as non-transparent later when texture information is available, but this poses no real problem
				++numStelModels;
			}
			objId = attributeArray[i].objectIndex;
		}
	}

	// Allocate memory for the StelModel and reset counters.
	m_numberOfStelModels = numStelModels;
	m_stelModels.resize(m_numberOfStelModels);
	numStelModels = 0;
	materialId = -1;
	objId = -1;

	// Build the StelModel. One StelModel for each unique material.
	for (int i=0; i<static_cast<int>(attributeArray.size()); ++i)
	{
		if (attributeArray[i].materialIndex != materialId || //if a new material is used
		   (attributeArray[i].objectIndex != objId && materialId>=0 && m_materials.at(materialId).hasTransparency)) //or a new object is used, material being transparent
		{
			materialId = attributeArray[i].materialIndex;
			objId = attributeArray[i].objectIndex;

			pStelModel = &m_stelModels[numStelModels++];
			pStelModel->triangleCount = 0;
			pStelModel->pMaterial = &m_materials[materialId];
			pStelModel->startIndex = i*3;
		}
		++pStelModel->triangleCount;
	}
}

void OBJ::generateNormals()
{
	const unsigned int *pTriangle = 0;
	Vertex *pVertex0 = 0;
	Vertex *pVertex1 = 0;
	Vertex *pVertex2 = 0;
	float edge1[3] = {0.0f, 0.0f, 0.0f};
	float edge2[3] = {0.0f, 0.0f, 0.0f};
	float normal[3] = {0.0f, 0.0f, 0.0f};
	float invlength = 0.0f;
	int totalVertices = getNumberOfVertices();
	int totalTriangles = getNumberOfTriangles();

	// Initialize all the vertex normals.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertexArray[i];
		pVertex0->normal[0] = 0.0f;
		pVertex0->normal[1] = 0.0f;
		pVertex0->normal[2] = 0.0f;
	}

	// Calculate the vertex normals.
	for (int i=0; i<totalTriangles; ++i)
	{
		pTriangle = &m_indexArray[i*3];

		pVertex0 = &m_vertexArray[pTriangle[0]];
		pVertex1 = &m_vertexArray[pTriangle[1]];
		pVertex2 = &m_vertexArray[pTriangle[2]];

		// Calculate triangle face normal.
		edge1[0] = static_cast<float>(pVertex1->position[0] - pVertex0->position[0]);
		edge1[1] = static_cast<float>(pVertex1->position[1] - pVertex0->position[1]);
		edge1[2] = static_cast<float>(pVertex1->position[2] - pVertex0->position[2]);

		edge2[0] = static_cast<float>(pVertex2->position[0] - pVertex0->position[0]);
		edge2[1] = static_cast<float>(pVertex2->position[1] - pVertex0->position[1]);
		edge2[2] = static_cast<float>(pVertex2->position[2] - pVertex0->position[2]);

		normal[0] = (edge1[1]*edge2[2]) - (edge1[2]*edge2[1]);
		normal[1] = (edge1[2]*edge2[0]) - (edge1[0]*edge2[2]);
		normal[2] = (edge1[0]*edge2[1]) - (edge1[1]*edge2[0]);

		// Accumulate the normals.

		pVertex0->normal[0] += normal[0];
		pVertex0->normal[1] += normal[1];
		pVertex0->normal[2] += normal[2];

		pVertex1->normal[0] += normal[0];
		pVertex1->normal[1] += normal[1];
		pVertex1->normal[2] += normal[2];

		pVertex2->normal[0] += normal[0];
		pVertex2->normal[1] += normal[1];
		pVertex2->normal[2] += normal[2];
	}

	// Normalize the vertex normals.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertexArray[i];

		invlength = 1.0f / sqrt(pVertex0->normal[0]*pVertex0->normal[0] +
					pVertex0->normal[1]*pVertex0->normal[1] +
					pVertex0->normal[2]*pVertex0->normal[2]);

		pVertex0->normal[0] *= invlength;
		pVertex0->normal[1] *= invlength;
		pVertex0->normal[2] *= invlength;
	}

	m_hasNormals = true;
}

void OBJ::generateTangents()
{
	const unsigned int *pTriangle = 0;
	Vertex *pVertex0 = 0;
	Vertex *pVertex1 = 0;
	Vertex *pVertex2 = 0;
	float edge1[3] = {0.0f, 0.0f, 0.0f};
	float edge2[3] = {0.0f, 0.0f, 0.0f};
	float texEdge1[2] = {0.0f, 0.0f};
	float texEdge2[2] = {0.0f, 0.0f};
	float tangent[3] = {0.0f, 0.0f, 0.0f};
	float bitangent[3] = {0.0f, 0.0f, 0.0f};
	float det = 0.0f;
	float nDotT = 0.0f;
	float bDotB = 0.0f;
	float invlength = 0.0f;
	int totalVertices = getNumberOfVertices();
	int totalTriangles = getNumberOfTriangles();

	// Initialize all the vertex tangents and bitangents.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertexArray[i];

		pVertex0->tangent[0] = 0.0f;
		pVertex0->tangent[1] = 0.0f;
		pVertex0->tangent[2] = 0.0f;
		pVertex0->tangent[3] = 0.0f;

		pVertex0->bitangent[0] = 0.0f;
		pVertex0->bitangent[1] = 0.0f;
		pVertex0->bitangent[2] = 0.0f;
	}

	// Calculate the vertex tangents and bitangents.
	for (int i=0; i<totalTriangles; ++i)
	{
		pTriangle = &m_indexArray[i*3];

		pVertex0 = &m_vertexArray[pTriangle[0]];
		pVertex1 = &m_vertexArray[pTriangle[1]];
		pVertex2 = &m_vertexArray[pTriangle[2]];

		// Calculate the triangle face tangent and bitangent.

		edge1[0] = static_cast<float>(pVertex1->position[0] - pVertex0->position[0]);
		edge1[1] = static_cast<float>(pVertex1->position[1] - pVertex0->position[1]);
		edge1[2] = static_cast<float>(pVertex1->position[2] - pVertex0->position[2]);

		edge2[0] = static_cast<float>(pVertex2->position[0] - pVertex0->position[0]);
		edge2[1] = static_cast<float>(pVertex2->position[1] - pVertex0->position[1]);
		edge2[2] = static_cast<float>(pVertex2->position[2] - pVertex0->position[2]);

		texEdge1[0] = pVertex1->texCoord[0] - pVertex0->texCoord[0];
		texEdge1[1] = pVertex1->texCoord[1] - pVertex0->texCoord[1];

		texEdge2[0] = pVertex2->texCoord[0] - pVertex0->texCoord[0];
		texEdge2[1] = pVertex2->texCoord[1] - pVertex0->texCoord[1];

		det = texEdge1[0]*texEdge2[1] - texEdge2[0]*texEdge1[1];

		if (fabs(det) < 1e-6f)
		{
			tangent[0] = 1.0f;
			tangent[1] = 0.0f;
			tangent[2] = 0.0f;

			bitangent[0] = 0.0f;
			bitangent[1] = 1.0f;
			bitangent[2] = 0.0f;
		}
		else
		{
			det = 1.0f / det;

			tangent[0] = (texEdge2[1]*edge1[0] - texEdge1[1]*edge2[0])*det;
			tangent[1] = (texEdge2[1]*edge1[1] - texEdge1[1]*edge2[1])*det;
			tangent[2] = (texEdge2[1]*edge1[2] - texEdge1[1]*edge2[2])*det;

			bitangent[0] = (-texEdge2[0]*edge1[0] + texEdge1[0]*edge2[0])*det;
			bitangent[1] = (-texEdge2[0]*edge1[1] + texEdge1[0]*edge2[1])*det;
			bitangent[2] = (-texEdge2[0]*edge1[2] + texEdge1[0]*edge2[2])*det;
		}

		// Accumulate the tangents and bitangents.

		pVertex0->tangent[0] += tangent[0];
		pVertex0->tangent[1] += tangent[1];
		pVertex0->tangent[2] += tangent[2];
		pVertex0->bitangent[0] += bitangent[0];
		pVertex0->bitangent[1] += bitangent[1];
		pVertex0->bitangent[2] += bitangent[2];

		pVertex1->tangent[0] += tangent[0];
		pVertex1->tangent[1] += tangent[1];
		pVertex1->tangent[2] += tangent[2];
		pVertex1->bitangent[0] += bitangent[0];
		pVertex1->bitangent[1] += bitangent[1];
		pVertex1->bitangent[2] += bitangent[2];

		pVertex2->tangent[0] += tangent[0];
		pVertex2->tangent[1] += tangent[1];
		pVertex2->tangent[2] += tangent[2];
		pVertex2->bitangent[0] += bitangent[0];
		pVertex2->bitangent[1] += bitangent[1];
		pVertex2->bitangent[2] += bitangent[2];
	}

	// Orthogonalize and normalize the vertex tangents.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertexArray[i];

		// Gram-Schmidt orthogonalize tangent with normal.

		nDotT = pVertex0->normal[0]*pVertex0->tangent[0] +
			pVertex0->normal[1]*pVertex0->tangent[1] +
			pVertex0->normal[2]*pVertex0->tangent[2];

		pVertex0->tangent[0] -= pVertex0->normal[0]*nDotT;
		pVertex0->tangent[1] -= pVertex0->normal[1]*nDotT;
		pVertex0->tangent[2] -= pVertex0->normal[2]*nDotT;

		// Normalize the tangent.

		invlength = 1.0f / sqrtf(pVertex0->tangent[0]*pVertex0->tangent[0] +
				      pVertex0->tangent[1]*pVertex0->tangent[1] +
				      pVertex0->tangent[2]*pVertex0->tangent[2]);

		pVertex0->tangent[0] *= invlength;
		pVertex0->tangent[1] *= invlength;
		pVertex0->tangent[2] *= invlength;

		// Calculate the handedness of the local tangent space.
		// The bitangent vector is the cross product between the triangle face
		// normal vector and the calculated tangent vector. The resulting
		// bitangent vector should be the same as the bitangent vector
		// calculated from the set of linear equations above. If they point in
		// different directions then we need to invert the cross product
		// calculated bitangent vector. We store this scalar multiplier in the
		// tangent vector's 'w' component so that the correct bitangent vector
		// can be generated in the normal mapping shader's vertex shader.
		//
		// Normal maps have a left handed coordinate system with the origin
		// located at the top left of the normal map texture. The x coordinates
		// run horizontally from left to right. The y coordinates run
		// vertically from top to bottom. The z coordinates run out of the
		// normal map texture towards the viewer. Our handedness calculations
		// must take this fact into account as well so that the normal mapping
		// shader's vertex shader will generate the correct bitangent vectors.
		// Some normal map authoring tools such as Crazybump
		// (http://www.crazybump.com/) includes options to allow you to control
		// the orientation of the normal map normal's y-axis.

		bitangent[0] = (pVertex0->normal[1]*pVertex0->tangent[2]) -
			       (pVertex0->normal[2]*pVertex0->tangent[1]);
		bitangent[1] = (pVertex0->normal[2]*pVertex0->tangent[0]) -
			       (pVertex0->normal[0]*pVertex0->tangent[2]);
		bitangent[2] = (pVertex0->normal[0]*pVertex0->tangent[1]) -
			       (pVertex0->normal[1]*pVertex0->tangent[0]);

		bDotB = bitangent[0]*pVertex0->bitangent[0] +
			bitangent[1]*pVertex0->bitangent[1] +
			bitangent[2]*pVertex0->bitangent[2];

		pVertex0->tangent[3] = (bDotB < 0.0f) ? 1.0f : -1.0f;

		pVertex0->bitangent[0] = bitangent[0];
		pVertex0->bitangent[1] = bitangent[1];
		pVertex0->bitangent[2] = bitangent[2];
	}

	m_hasTangents = true;
}

//! The first pass finds out how many triangles and vertices the model has, and imports the materials.
void OBJ::importFirstPass(QFile& pFile, MatCacheT& materialCache)
{

	m_hasTextureCoords = false;
	m_hasNormals = false;

	m_numberOfVertexCoords = 0;
	m_numberOfTextureCoords = 0;
	m_numberOfNormals = 0;
	m_numberOfTriangles = 0;

	QString line;
	QString buffer;
	QString name;

	QTextStream fileStream(&pFile),lineStream;

	//read a line while !EOF
	while ( !(line = fileStream.readLine()).isNull() )
	{
		lineStream.setString(&line,QIODevice::ReadOnly);

		//read the first word of the line
		lineStream>>buffer;

		//check if the line is empty
		if (buffer.isEmpty())
			continue;

		//check the first char of the line
		switch (buffer.at(0).toLatin1())
		{
			case 'f':   //! v, v//vn, v/vt or v/vt/vn.
				//hit a face definition
				//this can have a variable number of vertices

				//TODO: make this more robust against invalid files
				//the old version was not better, really

				//read first triangle
				lineStream>>buffer;
				lineStream>>buffer;
				lineStream>>buffer;
				++m_numberOfTriangles;

				//each VTX added now is a new triangle
				while(!lineStream.atEnd())
				{
					lineStream>>buffer;
					if(!buffer.isEmpty())
						++m_numberOfTriangles;
				}
				break;

			case 'm':   //! mtllib
				//found material, load it
				lineStream>>buffer;
				name = m_basePath;
				name += buffer;
				importMaterials(name, materialCache);
				break;

			case 'v':   //! v, vt, or vn
				//hit a vertex, texture or normal, add 1 to counter
				if (buffer.size()==1)
					++m_numberOfVertexCoords;
				else
				{
					switch (buffer.at(1).toLatin1())
					{
						case 't':
							++m_numberOfTextureCoords;
							break;
						case 'n':
							++m_numberOfNormals;
							break;
					}
				}
				break;

			default:
				//noop
				break;
		}
	}

	m_hasPositions = m_numberOfVertexCoords > 0;
	m_hasNormals = m_numberOfNormals > 0;
	m_hasTextureCoords = m_numberOfTextureCoords > 0;

	m_indexArray.resize(m_numberOfTriangles * 3);

	// Define a default material if no materials were loaded.
	if (m_numberOfMaterials == 0)
	{
		Material defaultMaterial;

		m_materials.push_back(defaultMaterial);
		materialCache[defaultMaterial.name] = 0;
	}
}

void OBJ::importSecondPass(FILE* pFile, const vertexOrder order, const MatCacheT& materialCache)
{
	//TODO convert to Qt IO functions (QFile)

	int v[3] = {0};
	int vt[3] = {0};
	int vn[3] = {0};
	unsigned int numVertices = 0;
	unsigned int numTexCoords = 0;
	unsigned int numNormals = 0;
	unsigned int numTriangles = 0;
	int activeMaterial = 0;
	int activeObject = 0;
	char buffer[1024] = {0};
	QString name;
	QMap<QString,int>::const_iterator iter;

	//these were member variables before, but were only used during loading, so we just define them here to save some memory
	AttributeVector attributeArray(m_numberOfTriangles);
	PosVector vertexCoords(m_numberOfVertexCoords);
	VF2Vector textureCoords(m_numberOfTextureCoords);
	VF3Vector normals(m_numberOfNormals);
	VertCacheT vertexCache;

	#define ADD_ATTRIB addFaceAttrib(attributeArray,numTriangles,activeMaterial,activeObject);

	float fTmp[3] = {0.0f};
	double dTmp[3] = {0.0};
	Vec3f tmpNrm;

	while (fscanf(pFile, "%s", buffer) != EOF)
	{
		switch (buffer[0])
		{
			case 'f': //! v, v//vn, v/vt, or v/vt/vn.
				v[0]  = v[1]  = v[2]  = 0;
				vt[0] = vt[1] = vt[2] = 0;
				vn[0] = vn[1] = vn[2] = 0;

				fscanf(pFile, "%s", buffer);

				if (strstr(buffer, "//")) //! v//vn
				{
					sscanf(buffer, "%d//%d", &v[0], &vn[0]);
					fscanf(pFile, "%d//%d", &v[1], &vn[1]);
					fscanf(pFile, "%d//%d", &v[2], &vn[2]);

					v[0] = (v[0] < 0) ? v[0]+numVertices-1 : v[0]-1;
					v[1] = (v[1] < 0) ? v[1]+numVertices-1 : v[1]-1;
					v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;

					vn[0] = (vn[0] < 0) ? vn[0]+numNormals-1 : vn[0]-1;
					vn[1] = (vn[1] < 0) ? vn[1]+numNormals-1 : vn[1]-1;
					vn[2] = (vn[2] < 0) ? vn[2]+numNormals-1 : vn[2]-1;

					ADD_ATTRIB
					addTrianglePosNormal(vertexCoords,normals,vertexCache,
							     numTriangles++,
							     v[0], v[1], v[2],
							vn[0], vn[1], vn[2]);

					v[1] = v[2];
					vn[1] = vn[2];

					while (fscanf(pFile, "%d//%d", &v[2], &vn[2]) > 0)
					{
						v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;
						vn[2] = (vn[2] < 0) ? vn[2]+numNormals-1 : vn[2]-1;

						ADD_ATTRIB
						addTrianglePosNormal(vertexCoords,normals,vertexCache,
								     numTriangles++,
								     v[0], v[1], v[2],
								vn[0], vn[1], vn[2]);

						v[1] = v[2];
						vn[1] = vn[2];
					}
				}
				else if (sscanf(buffer, "%d/%d/%d", &v[0], &vt[0], &vn[0]) == 3) //! v/vt/vn
				{
					fscanf(pFile, "%d/%d/%d", &v[1], &vt[1], &vn[1]);
					fscanf(pFile, "%d/%d/%d", &v[2], &vt[2], &vn[2]);

					v[0] = (v[0] < 0) ? v[0]+numVertices-1 : v[0]-1;
					v[1] = (v[1] < 0) ? v[1]+numVertices-1 : v[1]-1;
					v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;

					vt[0] = (vt[0] < 0) ? vt[0]+numTexCoords-1 : vt[0]-1;
					vt[1] = (vt[1] < 0) ? vt[1]+numTexCoords-1 : vt[1]-1;
					vt[2] = (vt[2] < 0) ? vt[2]+numTexCoords-1 : vt[2]-1;

					vn[0] = (vn[0] < 0) ? vn[0]+numNormals-1 : vn[0]-1;
					vn[1] = (vn[1] < 0) ? vn[1]+numNormals-1 : vn[1]-1;
					vn[2] = (vn[2] < 0) ? vn[2]+numNormals-1 : vn[2]-1;

					ADD_ATTRIB
					addTrianglePosTexCoordNormal(vertexCoords,textureCoords,normals,vertexCache,
								     numTriangles++,
								     v[0], v[1], v[2],
								     vt[0], vt[1], vt[2],
								     vn[0], vn[1], vn[2]);

					v[1] = v[2];
					vt[1] = vt[2];
					vn[1] = vn[2];

					while (fscanf(pFile, "%d/%d/%d", &v[2], &vt[2], &vn[2]) > 0)
					{
						v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;
						vt[2] = (vt[2] < 0) ? vt[2]+numTexCoords-1 : vt[2]-1;
						vn[2] = (vn[2] < 0) ? vn[2]+numNormals-1 : vn[2]-1;

						ADD_ATTRIB
						addTrianglePosTexCoordNormal(vertexCoords,textureCoords,normals,vertexCache,
									     numTriangles++,
									     v[0], v[1], v[2],
									     vt[0], vt[1], vt[2],
									     vn[0], vn[1], vn[2]);

						v[1] = v[2];
						vt[1] = vt[2];
						vn[1] = vn[2];
					}
				}
				else if (sscanf(buffer, "%d/%d", &v[0], &vt[0]) == 2) //! v/vt
				{
					fscanf(pFile, "%d/%d", &v[1], &vt[1]);
					fscanf(pFile, "%d/%d", &v[2], &vt[2]);

					v[0] = (v[0] < 0) ? v[0]+numVertices-1 : v[0]-1;
					v[1] = (v[1] < 0) ? v[1]+numVertices-1 : v[1]-1;
					v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;

					vt[0] = (vt[0] < 0) ? vt[0]+numTexCoords-1 : vt[0]-1;
					vt[1] = (vt[1] < 0) ? vt[1]+numTexCoords-1 : vt[1]-1;
					vt[2] = (vt[2] < 0) ? vt[2]+numTexCoords-1 : vt[2]-1;

					ADD_ATTRIB
					addTrianglePosTexCoord(vertexCoords,textureCoords,vertexCache,
							       numTriangles++,
							       v[0], v[1], v[2],
							       vt[0], vt[1], vt[2]);

					v[1] = v[2];
					vt[1] = vt[2];

					while (fscanf(pFile, "%d/%d", &v[2], &vt[2]) > 0)
					{
						v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;
						vt[2] = (vt[2] < 0) ? vt[2]+numTexCoords-1 : vt[2]-1;

						ADD_ATTRIB
						addTrianglePosTexCoord(vertexCoords,textureCoords,vertexCache,
								       numTriangles++,
								       v[0], v[1], v[2],
								       vt[0], vt[1], vt[2]);

						v[1] = v[2];
						vt[1] = vt[2];
					}
				}
				else //! v
				{
					sscanf(buffer, "%d", &v[0]);
					fscanf(pFile, "%d", &v[1]);
					fscanf(pFile, "%d", &v[2]);

					v[0] = (v[0] < 0) ? v[0]+numVertices-1 : v[0]-1;
					v[1] = (v[1] < 0) ? v[1]+numVertices-1 : v[1]-1;
					v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;

					ADD_ATTRIB
					addTrianglePos(vertexCoords, vertexCache, numTriangles++, v[0], v[1], v[2]);

					v[1] = v[2];

					while (fscanf(pFile, "%d", &v[2]) > 0)
					{
						v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;

						ADD_ATTRIB
						addTrianglePos(vertexCoords, vertexCache, numTriangles++, v[0], v[1], v[2]);

						v[1] = v[2];
					}
				}
				break;

			case 'u': //! usemtl
				fgets(buffer, sizeof(buffer), pFile);
				sscanf(buffer, "%s %s", buffer, buffer);
				name = buffer;
				iter = materialCache.find(buffer);
				activeMaterial = (iter == materialCache.end()) ? 0 : iter.value();
				break;
			case 'o':
			case 'g':
				//do this to make it skip the rest of the line, important!
				fgets(buffer, sizeof(buffer), pFile);
				//grouping separators, we consider treat o and g the same in that they may require splitting of objects
				//we ignore the grouping name
				++activeObject;
				break;

			case 'v': //! v, vn, or vt.
				switch (buffer[1])
				{
					case '\0': //! v
						fscanf(pFile, "%lf %lf %lf", &dTmp[0], &dTmp[1], &dTmp[2]);

						switch(order)
						{
							case XYZ:
								vertexCoords[numVertices] = VPos(dTmp[0],dTmp[1],dTmp[2]);
								break;
							case XZY:
								vertexCoords[numVertices] = VPos(dTmp[0],-dTmp[2],dTmp[1]);
								break;
							case YXZ:
								vertexCoords[numVertices] = VPos(dTmp[1],dTmp[0],dTmp[2]);
								break;
							case YZX:
								vertexCoords[numVertices] = VPos(dTmp[1],dTmp[2],dTmp[0]);
								break;
							case ZXY:
								vertexCoords[numVertices] = VPos(dTmp[2],dTmp[0],dTmp[1]);
								break;
							case ZYX:
								vertexCoords[numVertices] = VPos(dTmp[2],dTmp[1],dTmp[0]);
								break;
							default:
								Q_ASSERT(0);
								qDebug() << "OBJ::importSecondPass() vertex order not implemented. assuming XYZ.";
								vertexCoords[numVertices] = VPos(dTmp[0],dTmp[1],dTmp[2]);
								break;
						}

						++numVertices;
						break;

					case 'n': //! vn
						fscanf(pFile, "%f %f %f", &fTmp[0], &fTmp[1], &fTmp[2]);

						switch(order)
						{
							// Only the first two are known in practice.
							case XYZ:
								tmpNrm.set(fTmp[0],fTmp[1],fTmp[2]);
								break;
							case XZY:
								tmpNrm.set(fTmp[0], -fTmp[2], fTmp[1]);
								break;
							default: // all others: complain, but process as XYZ
								qDebug() << "OBJ::importSecondPass() vertex order for normals not implemented. assuming XYZ.";
								tmpNrm.set(fTmp[0],fTmp[1],fTmp[2]);
								break;
						}

						tmpNrm.normalize();
						normals[numNormals] = tmpNrm;
						++numNormals;
						break;

					case 't': //! vt
						fscanf(pFile, "%f %f", &fTmp[0], &fTmp[1]);
						textureCoords[numTexCoords] = Vec2f(fTmp[0],fTmp[1]);
						++numTexCoords;
						break;

					default:
						//Everything else shall not be parsed
						break;
				}
				break;

			default:
				fgets(buffer, sizeof(buffer), pFile);
				break;
		}
	}

	//some sanity assertions to notice loader errors as they happen
	Q_ASSERT(numVertices == m_numberOfVertexCoords);
	Q_ASSERT(numNormals == m_numberOfNormals);
	Q_ASSERT(numTexCoords == m_numberOfTextureCoords);
	Q_ASSERT(numTriangles == m_numberOfTriangles);

	//Build the StelModels
	buildStelModels(attributeArray);
	m_hasStelModels = m_numberOfStelModels > 0;
}

bool OBJ::importMaterials(const QString& filename, MatCacheT& materialCache)
{
	//TODO convert to Qt IO functions (Unicode filenames, type safety, security...)
	QByteArray ba = filename.toLatin1();
	FILE* pFile = fopen(ba.constData(), "r");

	if (!pFile)
		return false;

	Material* pMaterial = 0;
	int iTmp = 0;
	int numMaterials = 0;
	char buffer[256] = {0};

	// Count the number of materials in the MTL file.
	while (fscanf(pFile, "%s", buffer) != EOF)
	{
		switch (buffer[0])
		{
			case 'n': //! newmtl
				++numMaterials;
				fgets(buffer, sizeof(buffer), pFile);
				sscanf(buffer, "%s %s", buffer, buffer);
				break;

			default:
				fgets(buffer, sizeof(buffer), pFile);
				break;
		}
	}

	rewind(pFile);

	m_numberOfMaterials = numMaterials;
	numMaterials = 0;
	m_materials.resize(m_numberOfMaterials);

	// Load the materials in the MTL file.
	while (fscanf(pFile, "%s", buffer) != EOF)
	{
		switch (buffer[0])
		{
			case 'N':
				switch (buffer[1])
				{
					case 's': //! Ns
						fscanf(pFile, "%f", &pMaterial->shininess);
						pMaterial->shininess = qMin(128.0f, pMaterial->shininess);
						break;

					default:
						break;
				}
				break;

			case 'K': // Ka, Kd, or Ks
				switch (buffer[1])
				{
					case 'a': //! Ka
						fscanf(pFile, "%f %f %f",
						       &pMaterial->ambient[0],
						       &pMaterial->ambient[1],
						       &pMaterial->ambient[2]);
						break;

					case 'd': //! Kd
						fscanf(pFile, "%f %f %f",
						       &pMaterial->diffuse[0],
						       &pMaterial->diffuse[1],
						       &pMaterial->diffuse[2]);
						break;

					case 's': //! Ks
						fscanf(pFile, "%f %f %f",
						       &pMaterial->specular[0],
						       &pMaterial->specular[1],
						       &pMaterial->specular[2]);
						break;

					case 'e': //! Ke
						fscanf(pFile, "%f %f %f",
						       &pMaterial->emission[0],
						       &pMaterial->emission[1],
						       &pMaterial->emission[2]);
						break;

					default:
						fgets(buffer, sizeof(buffer), pFile);
						break;
				}
				break;

			case 'T': //! Tr
				switch (buffer[1])
				{
					case 'r': //! Tr
						fscanf(pFile, "%f", &pMaterial->alpha);
						//preliminarily mark as transparent for building of StelModels
						pMaterial->hasTransparency = true;
						break;

					default:
						fgets(buffer, sizeof(buffer), pFile);
						break;
				}
				break;

			case 'd':
				fscanf(pFile, "%f", &pMaterial->alpha);
				//preliminarily mark as transparent for building of StelModels
				pMaterial->hasTransparency = true;
				break;

			case 'i': // illum
				fscanf(pFile, "%d", &iTmp);

				if(iTmp>Material::I_SPECULAR && iTmp != Material::I_TRANSLUCENT)
				{
					qWarning()<<"[OBJ] Treating illum "<<iTmp<<"as TRANSLUCENT";
					iTmp = Material::I_TRANSLUCENT;
				}

				pMaterial->illum = (Material::Illum) iTmp;
				if(pMaterial->illum == Material::I_TRANSLUCENT)
				{
					//preliminarily mark as transparent for building of StelModels
					pMaterial->hasTransparency = true;
				}
				break;

			case 'm': //! map_Kd, map_bump
				if (strstr(buffer, "map_Kd") != 0)
				{
					fgets(buffer, sizeof(buffer), pFile);
					sscanf(buffer, "%[^\n]", buffer);

					pMaterial->textureName = parseTextureString(buffer);
				}
				else if (strstr(buffer, "map_Ke") != 0)
				{
					fgets(buffer, sizeof(buffer), pFile);
					sscanf(buffer, "%[^\n]", buffer);

					pMaterial->emissiveMapName = parseTextureString(buffer);
				}
				else if (strstr(buffer, "map_bump") != 0)
				{
					fgets(buffer, sizeof(buffer), pFile);
					sscanf(buffer, "%[^\n]", buffer);

					pMaterial->bumpMapName =  parseTextureString(buffer);
				}
				else if (strstr(buffer, "map_height") != 0)
				{
					fgets(buffer, sizeof(buffer), pFile);
					sscanf(buffer, "%[^\n]", buffer);

					pMaterial->heightMapName =  parseTextureString(buffer);
				}
				else
				{
					fgets(buffer, sizeof(buffer), pFile);
				}
				break;

			case 'n': //! newmtl
				fgets(buffer, sizeof(buffer), pFile);
				sscanf(buffer, "%s %s", buffer, buffer);

				//use default constructor here to prevent manual re-initialization, and make sure default values are honored
				m_materials[numMaterials] = Material();
				pMaterial = &m_materials[numMaterials];
				pMaterial->name = buffer;

				materialCache[pMaterial->name] = numMaterials;
				++numMaterials;
				break;

			case 'b': // bool flags alphatest or backfaceculling
				if (strstr(buffer, "bAlphatest") != 0)
				{
					fscanf(pFile, "%d", &iTmp);
					pMaterial->alphatest = iTmp;
				}
				else if (strstr(buffer, "bBackface") != 0)
				{
					fscanf(pFile, "%d", &iTmp);
					pMaterial->backfacecull = !iTmp;
				}
				break;

			default:
				fgets(buffer, sizeof(buffer), pFile);
				break;
		}
	}

	fclose(pFile);

	return true;
}

QString OBJ::parseTextureString(const char *buffer) const
{
	QString str(buffer);
	//trim whitespace
	str = str.trimmed();
	//replace backslashes with slashes
	return str.replace('\\','/');
}

void OBJ::Material::finalize()
{
	//check if texture has an alpha channel
	bool alphaChannel = false;
	if(!texture.isNull())
		alphaChannel = texture->hasAlphaChannel();


	//test if specular coefficient and shininess is non-zero
	hasSpecularity = specular.lengthSquared()>0.0001f  && shininess > 0.0001f;

	//test if we require blending
	if(alpha< .0f)
	{
		//no alpha value set, no transparency
		hasTransparency = false;
	}
	else if (alpha <1.0f)
	{
		//alpha value set, between 0 and 1
		hasTransparency = true;
	}
	else
	{
		//alpha = 1, check if texture has alpha channel, otherwise it makes no sense enabling transparency
		hasTransparency = alphaChannel;
	}

	if(illum>=0)
	{
		if(ambient[0]< .0f) //set to old default value
			ambient = QVector3D(0.2f,0.2f,0.2f);

		//an "Illum" was set, replace with "legacy" values
		switch(illum)
		{
			case I_DIFFUSE:
				//explicitely set Ka to Kd
				ambient = diffuse;
				hasSpecularity = false;
				hasTransparency = false;
				break;
			case I_DIFFUSE_AND_AMBIENT:
				//enable specular
				hasSpecularity = false;
				hasTransparency = false;
				break;
			case I_SPECULAR:
				hasSpecularity = true;
				hasTransparency = false;
				break;
			case I_TRANSLUCENT:
				hasSpecularity = false;
				if(alpha<.0f)
				{
					alpha = 1.0f;
					hasTransparency = alphaChannel;
				}
				else
				{
					hasTransparency = true;
				}
				break;
			default:
				break;
		}
	}
	else
	{
		if(ambient[0]< .0f)
		{
			//ambient was not set, old "illum 0" behaviour, set Ka to Kd
			ambient = diffuse;
		}

		if(alpha <.0f)
		{
			alpha = 1.0f;
		}
	}
}

Vec3f zSortValue;
bool zSortFunction(const OBJ::StelModel& mLeft, const OBJ::StelModel& mRight)
{
	float dist1 = (mLeft.centroid - zSortValue).lengthSquared();
	float dist2 = (mRight.centroid - zSortValue).lengthSquared();
	return dist1>dist2;
}

void OBJ::transparencyDepthSort(const Vec3f &position)
{
	if(m_firstTransparentIndex>=0)
	{
		zSortValue = position;
		std::sort(m_stelModels.begin()+m_firstTransparentIndex, m_stelModels.end(),zSortFunction);
	}
}

void OBJ::uploadTexturesGL()
{
	StelTextureMgr textureMgr;

	for(unsigned int i=0; i<m_numberOfMaterials; ++i)
	{
		Material* pMaterial = &getMaterial(i);

//        qDebug() << getTime() << "[OBJ]" << pMaterial->name.c_str();
//        qDebug() << getTime() << "[OBJ] Ka:" << pMaterial->ambient[0] << "," << pMaterial->ambient[1] << "," << pMaterial->ambient[2] << "," << pMaterial->ambient[3];
//        qDebug() << getTime() << "[OBJ] Kd:" << pMaterial->diffuse[0] << "," << pMaterial->diffuse[1] << "," << pMaterial->diffuse[2] << "," << pMaterial->diffuse[3];
//        qDebug() << getTime() << "[OBJ] Ks:" << pMaterial->specular[0] << "," << pMaterial->specular[1] << "," << pMaterial->specular[2] << "," << pMaterial->specular[3];
//        qDebug() << getTime() << "[OBJ] Shininess:" << pMaterial->shininess;
//        qDebug() << getTime() << "[OBJ] Alpha:" << pMaterial->alpha;
//        qDebug() << getTime() << "[OBJ] Illum:" << pMaterial->illum;

//        qDebug() << getTime() << "[OBJ] Uploading textures for Material: " << pMaterial->name.c_str();
//        qDebug() << getTime() << "[OBJ] Texture:" << pMaterial->textureName.c_str();
		if(!pMaterial->textureName.isEmpty())
		{
			StelTextureSP tex = textureMgr.createTexture(absolutePath(pMaterial->textureName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true));
			if(!tex.isNull())
			{
				pMaterial->texture = tex;
			}
			else
			{
				qWarning() << "[OBJ] Failed to load Texture:" << pMaterial->textureName;
			}
		}

		if(!pMaterial->emissiveMapName.isEmpty())
		{
			StelTextureSP tex = textureMgr.createTexture(absolutePath(pMaterial->emissiveMapName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true));
			if(!tex.isNull())
			{
				pMaterial->emissive_texture = tex;
			}
			else
			{
				qWarning() << "[OBJ] Failed to load emissive texture:" << pMaterial->emissiveMapName;
			}
		}

		//qDebug() << getTime() << "[OBJ] Normal Map:" << pMaterial->bumpMapName;
		if(!pMaterial->bumpMapName.isEmpty())
		{
			StelTextureSP bumpTex = textureMgr.createTexture(absolutePath(pMaterial->bumpMapName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true));
			if(!bumpTex.isNull())
			{
				pMaterial->bump_texture = bumpTex;
			}
			else
			{
				qWarning() << "[OBJ] Failed to load Normal Map:" << pMaterial->bumpMapName;
			}
		}

		//qDebug() << getTime() << "[OBJ] Height Map:" << pMaterial->heightMapName;
		if(!pMaterial->heightMapName.isEmpty())
		{
			StelTextureSP heightTex = textureMgr.createTexture(absolutePath(pMaterial->heightMapName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true));
			if(!heightTex.isNull())
			{
				pMaterial->height_texture = heightTex;
			}
			else
			{
				qWarning() << "[OBJ] Failed to load Height Map:" << pMaterial->heightMapName;
			}
		}
	}

	qDebug()<<"[OBJ] Uploaded OBJ textures to GL";
}

void OBJ::finalizeForRendering()
{
	//finalize all materials
	//we have texture information, now finalize the material
	for(int i = 0;i<m_materials.size();++i)
	{
		m_materials[i].finalize();
	}

	//optimize the StelModel order depending on their materials
	//among other criteria, it is always ensured that the transparent objects are now together among the end of the list
	std::sort(m_stelModels.begin(), m_stelModels.end(), StelModelCompFunc);

	//find the first stelmodel that is transparent
	//this is required for the depth sorting
	m_firstTransparentIndex = -1;
	for(int i=0;i<m_stelModels.size();++i)
	{
		if(m_stelModels.at(i).pMaterial->hasTransparency)
		{
			m_firstTransparentIndex = i;
			break;
		}
	}
}

void OBJ::uploadBuffersGL()
{
	if(vertexArraysSupported)
	{
		m_vertexArrayObject->create();
		m_vertexArrayObject->bind();
	}

	if(m_vertexBuffer.create() && m_indexBuffer.create())
	{
		if(m_vertexBuffer.bind())
		{
			m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
			//this is the upload
			m_vertexBuffer.allocate(m_vertexArray.constData(), sizeof(Vertex) * m_vertexArray.size());
			m_vertexBuffer.release();
		}
		else
		{
			qCritical()<<"[OBJ] Could not bind vertex buffer";
			m_vertexBuffer.destroy();
			m_indexBuffer.destroy();
			return;
		}

		if(m_indexBuffer.bind())
		{
			m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
			if(OBJ::indexBufferType == GL_UNSIGNED_INT)
			{
				//we can directly upload the index array
				m_indexBuffer.allocate(m_indexArray.constData(), sizeof(unsigned int) * m_indexArray.size());
			}
			else
			{
				Q_ASSERT(OBJ::indexBufferType == GL_UNSIGNED_SHORT);

				//TODO: find a way to skip this conversion
				//This is probably not so easy, because dynamic ANGLE/OGL builds may require a runtime selection between the 2 versions
				//and m_indexArray is used in quite a lot of places including the Heightmap class.

				//we have to convert to short
				QVector<unsigned short> indices;
				indices.resize(m_indexArray.size());
				for(int i =0;i<m_indexArray.size();++i)
				{
					indices[i] = m_indexArray[i];
				}

				//now upload the new data
				m_indexBuffer.allocate(indices.constData(), sizeof(unsigned short) * indices.size());
			}
			m_indexBuffer.release();
		}
		else
		{
			qCritical()<<"[OBJ] Could not bind index buffer";
			m_vertexBuffer.destroy();
			m_indexBuffer.destroy();
			return;
		}
	}
	else
	{
		qCritical()<<"[OBJ] Could not create OpenGL buffers!";
		m_vertexBuffer.destroy();
		m_indexBuffer.destroy();
		return;
	}

	if(vertexArraysSupported)
	{
		//binding and setting vertex attribs, stored in VAO
		bindBuffersGL();
		m_vertexArrayObject->release();
		unbindBuffersGL();
	}
	//TODO maybe release the client-side memory

	qDebug()<<"[OBJ] Uploaded OBJ vertex and index data to GL";
}

void OBJ::bindGL()
{
	if(vertexArraysSupported)
		m_vertexArrayObject->bind();
	else
		bindBuffersGL();
}

void OBJ::unbindGL()
{
	if(vertexArraysSupported)
		m_vertexArrayObject->release();
	else
	{
		unbindBuffersGL();
	}
}

void OBJ::bindBuffersGL()
{
	m_vertexBuffer.bind();

	//using qt wrappers here is not possible because the only implementation is in QOpenGLShaderProgram, which requires a shader program instance
	//this is a bit incorrect, because the following is global state that does not depend on a shader
	//(but may be stored in a VAO to enable faster binding/unbinding)

	//enable the attrib arrays
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_VERTEX);
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_NORMAL);
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_TEXCOORD);
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_TANGENT);
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_BITANGENT);

	const GLsizei stride = sizeof(Vertex);

	glVertexAttribPointer(ShaderMgr::ATTLOC_VERTEX,   3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, position)));
	glVertexAttribPointer(ShaderMgr::ATTLOC_NORMAL,   3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, normal)));
	glVertexAttribPointer(ShaderMgr::ATTLOC_TEXCOORD, 2,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, texCoord)));
	glVertexAttribPointer(ShaderMgr::ATTLOC_TANGENT,  4,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, tangent)));
	glVertexAttribPointer(ShaderMgr::ATTLOC_BITANGENT,3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, bitangent)));

	//vertex buffer does not need to remain bound, because the binding is stored by glVertexAttribPointer
	m_vertexBuffer.release();

	//index buffer must remain bound
	m_indexBuffer.bind();
}

void OBJ::unbindBuffersGL()
{
	//unbind the index buffer (vertex buffer is NOT bound by bindBuffersGL
	m_indexBuffer.release();

	//disable our attribute arrays
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_VERTEX);
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_NORMAL);
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_TEXCOORD);
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_TANGENT);
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_BITANGENT);
}

void OBJ::transform(QMatrix4x4 mat)
{
	QMatrix3x3 normalMat = mat.normalMatrix();

	//Transform all vertices and normals by mat
	for(int i=0; i<getNumberOfVertices(); ++i)
	{
		Vertex *pVertex = &m_vertexArray[i];

		QVector3D tf = mat * QVector3D(pVertex->position[0], pVertex->position[1], pVertex->position[2]);
		pVertex->position[0] = tf.x();
		pVertex->position[1] = tf.y();
		pVertex->position[2] = tf.z();

		tf = normalMat * QVector3D(pVertex->normal[0], pVertex->normal[1], pVertex->normal[2]);
		pVertex->normal[0] = tf.x();
		pVertex->normal[1] = tf.y();
		pVertex->normal[2] = tf.z();

		tf = normalMat * QVector3D(pVertex->tangent[0], pVertex->tangent[1], pVertex->tangent[2]);
		pVertex->tangent[0] = tf.x();
		pVertex->tangent[1] = tf.y();
		pVertex->tangent[2] = tf.z();

		tf = normalMat * QVector3D(pVertex->bitangent[0], pVertex->bitangent[1], pVertex->bitangent[2]);
		pVertex->bitangent[0] = tf.x();
		pVertex->bitangent[1] = tf.y();
		pVertex->bitangent[2] = tf.z();
	}

	//Update bounding box in case it changed
	findBounds();
}

void OBJ::findBounds()
{
	//Find Bounding Box for entire Scene
	pBoundingBox.reset();

	//Find AABB per Stel Model
	for(unsigned int i=0; i<m_numberOfStelModels; ++i)
	{
		StelModel& pStelModel = m_stelModels[i];
		pStelModel.bbox.reset();
		pStelModel.centroid = Vec3f(0.0f);

		for(int j=pStelModel.startIndex; j<(pStelModel.startIndex + pStelModel.triangleCount*3); ++j)
		{
			const Vertex& pVertex = m_vertexArray.at(m_indexArray[j]);

			Vec3f pos(pVertex.position[0],pVertex.position[1],pVertex.position[2]);

			pBoundingBox.expand(pos);
			pStelModel.bbox.expand(pos);
			pStelModel.centroid += pos;
		}

		pStelModel.centroid /= (pStelModel.triangleCount*3);
	}
}

void OBJ::renderAABBs()
{
	for(unsigned int i=0; i<m_numberOfStelModels; ++i)
	{
		m_stelModels.at(i).bbox.render();
	}
}

size_t OBJ::memoryUsage()
{
	size_t sz = sizeof(*this);
	sz+= m_stelModels.capacity() * (sizeof(StelModel) );
	sz+= m_materials.capacity() * sizeof(Material);
	sz+= m_vertexArray.capacity() * sizeof(Vertex);
	sz+= m_indexArray.capacity() * sizeof(unsigned int);
	return sz;
}

OBJ& OBJ::operator=(const OBJ& other)
{
	clean();

	m_loaded = other.m_loaded;
	m_hasPositions = other.m_hasPositions;
	m_hasTextureCoords = other.m_hasTextureCoords;
	m_hasNormals = other.m_hasNormals;
	m_hasTangents = other.m_hasTangents;
	m_hasStelModels = other.m_hasStelModels;

	m_firstTransparentIndex = other.m_firstTransparentIndex;

	m_numberOfVertexCoords = other.m_numberOfVertexCoords;
	m_numberOfTextureCoords = other.m_numberOfTextureCoords;
	m_numberOfNormals = other.m_numberOfNormals;
	m_numberOfTriangles = other.m_numberOfTriangles;
	m_numberOfMaterials = other.m_numberOfMaterials;
	m_numberOfStelModels = other.m_numberOfStelModels;

	pBoundingBox = other.pBoundingBox;

	m_basePath = other.m_basePath;

	m_stelModels = other.m_stelModels;
	m_materials = other.m_materials;
	m_vertexArray = other.m_vertexArray;
	m_indexArray = other.m_indexArray;

	return *this;
}
