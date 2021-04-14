/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#include <QtGlobal>

#include "GLFuncs.hpp"
#include "S3DRenderer.hpp"
#include "S3DScene.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelPainter.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "LandscapeMgr.hpp"
#include "SolarSystem.hpp"
#include "StelOBJ.hpp"

#include <QKeyEvent>
#include <QSettings>
#include <stdexcept>
#include <cmath>
#include <QOpenGLShaderProgram>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(s3drenderer, "stel.plugin.scenery3d.renderer")

// (cast return value to void to silence Coverity)
#define GET_GLERROR() (void)StelOpenGL::checkGLErrors(__FILE__,__LINE__);

//macro for easier uniform setting
#define SET_UNIFORM(shd,uni,val) shd->setUniformValue(shaderManager.uniformLocation(shd,uni),val)

static const float LUNAR_BRIGHTNESS_FACTOR=0.2f;
static const float VENUS_BRIGHTNESS_FACTOR=0.005f;

#ifndef QT_OPENGL_ES_2
//this is the place where this is initialized
GLExtFuncs* glExtFuncs;
#endif

#ifdef _MSC_VER
//disable a stupid warning about array value-initialization
#pragma warning(disable : 4351)
#endif

S3DRenderer::S3DRenderer(QObject *parent)
    :
      QObject(parent),
      sun(Q_NULLPTR), moon(Q_NULLPTR), venus(Q_NULLPTR),
      currentScene(Q_NULLPTR),
      supportsGSCubemapping(false), supportsShadows(false), supportsShadowFiltering(false), isANGLE(false), maximumFramebufferSize(0),
      defaultFBO(0),
      torchBrightness(0.5f), torchRange(5.0f), textEnabled(false), debugEnabled(false), fixShadowData(false),
      simpleShadows(false), fullCubemapShadows(false), cubemappingMode(S3DEnum::CM_TEXTURES), //set it to 6 textures as a safe default (Cubemap should work on ANGLE, but does not...)
      reinitCubemapping(true), reinitShadowmapping(true),
      cubemapSize(1024),shadowmapSize(1024),wasMovedInLastDrawCall(false),
      core(Q_NULLPTR), landscapeMgr(Q_NULLPTR),
      backfaceCullState(true), blendEnabled(false), lastMaterial(Q_NULLPTR), curShader(Q_NULLPTR),
      drawnTriangles(0), drawnModels(0), materialSwitches(0), shaderSwitches(0),
      requiresCubemap(false), cubemappingUsedLastFrame(false),
      lazyDrawing(false), updateOnlyDominantOnMoving(true), updateSecondDominantOnMoving(true), needsMovementEndUpdate(false),
      needsCubemapUpdate(true), needsMovementUpdate(false), lazyInterval(2.0), lastCubemapUpdate(0.0), lastCubemapUpdateRealTime(0), lastMovementEndRealTime(0),
      cubeMapCubeTex(0), cubeMapCubeDepth(0), cubeMapTex(), cubeRB(0), dominantFace(0), secondDominantFace(1), cubeFBO(0), cubeSideFBO(), cubeMappingCreated(false),
      cubeVertexBuffer(QOpenGLBuffer::VertexBuffer), transformedCubeVertexBuffer(QOpenGLBuffer::VertexBuffer), cubeIndexBuffer(QOpenGLBuffer::IndexBuffer), cubeIndexCount(0),
      lightOrthoNear(0.1f), lightOrthoFar(1000.0f), parallaxScale(0.015f)
{
	#ifndef NDEBUG
	qCDebug(s3drenderer)<<"Scenery3d constructor...";
	#endif
	//the arrays should all contain only zeroes
	Q_ASSERT(cubeMapTex[0]==0);
	Q_ASSERT(cubeSideFBO[0]==0);

	shaderParameters.openglES = false;
	shaderParameters.shadowTransform = false;
	shaderParameters.pixelLighting = false;
	shaderParameters.bump = false;
	shaderParameters.shadows = false;
	shaderParameters.shadowFilterQuality = S3DEnum::SFQ_LOW;
	shaderParameters.pcss = false;
	shaderParameters.geometryShader = false;
	shaderParameters.torchLight = false;
	shaderParameters.frustumSplits = 0;
	shaderParameters.hwShadowSamplers = false;

	renderShaderParameters=shaderParameters;

	debugTextFont.setFamily("Courier");
	debugTextFont.setPixelSize(16);

	#ifndef NDEBUG
	qCDebug(s3drenderer)<<"Scenery3d constructor...done";
	#endif
}

S3DRenderer::~S3DRenderer()
{
	cubeVertexBuffer.destroy();
	cubeIndexBuffer.destroy();

	deleteShadowmapping();
	deleteCubemapping();

#ifndef QT_OPENGL_ES_2
	//delete extension functions
	delete glExtFuncs;
#endif
}

void S3DRenderer::saveFrusts()
{
	fixShadowData = !fixShadowData;

	camFrustShadow.saveDrawingCorners();

	for(int i=0; i<shaderParameters.frustumSplits; i++)
	{
		if(fixShadowData) frustumArray[i].saveDrawingCorners();
		else frustumArray[i].resetCorners();
	}
}

void S3DRenderer::setupPassUniforms(QOpenGLShaderProgram *shader)
{
	//send projection matrix
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MAT_PROJECTION, projectionMatrix);

	//set alpha test threshold (this is scene-global for now)
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_FLOAT_ALPHA_THRESH,currentScene->getSceneInfo().transparencyThreshold);

	//torch attenuation factor
	SET_UNIFORM(shader, ShaderMgr::UNIFORM_TORCH_ATTENUATION, lightInfo.torchAttenuation);

	//-- Shadowing setup -- this was previously in generateCubeMap_drawSceneWithShadows
	//first check if shader supports shadows
	GLint loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_VEC_SPLITDATA);

	//ALWAYS update the shader matrices, even if "no" shadow is cast
	//this fixes weird time-dependent crashes (this was fun to debug)
	if(shaderParameters.shadows && loc >= 0)
	{
		//Holds the frustum splits necessary for the lookup in the shader
		Vec4f splitData;
		for(int i=0; i<shaderParameters.frustumSplits; i++)
		{
			float zVal;
			if(i<shaderParameters.frustumSplits-1)
			{
				//the frusta have a slight overlap
				//use the center of this overlap for more robust filtering
				zVal = (frustumArray.at(i).zFar + frustumArray.at(i+1).zNear) / 2.0f;
			}
			else
				zVal = frustumArray.at(i).zFar;

			//see Nvidia CSM example for this calculation
			//http://developer.download.nvidia.com/SDK/10/opengl/screenshots/samples/cascaded_shadow_maps.html
			//the distance needs to be in the final clip space, not in eye space (or it would be a clipping sphere instead of a plane!)
			splitData.v[i] = 0.5f*(-zVal * projectionMatrix.constData()[10] + projectionMatrix.constData()[14])/zVal + 0.5f;

			//Bind current depth map texture
			glActiveTexture(GL_TEXTURE4+static_cast<uint>(i));
			glBindTexture(GL_TEXTURE_2D, shadowMapsArray.at(i));

			SET_UNIFORM(shader,static_cast<ShaderMgr::UNIFORM>(ShaderMgr::UNIFORM_TEX_SHADOW0+i), 4+i);
			SET_UNIFORM(shader,static_cast<ShaderMgr::UNIFORM>(ShaderMgr::UNIFORM_MAT_SHADOW0+i), shadowCPM.at(i));
		}

		//Send squared splits to the shader
		shader->setUniformValue(loc, splitData.v[0], splitData.v[1], splitData.v[2], splitData.v[3]);

		if(shaderParameters.shadowFilterQuality>S3DEnum::SFQ_HARDWARE)
		{
			//send size of light ortho for each frustum
			loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_VEC_LIGHTORTHOSCALE);
			shader->setUniformValueArray(loc,shadowFrustumSize.constData(),shaderParameters.frustumSplits);
		}
	}

	loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MAT_CUBEMVP);
	if(loc>=0)
	{
		//upload cube mvp matrices
		shader->setUniformValueArray(loc,cubeMVP,6);
	}
}

void S3DRenderer::setupFrameUniforms(QOpenGLShaderProgram *shader)
{
	//-- Transform setup --
	//check if shader wants a MVP or separate matrices
	GLint loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MAT_MVP);
	if(loc>=0)
	{
		shader->setUniformValue(loc,projectionMatrix * modelViewMatrix);
	}

	//this macro saves a bit of writing
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MAT_MODELVIEW, modelViewMatrix);

	//-- Lighting setup --
	//check if we require a normal matrix, this is assumed to be required for all "shading" shaders
	loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MAT_NORMAL);
	if(loc>=0)
	{
		QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
		shader->setUniformValue(loc,normalMatrix);

		//assume light direction is only required when normal matrix is also used (would not make much sense alone)
		//check if the shader wants view space info
		loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_LIGHT_DIRECTION_VIEW);
		if(loc>=0)
			shader->setUniformValue(loc,(normalMatrix * lightInfo.lightDirectionWorld));
	}
}

void S3DRenderer::setupMaterialUniforms(QOpenGLShaderProgram* shader, const S3DScene::Material &mat)
{
	//ambient is calculated depending on illum model
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_AMBIENT,mat.Ka * lightInfo.ambient);

	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_DIFFUSE, mat.Kd * lightInfo.directional);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_TORCHDIFFUSE, mat.Kd * lightInfo.torchDiffuse);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_EMISSIVE,mat.Ke * lightInfo.emissive);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_SPECULAR,mat.Ks * lightInfo.specular);

	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MTL_SHININESS,mat.Ns);
	//force alpha to 1 here for non-translucent mats (fixes incorrect blending in cubemap)
	float baseAlpha = mat.traits.hasTransparency ? mat.d : 1.0f;
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MTL_ALPHA,mat.traits.isFading ? baseAlpha * mat.vis_fadeValue : baseAlpha);

	if(mat.traits.hasDiffuseTexture)
	{
		mat.tex_Kd->bind(0); //this already sets glActiveTexture(0)
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_DIFFUSE,0);
	}
	if(mat.traits.hasEmissiveTexture)
	{
		mat.tex_Ke->bind(1);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_EMISSIVE,1);
	}
	if(shaderParameters.bump && mat.traits.hasBumpTexture)
	{
		mat.tex_bump->bind(2);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_BUMP,2);
	}
	if(shaderParameters.bump && mat.traits.hasHeightTexture)
	{
		mat.tex_height->bind(3);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_HEIGHT,3);
	}
}

//depth sort helper
static Vec3f zSortValue;
bool zSortFunction(const StelOBJ::MaterialGroup* const & mLeft, const StelOBJ::MaterialGroup* const & mRight)
{
	//we can avoid taking the sqrt here
	float dist1 = (mLeft->centroid - zSortValue).lengthSquared();
	float dist2 = (mRight->centroid - zSortValue).lengthSquared();
	return dist1>dist2;
}

