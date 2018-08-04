/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti
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

#ifndef SCENEINFO_HPP
#define SCENEINFO_HPP

#include "StelLocation.hpp"
#include "VecMath.hpp"
#include "StelOBJ.hpp"

#include <QMap>
#include <QSharedPointer>

Q_DECLARE_LOGGING_CATEGORY(sceneInfo)

class QSettings;

//! Contains all the metadata necessary for a Scenery3d scene,
//! and can be loaded from special .ini files in a scene's folder.
struct SceneInfo
{
	SceneInfo() : isValid(false),id(),fullPath(),name(),author(),description(),copyright(),landscapeName(),modelScenery(),modelGround(),vertexOrder(),vertexOrderEnum(StelOBJ::XYZ),
		camNearZ(0.1f),camFarZ(1000.0f),shadowFarZ(1000.0f),shadowSplitWeight(0.5f),location(),lookAt_fov(0.0f,0.0f,25.0f),eyeLevel(0.0),
		altitudeFromModel(false),startPositionFromModel(false),groundNullHeightFromModel(false),groundNullHeight(0.0),
		transparencyThreshold(0.0f),sceneryGenerateNormals(false),groundGenerateNormals(false)
	{}
	//! If this is a valid sceneInfo object loaded from file
	bool isValid;
	//! ID of the scene (relative directory)
	QString id;
	//! The full path to the scene's folder. Other paths (model files) are to be seen relative to this path.
	QString fullPath;
	//! Name of the scene
	QString name;
	//! Author of the scene
	QString author;
	//! A description, which can be displayed in the GUI - supporting HTML tags!
	//! Note that this description, as stored in the scenery3d.ini, is only meant as a fallback.
	//! The better way would be to create description.<lang>.utf8 files in the scene's folder.
	//! This can then be retrieved using getLocalizedHTMLDescription.
	QString description;
	//! Copyright string
	QString copyright;
	//! The name of the landscape to switch to. The landscape's position is applied on loading.
	QString landscapeName;
	//! The file name of the scenery .obj model
	QString modelScenery;
	//! The file name of the optional seperate ground model (used as a heightmap for walking)
	QString modelGround;
	//! Optional string depicting vertex order of models (XYZ, ZXY, ...)
	QString vertexOrder;

	//! The vertex order of the corresponding OBJ file
	StelOBJ::VertexOrder vertexOrderEnum;

	//! Distance to cam near clipping plane. Default 0.3.
	float camNearZ;
	//! Distance to cam far clipping plane. Default 10000.0
	float camFarZ;
	//! An optional shadow far clipping plane, constraining the shadowmaps to a smaller region than is visible.
	//! Must be smaller or equal to camFarZ. Default equals camFarZ.
	float shadowFarZ;
	//! Weighting of the shadow frustum splits between uniform (at 0) and logarithmic (at 1) splits
	//! When -1, should be calculated from the scene using the old algorithm
	float shadowSplitWeight;

	//! Optional more accurate location information, which will override the landscape's position.
	QSharedPointer<StelLocation> location;
	//! Optional initial look-at vector (azimuth, elevation and FOV in degrees)
	Vec3f lookAt_fov; // (az_deg, alt_deg, fov_deg)

	//! The height at which the observer's eyes are placed. Default 1.65.
	double eyeLevel;
	//! The name of the grid space for displaying the world positon
	QString gridName;
	//! Offset of the center of the model in a given grid space
	Vec3d modelWorldOffset;
	//! The world grid space offset where the observer is placed upon loading
	Vec3d startWorldOffset;
	//! Relative start position in model space, calculated from the world offset and start offset with Z rotation, etc.
	Vec3d relativeStartPosition;
	//! If true, it indicates that the model file's bounding box is used for altitude calculation.
	bool altitudeFromModel;
	//! If true, it indicates that the model file's bounding box is used for starting position calculation.
	bool startPositionFromModel;
	//! If true, it indicates that the model file's bounding box is used for starting height calculation.
	bool groundNullHeightFromModel;

