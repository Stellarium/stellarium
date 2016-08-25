/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-16 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef _S3DSCENE_HPP_
#define _S3DSCENE_HPP_

#include "StelOBJ.hpp"
#include "StelTextureTypes.hpp"
#include "StelOpenGLArray.hpp"
#include "SceneInfo.hpp"
#include "Heightmap.hpp"

class S3DScene
{
public:
	//! Extension of StelOBJ::Material which provides Scenery3d specific stuff
	struct Material : public StelOBJ::Material
	{
		Material() : traits()
		{

		}

		Material(const StelOBJ::Material& stelMat) : StelOBJ::Material(stelMat), traits()
		{

		}

		struct Traits
		{
			//bool hasAmbientTexture; //! True when tex_Ka is a valid texture
			bool hasDiffuseTexture; //! True when tex_Kd is a valid texture
			//bool hasSpecularTexture; //! True when tex_Ks is a valid texture
			bool hasEmissiveTexture; //! True when tex_Ke is a valid texture
			bool hasBumpTexture; //! True when tex_bump is a valid texture
			bool hasHeightTexture; //! True when tex_height is a valid texture
			bool hasSpecularity; //! True when both Ks and Ns are non-zero
			bool hasTransparency; //! True when the material exhibits "true" transparency, that is when the tex_Kd has an alpha channel or the d value is smaller than 1
		} traits;

		//the actual texture instances
		StelTextureSP tex_Kd;
		StelTextureSP tex_Ke;
		StelTextureSP tex_bump;
		StelTextureSP tex_height;

		//! Starts loading the textures in this material asynchronously
		void loadTexturesAsync();
		//! Re-calculates the material traits. This requires all textures to be fully loaded.
		void calculateTraits();
	};

	typedef QVector<Material> MaterialList;
	//for now, this does not use custom extensions...
	typedef StelOBJ::ObjectList ObjectList;

	explicit S3DScene(const SceneInfo& info);

	void setModel(const StelOBJ& model);
	void setGround(const StelOBJ& ground);

	const SceneInfo& getSceneInfo() const { return info; }

	Material& getMaterial(int index) { return materials[index]; }
	const Material& getMaterial(int index) const { return materials.at(index); }
	const ObjectList& getObjects() const { return objects; }

	//! Moves the viewer according to the given move vector
	//!  (which is specified relative to the view direction and current position)
	//! The 3rd component of the vector specifies the eye height change.
	//! The foot height is set to the heightmap position.
	void moveViewer(const Vec3d& moveView);

	//! Sets the viewer (foot) position
	void setViewerPosition(const Vec3d& pos);
	//! Sets the viewer foot position on the given x/y point on the heightmap
	void setViewerPositionOnHeightmap(const Vec2d& pos);
	//! Get current viewer foot position
	inline const Vec3d& getViewerPosition() const { return position; }
	//! Returns the viewer foot position, without the height
	inline Vec2d getViewer2DPosition() const { return Vec2d(position[0], position[1]); }
	//! Get current eye position (= foot position + eye height)
	inline const Vec3d& getEyePosition() const { return eyePosition; }
	inline double getEyeHeight() const { return eye_height; }
	inline void setEyeHeight(double height) { eye_height = height; recalcEyePos();}
	inline const AABBox& getSceneAABB() const { return sceneAABB; }
	float getGroundHeightAtViewer() const;
	inline void setViewDirection(const Vec3d& viewDir) { viewDirection = viewDir; }
	inline const Vec3d& getViewDirection() const { return viewDirection; }

	Vec3d getGridPosition() const;
	void setGridPosition(const Vec3d& gridPos);

	//! Makes the scene ready for GL rendering. Needs a valid GL context
	bool glLoad();
	//! Returns true if the scene is ready for GL rendering (glLoad succeded)
	bool isGLReady() const { return glReady; }
	// Basic wrappers alround StelOpenGLArray
	inline void glBind() { glArray.bind(); }
	inline void glRelease() { glArray.release(); }
	inline void glDraw(int offset, int count) const { glArray.draw(offset,count); }

private:
	inline void recalcEyePos() { eyePosition = position; eyePosition[2]+=eye_height; }
	MaterialList materials;
	ObjectList objects;


	bool glReady;

	SceneInfo info;
	QMatrix4x4 zRot2Grid;

	AABBox sceneAABB;

	// current view direction vector
	Vec3d viewDirection;
	// current foot position in model
	Vec3d position;
	// current eye height (relative to foot position)
	double eye_height;
	// current eye position in model (stored to avoid repeated re-calculations)
	Vec3d eyePosition;

	//the model data is only retained before loaded into GL
	StelOBJ modelData;
	Heightmap heightmap;
	StelOpenGLArray glArray;

	static void finalizeTexture(StelTextureSP& tex);
};

#endif // _S3DSCENE_HPP_
