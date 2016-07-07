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

	//! Defines a material loaded from an .mtl file.
	//! It uses QVector3D instead of Vec3f, etc, to be more compatible with
	//! Qt's OpenGL wrappers.
	struct Material
	{
		//! Name of the material as defined in the .mtl
		QString name;

		//! Ambient coefficient
		QVector3D Ka;
		//! Diffuse coefficient
		QVector3D Kd;
		//! Specular coefficient
		QVector3D Ks;
		//! Emissive coefficient
		QVector3D Ke;
		//! Specular shininess (exponent), should be > 0
		float Ns;
		//! Alpha value (1 means opaque)
		float d;

		//! The ambient map path
		QString map_Ka;
		//! The diffuse map path
		QString map_Kd;
		//! The specular map path
		QString map_Ks;
		//! The emissive map path
		QString map_Ke;
		//! The bump/normal map path
		QString map_bump;
		//! The height map path
		QString map_height;

		//! Nonstandard extension:
		//! if to render backface, default false
		bool bBackface;
		//! Nonstandard extension:
		//! if to perform binary alpha testing, default false
		bool bAlphatest;
		//! Nonstandard extension:
		//! the alpha threshold to use for alpha testing when bAlphatest is true, default 0.5
		float fAlphaThreshold;

		//! Loads all materials contained in an .mtl file
		static QVector<Material> loadFromFile(const QString& filename);
	};

	//! Represents a bunch of faces following after each other
	//! that use the same material
	struct MaterialGroup{
		//! The starting index in the index list
		int startIndex;
		//! The amount of indices after the start index which belong to this material group
		int indexCount;

		//! The index of the object this group belongs to
		int objectIndex;
		//! The index of the material that this group uses
		int materialIndex;
	};

	//! Represents an OBJ object as defined with the 'o' statement.
	//! There is an default object for faces defined before any 'o' statement
	struct Object
	{
		//! True if this object was automatically generated because no 'o' statements
		//! were before the first 'f' statement
		bool isDefaultObject;

		//! The name of the object. May be empty
		QString name;

		//! The centroid of this object at load time
		Vec3f centroid;

		//! The list of material groups in this object
		QVector<MaterialGroup> groups;
	};

	typedef QVector<Vertex> VertexList;
	typedef QVector<unsigned int> IndexList;
	typedef QVector<Material> MaterialList;
	typedef QMap<QString, int> MaterialMap;
	typedef QVector<Object> ObjectList;
	typedef QMap<QString, int> ObjectMap;

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
	typedef QVector<QStringRef> ParseParams;
	typedef QHash<Vertex, int> VertexCache;

	struct CurrentParserState
	{
		int currentMaterialIdx;
		MaterialGroup* currentMaterialGroup;
		Object* currentObject;
	};

	//all vertex data is contained in this list
	VertexList m_vertices;
	//all index data is contained in this list
	IndexList m_indices;
	//all material data is contained in this list
	MaterialList m_materials;
	MaterialMap m_materialMap;
	ObjectList m_objects;
	ObjectMap m_objectMap;

	//! Get or create the current parsed object
	inline Object* getCurrentObject(CurrentParserState& state);
	//! Get or create the current parsed material group
	inline MaterialGroup* getCurrentMaterialGroup(CurrentParserState& state);
	//! Get or create the current material index for parsing
	inline int getCurrentMaterialIndex(CurrentParserState& state);
	//! Parse a single bool
	inline static bool parseBool(const ParseParams& params, bool& out);
	//! Parse a single string
	inline static bool parseString(const ParseParams &params, QString &out);
	//! Parse a single float
	inline static bool parseFloat(const ParseParams& params, float& out);
	//! Generic Vec3 parse method, used for position, normals, colors, etc.
	//! Templated to allow for both use of QVector3D as well as Vec3f, etc, with a single implementation
	//! Only requirement is that operator[] is defined.
	template<typename T>
	inline static bool parseVec3(const ParseParams& params, T& out);
	//! Generic Vec2 parse method
	//! Templated to allow for both use of QVector2D as well as Vec2f, etc, with a single implementation
	//! Only requirement is that operator[] is defined.
	template<typename T>
	inline static bool parseVec2(const ParseParams& params, T& out);
	inline bool parseFace(const ParseParams& params, const V3Vec& posList, const V3Vec& normList, const V2Vec& texList,
			      CurrentParserState &state, VertexCache& vertCache);
};

//! Implements the qHash method for the Vertex type
inline uint qHash(const StelOBJ::Vertex& key, uint seed)
{
	return qHashBits(&key, sizeof(StelOBJ::Vertex), seed);
}

#endif // _STELOBJ_HPP_