bool S3DRenderer::drawArrays(bool shading, bool blendAlphaAdditive)
{
	//override some shader Params
	renderShaderParameters = shaderParameters;
	switch(lightInfo.shadowCaster)
	{
		case LightParameters::SC_Venus:
			//turn shadow filter off to get sharper shadows
			renderShaderParameters.shadowFilterQuality = S3DEnum::SFQ_OFF;
			break;
		case LightParameters::SC_None:
			//disable shadow rendering to speed things up
			renderShaderParameters.shadows = false;
			break;
		default:
			break;
	}

	//bind VAO
	currentScene->glBind();

	//assume backfaceculling is on, and blending turned off
	backfaceCullState = true;
	blendEnabled = false;
	lastMaterial = Q_NULLPTR;
	curShader = Q_NULLPTR;
	initializedShaders.clear();
	transparentGroups.clear();
	bool success = true;

	//TODO optimize: clump models with same material together when first loading to minimize state changes

	const S3DScene::ObjectList& objectList = currentScene->getObjects();
	for(int i=0; i<objectList.size(); ++i)
	{
		const StelOBJ::Object& obj = objectList.at(i);
		const StelOBJ::MaterialGroupList& matGroups = obj.groups;

		for(int j = 0; j < matGroups.size();++j)
		{
			const StelOBJ::MaterialGroup& matGroup = matGroups.at(j);
			const S3DScene::Material* pMaterial = &currentScene->getMaterial(matGroup.materialIndex);
			Q_ASSERT(pMaterial);

			if(pMaterial->traits.isFullyTransparent)
				continue; //dont render fully invisible objects

			if(shading)
			{
				if(pMaterial->traits.hasTransparency || pMaterial->traits.isFading)
				{
					//process transparent objects later, with Z sorting
					transparentGroups.append(&matGroup);
					continue;
				}
			}
			else
			{
				//objects start casting shadows with at least 0.2 opacity
				if(pMaterial->d * pMaterial->vis_fadeValue < 0.2f)
					continue;
			}

			success = drawMaterialGroup(matGroup,shading,blendAlphaAdditive);
			if(!success)
				break;
		}
	}

	//sort and render transparent objects
	if(transparentGroups.size()>0)
	{
		zSortValue = currentScene->getEyePosition().toVec3f();
		std::sort(transparentGroups.begin(),transparentGroups.end(),zSortFunction);

		for(int i = 0; i<transparentGroups.size();++i)
		{
			success = drawMaterialGroup(*transparentGroups[i],shading,blendAlphaAdditive);
			if(!success)
				break;
		}
	}

	//release last used shader and VAO
	if(curShader)
		curShader->release();
	currentScene->glRelease();

	//reset to default states
	if(!backfaceCullState)
		glEnable(GL_CULL_FACE);

	if(blendEnabled)
		glDisable(GL_BLEND);


	return success;
}

bool S3DRenderer::drawMaterialGroup(const StelOBJ::MaterialGroup &matGroup, bool shading, bool blendAlphaAdditive)
{
	const S3DScene::Material* pMaterial = &currentScene->getMaterial(matGroup.materialIndex);

	if(lastMaterial!=pMaterial)
	{
		++materialSwitches;
		lastMaterial = pMaterial;

		//get a shader from shadermgr that fits the current state + material combo
		QOpenGLShaderProgram* newShader = shaderManager.getShader(renderShaderParameters,pMaterial);
		if(!newShader)
		{
			//shader invalid, can't draw
			rendererMessage(q_("Scenery3d shader error, can't draw. Check debug output for details."));
			return false;
		}
		if(newShader!=curShader)
		{
			curShader = newShader;
			curShader->bind();
			if(!initializedShaders.contains(curShader))
			{
				++shaderSwitches;

				//needs first-time initialization for this pass
				if(shading)
				{
					setupPassUniforms(curShader);
					setupFrameUniforms(curShader);
				}
				else
				{
					//really only mvp+alpha thresh required, so only set this
					SET_UNIFORM(curShader,ShaderMgr::UNIFORM_MAT_MVP,projectionMatrix * modelViewMatrix);
					SET_UNIFORM(curShader,ShaderMgr::UNIFORM_FLOAT_ALPHA_THRESH,currentScene->getSceneInfo().transparencyThreshold);
				}

				//we remember if we have initialized this shader already, so we can skip "global" initialization later if we encounter it again
				initializedShaders.insert(curShader);
			}
		}
		if(shading)
		{
			//perform full material setup
			setupMaterialUniforms(curShader,*pMaterial);
		}
		else
		{
			//set diffuse tex if possible for alpha testing
			if( ! pMaterial->tex_Kd.isNull())
			{
				pMaterial->tex_Kd->bind(0);
				SET_UNIFORM(curShader,ShaderMgr::UNIFORM_TEX_DIFFUSE,0);
			}
		}

		if(shading && (pMaterial->traits.hasTransparency || pMaterial->traits.isFading))
		{
			if(!blendEnabled)
			{
				glEnable(GL_BLEND);
				if(blendAlphaAdditive)
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				else //traditional direct blending
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				blendEnabled = true;
			}
		}
		else if(blendEnabled)
		{
			glDisable(GL_BLEND);
			blendEnabled=false;
		}

		if(backfaceCullState && pMaterial->bBackface)
		{
			glDisable(GL_CULL_FACE);
			backfaceCullState = false;
		}
		else if(!backfaceCullState && !pMaterial->bBackface)
		{
			glEnable(GL_CULL_FACE);
			backfaceCullState = true;
		}
	}


	currentScene->glDraw(matGroup.startIndex,matGroup.indexCount);
	++drawnModels;
	drawnTriangles+=matGroup.indexCount/3;
	return true;
}

void S3DRenderer::computeFrustumSplits(const Vec3d& viewPos, const Vec3d& viewDir, const Vec3d& viewUp)
{
	//the frustum arrays all already contain the same adjusted frustum from adjustFrustum
	float zNear = frustumArray[0].zNear;
	float zFar = frustumArray[0].zFar;
	float zRatio = zFar / zNear;
	float zRange = zFar - zNear;

	const SceneInfo& info = currentScene->getSceneInfo();

	//Compute the z-planes for the subfrusta
	for(int i=1; i<shaderParameters.frustumSplits; i++)
	{
		float s_i = i/static_cast<float>(shaderParameters.frustumSplits);

		frustumArray[i].zNear = info.shadowSplitWeight*(zNear*powf(zRatio, s_i)) + (1.0f-info.shadowSplitWeight)*(zNear + (zRange)*s_i);
		//Set the previous zFar to the newly computed zNear
		//use a small overlap for robustness
		frustumArray[i-1].zFar = frustumArray[i].zNear * 1.005f;

		frustumArray[i-1].calcFrustum(viewPos,viewDir,viewUp);
	}

	//last zFar is already the zFar of the adjusted frustum
	frustumArray[shaderParameters.frustumSplits-1].calcFrustum(viewPos,viewDir,viewUp);
}

void S3DRenderer::computePolyhedron(Polyhedron& body,const Frustum& frustum,const Vec3f& shadowDir)
{
	//Building a convex body for directional lights according to Wimmer et al. 2006

	const AABBox& sceneAABB = currentScene->getSceneAABB();
	//Add the Frustum to begin with
	body.add(frustum);
	//Intersect with the scene AABB
	body.intersect(sceneAABB);
	//Extrude towards light direction
	body.extrude(shadowDir, sceneAABB);
}

void S3DRenderer::computeOrthoProjVals(const Vec3f shadowDir,float& orthoExtent,float& orthoNear,float& orthoFar)
{
	//Focus the light first on the entire scene
	float maxZ = -std::numeric_limits<float>::max();
	float minZ = std::numeric_limits<float>::max();
	orthoExtent = 0.0f;

	Vec3f eye = shadowDir;
	Vec3f vDir = -eye;
	vDir.normalize();
	Vec3f up = Vec3f(0.0f, 0.0f, 1.0f);
	Vec3f down = -up;
	Vec3f left = vDir^up;
	left.normalize();
	Vec3f right = -left;

	const AABBox& bbox = currentScene->getSceneAABB();

	for(unsigned int i=0; i<AABBox::CORNERCOUNT; i++)
	{
		Vec3f v = bbox.getCorner(static_cast<AABBox::Corner>(i));
		Vec3f toCam = v - eye;

		float dist = toCam.dot(vDir);
		maxZ = std::max(dist, maxZ);
		minZ = std::min(dist, minZ);

		orthoExtent = std::max(std::abs(toCam.dot(left)), orthoExtent);
		orthoExtent = std::max(std::abs(toCam.dot(right)), orthoExtent);
		orthoExtent = std::max(std::abs(toCam.dot(up)), orthoExtent);
		orthoExtent = std::max(std::abs(toCam.dot(down)), orthoExtent);
	}

	//Make sure planes arent too small
	orthoNear = minZ;
	orthoFar = maxZ;
	//orthoNear = std::max(minZ, 0.01f);
	//orthoFar = std::max(maxZ, orthoNear + 1.0f);
}

void S3DRenderer::computeCropMatrix(QMatrix4x4& cropMatrix, QVector4D& orthoScale, Polyhedron& focusBody,const QMatrix4x4& lightProj, const QMatrix4x4& lightMVP)
{
	float maxX = -std::numeric_limits<float>::max();
	float maxY = maxX;
	float maxZ = maxX;
	float minX = std::numeric_limits<float>::max();
	float minY = minX;
	float minZ = minX;

	//Project the frustum into light space and find the boundaries
	for(int i=0; i<focusBody.getVertCount(); i++)
	{
		const Vec3f tmp = focusBody.getVerts().at(i);
		QVector4D transf4 = lightMVP*QVector4D(tmp.v[0], tmp.v[1], tmp.v[2], 1.0f);
		QVector3D transf = transf4.toVector3DAffine();

		if(transf.x() > maxX) maxX = transf.x();
		if(transf.x() < minX) minX = transf.x();
		if(transf.y() > maxY) maxY = transf.y();
		if(transf.y() < minY) minY = transf.y();
		if(transf.z() > maxZ) maxZ = transf.z();
		if(transf.z() < minZ) minZ = transf.z();
	}

	//To avoid artifacts caused by far plane clipping, extend far plane by 5%
	//or if cubemapping is used, set it to 1
	if(!requiresCubemap  || fullCubemapShadows)
	{
		float zRange = maxZ-minZ;
		maxZ = std::min(maxZ + zRange*0.05f, 1.0f);
	}
	else
	{
		maxZ = 1.0f;
	}


	//minZ = std::max(minZ - zRange*0.05f, 0.0f);

#ifdef QT_DEBUG
	//AABBox deb(Vec3f(minX,minY,minZ),Vec3f(maxX,maxY,maxZ));
	//focusBody.debugBox = deb.toBox();
	//focusBody.debugBox.transform(lightMVP.inverted());
#endif

	//Build the crop matrix and apply it to the light projection matrix
	float scaleX = 2.0f/(maxX - minX);
	float scaleY = 2.0f/(maxY - minY);
	float scaleZ = 1.0f/(maxZ - minZ); //could also be 1, but this rescales the Z range to fit better
	//float scaleZ = 1.0f;

	float offsetZ = -minZ * scaleZ;
	//float offsetZ = 0.0f;

	//Reducing swimming as specified in Practical cascaded shadow maps by Zhang et al.
	const float quantizer = 64.0f;
	scaleX = 1.0f/std::ceil(1.0f/scaleX*quantizer) * quantizer;
	scaleY = 1.0f/std::ceil(1.0f/scaleY*quantizer) * quantizer;

	orthoScale = QVector4D(scaleX,scaleY,minZ,maxZ);

	float offsetX = -0.5f*(maxX + minX)*scaleX;
	float offsetY = -0.5f*(maxY + minY)*scaleY;

	float halfTex = 0.5f*shadowmapSize;
	offsetX = std::ceil(offsetX*halfTex)/halfTex;
	offsetY = std::ceil(offsetY*halfTex)/halfTex;

	//Making the crop matrix
	QMatrix4x4 crop(scaleX, 0.0f,   0.0f, offsetX,
			0.0f,   scaleY, 0.0f, offsetY,
			0.0f,   0.0f,   scaleZ, offsetZ,
			0.0f,   0.0f,   0.0f, 1.0f);

	//Crop the light projection matrix
	projectionMatrix = crop * lightProj;

	//Calculate texture matrix for projection
	//This matrix takes us from eye space to the light's clip space
	//It is postmultiplied by the inverse of the current view matrix when specifying texgen
	static const QMatrix4x4 biasMatrix(0.5f, 0.0f, 0.0f, 0.5f,
					   0.0f, 0.5f, 0.0f, 0.5f,
					   0.0f, 0.0f, 0.5f, 0.5f,
					   0.0f, 0.0f, 0.0f, 1.0f);	//bias from [-1, 1] to [0, 1]

	//calc final matrix
	cropMatrix = biasMatrix * projectionMatrix * modelViewMatrix;
}

