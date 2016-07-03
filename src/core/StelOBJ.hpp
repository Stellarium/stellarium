/*
 * Stellarium
 * Copyright (C) 2016 Florian Schaukowitsch
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

#ifndef _STELOBJ_HPP_
#define _STELOBJ_HPP_

#include "VecMath.hpp"

#include <qopengl.h>
#include <QLoggingCategory>
#include <QString>
#include <QIODevice>
#include <QVector>
#include <QHash>

Q_DECLARE_LOGGING_CATEGORY(stelOBJ)

//! Representation of a custom subset of a [Wavefront .obj file](https://en.wikipedia.org/wiki/Wavefront_.obj_file),
//! including only triangle data and materials.
class StelOBJ
{
public:
	//! A Vertex struct holds the vertex itself (position), corresponding texture coordinates, normals, tangents and bitangents
	//! It does not use Vec3f etc. to be POD compliant (needed for offsetof)
	struct Vertex
	{
		//! The XYZ position
		GLfloat position[3];
		//! The UV texture coordinate
		GLfloat texCoord[2];
		//! The vertex normal
		GLfloat normal[3];
		//! The vertex tangent
		GLfloat tangent[4];
		//! The vertex bitangent
		GLfloat bitangent[3];

		bool operator==(const Vertex& b) const { return !memcmp(this,&b,sizeof(Vertex)); }
	};

	typedef QVector<Vertex> VertexList;
	typedef QVector<unsigned int> IndexList;

	//! Constructs an empty StelOBJ. Use load() to load data from a .obj file.
	StelOBJ();
	virtual ~StelOBJ();

	//! Resets all data contained in this StelOBJ
	void clear();

	//! Returns the number of faces. We only use triangle faces, so this is
	//! always the index count divided by 3.
	inline unsigned int getFaceCount() const { return m_indices.size() / 3; }

	inline const VertexList& getVertexList() const { return m_vertices; }
	inline const IndexList& getIndexList() const { return m_indices; }

	//! Loads an .obj file by name. Supports .gz decompression, and
	//! then calls load(QIODevice) for the actual loading.
	//! @return true if load was successful
	bool load(const QString& filename);
	//! Loads an .obj file from the specified device.
	//! @param device The device to load OBJ data from
	//! @param basePath The path to use to find additional files (like material definitions)
	//! @return true if load was successful
	bool load(QIODevice& device, const QString& basePath);

	//! Rebuilds vertex normals as the average of face normals.
	void rebuildNormals();
private:
	typedef QVector<Vec3f> V3Vec;
	typedef QVector<Vec2f> V2Vec;
	typedef const QVector<QStringRef>& ParseParams;
	typedef QHash<Vertex, int> VertexCache;

	//all vertex data is contained in this list
	VertexList m_vertices;
	//all index data is contained in this list
	IndexList m_indices;

	inline bool parseVertPos(ParseParams params, V3Vec& posList) const;
	inline bool parseVertNorm(ParseParams params, V3Vec& normList) const;
	inline bool parseVertTex(ParseParams params, V2Vec& texList) const;
	inline bool parseFace(ParseParams params, const V3Vec& posList, const V3Vec& normList, const V2Vec& texList,
			      VertexCache& vertCache);
};

//! Implements the qHash method for the Vertex type
inline uint qHash(const StelOBJ::Vertex& key, uint seed)
{
	return qHashBits(&key, sizeof(StelOBJ::Vertex), seed);
}

#endif // _STELOBJ_HPP_
