/*
 * Stellarium
 * Copyright (C) 2012 Andrei Borza
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

#ifndef STELOBJ_HPP
#define STELOBJ_HPP

#include "GeomMath.hpp"

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
	//! Possible vertex orderings with load()
	enum VertexOrder
	{
		XYZ, XZY, YXZ, YZX, ZXY, ZYX
	};

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

		//! Checks if the 2 vertices correspond to the same data using memcmp
		bool operator==(const Vertex& b) const { return !memcmp(this,&b,sizeof(Vertex)); }
	};

	//! Defines a material loaded from an .mtl file.
	//! It uses QVector3D instead of Vec3f, etc, to be more compatible with
	//! Qt's OpenGL wrappers.
	struct Material
	{
		//! MTL Illumination models, see the developer doc for info.
		//! @deprecated If possible, do not use. Illum use is very inconsistent between different modeling programs
		enum Illum { I_NONE=-1, I_DIFFUSE=0, I_DIFFUSE_AND_AMBIENT=1, I_SPECULAR=2, I_TRANSLUCENT=9 } illum;

		Material()
			: illum(I_NONE),
			  Ka(-1.0f,-1.0f,-1.0f),Kd(-1.0f,-1.0f,-1.0f),Ks(-1.0f,-1.0f,-1.0f),Ke(-1.0f,-1.0f,-1.0f),
			  Ns(8.0f), d(-1.0f)
		{
		}

		//! Name of the material as defined in the .mtl, default empty
		QString name;

		//! Ambient coefficient. Contains all -1 if not set by .mtl
		QVector3D Ka;
		//! Diffuse coefficient. Contains all -1 if not set by .mtl
		QVector3D Kd;
		//! Specular coefficient. Contains all -1 if not set by .mtl
		QVector3D Ks;
		//! Emissive coefficient. Contains all -1 if not set by .mtl
		QVector3D Ke;
		//! Specular shininess (exponent), should be > 0. Default 8.0
		float Ns;
		//! Alpha value (1 means opaque). -1 if not set by .mtl
		//! Note that both the \c d and \c Tr statements can change this value
		float d;

		//! The ambient texture map file path
		QString map_Ka;
		//! The diffuse texture map file path
		QString map_Kd;
		//! The specular texture map file path
		QString map_Ks;
		//! The emissive texture map file path
		QString map_Ke;
		//! The bump/normal texture map file path
		QString map_bump;
		//! The height map texture file path
		QString map_height;


		typedef QMap<QString,QStringList> ParamsMap;
		//! Contains all other material parameters that are not recognized by this class,
		//! but can still be accessed by class users this way.
		//! The key is the statement name (e.g. \c Ka, \c map_bump etc.), the value is the list of
		//! space-separated parameters to this statement
		ParamsMap additionalParams;

		//! Loads all materials contained in an .mtl file.
		//! Does not check if the texture map files exist.
		//! @return empty vector on error
		static QVector<Material> loadFromFile(const QString& filename);

	protected:
		//! Parses a bool from a parameter list (like included in the ::additionalParams)
		//! using the same logic StelOBJ uses internally
		//! @returns true if successful, the output is written into \p out
		static bool parseBool(const QStringList& params, bool &out);
		//! Parses a float from a parameter list (like included in the ::additionalParams)
		//! using the same logic StelOBJ uses internally
		//! @returns true if successful, the output is written into \p out
		static bool parseFloat(const QStringList &params, float &out);
		//! Parses a Vec2d from a parameter list (like included in the ::additionalParams)
		//! using the same logic StelOBJ uses internally
		//! @returns true if successful, the output is written into \p out
		static bool parseVec2d(const QStringList &params, Vec2d &out);
	};


	//! Represents a bunch of faces following after each other
	//! that use the same material
	struct MaterialGroup{
		MaterialGroup()
			: startIndex(0),
			  indexCount(0),
			  objectIndex(-1),
			  materialIndex(-1),
			  centroid(0.)
		{
		}

		//! The starting index in the index list
		int startIndex;
		//! The amount of indices after the start index which belong to this material group
		int indexCount;

		//! The index of the object this group belongs to
		int objectIndex;
		//! The index of the material that this group uses
		int materialIndex;

		//! The centroid of this group at load time
		//! @note This is a very simple centroid calculation which simply accumulates all vertex positions
		//! and divides by their number. Most notably, it does not take vertex density into account, so this may not
		//! correspond to the geometric center/center of mass of the object
		Vec3f centroid;
		//! The AABB of this group at load time
		AABBox boundingbox;
	};

	typedef QVector<MaterialGroup> MaterialGroupList;

	//! Represents an OBJ object as defined with the 'o' statement.
	//! There is an default object for faces defined before any 'o' statement
	struct Object
	{
		Object()
			: isDefaultObject(false),
			  centroid(0.)
		{
		}

		//! True if this object was automatically generated because no 'o' statements
		//! were before the first 'f' statement
		bool isDefaultObject;

		//! The name of the object. May be empty
		QString name;

		//! The centroid of this object at load time.
		//! @note This is a very simple centroid calculation which simply accumulates all vertex positions
		//! and divides by their number. Most notably, it does not take vertex density into account, so this may not
		//! correspond to the geometric center/center of mass of the object
		Vec3f centroid;
		//! The AABB of this object at load time
		AABBox boundingbox;

		//! The list of material groups in this object
		MaterialGroupList groups;

	private:
		void postprocess(const StelOBJ& obj, Vec3d& centroid);
		friend class StelOBJ;
	};

	typedef QVector<Vec3f> V3Vec;
	typedef QVector<Vec2f> V2Vec;
	typedef QVector<Vertex> VertexList;
	typedef QVector<unsigned int> IndexList;
	typedef QVector<unsigned short> ShortIndexList;
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
	inline unsigned int getFaceCount() const { return static_cast<unsigned int>(m_indices.size()) / 3; }

	//! Returns an vertex list, suitable for loading into OpenGL arrays
	inline const VertexList& getVertexList() const { return m_vertices; }
	//! Returns an index list, suitable for use with OpenGL element arrays
	inline const IndexList& getIndexList() const { return m_indices; }
	//! Returns the list of materials
	inline const MaterialList& getMaterialList() const { return m_materials; }
	//! Returns the list of objects
	inline const ObjectList& getObjectList() const { return m_objects; }
	//! Returns the object map (mapping the object names to their indices in the object list)
	inline const ObjectMap& getObjectMap() const { return m_objectMap; }
	//! Returns the global AABB of all vertices of the OBJ
	inline const AABBox& getAABBox() const { return m_bbox; }
	//! Returns the global centroid of all vertices of the OBJ.
	//! @note This is a very simple centroid calculation which simply accumulates all vertex positions
	//! and divides by their number. Most notably, it does not take vertex density into account, so this may not
	//! correspond to the geometric center/center of mass of the object
	inline const Vec3f& getCentroid() const { return m_centroid; }

	//! Loads an .obj file by name. Supports .gz decompression, and
	//! then calls load(QIODevice) for the actual loading.
	//! @return true if load was successful
	bool load(const QString& filename, const VertexOrder vertexOrder = VertexOrder::XYZ, const bool forceCreateNormals=false);
	//! Loads an .obj file from the specified device.
	//! @param device The device to load OBJ data from
	//! @param basePath The path to use to find additional files (like material definitions)
	//! @param vertexOrder The order to use for vertex positions
	//! @param forceCreateNormals set true to force creation of normals even if they exist
	//! @return true if load was successful
	bool load(QIODevice& device, const QString& basePath, const VertexOrder vertexOrder = VertexOrder::XYZ, const bool forceCreateNormals=false);

	//! Returns true if this object contains valid data from a load() method
	bool isLoaded() const { return m_isLoaded; }

	//! Rebuilds vertex normals as the average of face normals.
	void rebuildNormals();

	//! Returns if unsigned short indices can be used instead of unsigned int indices,
	//! to save some memory. This can only be done if the model has less vertices than
	//! std::numeric_limits<unsigned short>::max()
	inline bool canUseShortIndices() const { return m_vertices.size() < std::numeric_limits<unsigned short>::max(); }

	//! Converts the index list (as returned by getIndexList())
	//! to use unsigned short instead of integer.
	//! If this is not possible (canUseShortIndices() returns false),
	//! an empty list is returned.
	ShortIndexList getShortIndexList() const;

	//! Scales the vertex positions according to the given factor.
	//! This may be useful for importing, because many exporters
	//! don't handle very large or very small models well.
	//! For example, the solar system objects are modeled with kilometer units,
	//! and then converted to AU on loading.
	void scale(double factor);

	//! Applies the given transformation matrix to the vertex data.
	//! @param onlyPosition If true, only the position information is transformed, the normals/tangents are skipped
	void transform(const QMatrix4x4& mat, bool onlyPosition = false);

	//! Splits the vertex data into separate arrays.
	//! If a given parameter vector is null, it is not filled.
	void splitVertexData(V3Vec* position,
			     V2Vec* texCoord = Q_NULLPTR,
			     V3Vec* normal = Q_NULLPTR,
			     V3Vec* tangent = Q_NULLPTR,
			     V3Vec* bitangent = Q_NULLPTR) const;

	//! Clears the internal vertex list to save space, meaning getVertexList() returns
	//! an empty list! The other members are unaffected (indices, materials,
	//! objects etc. still work). The vertex list can only be restored
	//! when the OBJ data is freshly loaded again, so don't do this if
	//! you require it later.
	//!
	//! This is intended to be used together with splitVertexData(), when you want your own vertex format.
	void clearVertexData();
private:
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	typedef QStringView ParseParam;
	typedef QVector<QStringView> ParseParams;
#else
	typedef QStringRef ParseParam;
	typedef QVector<QStringRef> ParseParams;
#endif
	typedef QHash<Vertex, int> VertexCache;

	struct CurrentParserState
	{
		int currentMaterialIdx;
		MaterialGroup* currentMaterialGroup;
		Object* currentObject;
	};

	bool m_isLoaded;
	//all vertex data is contained in this list
	VertexList m_vertices;
	//all index data is contained in this list
	IndexList m_indices;
	//all material data is contained in this list
	MaterialList m_materials;
	MaterialMap m_materialMap;
	ObjectList m_objects;
	ObjectMap m_objectMap;

	//global bounding box
	AABBox m_bbox;
	//global centroid
	Vec3f m_centroid;

	//! Get or create the current parsed object
	inline Object* getCurrentObject(CurrentParserState& state);
	//! Get or create the current parsed material group
	inline MaterialGroup* getCurrentMaterialGroup(CurrentParserState& state);
	//! Get or create the current material index for parsing
	inline int getCurrentMaterialIndex(CurrentParserState& state);
	//! Parse a single bool
	inline static bool parseBool(const ParseParams& params, bool& out, int paramsStart=1);
	//! Parse a single int
	inline static bool parseInt(const ParseParams& params, int& out, int paramsStart=1);
	//! Parse a single string
	inline static bool parseString(const ParseParams &params, QString &out, int paramsStart=1);
	inline static QString getRestOfString(const QString &strip, const QString& line);
	//! Parse a single float
	inline static bool parseFloat(const ParseParams& params, float& out, int paramsStart=1);
	//! Generic Vec3 parse method, used for position, normals, colors, etc.
	//! Templated to allow for both use of QVector3D as well as Vec3f, etc, with a single implementation
	//! Only requirement is that operator[] is defined.
	template<typename T>
	inline static bool parseVec3(const ParseParams& params, T& out, int paramsStart=1);
	//! Generic Vec2 parse method
	//! Templated to allow for both use of QVector2D as well as Vec2f, etc, with a single implementation
	//! Only requirement is that operator[] is defined.
	template<typename T>
	inline static bool parseVec2(const ParseParams& params, T& out, int paramsStart=1);
	inline bool parseFace(const ParseParams& params, const V3Vec& posList, const V3Vec& normList, const V2Vec& texList,
			      CurrentParserState &state, VertexCache& vertCache);

	inline void addObject(const QString& name, CurrentParserState& state);

	//! Regenerate all normals in the vertex list
	void generateNormals();

	//! Calculates tangents and bitangents
	void generateTangents();

	//! Calculates AABBs of objects (and also centroids)
	void generateAABB();

	//! Performs post-processing steps, like finding centroids and bounding boxes
	//! This is called after a model has been loaded
	void performPostProcessing(bool genNormals);
};

//! Implements the qHash method for the Vertex type
inline uint qHash(const StelOBJ::Vertex& key, uint seed)
{
	//hash the whole vertex raw memory directly
	return qHashBits(&key, sizeof(StelOBJ::Vertex), seed);
}

#endif // STELOBJ_HPP