void S3DRenderer::adjustShadowFrustum(const Vec3d& viewPos, const Vec3d& viewDir, const Vec3d& viewUp, const float fov, const float aspect)
{
	if(fixShadowData)
		return;
	//calc cam frustum for shadowing range
	//note that this is only correct in the perspective projection case, cubemapping WILL introduce shadow artifacts in most cases

	//TODO make shadows in cubemapping mode better by projecting the frusta, more closely estimating the required shadow extents
	const SceneInfo& info = currentScene->getSceneInfo();
	camFrustShadow.setCamInternals(fov,aspect,info.camNearZ,info.shadowFarZ);
	camFrustShadow.calcFrustum(viewPos, viewDir, viewUp);

	//Compute H = V intersect S according to Zhang et al.
	Polyhedron p;
	p.add(camFrustShadow);
	p.intersect(currentScene->getSceneAABB());
	p.makeUniqueVerts();

	//Find the boundaries
	float maxZ = -std::numeric_limits<float>::max();
	float minZ = std::numeric_limits<float>::max();

	Vec3f eye = viewPos.toVec3f();

	Vec3f vDir = viewDir.toVec3f();
	vDir.normalize();

	const QVector<Vec3f> &verts = p.getVerts();
	for(int i=0; i<p.getVertCount(); i++)
	{
		//Find the distance to the camera
		Vec3f v = verts[i];
		Vec3f toCam = v - eye;
		float dist = toCam.dot(vDir);

		maxZ = std::max(dist, maxZ);
		minZ = std::min(dist, minZ);
	}

	//Setup the newly found near and far planes but make sure they're not too small
	//minZ = std::max(minZ, 0.01f);
	//maxZ = std::max(maxZ, minZ+1.0f);

	//save adjusted values and recalc combined frustum for debugging
	camFrustShadow.setCamInternals(fov,aspect,minZ,maxZ);
	camFrustShadow.calcFrustum(viewPos,viewDir,viewUp);

	//Re-set the subfrusta
	for(int i=0; i<shaderParameters.frustumSplits; i++)
	{
		frustumArray[i].setCamInternals(fov, aspect, minZ, maxZ);
	}

	//Compute and set z-distances for each split
	computeFrustumSplits(viewPos,viewDir,viewUp);
}

void S3DRenderer::calculateShadowCaster()
{
	//shadow source and direction has been calculated in calculateLightSource
	static const QVector3D vZero = QVector3D();
	static const QVector3D vZeroZeroOne = QVector3D(0,0,1);

	//calculate lights modelview matrix
	lightInfo.shadowModelView.setToIdentity();
	lightInfo.shadowModelView.lookAt(lightInfo.lightDirectionWorld,vZero,vZeroZeroOne);
}

bool S3DRenderer::renderShadowMaps()
{
	if(fixShadowData)
		return true;

	shaderParameters.shadowTransform = true;

	//projection matrix gets updated below in updateCropMatrix
	modelViewMatrix = lightInfo.shadowModelView;

	//Fix selfshadowing
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f,2.0f);

	//GL state
	//enable depth + front face culling
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	//frontface culling for ESM!
	glCullFace(GL_FRONT);

	//Set viewport to shadowmap
	glViewport(0, 0, shadowmapSize, shadowmapSize);

	//Compute an orthographic projection that encompasses the whole scene
	//a crop matrix is used to restrict this projection to the subfrusta
	float orthoExtent;
	computeOrthoProjVals(lightInfo.lightDirectionV3f,orthoExtent,lightOrthoNear,lightOrthoFar);

	QMatrix4x4 lightProj;
	lightProj.ortho(-orthoExtent,orthoExtent,-orthoExtent,orthoExtent,lightOrthoNear,lightOrthoFar);

	//multiply with lights modelView matrix
	QMatrix4x4 lightMVP = lightProj*modelViewMatrix;

	bool success = true;

	//For each split
	for(int i=0; i<shaderParameters.frustumSplits; i++)
	{
		//Find the convex body that encompasses all shadow receivers and casters for this split
		focusBodies[i].clear();
		computePolyhedron(focusBodies[i],frustumArray[i],lightInfo.lightDirectionV3f);

		//qDebug() << i << ".split vert count:" << focusBodies[i]->getVertCount();

		glBindFramebuffer(GL_FRAMEBUFFER,shadowFBOs.at(i));
		//Clear everything, also if focusbody is empty
		glClear(GL_DEPTH_BUFFER_BIT);

		if(lightInfo.shadowCaster != LightParameters::SC_None && focusBodies[i].getVertCount())
		{
			//Calculate the crop matrix so that the light's frustum is tightly fit to the current split's PSR+PSC polyhedron
			//This alters the ProjectionMatrix of the light
			//the final light matrix used for lookups is stored in shadowCPM
			computeCropMatrix(shadowCPM[i], shadowFrustumSize[i], focusBodies[i],lightProj,lightMVP);

			//the shadow frustum size is only the scaling, multiply it with the extents of the original matrix
			shadowFrustumSize[i] = QVector4D(shadowFrustumSize[i][0] / orthoExtent, shadowFrustumSize[i][1] / orthoExtent,
					//shadowFrustumSize[i][2], shadowFrustumSize[i][3]);
					(.5f * shadowFrustumSize[i][2] + .5f) *(lightOrthoFar - lightOrthoNear) + lightOrthoNear,
					(.5f * shadowFrustumSize[i][3] + .5f) *(lightOrthoFar - lightOrthoNear) + lightOrthoNear );

			//Draw the scene
			if(!drawArrays(false))
			{
				success = false;
				break;
			}
		}
	}


	//Unbind
	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(defaultFBO));

	//reset viewport (see StelPainter::setProjector)
	const Vec4i& vp = altAzProjector->getViewport();
	glViewport(vp[0], vp[1], vp[2], vp[3]);

	//Move polygons back to normal position
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f,0.0f);

	//Reset
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	shaderParameters.shadowTransform = false;

	return success;
}

void S3DRenderer::calculateLighting()
{
	//calculate which light source we need + intensity
	float ambientBrightness=0.0f, directionalBrightness=0.0f, emissiveFactor;
	float eclipseFactor=static_cast<float>(GETSTELMODULE(SolarSystem)->getEclipseFactor(StelApp::getInstance().getCore()).first);

	Vec3d sunPosition = sun->getAltAzPosAuto(core);
	sunPosition.normalize();
	Vec3d moonPosition = moon->getAltAzPosAuto(core);
	//const float moonPhaseAngle = moon->getPhase(core->getObserverHeliocentricEclipticPos());
	const float moonBrightnessFromMag=moon->getVMagnitudeWithExtinction(core)/-12.73f; // This can handle eclipse situations better than just phase angle. Max. mag is around -13 (full moon surge).
	moonPosition.normalize();
	Vec3d venusPosition = venus->getAltAzPosAuto(core);
	const float venusPhaseAngle = venus->getPhase(core->getObserverHeliocentricEclipticPos());
	venusPosition.normalize();

	// The light model here: ambient light consists of solar twilight and day ambient,
	// plus lunar ambient, plus a base constant AMBIENT_BRIGHTNESS_FACTOR[0.1?],
	// plus an artificial "torch" that can be toggled via Ctrl-L[ight].
	// We define the ambient solar brightness zero when the sun is 18 degrees below the horizon, and lift the sun by 18 deg.
	// ambient brightness component of the sun is then  MIN(0.3, sin(sun)+0.3)
	// With the sun above the horizon, we raise only the directional component.
	// ambient brightness component of the moon is sqrt(sin(alt_moon)*(cos(moon.phase_angle)+1)/2)*LUNAR_BRIGHTNESS_FACTOR[0.2?]
	// Directional brightness factor: sqrt(sin(alt_sun)) if sin(alt_sun)>0 --> NO: MIN(0.7, sin(sun)+0.1), i.e. sun 6 degrees higher.
	//                                sqrt(sin(alt_moon)*(cos(moon.phase_angle)+1)/2)*LUNAR_BRIGHTNESS_FACTOR if sin(alt_moon)>0
	//                                sqrt(sin(alt_venus)*(cos(venus.phase_angle)+1)/2)*VENUS_BRIGHTNESS_FACTOR[0.15?]
	// Note the sqrt(sin(alt))-terms: they are to increase brightness sooner than with the Lambert law.
	//float sinSunAngleRad = sin(qMin(M_PI_2, asin(sunPosition[2])+8.*M_PI/180.));
	//float sinMoonAngleRad = moonPosition[2];

	const float sinSunAngle  =  static_cast<float>(sunPosition[2]);
	const float sinMoonAngle =  static_cast<float>(moonPosition[2]);
	const float sinVenusAngle = static_cast<float>(venusPosition[2]);

	//set the minimum ambient brightness
	//this uses the LandscapeMgr values
	Landscape* l = landscapeMgr->getCurrentLandscape();
	if (landscapeMgr->getFlagLandscapeUseMinimalBrightness())
	{
		// Setting from landscape.ini has priority if enabled
		if (landscapeMgr->getFlagLandscapeSetsMinimalBrightness() && l && l->getLandscapeMinimalBrightness()>=0)
			ambientBrightness = static_cast<float>(l->getLandscapeMinimalBrightness());
		else
			ambientBrightness = static_cast<float>(landscapeMgr->getDefaultMinimalBrightness());
	}

	lightInfo.shadowCaster = LightParameters::SC_None;
	lightInfo.backgroundAmbient = ambientBrightness;

	//assume light=sun for a start.
	Vec3d lightPosition = sunPosition;
	lightInfo.directionalSource = LightParameters::DS_Sun_Horiz;

	//calculate emissive factor
	if(l!=Q_NULLPTR)
	{
		if(requiresCubemap && lazyDrawing)
		{
			emissiveFactor = static_cast<float>(l->getTargetLightscapeBrightness());
		}
		else
		{
			//use an interpolated value for smooth fade in/out
			emissiveFactor = static_cast<float>(l->getEffectiveLightscapeBrightness());
		}
	}
	else
	{
		// I don't know if this can ever happen, but in this case,
		// directly use the same model as LandscapeMgr::update uses for the lightscapeBrightness
		emissiveFactor = 0.0f;
		if (sinSunAngle<-0.14f) emissiveFactor=1.0f;
		else if (sinSunAngle<-0.05f) emissiveFactor = 1.0f-(sinSunAngle+0.14f)/(-0.05f+0.14f);
	}

	// calculate ambient light
	if(sinSunAngle > -0.3f) // sun above -18 deg?
	{
		lightInfo.sunAmbient = qMin(0.3f, sinSunAngle+0.3f)*eclipseFactor;
		ambientBrightness += lightInfo.sunAmbient;
	}
	else
		lightInfo.sunAmbient = 0.0f;

	if ((sinMoonAngle>0.0f) && (sinSunAngle<0.0f))
	{
		//lightInfo.moonAmbient = sqrtf(sinMoonAngle * ((cosf(moonPhaseAngle)+1.0f)*0.5f)) * LUNAR_BRIGHTNESS_FACTOR;
		lightInfo.moonAmbient = sqrtf(sinMoonAngle * moonBrightnessFromMag) * LUNAR_BRIGHTNESS_FACTOR;
		ambientBrightness += lightInfo.moonAmbient;
	}
	else
		lightInfo.moonAmbient = 0.0f;

	// Now find shadow caster + directional light, if any:
	if (sinSunAngle>-0.1f)
	{
		directionalBrightness=qMin(0.7f, sqrtf(sinSunAngle+0.1f))*eclipseFactor; // limit to 0.7 in order to keep total below 1.
		//redundant
		//lightPosition = sunPosition;
		if (shaderParameters.shadows) lightInfo.shadowCaster = LightParameters::SC_Sun;
		lightInfo.directionalSource = LightParameters::DS_Sun;
	}
	// "else" is required now, else we have lunar shadow with sun above horizon...
	// and "else" implies sinSunAngle<-0.1 or sun <-6 degrees.
	else if (sinMoonAngle>0.0f)
	{
		float moonBrightness = sqrtf(sinMoonAngle) * moonBrightnessFromMag * LUNAR_BRIGHTNESS_FACTOR;
		moonBrightness -= (ambientBrightness-0.05f)*0.5f;
		moonBrightness = qMax(0.0f,moonBrightness);
		if (moonBrightness > 0.0f)
		{
			directionalBrightness = moonBrightness;
			lightPosition = moonPosition;
			if (shaderParameters.shadows) lightInfo.shadowCaster = LightParameters::SC_Moon;
		}
		lightInfo.directionalSource = LightParameters::DS_Moon;
		//Alternately, construct a term around lunar brightness, like
		// directionalBrightness=(mag/-10)
	}
	else if (sinVenusAngle>0.0f)
	{
		float venusBrightness = sqrtf(sinVenusAngle)*((cosf(venusPhaseAngle)+1)*0.5f) * VENUS_BRIGHTNESS_FACTOR;
		venusBrightness -= (ambientBrightness-0.05f)/2.0f;
		venusBrightness = qMax(0.0f, venusBrightness);
		if (venusBrightness > 0.0f)
		{
			directionalBrightness = venusBrightness;
			lightPosition = venusPosition;
			if (shaderParameters.shadows) lightInfo.shadowCaster = LightParameters::SC_Venus;
			lightInfo.directionalSource = LightParameters::DS_Venus;
		} else lightInfo.directionalSource = LightParameters::DS_Venus_Ambient;
		//Alternately, construct a term around Venus brightness, like
		// directionalBrightness=(mag/-100)
	}

	//convert to float
	lightInfo.lightDirectionV3f = lightPosition.toVec3f();
	lightInfo.lightDirectionWorld = lightPosition.toQVector3D();

	lightInfo.landscapeOpacity = 0.0f;

	//check landscape occlusion, modify directional if needed
	if(directionalBrightness>0)
	{
		if(l)
		{
			//TODO the changes are currently rather harsh, find a better method (like angular distance of light source to horizon, or bitmap interpolation for the alpha values)
			lightInfo.landscapeOpacity = l->getOpacity(lightPosition);

			//lerp between the determined opacity and 1.0, depending on landscape fade (visibility)
			float fadeValue = 1.0f + l->getEffectiveLandFadeValue() * (-lightInfo.landscapeOpacity);
			directionalBrightness *= fadeValue;
		}
	}

	//specular factor is calculated from other values for now
	float specular = std::min(ambientBrightness*directionalBrightness*5.0f,1.0f);

	//if the night vision mode is on, use red-tinted lighting
	bool red=StelApp::getInstance().getVisionModeNight();

	float torchDiff = shaderParameters.torchLight ? torchBrightness : 0.0f;
	lightInfo.torchAttenuation = 1.0f / (torchRange * torchRange);

	if(red)
	{
		lightInfo.ambient = QVector3D(ambientBrightness,0, 0);
		lightInfo.directional = QVector3D(directionalBrightness,0,0);
		lightInfo.emissive = QVector3D(emissiveFactor,0,0);
		lightInfo.specular = QVector3D(specular,0,0);
		lightInfo.torchDiffuse = QVector3D(torchDiff,0,0);
	}
	else
	{
		//for now, lighting is only white
		lightInfo.ambient = QVector3D(ambientBrightness,ambientBrightness, ambientBrightness);
		lightInfo.directional = QVector3D(directionalBrightness,directionalBrightness,directionalBrightness);
		lightInfo.emissive = QVector3D(emissiveFactor,emissiveFactor,emissiveFactor);
		lightInfo.specular = QVector3D(specular,specular,specular);
		lightInfo.torchDiffuse = QVector3D(torchDiff,torchDiff,torchDiff);
	}
}