	//! If only a non-georeferenced OBJ can be provided, you can specify a matrix via .ini/[model]/obj_world_trafo.
	//! This will be applied to make sure that X=Grid-East, Y=Grid-North, Z=height.
	Mat4d obj2gridMatrix;
	//! The vertical axis rotation that must be applied to the scene, for meridian convergence.
	//! This is calculated from other fields in the file.
	Mat4d zRotateMatrix;
	//! The height value outside the ground model's heightmap, or used if no ground model exists.
	double groundNullHeight;

	//! Threshold for cutout transparency (no blending). Default is 0.5f
	float transparencyThreshold;
	//! Recalculate normals of the scene from face normals? Default false.
	bool sceneryGenerateNormals;
	//! Recalculate normals of the ground from face normals? Default false.
	bool groundGenerateNormals;

	//! Returns true if the location object is valid
	bool hasLocation() const { return !location.isNull(); }
	//! Returns true if the lookat_fov is valid
	bool hasLookAtFOV() const { return lookAt_fov[2] >= 0; }

	//! Tries to find a description.<language>.utf8 file for this scene, and returns its contents.
	//! If no such file exists, the description.en.utf8 is checked. If this is also missing,
	//! a null QString is returned. Consider using the value of SceneInfo::description in this case.
	QString getLocalizedHTMLDescription() const;

	//! The folder for scenery is found here
	static const QString SCENES_PATH;
	//! Loads the scene metadata associated with this ID (directory) into the given object. Returns true on success.
	static bool loadByID(const QString& id, SceneInfo &info);
	//! Convenience method that finds the ID for the given name and calls loadByID.
	static bool loadByName(const QString& name, SceneInfo& info);
	//! Returns the ID for the given scene name.
	//! If multiple scenes exist with the same name, the first one found is returned. If no scene is found with this name, an empty string is returned.
	static QString getIDFromName(const QString& name);
	//! Returns all available scene IDs
	static QStringList getAllSceneIDs();
	//! Returns all available scene names
	static QStringList getAllSceneNames();
	//! Builds a mapping of available scene names to the folders they are contained in, similar to the LandscapeMgr's method
	static QMap<QString,QString> getNameToIDMap();

private:
	//! The meta type ID associated to the SceneInfo type
	static int metaTypeId;
};

Q_DECLARE_LOGGING_CATEGORY(storedView)
struct StoredView;
typedef QList<StoredView> StoredViewList;

//! A structure which stores a specific view position, view direction and FOV, together with a textual description.
struct StoredView
{
	StoredView() : position(0,0,0,0), view_fov(0,0,-1000), isGlobal(false), jd(0.0), jdIsRelevant(false)
	{}

	//! A descriptive label
	QString label;
	//! A description of the view
	QString description;
	//! Stored grid position + current eye height in 4th component
	Vec4d position;
	//! Alt/Az angles in degrees + field of view
	Vec3f view_fov;
	//! True if this is a position stored next to the scene definition (viewpoints.ini). If false, this is a user-defined view (from userdir\stellarium\scenery3d\userviews.ini).
	bool isGlobal;
	//! Julian Date of interest.
	double jd;
	//! Indicate if stored date is potentially relevant
	bool jdIsRelevant;

	//! Returns a list of all global views of a scene.
	//! If the scene is invalid, an empty list is returned.
	static StoredViewList getGlobalViewsForScene(const SceneInfo& scene);
	//! Returns a list of all user-generated views of a scene.
	//! If the scene is invalid, an empty list is returned.
	static StoredViewList getUserViewsForScene(const SceneInfo& scene);
	//! Saves the given user views to userviews.ini, replacing all views existing for this scene
	//! The scene MUST be valid, will throw an assertion if not.
	static void saveUserViews(const SceneInfo& scene, const StoredViewList& list);

	static QSettings* userviews;
	static const QString USERVIEWS_FILE;
private:
	static void readArray(QSettings& ini,StoredViewList& list, int size, bool isGlobal);
	static void writeArray(QSettings& ini, const StoredViewList& list);
	static QSettings* getUserViews();
};

Q_DECLARE_METATYPE(SceneInfo)

#endif
