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

#include <QOpenGLVertexArrayObject>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool StelModelCompFunc(const OBJ::StelModel& lhs, const OBJ::StelModel& rhs)
    {
        return lhs.pMaterial->alpha > rhs.pMaterial->alpha;
    }
}

bool OBJ::vertexArraysSupported=false;

//static function
void OBJ::setupGL()
{
	//disable VAOs on Intel because of serious bugs in their implemenation...
	QString vendor(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	if(vendor.contains("Intel",Qt::CaseInsensitive))
	{
		OBJ::vertexArraysSupported = false;
		qWarning()<<"[Scenery3d] Disabling VAO usage because of Intel bugs";
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
		qDebug()<<"[Scenery3d] Vertex Array Objects are supported";
	}
	else
	{
		qWarning()<<"[Scenery3d] Vertex Array Objects are not supported on your hardware";
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

    m_numberOfVertexCoords = 0;
    m_numberOfTextureCoords = 0;
    m_numberOfNormals = 0;
    m_numberOfTriangles = 0;
    m_numberOfMaterials = 0;
    m_numberOfStelModels = 0;

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

bool OBJ::load(const QString& filename, const enum vertexOrder order, bool rebuildNormals)
{
    QFile qtFile(filename);
    if(!qtFile.open(QIODevice::ReadOnly))
        return false;

    //Extract the base path, will be used to load the MTL file later on
    m_basePath.clear();
    m_basePath = StelFileMgr::dirName(filename) + "/";

    MatCacheT materialCache;

    //Parse the file
    importFirstPass(qtFile,materialCache);
    qtFile.close();

    //TODO make the second pass support Qt IO (unicode file handling, etc.)
    QByteArray ba = filename.toLocal8Bit();
    FILE* pFile = fopen(ba.constData(), "r");
    importSecondPass(pFile, order, materialCache);

    //Done parsing, close file
    fclose(pFile);


    //Find bounding extrema
    findBounds();

    //Create vertex normals if specified or required
    if(rebuildNormals)
    {
        generateNormals();
    }
    else
    {
        if(!hasNormals())
            generateNormals();
    }

    //Create tangents
    generateTangents();

    //Loaded
    qDebug() << getTime() << "[Scenery3d] Loaded OBJ successfully: " << filename;
    qDebug() << getTime() << "[Scenery3d] Triangles#: " << m_numberOfTriangles;
    qDebug() << getTime() << "[Scenery3d] Vertices#: " << m_numberOfVertexCoords<<" unique / "<< m_vertexArray.size()<<" total";
    qDebug() << getTime() << "[Scenery3d] Normals#: " << m_numberOfNormals;
    qDebug() << getTime() << "[Scenery3d] StelModels#: " << m_numberOfStelModels;
    qDebug() << getTime() << "[Scenery3d] Bounding Box";
    qDebug() << getTime() << "[Scenery3d] X: [" << pBoundingBox.min[0] << ", " << pBoundingBox.max[0] << "] ";
    qDebug() << getTime() << "[Scenery3d] Y: [" << pBoundingBox.min[1] << ", " << pBoundingBox.max[1] << "] ";
    qDebug() << getTime() << "[Scenery3d] Z: [" << pBoundingBox.min[2] << ", " << pBoundingBox.max[2] << "] ";

    m_loaded = true;
    return true;
}

void OBJ::addTrianglePos(const PosVector &vertexCoords, IntVector& attributeArray, VertCacheT& vertexCache, unsigned int index, int material, int v0, int v1, int v2)
{
    Vertex vertex;

    //Add the material index to the index for grouping models
    attributeArray[index] = material;

    //Add v0
    vertex.position = vertexCoords[v0];
    m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

    //Add v1
    vertex.position = vertexCoords[v1];
    m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

    //Add v2
    vertex.position = vertexCoords[v2];
    m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

void OBJ::addTrianglePosNormal(const PosVector &vertexCoords, const VF3Vector &normals, IntVector& attributeArray, VertCacheT& vertexCache,
			       unsigned int index, int material, int v0, int v1, int v2, int vn0, int vn1, int vn2)
{
    Vertex vertex;

    //Add the material index to the index for grouping models
    attributeArray[index] = material;

    //Add v0//vn0
    vertex.position = vertexCoords[v0];
    vertex.normal = normals[vn0];
    m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

    //Add v1//n1
    vertex.position = vertexCoords[v1];
    vertex.normal = normals[vn1];
    m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

    //Add v2//n2
    vertex.position = vertexCoords[v2];
    vertex.normal = normals[vn2];
    m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

void OBJ::addTrianglePosTexCoord(const PosVector &vertexCoords, const VF2Vector &textureCoords, IntVector& attributeArray, VertCacheT& vertexCache,
				 unsigned int index, int material, int v0, int v1, int v2, int vt0, int vt1, int vt2)
{
    Vertex vertex;

    //Add the material index to the index for grouping models
    attributeArray[index] = material;

    //Add v0/vt0
    vertex.position = vertexCoords[v0];
    vertex.texCoord = textureCoords[vt0];
    m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

    //Add v1/vt1
    vertex.position = vertexCoords[v1];
    vertex.texCoord = textureCoords[vt1];
    m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

    //Add v2/vt2
    vertex.position = vertexCoords[v2];
    vertex.texCoord = textureCoords[vt2];
    m_indexArray[index*3+2] = addVertex(vertexCache, v2, &vertex);
}

void OBJ::addTrianglePosTexCoordNormal(PosVector& vertexCoords, const VF2Vector &textureCoords, const VF3Vector &normals, IntVector& attributeArray, VertCacheT& vertexCache,
				       unsigned int index, int material, int v0, int v1, int v2, int vt0, int vt1, int vt2, int vn0, int vn1, int vn2)
{
    Vertex vertex;

    //Add the material index to the index for grouping models
    attributeArray[index] = material;

    //Add v0/vt0/vn0
    vertex.position = vertexCoords[v0];
    vertex.texCoord = textureCoords[vt0];
    vertex.normal = normals[vn0];
    m_indexArray[index*3] = addVertex(vertexCache, v0, &vertex);

    //Add v1/vt1/vn1
    vertex.position = vertexCoords[v1];
    vertex.texCoord = textureCoords[vt1];
    vertex.normal = normals[vn1];
    m_indexArray[index*3+1] = addVertex(vertexCache, v1, &vertex);

    //Add v2/vt2/vn2
    vertex.position = vertexCoords[v2];
    vertex.texCoord = textureCoords[vt2];
    vertex.normal = normals[vn2];
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

void OBJ::buildStelModels(const IntVector& attributeArray)
{
    //Group model's triangles based on material
    StelModel* pStelModel = 0;
    int materialId = -1;
    int numStelModels = 0;

    // Count the number of meshes.
    for (int i=0; i<static_cast<int>(attributeArray.size()); ++i)
    {
	if (attributeArray[i] != materialId)
        {
	    materialId = attributeArray[i];
            ++numStelModels;
        }
    }

    // Allocate memory for the StelModel and reset counters.
    m_numberOfStelModels = numStelModels;
    m_stelModels.resize(m_numberOfStelModels);
    numStelModels = 0;
    materialId = -1;

    // Build the StelModel. One StelModel for each unique material.
    for (int i=0; i<static_cast<int>(attributeArray.size()); ++i)
    {
	if (attributeArray[i] != materialId)
        {
	    materialId = attributeArray[i];
            pStelModel = &m_stelModels[numStelModels++];
			pStelModel->triangleCount = 0;
            pStelModel->pMaterial = &m_materials[materialId];
            pStelModel->startIndex = i*3;
            ++pStelModel->triangleCount;
        }
        else
        {
            ++pStelModel->triangleCount;
        }
    }

    // Sort the meshes based on its material alpha. Fully opaque meshes
    // towards the front and fully transparent towards the back.
    std::sort(m_stelModels.begin(), m_stelModels.end(), StelModelCompFunc);
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
    float length = 0.0f;
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

        length = 1.0f / sqrtf(pVertex0->normal[0]*pVertex0->normal[0] +
                              pVertex0->normal[1]*pVertex0->normal[1] +
                              pVertex0->normal[2]*pVertex0->normal[2]);

        pVertex0->normal[0] *= length;
        pVertex0->normal[1] *= length;
        pVertex0->normal[2] *= length;
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
    float length = 0.0f;
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

        length = 1.0f / sqrtf(pVertex0->tangent[0]*pVertex0->tangent[0] +
                              pVertex0->tangent[1]*pVertex0->tangent[1] +
                              pVertex0->tangent[2]*pVertex0->tangent[2]);

        pVertex0->tangent[0] *= length;
        pVertex0->tangent[1] *= length;
        pVertex0->tangent[2] *= length;

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

    unsigned int v[3] = {0};
    unsigned int vt[3] = {0};
    unsigned int vn[3] = {0};
    int numVertices = 0;
    int numTexCoords = 0;
    int numNormals = 0;
    unsigned int numTriangles = 0;
    int activeMaterial = 0;
    char buffer[256] = {0};
    QString name;
    QMap<QString,int>::const_iterator iter;

    //these were member variables before, but were only used during loading, so we just define them here to save some memory
    IntVector attributeArray(m_numberOfTriangles);
    PosVector vertexCoords(m_numberOfVertexCoords);
    VF2Vector textureCoords(m_numberOfTextureCoords);
    VF3Vector normals(m_numberOfNormals);
    VertCacheT vertexCache;

    float fTmp[3] = {0.0f};
    double dTmp[3] = {0.0};

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

		addTrianglePosNormal(vertexCoords,normals,attributeArray,vertexCache,
					numTriangles++, activeMaterial,
                                     v[0], v[1], v[2],
                                     vn[0], vn[1], vn[2]);

                v[1] = v[2];
                vn[1] = vn[2];

                while (fscanf(pFile, "%d//%d", &v[2], &vn[2]) > 0)
                {
                    v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;
                    vn[2] = (vn[2] < 0) ? vn[2]+numNormals-1 : vn[2]-1;

		    addTrianglePosNormal(vertexCoords,normals,attributeArray,vertexCache,
					    numTriangles++, activeMaterial,
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

		addTrianglePosTexCoordNormal(vertexCoords,textureCoords,normals,attributeArray,vertexCache,
					     numTriangles++, activeMaterial,
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

		    addTrianglePosTexCoordNormal(vertexCoords,textureCoords,normals,attributeArray,vertexCache,
						 numTriangles++, activeMaterial,
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

		addTrianglePosTexCoord(vertexCoords,textureCoords,attributeArray,vertexCache,
				       numTriangles++, activeMaterial,
                                       v[0], v[1], v[2],
                                       vt[0], vt[1], vt[2]);

                v[1] = v[2];
                vt[1] = vt[2];

                while (fscanf(pFile, "%d/%d", &v[2], &vt[2]) > 0)
                {
                    v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;
                    vt[2] = (vt[2] < 0) ? vt[2]+numTexCoords-1 : vt[2]-1;

		    addTrianglePosTexCoord(vertexCoords,textureCoords,attributeArray,vertexCache,
					   numTriangles++, activeMaterial,
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

		addTrianglePos(vertexCoords, attributeArray, vertexCache, numTriangles++, activeMaterial, v[0], v[1], v[2]);

                v[1] = v[2];

                while (fscanf(pFile, "%d", &v[2]) > 0)
                {
                    v[2] = (v[2] < 0) ? v[2]+numVertices-1 : v[2]-1;

		    addTrianglePos(vertexCoords, attributeArray, vertexCache, numTriangles++, activeMaterial, v[0], v[1], v[2]);

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

        case 'v': //! v, vn, or vt.
            switch (buffer[1])
            {
            case '\0': //! v
                fscanf(pFile, "%lf %lf %lf", &dTmp[0], &dTmp[1], &dTmp[2]);

                switch(order)
                {
		case XZY:
			vertexCoords[numVertices] = VPos(dTmp[0],-dTmp[2],dTmp[1]);
                    break;

                default: //! XYZ
			vertexCoords[numVertices] = VPos(dTmp[0],dTmp[1],dTmp[2]);
                    break;
                }

                ++numVertices;
                break;

            case 'n': //! vn
                fscanf(pFile, "%f %f %f", &fTmp[0], &fTmp[1], &fTmp[2]);

                switch(order)
                {
                case XZY:
			normals[numNormals] = Vec3f(fTmp[0], -fTmp[2], fTmp[1]);
                    break;

                default: //! XYZ
			normals[numNormals] = Vec3f(fTmp[0],fTmp[1],fTmp[2]);
                    break;
                }

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
    int illum = 0;
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
                pMaterial->ambient[3] = 1.0f;
                break;

            case 'd': //! Kd
                fscanf(pFile, "%f %f %f",
                       &pMaterial->diffuse[0],
                       &pMaterial->diffuse[1],
                       &pMaterial->diffuse[2]);
                pMaterial->diffuse[3] = 1.0f;
                break;

            case 's': //! Ks
                fscanf(pFile, "%f %f %f",
                       &pMaterial->specular[0],
                       &pMaterial->specular[1],
                       &pMaterial->specular[2]);
                pMaterial->specular[3] = 1.0f;
                break;

            case 'e': //! Ke
                fscanf(pFile, "%f %f %f",
                       &pMaterial->emission[0],
                       &pMaterial->emission[1],
                       &pMaterial->emission[2]);
                pMaterial->emission[3] = 1.0f;
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
                break;

            default:
                fgets(buffer, sizeof(buffer), pFile);
                break;
            }
            break;

        case 'd':
            fscanf(pFile, "%f", &pMaterial->alpha);
            break;

        case 'i': // illum
            fscanf(pFile, "%d", &illum);

            pMaterial->illum = (Illum) illum;
            break;

        case 'm': //! map_Kd, map_bump
            if (strstr(buffer, "map_Kd") != 0)
            {
                fgets(buffer, sizeof(buffer), pFile);
                sscanf("%[^\n]", buffer);

                //TODO convert to QString
                std::string tex;
                parseTextureString(buffer, tex);
		pMaterial->textureName = QString::fromStdString(tex);
            }
            else if (strstr(buffer, "map_bump") != 0)
            {
                fgets(buffer, sizeof(buffer), pFile);
                sscanf(buffer, "%[^\n]", buffer);

                std::string bump;
                parseTextureString(buffer, bump);
		pMaterial->bumpMapName = QString::fromStdString(bump);
            }
            else if (strstr(buffer, "map_height") != 0)
            {
                fgets(buffer, sizeof(buffer), pFile);
                sscanf(buffer, "%[^\n]", buffer);

                std::string height;
                parseTextureString(buffer, height);
		pMaterial->heightMapName = QString::fromStdString(height);
            }
            else
            {
                fgets(buffer, sizeof(buffer), pFile);
            }
            break;

        case 'n': //! newmtl
            fgets(buffer, sizeof(buffer), pFile);
            sscanf(buffer, "%s %s", buffer, buffer);

            pMaterial = &m_materials[numMaterials];
            pMaterial->ambient[0] = 0.2f;
            pMaterial->ambient[1] = 0.2f;
            pMaterial->ambient[2] = 0.2f;
            pMaterial->ambient[3] = 1.0f;
            pMaterial->diffuse[0] = 0.8f;
            pMaterial->diffuse[1] = 0.8f;
            pMaterial->diffuse[2] = 0.8f;
            pMaterial->diffuse[3] = 1.0f;
            pMaterial->specular[0] = 0.0f;
            pMaterial->specular[1] = 0.0f;
            pMaterial->specular[2] = 0.0f;
            pMaterial->specular[3] = 1.0f;
	    pMaterial->shininess = 8.0f;
            pMaterial->alpha = 1.0f;
            pMaterial->name = buffer;
            pMaterial->textureName.clear();
            pMaterial->texture.clear();
            pMaterial->bumpMapName.clear();
            pMaterial->bump_texture.clear();
            pMaterial->heightMapName.clear();
            pMaterial->height_texture.clear();


	    materialCache[pMaterial->name] = numMaterials;
            ++numMaterials;
            break;

        default:
            fgets(buffer, sizeof(buffer), pFile);
            break;
        }
    }

    fclose(pFile);

    return true;
}

void OBJ::uploadTexturesGL()
{
    StelTextureMgr textureMgr;

    for(unsigned int i=0; i<m_numberOfMaterials; ++i)
    {
        Material* pMaterial = &getMaterial(i);

//        qDebug() << getTime() << "[Scenery3d]" << pMaterial->name.c_str();
//        qDebug() << getTime() << "[Scenery3d] Ka:" << pMaterial->ambient[0] << "," << pMaterial->ambient[1] << "," << pMaterial->ambient[2] << "," << pMaterial->ambient[3];
//        qDebug() << getTime() << "[Scenery3d] Kd:" << pMaterial->diffuse[0] << "," << pMaterial->diffuse[1] << "," << pMaterial->diffuse[2] << "," << pMaterial->diffuse[3];
//        qDebug() << getTime() << "[Scenery3d] Ks:" << pMaterial->specular[0] << "," << pMaterial->specular[1] << "," << pMaterial->specular[2] << "," << pMaterial->specular[3];
//        qDebug() << getTime() << "[Scenery3d] Shininess:" << pMaterial->shininess;
//        qDebug() << getTime() << "[Scenery3d] Alpha:" << pMaterial->alpha;
//        qDebug() << getTime() << "[Scenery3d] Illum:" << pMaterial->illum;

//        qDebug() << getTime() << "[Scenery3d] Uploading textures for Material: " << pMaterial->name.c_str();
//        qDebug() << getTime() << "[Scenery3d] Texture:" << pMaterial->textureName.c_str();
        if(!pMaterial->textureName.isEmpty())
        {
	    StelTextureSP tex = textureMgr.createTexture(absolutePath(pMaterial->textureName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true));
            if(!tex.isNull())
            {
                pMaterial->texture = tex;
            }
            else
            {
                qWarning() << getTime() << "[Scenery3d] Failed to load Texture:" << pMaterial->textureName;
            }
        }

        //qDebug() << getTime() << "[Scenery3d] Normal Map:" << pMaterial->bumpMapName;
        if(!pMaterial->bumpMapName.isEmpty())
        {
            StelTextureSP bumpTex = textureMgr.createTexture(absolutePath(pMaterial->bumpMapName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
            if(!bumpTex.isNull())
            {
                pMaterial->bump_texture = bumpTex;
            }
            else
            {
                qWarning() << getTime() << "[Scenery3d] Failed to load Normal Map:" << pMaterial->bumpMapName;
            }
        }

        //qDebug() << getTime() << "[Scenery3d] Height Map:" << pMaterial->heightMapName;
        if(!pMaterial->heightMapName.isEmpty())
        {
            StelTextureSP heightTex = textureMgr.createTexture(absolutePath(pMaterial->heightMapName), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
            if(!heightTex.isNull())
            {
                pMaterial->height_texture = heightTex;
            }
            else
            {
                qWarning() << getTime() << "[Scenery3d] Failed to load Height Map:" << pMaterial->heightMapName;
            }
        }
    }

    qDebug()<<"[Scenery3d] Uploaded OBJ textures to GL";
}

void OBJ::uploadBuffersGL()
{
	if(vertexArraysSupported)
	{
		m_vertexArrayObject->create();
		m_vertexArrayObject->bind();
	}

	m_vertexBuffer.create();
	m_vertexBuffer.bind();
	m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	//this is the upload
	m_vertexBuffer.allocate(m_vertexArray.constData(), sizeof(Vertex) * m_vertexArray.size());
	m_vertexBuffer.release();

	m_indexBuffer.create();
	m_indexBuffer.bind();
	m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	m_indexBuffer.allocate(m_indexArray.constData(), sizeof(unsigned int) * m_indexArray.size());
	m_indexBuffer.release();

	if(vertexArraysSupported)
	{
		//binding and setting vertex attribs, stored in VAO
		bindBuffersGL();
		m_vertexArrayObject->release();
		unbindBuffersGL();
	}
	//TODO maybe release the client-side memory

	qDebug()<<"[Scenery3d] Uploaded OBJ vertex and index data to GL";
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

void OBJ::bindGLFixedFunction()
{
	m_vertexBuffer.bind();

	const GLsizei stride = sizeof(Vertex);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, reinterpret_cast<const void *>(offsetof(struct Vertex, position)));
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, stride, reinterpret_cast<const void *>(offsetof(struct Vertex, normal)));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, stride, reinterpret_cast<const void *>(offsetof(struct Vertex, texCoord)));

	m_vertexBuffer.release();

	m_indexBuffer.bind();
}

void OBJ::unbindGLFixedFunction()
{
	m_indexBuffer.release();

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
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
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_TEXTURE);
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_TANGENT);
	glEnableVertexAttribArray(ShaderMgr::ATTLOC_BITANGENT);

	const GLsizei stride = sizeof(Vertex);

	glVertexAttribPointer(ShaderMgr::ATTLOC_VERTEX,   3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, position)));
	glVertexAttribPointer(ShaderMgr::ATTLOC_NORMAL,   3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, normal)));
	glVertexAttribPointer(ShaderMgr::ATTLOC_TEXTURE,  2,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(offsetof(struct Vertex, texCoord)));
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
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_TEXTURE);
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_TANGENT);
	glDisableVertexAttribArray(ShaderMgr::ATTLOC_BITANGENT);
}

void OBJ::transform(Mat4d mat)
{
    m = mat;
    pBoundingBox.min = Vec3f(std::numeric_limits<float>::max());
    pBoundingBox.max = Vec3f(-std::numeric_limits<float>::max()); //numeric_limits<T>::lowest() is C++11, but this is the same

    //Transform all vertices and normals by mat
    for(int i=0; i<getNumberOfVertices(); ++i)
    {
        Vertex *pVertex = &m_vertexArray[i];

        Vec3d pos = Vec3d(pVertex->position.v[0], pVertex->position.v[1], pVertex->position.v[2]);
        mat.transfo(pos);
        pVertex->position.v[0] = pos.v[0];
        pVertex->position.v[1] = pos.v[1];
        pVertex->position.v[2] = pos.v[2];

        Vec3d nor = Vec3d(pVertex->normal.v[0], pVertex->normal.v[1], pVertex->normal.v[2]);
        mat.transfo(nor);
        pVertex->normal.v[0] = nor.v[0];
        pVertex->normal.v[1] = nor.v[1];
        pVertex->normal.v[2] = nor.v[2];

        Vec3d tang = Vec3d(pVertex->tangent.v[0], pVertex->tangent.v[1], pVertex->tangent.v[2]);
        mat.transfo(tang);
        pVertex->tangent.v[0] = tang.v[0];
        pVertex->tangent.v[1] = tang.v[1];
        pVertex->tangent.v[2] = tang.v[2];

        Vec3d biTang = Vec3d(pVertex->bitangent.v[0], pVertex->bitangent.v[1], pVertex->bitangent.v[2]);
        pVertex->bitangent.v[0] = biTang.v[0];
        pVertex->bitangent.v[1] = biTang.v[1];
        pVertex->bitangent.v[2] = biTang.v[2];

        //Update bounding box in case it changed
	pBoundingBox.min = Vec3f( std::min(static_cast<float>(pVertex->position[0]), pBoundingBox.min[0]),
				  std::min(static_cast<float>(pVertex->position[1]), pBoundingBox.min[1]),
				  std::min(static_cast<float>(pVertex->position[2]), pBoundingBox.min[2]));

	pBoundingBox.max = Vec3f( std::max(static_cast<float>(pVertex->position[0]), pBoundingBox.max[0]),
				  std::max(static_cast<float>(pVertex->position[1]), pBoundingBox.max[1]),
				  std::max(static_cast<float>(pVertex->position[2]), pBoundingBox.max[2]));
    }
}

void OBJ::findBounds()
{
    //Find Bounding Box for entire Scene
    pBoundingBox.min = Vec3f(std::numeric_limits<float>::max());
    pBoundingBox.max = Vec3f(-std::numeric_limits<float>::max());

    for(int i=0; i<getNumberOfVertices(); ++i)
    {
        Vertex* pVertex = &m_vertexArray[i];

	pBoundingBox.min = Vec3f( std::min(static_cast<float>(pVertex->position[0]), pBoundingBox.min[0]),
				  std::min(static_cast<float>(pVertex->position[1]), pBoundingBox.min[1]),
				  std::min(static_cast<float>(pVertex->position[2]), pBoundingBox.min[2]));

	pBoundingBox.max = Vec3f( std::max(static_cast<float>(pVertex->position[0]), pBoundingBox.max[0]),
				  std::max(static_cast<float>(pVertex->position[1]), pBoundingBox.max[1]),
				  std::max(static_cast<float>(pVertex->position[2]), pBoundingBox.max[2]));
    }

    //Find AABB per Stel Model
    for(unsigned int i=0; i<m_numberOfStelModels; ++i)
    {
        StelModel* pStelModel = &m_stelModels[i];
    pStelModel->bbox = AABB(Vec3f(std::numeric_limits<float>::max()), Vec3f(-std::numeric_limits<float>::max()));

        for(int j=pStelModel->startIndex; j<pStelModel->triangleCount*3; ++j)
        {
            unsigned int vertexIndex = m_indexArray[j];
            Vertex* pVertex = &m_vertexArray[vertexIndex];

	    pStelModel->bbox.min = Vec3f( std::min(static_cast<float>(pVertex->position[0]), pStelModel->bbox.min[0]),
					  std::min(static_cast<float>(pVertex->position[1]), pStelModel->bbox.min[1]),
					  std::min(static_cast<float>(pVertex->position[2]), pStelModel->bbox.min[2]));

	    pStelModel->bbox.max = Vec3f( std::max(static_cast<float>(pVertex->position[0]), pStelModel->bbox.max[0]),
					  std::max(static_cast<float>(pVertex->position[1]), pStelModel->bbox.max[1]),
					  std::max(static_cast<float>(pVertex->position[2]), pStelModel->bbox.max[2]));
        }
    }
}

void OBJ::renderAABBs()
{
    for(unsigned int i=0; i<m_numberOfStelModels; ++i)
    {
        StelModel* pStelModel = &m_stelModels[i];
	pStelModel->bbox.render(&m);
    }
}

size_t OBJ::memoryUsage()
{
	size_t sz = sizeof(this);
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

	m_numberOfVertexCoords = other.m_numberOfVertexCoords;
	m_numberOfTextureCoords = other.m_numberOfTextureCoords;
	m_numberOfNormals = other.m_numberOfNormals;
	m_numberOfTriangles = other.m_numberOfTriangles;
	m_numberOfMaterials = other.m_numberOfMaterials;
	m_numberOfStelModels = other.m_numberOfStelModels;

	pBoundingBox = other.pBoundingBox;
	m = other.m;

	m_basePath = other.m_basePath;

	m_stelModels = other.m_stelModels;
	m_materials = other.m_materials;
	m_vertexArray = other.m_vertexArray;
	m_indexArray = other.m_indexArray;

	return *this;
}