void S3DRenderer::calcCubeMVP(const Vec3d translation)
{
	QMatrix4x4 tmp;
	for(int i = 0;i<6;++i)
	{
		tmp = cubeRotation[i];
		tmp.translate(static_cast<float>(translation.v[0]), static_cast<float>(translation.v[1]), static_cast<float>(translation.v[2]));
		cubeMVP[i] = projectionMatrix * tmp;
	}
}

void S3DRenderer::renderIntoCubemapGeometryShader()
{
	//single FBO
	glBindFramebuffer(GL_FRAMEBUFFER,cubeFBO);

	//Hack: because the modelviewmatrix is used for lighting in shader, but we dont want to perform MV transformations 6 times,
	// we just set the position because that currently is all that is needeed for correct lighting
	modelViewMatrix.setToIdentity();
	Vec3d negEyePos = -currentScene->getEyePosition();
	modelViewMatrix.translate(static_cast<float>(negEyePos.v[0]), static_cast<float>(negEyePos.v[1]), static_cast<float>(negEyePos.v[2]));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render all 6 faces at once
	shaderParameters.geometryShader = true;
	//calculate the final required matrices for each face
	calcCubeMVP(negEyePos);
	drawArrays(true,true);
	shaderParameters.geometryShader = false;
}

void S3DRenderer::renderShadowMapsForFace(int face)
{
	//extract view dir from the MV matrix
	QVector4D viewDir = -cubeRotation[face].row(2);

	//somewhere, there are problems when the view direction points exactly up or down, causing missing shadows
	//this is NOT fixed by choosing a different up vector here as could be expected
	//the problem seems to occur during final rendering because shadowmap textures look alright and the scaling values seem valid
	//for now, fix this by adding a tiny value to X in these cases
	adjustShadowFrustum(currentScene->getEyePosition(),
			    Vec3d(static_cast<double>(face>3?viewDir[0]+0.000001f:viewDir[0]),static_cast<double>(viewDir[1]),static_cast<double>(viewDir[2])),
			Vec3d(0,0,1),90.0f,1.0f);
	//render shadowmap
	if(!renderShadowMaps())
		return;

	//gl state + viewport must be reset
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glViewport(0, 0, cubemapSize, cubemapSize);
}

void S3DRenderer::renderIntoCubemapSixPasses()
{
	//store current projection (= 90° cube projection)
	QMatrix4x4 squareProjection = projectionMatrix;

	const Vec3f& eyePos = currentScene->getEyePosition().toVec3f();

	if(needsMovementUpdate && updateOnlyDominantOnMoving)
	{
		if(shaderParameters.shadows && fullCubemapShadows)
		{
			//in the BASIC and FULL modes, the shadow frustum needs to be adapted to the cube side
			renderShadowMapsForFace(dominantFace);
			//projection needs to be reset
			projectionMatrix = squareProjection;
		}

		//update only the dominant face
		glBindFramebuffer(GL_FRAMEBUFFER, cubeSideFBO[dominantFace]);
		modelViewMatrix = cubeRotation[dominantFace];
		modelViewMatrix.translate(-eyePos.v[0], -eyePos.v[1], -eyePos.v[2]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawArrays(true,true);

		if(updateSecondDominantOnMoving)
		{
			if(shaderParameters.shadows && fullCubemapShadows)
			{
				//in the BASIC and FULL modes, the shadow frustum needs to be adapted to the cube side
				renderShadowMapsForFace(secondDominantFace);
				//projection needs to be reset
				projectionMatrix = squareProjection;
			}

			//update also the second-most dominant face
			glBindFramebuffer(GL_FRAMEBUFFER, cubeSideFBO[secondDominantFace]);

			modelViewMatrix = cubeRotation[secondDominantFace];
			modelViewMatrix.translate(-eyePos.v[0], -eyePos.v[1], -eyePos.v[2]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			drawArrays(true,true);
		}
	}
	else
	{
		//traditional 6-pass version
		for(int i=0;i<6;++i)
		{
			if(shaderParameters.shadows && fullCubemapShadows)
			{
				//in the BASIC and FULL modes, the shadow frustum needs to be adapted to the cube side
				renderShadowMapsForFace(i);
				//projection needs to be reset
				projectionMatrix = squareProjection;
			}

			//bind a single side of the cube
			glBindFramebuffer(GL_FRAMEBUFFER, cubeSideFBO[i]);

			modelViewMatrix = cubeRotation[i];
			modelViewMatrix.translate(-eyePos.v[0], -eyePos.v[1], -eyePos.v[2]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			drawArrays(true,true);
		}
	}
}

void S3DRenderer::generateCubeMap()
{
	//recalculate lighting info
	calculateLighting();

	//do shadow pass
	//only calculate shadows if enabled
	if(shaderParameters.shadows)
	{
		//shadow caster info only needs to be calculated once
		calculateShadowCaster();

		//GS mode only supports the perspective shadows
		if(!fullCubemapShadows || cubemappingMode == S3DEnum::CM_CUBEMAP_GSACCEL)
		{
			//in this mode, shadow frusta are calculated the same as in perspective mode
			float fov = altAzProjector->getFov();
			float aspect = static_cast<float>(altAzProjector->getViewportWidth()) / static_cast<float>(altAzProjector->getViewportHeight());

			adjustShadowFrustum(currentScene->getEyePosition(),currentScene->getViewDirection(),Vec3d(0.,0.,1.),fov,aspect);
			if(!renderShadowMaps())
				return; //shadow map rendering failed, do an early abort
		}
	}

	const SceneInfo& info = currentScene->getSceneInfo();

	//setup projection matrix - this is a 90-degree perspective with aspect 1.0
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(90.0f,1.0f,info.camNearZ,info.camFarZ);

	//set opengl viewport to the size of cubemap
	glViewport(0, 0, cubemapSize, cubemapSize);

	//set GL state - we want depth test + culling
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);

	if(cubemappingMode == S3DEnum::CM_CUBEMAP_GSACCEL)
	{
		//In this mode, only the "perspective" shadow mode can be used (otherwise it would need up to 6*4 shadowmaps at once)
		renderIntoCubemapGeometryShader();
	}
	else
	{
		renderIntoCubemapSixPasses();
	}

	//cubemap fbo must be released
	glBindFramebuffer(GL_FRAMEBUFFER,defaultFBO);

	//reset GL state
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	//reset viewport (see StelPainter::setProjector)
	const Vec4i& vp = altAzProjector->getViewport();
	glViewport(vp[0], vp[1], vp[2], vp[3]);

	if(needsCubemapUpdate)
	{
		lastCubemapUpdate = core->getJD();
		lastCubemapUpdateRealTime = QDateTime::currentMSecsSinceEpoch();
	}
}

void S3DRenderer::drawFromCubeMap()
{
	QOpenGLShaderProgram* cubeShader;

	if(cubemappingMode>=S3DEnum::CM_CUBEMAP)
		cubeShader = shaderManager.getCubeShader();
	else
		cubeShader = shaderManager.getTextureShader();

	cubeShader->bind();

	//We simulate the generate behavoir of drawStelVertexArray ourselves
	//check if discontinuties exist
	//if(altAzProjector->hasDiscontinuity())
	//{
	//TODO fix similar to StelVertexArray::removeDiscontinuousTriangles
	//this may only happen for some projections, and even then it may be preferable to simply ignore them (as done now) to retain performance
	//}

	//transform vertices on CPU side - maybe we could do this multithreaded, kicked off at the beginning of the frame?
	altAzProjector->project(cubeVertices.count(),cubeVertices.constData(),transformedCubeVertices.data());

	//setup shader params
	projectionMatrix = altAzProjector->getProjectionMatrix().convertToQMatrix();
	cubeShader->setUniformValue(shaderManager.uniformLocation(cubeShader,ShaderMgr::UNIFORM_MAT_PROJECTION), projectionMatrix);
	cubeShader->setUniformValue(shaderManager.uniformLocation(cubeShader,ShaderMgr::UNIFORM_TEX_DIFFUSE),0);
	cubeVertexBuffer.bind();
	if(cubemappingMode>=S3DEnum::CM_CUBEMAP)
		cubeShader->setAttributeBuffer(StelOpenGLArray::ATTLOC_TEXCOORD,GL_FLOAT,0,3);
	else // 2D tex coords are stored in the same buffer, but with an offset
		cubeShader->setAttributeBuffer(StelOpenGLArray::ATTLOC_TEXCOORD,GL_FLOAT,cubeVertices.size() * static_cast<int>(sizeof(Vec3f)),2);
	cubeShader->enableAttributeArray(StelOpenGLArray::ATTLOC_TEXCOORD);
	cubeVertexBuffer.release();

	//upload transformed vertex data
	transformedCubeVertexBuffer.bind();
	transformedCubeVertexBuffer.allocate(transformedCubeVertices.constData(), transformedCubeVertices.size() * static_cast<int>(sizeof(Vec3f)));
	cubeShader->setAttributeBuffer(StelOpenGLArray::ATTLOC_VERTEX, GL_FLOAT, 0,3);
	cubeShader->enableAttributeArray(StelOpenGLArray::ATTLOC_VERTEX);
	transformedCubeVertexBuffer.release();

	glEnable(GL_BLEND);
	//note that GL_ONE is required here for correct blending (see drawArrays)
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	//depth test and culling is necessary for correct display,
	//because the cube faces can be projected in quite "weird" ways
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);

	glClear(GL_DEPTH_BUFFER_BIT);

	cubeIndexBuffer.bind();
	glActiveTexture(GL_TEXTURE0);
	if(cubemappingMode>=S3DEnum::CM_CUBEMAP)
	{
		//can render in a single draw call
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubeMapCubeTex);
		glDrawElements(GL_TRIANGLES,cubeIndexCount,GL_UNSIGNED_SHORT, Q_NULLPTR);
	}
	else
	{
		//use 6 drawcalls
		int faceIndexCount = cubeIndexCount / 6;
		for(int i =0;i<6;++i)
		{
			glBindTexture(GL_TEXTURE_2D, cubeMapTex[i]);
			glDrawElements(GL_TRIANGLES,faceIndexCount, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(static_cast<size_t>(i * faceIndexCount) * sizeof(short)));
		}
	}
	cubeIndexBuffer.release();

	cubeShader->disableAttributeArray(StelOpenGLArray::ATTLOC_TEXCOORD);
	cubeShader->disableAttributeArray(StelOpenGLArray::ATTLOC_VERTEX);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	cubeShader->release();
}

