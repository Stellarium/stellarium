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

#ifndef S3DSCENE_HPP
#define S3DSCENE_HPP

#include "StelOBJ.hpp"
#include "StelTextureTypes.hpp"
#include "StelOpenGLArray.hpp"
#include "SceneInfo.hpp"
#include "Heightmap.hpp"

#include <cfloat>

Q_DECLARE_LOGGING_CATEGORY(s3dscene)

class S3DScene
{
public:
	//! Extension of StelOBJ::Material which provides Scenery3d specific stuff
	struct Material : public StelOBJ::Material
	{
		Material() : traits(), bAlphatest(false), bBackface(false), fAlphaThreshold(0.5),
			     vis_fadeIn(-DBL_MAX, -DBL_MAX),vis_fadeOut(DBL_MAX,DBL_MAX),vis_fadeValue(1.0)
		{
		}

		Material(const StelOBJ::Material& stelMat)
			: StelOBJ::Material(stelMat), traits(), bAlphatest(false), bBackface(false), fAlphaThreshold(0.5),
			  vis_fadeIn(-DBL_MAX, -DBL_MAX),vis_fadeOut(DBL_MAX,DBL_MAX),vis_fadeValue(1.0)
		{
			if(additionalParams.contains("bAlphatest"))
				parseBool(additionalParams.value("bAlphatest"), bAlphatest);
			if(additionalParams.contains("bBackface"))
				parseBool(additionalParams.value("bBackface"), bBackface);
			if(additionalParams.contains("fAlphaThreshold"))
				parseFloat(additionalParams.value("fAlphaThreshold"), fAlphaThreshold);
			if(additionalParams.contains("vis_fadeIn"))
			{
				traits.hasTimeFade = true;
				parseVec2d(additionalParams.value("vis_fadeIn"), vis_fadeIn);
			}
			if(additionalParams.contains("vis_fadeOut"))
			{
				traits.hasTimeFade = true;
				parseVec2d(additionalParams.value("vis_fadeOut"), vis_fadeOut);
			}
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
			bool hasTimeFade; //! True when the material has time-dependent display
			bool isFullyTransparent; //! True when the material is (currently) completely invisible
			bool isFading; //! True while the material is currently fading
		} traits;

		//the actual texture instances
		StelTextureSP tex_Kd;
		StelTextureSP tex_Ke;
		StelTextureSP tex_bump;
		StelTextureSP tex_height;

		bool bAlphatest;
		bool bBackface;
		float fAlphaThreshold;

		Vec2d vis_fadeIn;  // JD of begin of first visibility and begin of full visibility.
		Vec2d vis_fadeOut; // JD of end of full visibility and end of last visibility.
		//! Updated by S3DRenderer when necessary, otherwise always 1.0
		float vis_fadeValue;

		//! Starts loading the textures in this material asynchronously
		void loadTexturesAsync();
		//! Re-calculates the material traits, and sets invalid material fields to valid values.
		//! This requires all textures to be fully loaded.
		void fixup();
		//! Re-calculates time fade info, returns true if the object should be rendered
		bool updateFadeInfo(double currentJD);
	};

	typedef QVector<Material> MaterialList;
	//for now, this does not use custom extensions...
	typedef StelOBJ::ObjectList ObjectList;

	explicit S3DScene(const SceneInfo& info);

	void setModel(const StelOBJ& model);
	void setGround(const StelOBJ& ground);

	const SceneInfo& getSceneInfo() const { return info; }

	MaterialList& getMaterialList() { return materials; }
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

#endif // S3DSCENE_HPP