void S3DRenderer::drawDirect() // for Perspective Projection only!
{
    //calculate standard perspective projection matrix, use QMatrix4x4 for that
    float fov = altAzProjector->getFov();
    float aspect = static_cast<float>(altAzProjector->getViewportWidth()) / static_cast<float>(altAzProjector->getViewportHeight());

    //calc modelview transform
    QMatrix4x4 mvMatrix = altAzProjector->getModelViewTransform()->getApproximateLinearTransfo().convertToQMatrix();
    mvMatrix.optimize(); //may make inversion faster?

    //recalculate lighting info
    calculateLighting();

    const Vec3d& eyePos = currentScene->getEyePosition();
    const SceneInfo& info = currentScene->getSceneInfo();

    //do shadow pass
    //only calculate shadows if enabled
    if(shaderParameters.shadows)
    {
	    calculateShadowCaster();

	    //no need to extract view information, use the direction from stellarium
	    adjustShadowFrustum(eyePos,currentScene->getViewDirection(),Vec3d(0.0,0.0,1.0),fov,aspect);

	    //this call modifies projection + mv matrices, so we have to set them afterwards
	    if(!renderShadowMaps())
		    return; //shadow map rendering failed, do an early abort
    }

    mvMatrix.translate(static_cast<float>(-eyePos.v[0]),static_cast<float>(-eyePos.v[1]),static_cast<float>(-eyePos.v[2]));

    //set final rendering matrices
    modelViewMatrix = mvMatrix;
    projectionMatrix.setToIdentity();

    //without viewport offset, you could simply call this:
    //projectionMatrix.perspective(fov,aspect,currentScene.camNearZ,currentScene.camFarZ);
    //these 2 lines replicate gluPerspective with glFrustum
    float fH = std::tan( fov / 360.0f * M_PIf ) * info.camNearZ;
    float fW = fH * aspect;

    //apply offset values
    Vector2<qreal> vp = altAzProjector->getViewportCenterOffset();
    float horizOffset = 2.0f * fW * static_cast<float>(vp[0]);
    float vertOffset = - 2.0f * fH * static_cast<float>(vp[1]);

    //final projection matrix
    projectionMatrix.frustum(-fW + horizOffset, fW + horizOffset,
			     -fH + vertOffset, fH + vertOffset,
			     info.camNearZ, info.camFarZ);

    //depth test needs enabling, clear depth buffer, color buffer already contains background so it stays
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    //enable backface culling for increased performance
    glEnable(GL_CULL_FACE);

    //only 1 call needed here
    drawArrays(true);

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

void S3DRenderer::drawWithCubeMap()
{
	if(needsCubemapUpdate || needsMovementUpdate)
	{
		//lazy redrawing: update cubemap in slower intervals
		generateCubeMap();
	}
	drawFromCubeMap();
}

void S3DRenderer::drawCoordinatesText()
{
	StelPainter painter(altAzProjector);
	painter.setFont(debugTextFont);
	painter.setColor(1.0f,0.5f,1.0f);

	// Attempt at better font scaling for Macs and HiDPI.
	int fontSize=debugTextFont.pixelSize();

	float devicePixelscaling = static_cast<float>(altAzProjector->getDevicePixelsPerPixel())*StelApp::getInstance().getGlobalScalingRatio();
	float screen_x = altAzProjector->getViewportWidth()  - 240.0f*devicePixelscaling;
	float screen_y = altAzProjector->getViewportHeight() -  60.0f*devicePixelscaling;
	QString str;

	Vec3d gridPos = currentScene->getGridPosition();

	const SceneInfo& info = currentScene->getSceneInfo();

	// problem: long grid names!
	painter.drawText(altAzProjector->getViewportWidth()-10.f-devicePixelscaling*qMax(240, painter.getFontMetrics().boundingRect(info.gridName).width()),
			 screen_y, info.gridName);
	screen_y -= (fontSize+1)*devicePixelscaling;
	str = QString("East:   %1m").arg(gridPos[0], 10, 'f', 2);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= (fontSize-1)*devicePixelscaling;
	str = QString("North:  %1m").arg(gridPos[1], 10, 'f', 2);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= (fontSize-1)*devicePixelscaling;
	str = QString("Height: %1m").arg(gridPos[2], 10, 'f', 2);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= (fontSize-1)*devicePixelscaling;
	str = QString("Eye:    %1m").arg(currentScene->getEyeHeight(), 10, 'f', 2);
	painter.drawText(screen_x, screen_y, str);

//	DEBUG AIDS:
//	screen_y -= 15.0f;
//	str = QString("model_X:%1m").arg(model_pos[0], 10, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
//	str = QString("model_Y:%1m").arg(model_pos[1], 10, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
//	str = QString("model_Z:%1m").arg(model_pos[2], 10, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
//	str = QString("abs_X:  %1m").arg(absolutePosition.v[0], 10, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
//	str = QString("abs_Y:  %1m").arg(absolutePosition.v[1], 10, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
//	str = QString("abs_Z:  %1m").arg(absolutePosition.v[2], 10, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
//	str = QString("groundNullHeight: %1m").arg(groundNullHeight, 7, 'f', 2);
//	painter.drawText(screen_x, screen_y, str);
}

void S3DRenderer::drawDebug()
{
	//frustum/box debug rendering only on desktop GL
#ifndef QT_OPENGL_ES_2
	if(!shaderParameters.openglES)
	{
		QOpenGLShaderProgram* debugShader = shaderManager.getDebugShader();
		if(debugShader)
		{
			debugShader->bind();

			//ensure that opengl matrix stack is empty
			glExtFuncs->glMatrixMode(GL_MODELVIEW);
			glExtFuncs->glLoadIdentity();
			glExtFuncs->glMatrixMode(GL_PROJECTION);
			glExtFuncs->glLoadIdentity();

			//set mvp
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_MAT_MVP,projectionMatrix * modelViewMatrix);

			//SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(1.0f,1.0f,1.0f,1.0f));
			//sceneBoundingBox.render();

			//SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(0.4f,0.4f,0.4f,1.0f));
			//objModel->renderAABBs();

			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(1.0f,1.0f,1.0f,1.0f));

			if(fixShadowData)
			{
				camFrustShadow.drawFrustum();
				/*
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(1.0f,0.0f,1.0f,1.0f));
			frustumArray.at(0).drawFrustum();
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(0.0f,1.0f,0.0f,1.0f));
			focusBodies.at(0).render();
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(0.0f,1.0f,1.0f,1.0f));
			focusBodies.at(0).debugBox.render();
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(1.0f,0.0f,0.0f,1.0f));
			focusBodies.at(1).render();
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(1.0f,0.0f,1.0f,1.0f));
			focusBodies.at(1).debugBox.render();
			*/
			}

			debugShader->release();
		}
		else
		{
			qCWarning(s3drenderer)<<"Cannot use debug shader, probably on OpenGL ES context";
		}
	}
#endif

	const SceneInfo& info = currentScene->getSceneInfo();

	StelPainter painter(altAzProjector);

	const QStringList shadowCasterNames = {"None", "Sun", "Moon", "Venus"};
	QString shadowCasterName="Error!!!";
	Q_ASSERT(lightInfo.shadowCaster<=LightParameters::SC_Venus);
	shadowCasterName = shadowCasterNames.at(lightInfo.shadowCaster);

	const QStringList directionalSourceStrings = { "(Sun, below horiz.)", "Sun", "Moon", "Venus", "(Venus, flooded by ambient)"};
	QString directionalSourceString="Error!!!";
	Q_ASSERT(lightInfo.directionalSource<=LightParameters::DS_Venus_Ambient);
	directionalSourceString = directionalSourceStrings.at(lightInfo.directionalSource);

	const QString lightMessage=QString("Ambient: %1 Directional: %2. Shadows cast by: %3 from %4/%5/%6")
			.arg(lightInfo.ambient[0], 6, 'f', 4).arg(lightInfo.directional[0], 6, 'f', 4)
			.arg(shadowCasterName).arg(lightInfo.lightDirectionV3f.v[0], 6, 'f', 4)
			.arg(lightInfo.lightDirectionV3f.v[1], 6, 'f', 4).arg(lightInfo.lightDirectionV3f.v[2], 6, 'f', 4);
	const QString lightMessage2=QString("Contributions: Ambient     Sun: %1, Moon: %2, Background+^L: %3")
			.arg(lightInfo.sunAmbient, 6, 'f', 4).arg(lightInfo.moonAmbient, 6, 'f', 4).arg(lightInfo.backgroundAmbient, 6, 'f', 4);
	const QString lightMessage3=QString("               Directional %1 by: %2, emissive factor: %3, landscape opacity: %4")
			.arg(lightInfo.directional[0], 6, 'f', 4).arg(directionalSourceString).arg(lightInfo.emissive[0]).arg(lightInfo.landscapeOpacity);


	painter.setFont(debugTextFont);
	painter.setColor(1.f,0.f,1.f,1.f);
	// For now, these messages print light mixture values.
	painter.drawText(20, 160, lightMessage);
	painter.drawText(20, 145, lightMessage2);
	painter.drawText(20, 130, lightMessage3);
	painter.drawText(20, 115, QString("Torch range %1, brightness %2/%3/%4").arg(torchRange).arg(lightInfo.torchDiffuse[0]).arg(lightInfo.torchDiffuse[1]).arg(lightInfo.torchDiffuse[2]));

	const AABBox& bbox = currentScene->getSceneAABB();
	QString str = QString("BB: %1/%2/%3 %4/%5/%6").arg(bbox.min.v[0], 7, 'f', 2).arg(bbox.min.v[1], 7, 'f', 2).arg(bbox.min.v[2], 7, 'f', 2)
			.arg(bbox.max.v[0], 7, 'f', 2).arg(bbox.max.v[1], 7, 'f', 2).arg(bbox.max.v[2], 7, 'f', 2);
	painter.drawText(10, 100, str);
	// PRINT OTHER MESSAGES HERE:

	float screen_x = altAzProjector->getViewportWidth()  - 500.0f;
	float screen_y = altAzProjector->getViewportHeight() - 300.0f;

	//Show some debug aids
	if(debugEnabled)
	{
		float debugTextureSize = 128.0f;
		float screen_x = altAzProjector->getViewportWidth() - debugTextureSize - 30;
		float screen_y = altAzProjector->getViewportHeight() - debugTextureSize - 30;

		if(shaderParameters.shadows)
		{
			QString cap("SM %1");

			for(int i=0; i<shaderParameters.frustumSplits; i++)
			{
				painter.drawText(screen_x+70, screen_y+130, cap.arg(i));

				glBindTexture(GL_TEXTURE_2D, shadowMapsArray[i]);
				painter.drawSprite2dMode(screen_x, screen_y, debugTextureSize);

				const int tmp = qRound(screen_y - debugTextureSize)-30;
				painter.drawText(screen_x-125, tmp, QString("cam n/f: %1/%2").arg(frustumArray[i].zNear, 7, 'f', 2).arg(frustumArray[i].zFar, 7, 'f', 2));
				painter.drawText(screen_x-125, tmp-15.0f, QString("uv scale: %1/%2").arg(shadowFrustumSize[i].x(), 7, 'f', 2).arg(shadowFrustumSize[i].y(),7,'f',2));
				painter.drawText(screen_x-125, tmp-30.0f, QString("ortho n/f: %1/%2").arg(shadowFrustumSize[i].z(), 7, 'f', 2).arg(shadowFrustumSize[i].w(),7,'f',2));

				screen_x -= 290;
			}
			painter.drawText(screen_x+165.0f, screen_y-215.0f, QString("Splitweight: %1").arg(info.shadowSplitWeight, 3, 'f', 2));
			painter.drawText(screen_x+165.0f, screen_y-230.0f, QString("Light near/far: %1/%2").arg(lightOrthoNear, 3, 'f', 2).arg(lightOrthoFar, 3, 'f', 2));
		}
	}

	screen_y -= 100.f;
	str = QString("Last frame stats:");
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	str = QString("%1 tris, %2 mdls").arg(drawnTriangles).arg(drawnModels);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	str = QString("%1 mats, %2 shaders").arg(materialSwitches).arg(shaderSwitches);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	str = "View Pos";
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	const Vec3d& viewPos = currentScene->getEyePosition();
	str = QString("%1 %2 %3").arg(viewPos.v[0], 7, 'f', 2).arg(viewPos.v[1], 7, 'f', 2).arg(viewPos.v[2], 7, 'f', 2);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	str = "View Dir, dominant faces";
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	Vec3d mainViewDir = currentScene->getViewDirection();
	str = QString("%1 %2 %3, %4/%5").arg(mainViewDir.v[0], 7, 'f', 2).arg(mainViewDir.v[1], 7, 'f', 2).arg(mainViewDir.v[2], 7, 'f', 2).arg(dominantFace).arg(secondDominantFace);
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	str = "View Up";
	painter.drawText(screen_x, screen_y, str);
	screen_y -= 15.0f;
	if(requiresCubemap)
	{
		screen_y -= 15.0f;
		str = QString("Last cubemap update: %1ms ago").arg(QDateTime::currentMSecsSinceEpoch() - lastCubemapUpdateRealTime);
		painter.drawText(screen_x, screen_y, str);
		screen_y -= 15.0f;
		str = QString("Last cubemap update JDAY: %1").arg(qAbs(core->getJD()-lastCubemapUpdate) * StelCore::ONE_OVER_JD_SECOND);
		painter.drawText(screen_x, screen_y, str);
	}

	screen_y -= 30.0f;
	str = QString("Venus: %1").arg(lightInfo.shadowCaster == LightParameters::SC_Venus);
	painter.drawText(screen_x, screen_y, str);
}

void S3DRenderer::determineFeatureSupport()
{
	QOpenGLContext* ctx = QOpenGLContext::currentContext();

	// graphics hardware without FrameBufferObj extension cannot use the cubemap rendering and shadow mapping.
	// In this case, set cubemapSize to 0 to signal auto-switch to perspective projection.
	// OpenGL ES2 has framebuffers in the Spec
	if ( !ctx->functions()->hasOpenGLFeature(QOpenGLFunctions::Framebuffers) )
	{
		//TODO FS: it seems like the current stellarium requires a working framebuffer extension anyway, so skip this check?
		qCWarning(s3drenderer) << "Your hardware does not support EXT_framebuffer_object.";
		qCWarning(s3drenderer) << "Shadow mapping disabled, and display limited to perspective projection.";

		setCubemapSize(0);
		setShadowmapSize(0);
	}
	else
	{
		//determine maximum framebuffer size as minimum of texture, viewport and renderbuffer size
		GLint texSize,viewportSize[2],rbSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportSize);
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &rbSize);

		qCDebug(s3drenderer)<<"Maximum texture size:"<<texSize;
		qCDebug(s3drenderer)<<"Maximum viewport dims:"<<viewportSize[0]<<viewportSize[1];
		qCDebug(s3drenderer)<<"Maximum renderbuffer size:"<<rbSize;

		maximumFramebufferSize = qMin(texSize,qMin(rbSize,qMin(viewportSize[0],viewportSize[1])));
		qCDebug(s3drenderer)<<"Maximum framebuffer size:"<<maximumFramebufferSize;
	}

	QString renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	isANGLE = renderer.contains("ANGLE");

	//check if GS cubemapping is possible
	if(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry,ctx)) //this checks if version >= 3.2
	{
		this->supportsGSCubemapping = true;
		qCDebug(s3drenderer)<<"Geometry shader supported";
	}
	else
		qCWarning(s3drenderer)<<"Geometry shader not supported on this hardware";

	//Query how many texture units we have at disposal in a fragment shader
	//we currently need 8 in the worst case: diffuse, emissive, bump, height + 4x shadowmap
	GLint texUnits,combUnits;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texUnits);
	qCDebug(s3drenderer) << "GL_MAX_TEXTURE_IMAGE_UNITS:" << texUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &combUnits);
	qCDebug(s3drenderer) << "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:" << combUnits;
	if(texUnits < 8 || combUnits < 8)
	{
		qCWarning(s3drenderer)<<"Insufficient texture units available for all effects, should have at least 8!";
	}

	if(shaderParameters.openglES)
	{
		//shadows in our implementation require depth textures
		if(ctx->hasExtension("GL_OES_depth_texture") ||
			ctx->hasExtension("GL_ANGLE_depth_texture"))
		{
			supportsShadows = true;
			qCDebug(s3drenderer)<<"Shadows are supported";
		}
		else
		{
			supportsShadows = false;
			qCDebug(s3drenderer)<<"Shadows are not supported on this hardware";
		}
		//shadow filtering is completely disabled for now on ES
		supportsShadowFiltering = false;
	}
	else
	{
		//assume everything is available on Desktop GL for now (should be ok on GL>2.0 as Stellarium base requires)
		supportsShadows = true;
		supportsShadowFiltering = true;
	}
}

void S3DRenderer::init()
{
	initializeOpenGLFunctions();	
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	Q_ASSERT(ctx);

#ifndef QT_OPENGL_ES_2
	//initialize additional functions needed and not provided through StelOpenGL
	glExtFuncs = new GLExtFuncs();
	glExtFuncs->init(ctx);
#endif

	//save opengl ES state
	shaderParameters.openglES = ctx->isOpenGLES();

	//find out what features we can enable
	determineFeatureSupport();

	cubeVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	cubeVertexBuffer.create();
	transformedCubeVertexBuffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
	transformedCubeVertexBuffer.create();
	cubeIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	cubeIndexBuffer.create();

	//enable seamless cubemapping if HW supports it
	if(ctx->hasExtension("GL_ARB_seamless_cube_map"))
	{
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		qCDebug(s3drenderer)<<"Seamless cubemap filtering enabled";
#endif
	}


	//shadow map init happens on first usage of shadows

	//finally, set core to enable update().
	this->core=StelApp::getInstance().getCore();
	//init planets
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	sun = ssystem->getSun();
	moon = ssystem->getMoon();
	venus = ssystem->searchByEnglishName("Venus");
	landscapeMgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(landscapeMgr);
}

void S3DRenderer::deleteCubemapping()
{
	if(cubeMappingCreated)
	{
		//delete cube map - we have to check each possible variable because we dont know which ones are active
		//delete in reverse, FBOs first - but it should not matter
		if(cubeFBO)
		{
			glDeleteFramebuffers(1,&cubeFBO);
			cubeFBO = 0;
		}

		if(cubeSideFBO[0])
		{
			//we assume if one is created, all have been created
			glDeleteFramebuffers(6,cubeSideFBO);
			std::fill(cubeSideFBO,cubeSideFBO + 6,0);
		}

		//delete depth
		if(cubeRB)
		{
			glDeleteRenderbuffers(1,&cubeRB);
			cubeRB = 0;
		}

		if(cubeMapCubeDepth)
		{
			glDeleteTextures(1,&cubeMapCubeDepth);
			cubeMapCubeDepth = 0;
		}

		//delete colors
		if(cubeMapTex[0])
		{
			glDeleteTextures(6,cubeMapTex);
			std::fill(cubeMapTex, cubeMapTex + 6,0);
		}

		if(cubeMapCubeTex)
		{
			glDeleteTextures(1,&cubeMapCubeTex);
			cubeMapCubeTex = 0;
		}

		cubeMappingCreated = false;
	}
}

bool S3DRenderer::initCubemapping()
{
	GET_GLERROR()

	bool ret = false;
	qCDebug(s3drenderer)<<"Initializing cubemap...";

	//remove old cubemap objects if they exist
	deleteCubemapping();

	GET_GLERROR()

	if(cubemapSize<=0)
	{
		qCWarning(s3drenderer)<<"Cubemapping not supported or disabled";
		rendererMessage(q_("Your hardware does not support cubemapping, please switch to 'Perspective' projection!"));
		return false;
	}

	cubeMappingCreated = true;

	//last compatibility check before possible crash
	if( !isGeometryShaderCubemapSupported() && cubemappingMode == S3DEnum::CM_CUBEMAP_GSACCEL)
	{
		rendererMessage(q_("Geometry shader is not supported. Falling back to '6 Textures' mode."));
		qCWarning(s3drenderer)<<"GS not supported, fallback to '6 Textures'";
		cubemappingMode = S3DEnum::CM_TEXTURES;
	}

	//TODO the ANGLE version included with Qt 5.4 includes a bug that prevents Cubemapping to be used
	//Remove this if this is ever fixed
	if(isANGLEContext() && cubemappingMode >= S3DEnum::CM_CUBEMAP)
	{
		//Fall back to "6 Textures" mode
		rendererMessage(q_("Falling back to '6 Textures' because of ANGLE bug"));
		qCWarning(s3drenderer)<<"On ANGLE, fallback to '6 Textures'";
		cubemappingMode = S3DEnum::CM_TEXTURES;
	}


#ifndef QT_OPENGL_ES_2
	//if we are on an ES context, it may not be possible to specify texture bitdepth
	bool isEs = QOpenGLContext::currentContext()->isOpenGLES();
	GLint colorFormat = isEs ? GL_RGBA : GL_RGBA8;
	GLint depthFormat = isEs ? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT24;
	GLenum rbDepth = isEs ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24;
#else
	GLint colorFormat = GL_RGBA;
	GLint depthFormat = GL_DEPTH_COMPONENT;
	GLenum rbDepth = GL_DEPTH_COMPONENT16;
#endif

	glActiveTexture(GL_TEXTURE0);

	if(cubemappingMode >= S3DEnum::CM_CUBEMAP) //CUBEMAP or CUBEMAP_GSACCEL
	{
		//gen cube tex
		glGenTextures(1,&cubeMapCubeTex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapCubeTex);

		GET_GLERROR()

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		GET_GLERROR()

		//create faces
		for (uint i=0;i<6;++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,colorFormat,
				     cubemapSize,cubemapSize,0,GL_RGBA,GL_UNSIGNED_BYTE,Q_NULLPTR);
			GET_GLERROR()
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
	else //TEXTURES mode
	{
		//create 6 textures
		glGenTextures(6,cubeMapTex);

		GET_GLERROR()

		for(int i = 0;i<6;++i)
		{
			glBindTexture(GL_TEXTURE_2D, cubeMapTex[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			GET_GLERROR()

			glTexImage2D(GL_TEXTURE_2D,0,colorFormat,
				     cubemapSize,cubemapSize,0,GL_RGBA,GL_UNSIGNED_BYTE,Q_NULLPTR);

			GET_GLERROR()
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//create depth texture/RB
	if(cubemappingMode == S3DEnum::CM_CUBEMAP_GSACCEL)
	{
		//a single cubemap depth texture
		glGenTextures(1,&cubeMapCubeDepth);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapCubeDepth);
		//this all has probably not much effect on depth processing because we don't intend to sample
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		GET_GLERROR()

		//create faces
		for (uint i=0;i<6;++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,depthFormat,
				     cubemapSize,cubemapSize,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,Q_NULLPTR);

			GET_GLERROR()
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
	else
	{
		//gen renderbuffer for single-face depth, reused for all faces to save some memory
		int val = 0;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&val);
		qCDebug(s3drenderer)<<"Max Renderbuffer size"<<val;

		glGenRenderbuffers(1,&cubeRB);
		glBindRenderbuffer(GL_RENDERBUFFER,cubeRB);
		glRenderbufferStorage(GL_RENDERBUFFER, rbDepth, cubemapSize,cubemapSize);
		GLenum err=glGetError();

		switch(err){
			case GL_NO_ERROR:
					break;
			case GL_INVALID_ENUM:
					qCWarning(s3drenderer)<<"RB: invalid depth format?";
					break;
			case GL_INVALID_VALUE:
					qCWarning(s3drenderer)<<"RB: invalid renderbuffer size";
					break;
			case GL_OUT_OF_MEMORY:
					qCWarning(s3drenderer)<<"RB: out of memory. Cannot create renderbuffer.";
					break;
				default:
				qCWarning(s3drenderer)<<"RB: unexpected OpenGL error:" << err;
		}

		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	//generate FBO/FBOs
	if(cubemappingMode == S3DEnum::CM_CUBEMAP_GSACCEL)
	{
		//only 1 FBO used
		//create fbo
		glGenFramebuffers(1,&cubeFBO);
		glBindFramebuffer(GL_FRAMEBUFFER,cubeFBO);

		GET_GLERROR()

#ifndef QT_OPENGL_ES_2
		//attach cube tex + cube depth
		//note that this function will be a NULL pointer if GS is not supported, so it is important to check support before using
		glExtFuncs->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,cubeMapCubeTex,0);
		glExtFuncs->glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapCubeDepth, 0);
#endif

		GET_GLERROR()

		//check validity
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			qCWarning(s3drenderer) << "glCheckFramebufferStatus failed, probably can't use cube map";
		}
		else
			ret = true;
	}
	else
	{
		//6 FBOs used
		glGenFramebuffers(6,cubeSideFBO);

		GET_GLERROR()

		for(uint i=0;i<6;++i)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, cubeSideFBO[i]);

			//attach color - 1 side of cubemap or single texture
			if(cubemappingMode == S3DEnum::CM_CUBEMAP)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,cubeMapCubeTex,0);
			else
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cubeMapTex[i],0);

			GET_GLERROR()

			//attach shared depth buffer
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, cubeRB);

			GET_GLERROR()

			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				qCWarning(s3drenderer) << "glCheckFramebufferStatus failed, probably can't use cube map";
				ret = false;
				break;
			}
			else
				ret = true;
		}
	}

	//unbind last framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER,defaultFBO);

	//initialize cube rotations... found by trial and error :)
	QMatrix4x4 stackBase;

	//all angles were found using some experimenting :)
	//this is the EAST face (y=1)
	stackBase.rotate(90.0f,-1.0f,0.0f,0.0f);

	if(cubemappingMode >= S3DEnum::CM_CUBEMAP)
	{
		//cubemap mode needs other rotations than texture mode

		//south (x=1) ok
		cubeRotation[0] = stackBase;
		cubeRotation[0].rotate(-90.0f,0.0f,1.0f,0.0f);
		cubeRotation[0].rotate(90.0f,0.0f,0.0f,1.0f);
		//NORTH (x=-1) ok
		cubeRotation[1] = stackBase;
		cubeRotation[1].rotate(90.0f,0.0f,1.0f,0.0f);
		cubeRotation[1].rotate(-90.0f,0.0f,0.0f,1.0f);
		//EAST (y=1) ok
		cubeRotation[2] = stackBase;
		//west (y=-1) ok
		cubeRotation[3] = stackBase;
		cubeRotation[3].rotate(180.0f,-1.0f,0.0f,0.0f);
		//top (z=1) ok
		cubeRotation[4] = stackBase;
		cubeRotation[4].rotate(-90.0f,1.0f,0.0f,0.0f);
		//bottom (z=-1)
		cubeRotation[5] = stackBase;
		cubeRotation[5].rotate(90.0f,1.0f,0.0f,0.0f);
		cubeRotation[5].rotate(180.0f,0.0f,0.0f,1.0f);
	}
	else
	{
		cubeRotation[0] = stackBase;
		cubeRotation[0].rotate(90.0f,0.0f,0.0f,1.0f);

		cubeRotation[1] = stackBase;
		cubeRotation[1].rotate(90.0f,0.0f,0.0f,-1.0f);

		cubeRotation[2] = stackBase;

		cubeRotation[3] = stackBase;
		cubeRotation[3].rotate(180.0f,0.0f,0.0f,1.0f);

		cubeRotation[4] = stackBase;
		cubeRotation[4].rotate(90.0f,-1.0f,0.0f,0.0f);

		cubeRotation[5] = stackBase;
		cubeRotation[5].rotate(90.0f,1.0f,0.0f,0.0f);
	}


	//create a 20x20 cube subdivision to give a good approximation of non-linear projections
	const int sub = 20;
	const int vtxCount = (sub+1) * (sub+1);
	const float d_sub_v = 2.0f / sub;
	const float d_sub_tex = 1.0f / sub;

	//create the front cubemap face vertices
	QVector<Vec3f> cubePlaneFront;
	QVector<Vec2f> cubePlaneFrontTex;
	QVector<unsigned short> frontIndices;
	cubePlaneFront.reserve(vtxCount);
	cubePlaneFrontTex.reserve(vtxCount);

	//store the indices of the vertices
	//this could easily be recalculated as needed but this makes it a bit more readable
	unsigned short vertexIdx[sub+1][sub+1];

	//first, create the actual vertex positions, (20+1)^2 vertices
	for (int y = 0; y <= sub; y++) {
		for (int x = 0; x <= sub; x++) {
			float xp = -1.0f + x * d_sub_v;
			float yp = -1.0f + y * d_sub_v;

			float tx = x * d_sub_tex;
			float ty = y * d_sub_tex;

			cubePlaneFront<< Vec3f(xp, 1.0f, yp);
			cubePlaneFrontTex<<Vec2f(tx,ty);

			vertexIdx[y][x] = static_cast<unsigned short>(y*(sub+1)+x);
		}
	}

	Q_ASSERT(cubePlaneFrontTex.size() == vtxCount);
	Q_ASSERT(cubePlaneFront.size() == vtxCount);

	//generate indices for each of the 20x20 subfaces
	//TODO optimize for TRIANGLE_STRIP?
	for ( int y = 0; y < sub; y++)
	{
		for( int x = 0; x<sub; x++)
		{
			//first tri (top one)
			frontIndices<<vertexIdx[y+1][x];
			frontIndices<<vertexIdx[y][x];
			frontIndices<<vertexIdx[y+1][x+1];

			//second tri
			frontIndices<<vertexIdx[y+1][x+1];
			frontIndices<<vertexIdx[y][x];
			frontIndices<<vertexIdx[y][x+1];
		}
	}

	int idxCount = frontIndices.size();

	//create the other faces
	//note that edge vertices of the faces are duplicated

	cubeVertices.clear();
	cubeVertices.reserve(vtxCount * 6);
	cubeTexcoords.clear();
	cubeTexcoords.reserve(vtxCount * 6);
	QVector<unsigned short> cubeIndices; //index data is not needed afterwards on CPU side, so use a local vector
	cubeIndices.reserve(idxCount * 6);
	//init with copies of front face
	for(int i = 0;i<6;++i)
	{
		//order of geometry should be as follows:
		//basically "reversed" cubemap order
		//S face x=1
		//N face x=-1
		//E face y=1
		//W face y=-1
		//up face z=1
		//down face z=-1
		cubeVertices<<cubePlaneFront;
		cubeTexcoords<<cubePlaneFrontTex;
		cubeIndices<<frontIndices;
	}

	Q_ASSERT(cubeVertices.size() == cubeTexcoords.size());

	transformedCubeVertices.resize(cubeVertices.size());
	cubeIndexCount = cubeIndices.size();

	qCDebug(s3drenderer)<<"Using cube with"<<cubeVertices.size()<<"vertices and" <<cubeIndexCount<<"indices";

	//create the other cube faces by rotating the front face
#define PLANE(_PLANEID_, _MAT_) for(int i=_PLANEID_ * vtxCount;i < (_PLANEID_ + 1)*vtxCount;i++){ _MAT_.transfo(cubeVertices[i]); }\
	for(int i =_PLANEID_ * idxCount; i < (_PLANEID_+1)*idxCount;++i) { cubeIndices[i] = cubeIndices[i] + _PLANEID_ * vtxCount; }

	PLANE(0, Mat4f::zrotation(-M_PI_2f)); //S
	PLANE(1, Mat4f::zrotation(M_PI_2f));  //N
	PLANE(2, Mat4f::identity());  //E
	PLANE(3, Mat4f::zrotation(M_PIf)); //W
	PLANE(4, Mat4f::xrotation(M_PI_2f)); //U
	PLANE(5, Mat4f::xrotation(-M_PI_2f)); //D
#undef PLANE

	//upload original cube vertices + indices to GL
	cubeVertexBuffer.bind();
	//store original vertex pos (=3D vertex coords) + 2D tex coords in same buffer
	cubeVertexBuffer.allocate(cubeVertices.size() * static_cast<int>((sizeof(Vec3f) + sizeof(Vec2f))) );
	cubeVertexBuffer.write(0, cubeVertices.constData(), cubeVertices.size() * static_cast<int>(sizeof(Vec3f)));
	cubeVertexBuffer.write(cubeVertices.size() * static_cast<int>(sizeof(Vec3f)), cubeTexcoords.constData(), cubeTexcoords.size() * static_cast<int>(sizeof(Vec2f)));
	cubeVertexBuffer.release();

	cubeIndexBuffer.bind();
	cubeIndexBuffer.allocate(cubeIndices.constData(),cubeIndices.size() * static_cast<int>(sizeof(unsigned short)));
	cubeIndexBuffer.release();

	//reset cubemap timer to make sure it is rerendered immediately after re-init
	invalidateCubemap();

	qCDebug(s3drenderer)<<"Initializing cubemap...done!";

	if(!ret)
	{
		rendererMessage("Cannot use cubemapping with current settings");
		deleteCubemapping();
	}
	return ret;
}

void S3DRenderer::deleteShadowmapping()
{
	if(shadowFBOs.size()>0) //kinda hack that finds out if shadowmap related objects have been created
	{
		//we can delete them all at once then
		glDeleteFramebuffers(shadowFBOs.size(),shadowFBOs.constData());
		glDeleteTextures(shadowMapsArray.size(),shadowMapsArray.constData());

		shadowFBOs.clear();
		shadowMapsArray.clear();
		shadowCPM.clear();
		shadowFrustumSize.clear();
		frustumArray.clear();
		focusBodies.clear();

		qCDebug(s3drenderer)<<"Shadowmapping objects cleaned up";
	}
}

bool S3DRenderer::initShadowmapping()
{
	deleteShadowmapping();

	bool valid = false;

	if(simpleShadows)
	{
		shaderParameters.frustumSplits = 1;
	}
	else
	{
		//TODO support changing this option by the user and/or the scene?
		shaderParameters.frustumSplits = 4;
	}

	if(!areShadowsSupported())
	{
		qCWarning(s3drenderer)<<"Tried to initialize shadows without shadow support!";
		return false;
	}

	if(shadowmapSize>0)
	{
		//Define shadow maps array - holds MAXSPLITS textures
		shadowFBOs.resize(shaderParameters.frustumSplits);
		shadowMapsArray.resize(shaderParameters.frustumSplits);
		shadowCPM.resize(shaderParameters.frustumSplits);
		shadowFrustumSize.resize(shaderParameters.frustumSplits);
		frustumArray.resize(shaderParameters.frustumSplits);
		focusBodies.resize(shaderParameters.frustumSplits);

		//For shadowmapping, we use create 1 SM FBO for each frustum split - this seems to be the optimal solution on modern GPUs,
		//see http://www.reddit.com/r/opengl/comments/1rsnhy/most_efficient_fbo_usage_in_multipass_pipeline/
		//The point seems to be that switching attachments may cause re-validation of the FB.

		//Generate the FBO ourselves. We do this because Qt does not support depth-only FBOs to save some memory.
		glGenFramebuffers(shaderParameters.frustumSplits,shadowFBOs.data());
		glGenTextures(shaderParameters.frustumSplits,shadowMapsArray.data());

		for(int i=0; i<shaderParameters.frustumSplits; i++)
		{
			//Bind the FBO
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBOs.at(i));

			//Activate the texture unit - we want sahdows + textures so this is crucial with the current Stellarium pipeline - we start at unit 4
			glActiveTexture(GL_TEXTURE4+static_cast<uint>(i));

			//Bind the depth map and setup parameters
			glBindTexture(GL_TEXTURE_2D, shadowMapsArray.at(i));

#ifndef QT_OPENGL_ES_2
			bool isEs = QOpenGLContext::currentContext()->isOpenGLES();
			GLenum depthPcss = isEs ? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT32F;
			GLenum depthNormal = isEs ? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT16;
#else
			GLenum depthPcss = GL_DEPTH_COMPONENT;
			GLenum depthNormal = GL_DEPTH_COMPONENT;
#endif
			//pcss is only enabled if filtering is also enabled
			bool pcssEnabled = shaderParameters.pcss && (shaderParameters.shadowFilterQuality == S3DEnum::SFQ_LOW || shaderParameters.shadowFilterQuality == S3DEnum::SFQ_HIGH);

			//for OpenGL ES2, type has to be UNSIGNED_SHORT or UNSIGNED_INT for depth textures, desktop does probably not care
			glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(pcssEnabled ? depthPcss : depthNormal), shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, Q_NULLPTR);

			//we use hardware-accelerated depth compare mode, unless pcss is used
			shaderParameters.hwShadowSamplers = false;
			//NOTE: cant use depth compare mode on ES2
			if(!pcssEnabled)
			{
#ifndef QT_OPENGL_ES_2
				if(!isEs)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
					shaderParameters.hwShadowSamplers = true;
				}
#endif
			}

			//IF we support hw shadow sampling, then we may enable linear filtering, otherwise filtering depth values directly would not make much sense
			GLint filter = shaderParameters.hwShadowSamplers && (shaderParameters.shadowFilterQuality == S3DEnum::SFQ_HARDWARE
					|| shaderParameters.shadowFilterQuality == S3DEnum::SFQ_LOW_HARDWARE
					|| shaderParameters.shadowFilterQuality == S3DEnum::SFQ_HIGH_HARDWARE) ? GL_LINEAR : GL_NEAREST;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
#ifndef QT_OPENGL_ES_2
			if(!isEs)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL,0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				const float ones[] = {1.0f, 1.0f, 1.0f, 1.0f};
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, ones);
			}
#endif

			//Attach the depthmap to the Buffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapsArray[i], 0);

			//NOTE: disabling the drawbuffer should be required
			//but the respective functions are not available on GLES2?
			//On ANGLE, it seems to work without this settings (framebuffer is complete, etc.)
			//but I don't know if it will work on other ES platforms?
#ifndef QT_OPENGL_ES_2
			if(!isEs)
			{
				glExtFuncs->glDrawBuffer(GL_NONE); // essential for depth-only FBOs!!!
				glExtFuncs->glReadBuffer(GL_NONE);
			}
#endif

			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				qCWarning(s3drenderer) << "glCheckFramebufferStatus failed, can't use FBO";
				break;
			}
			else if (i==shaderParameters.frustumSplits-1)
			{
				valid = true;
			}
		}

		//Done. Unbind and switch to normal texture unit 0
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
		glActiveTexture(GL_TEXTURE0);

		qCDebug(s3drenderer)<<"shadowmapping initialized";
	}
	else
	{
		qCWarning(s3drenderer)<<"shadowmapping not supported or disabled";
	}

	if(!valid)
	{
		deleteShadowmapping();
		rendererMessage(q_("Shadow mapping can not be used on your hardware, check logs for details"));
	}
	return valid;
}

void S3DRenderer::draw(StelCore* core, S3DScene &scene)
{
	if(!scene.isGLReady())
	{
		scene.glLoad();
		invalidateCubemap();
	}

	//find out the default FBO
	defaultFBO = StelApp::getInstance().getDefaultFBO();
	currentScene = &scene;

	//reset render statistic
	drawnTriangles = drawnModels = materialSwitches = shaderSwitches = 0;

	requiresCubemap = core->getCurrentProjectionType() != StelCore::ProjectionPerspective;
	//update projector from core
	altAzProjector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	if(requiresCubemap)
	{
		if(!cubeMappingCreated || reinitCubemapping)
		{
			//init cubemaps
			if(!initCubemapping())
				return;
			reinitCubemapping = false;
		}

		if(lazyDrawing)
		{
			//if the viewer was moved after the last draw call
			bool wasMoved = lastDrawnPosition != currentScene->getEyePosition();

			//get current time
			double curTime = core->getJD();
			qint64 curMS = QDateTime::currentMSecsSinceEpoch();

			needsMovementUpdate = false;

			//check if cubemap requires redraw
			if(qAbs(curTime-lastCubemapUpdate) > lazyInterval * StelCore::JD_SECOND || reinitCubemapping)
			{
				needsCubemapUpdate = true;
				needsMovementEndUpdate = false;
			}
			else if (wasMoved) //we have been moved currently
			{
				if(updateOnlyDominantOnMoving)
				{
					needsMovementUpdate = true;
					needsMovementEndUpdate = true;
					needsCubemapUpdate = false;
				}
				else
				{
					needsCubemapUpdate = true;
					needsMovementEndUpdate = false;
				}
			}
			else
			{
				if(wasMovedInLastDrawCall) // we have been moved in the last draw call, but no longer
					lastMovementEndRealTime = curMS;

				if(needsMovementEndUpdate && (curMS - lastMovementEndRealTime)  > 700)
				{
					//if the last movement was some time ago, update the whole cubemap
					needsCubemapUpdate = true;
					needsMovementEndUpdate = false;
				}
				else
					needsCubemapUpdate = false;
			}

			wasMovedInLastDrawCall = wasMoved;
		}
		else
		{
			needsCubemapUpdate = true;
		}


		const Vec3d& mainViewDir = currentScene->getViewDirection();
		//find cubemap face this vector points at
		//only consider horizontal plane (XY)
		dominantFace = qAbs(mainViewDir.v[0])<qAbs(mainViewDir.v[1]);
		secondDominantFace = !dominantFace;

		//uncomment this to also consider up/down faces
		/*
		double max = qAbs(viewDir.v[dominantFace]);
		if(qAbs(viewDir.v[2])>max)
		{
			secondDominantFace = dominantFace;
			dominantFace = 2;
		}
		else if (qAbs(viewDir.v[2])>qAbs(viewDir.v[secondDominantFace]))
		{
			secondDominantFace = 2;
		}
		*/

		//check sign
		dominantFace = dominantFace*2 + (mainViewDir.v[dominantFace]<0.0);
		secondDominantFace = secondDominantFace*2 + (mainViewDir.v[secondDominantFace]<0.0);
	}
	else
	{
		//remove cubemapping objects when switching to perspective proj to save GPU memory
		deleteCubemapping();
	}

	//turn off blending, because it seems to be enabled somewhere we do not have access
	glDisable(GL_BLEND);

	if (shaderParameters.shadows)
	{
		//test if shadow mapping has been initialized,
		//or needs to be re-initialized because of setting changes
		if(reinitShadowmapping || shadowFBOs.size()==0 || (cubemappingUsedLastFrame != requiresCubemap))
		{
			reinitShadowmapping = false;
			if(!initShadowmapping())
				return; //can't use shadowmaps
		}
	}
	else
	{
		//remove the shadow mapping stuff if not in use, this is only done once
		deleteShadowmapping();
	}

	if (!requiresCubemap)
	{
		//when Stellarium uses perspective projection we can use the fast direct method
		drawDirect();
	}
	else
	{
		//we have to use a workaround using cubemapping
		drawWithCubeMap();
	}
	if (textEnabled) drawCoordinatesText();
	if (debugEnabled)
	{
		drawDebug();
	}

	lastDrawnPosition = currentScene->getEyePosition();
	cubemappingUsedLastFrame = requiresCubemap;
	currentScene = Q_NULLPTR;
}

void S3DRenderer::rendererMessage(const QString &msg) const
{
	emit message(msg);
}
