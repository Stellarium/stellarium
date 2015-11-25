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

#if !defined(Q_OS_WIN)
//exclude StelOpenGL here on windows because of conflicts with GLFuncs.hpp otherwise (uses QOpenGLFunctions_1_0 directly)
#include "StelOpenGL.hpp"
#endif
//needs to be included before StelOpenGL on Windows
#include "GLFuncs.hpp"

#include "Scenery3d.hpp"

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

#include "Scenery3dMgr.hpp"
#include "AABB.hpp"

#include <QKeyEvent>
#include <QSettings>
#include <stdexcept>
#include <cmath>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>

#define GET_GLERROR()                                   \
{                                                       \
    GLenum err = glGetError();                          \
    if (err != GL_NO_ERROR) {                           \
    qWarning("[line %d] GL Error: %d",__LINE__, err);   \
    }                                                   \
}

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

Scenery3d::Scenery3d(Scenery3dMgr* parent)
    : parent(parent), currentScene(), loadingScene(),
      //sun(NULL), moon(NULL), venus(NULL),
      supportsGSCubemapping(false), supportsShadows(false), supportsShadowFiltering(false), isANGLE(false), maximumFramebufferSize(0),
      torchBrightness(0.5f), torchRange(5.0f), textEnabled(false), debugEnabled(false), fixShadowData(false),
      simpleShadows(false), fullCubemapShadows(false), cubemappingMode(S3DEnum::CM_TEXTURES), //set it to 6 textures as a safe default (Cubemap should work on ANGLE, but does not...)
      reinitCubemapping(true), reinitShadowmapping(true),
      loadCancel(false),
      cubemapSize(1024),shadowmapSize(1024),
      absolutePosition(0.0, 0.0, 0.0), moveVector(0.0, 0.0, 0.0), movement(0.0f,0.0f,0.0f), eye_height(0.0f),
      core(NULL), landscapeMgr(NULL),  heightmap(NULL), heightmapLoad(NULL),
      mainViewUp(0.0, 0.0, 1.0), mainViewDir(1.0, 0.0, 0.0), viewPos(0.0, 0.0, 0.0),
      drawnTriangles(0), drawnModels(0), materialSwitches(0), shaderSwitches(0),
      requiresCubemap(false), cubemappingUsedLastFrame(false),
      lazyDrawing(false), updateOnlyDominantOnMoving(true), updateSecondDominantOnMoving(true), needsMovementEndUpdate(false),
      needsCubemapUpdate(true), needsMovementUpdate(false), lazyInterval(2.0), lastCubemapUpdate(0.0), lastCubemapUpdateRealTime(0), lastMovementEndRealTime(0),
      cubeMapCubeTex(0), cubeMapCubeDepth(0), cubeMapTex(), cubeRB(0), dominantFace(0), secondDominantFace(1), cubeFBO(0), cubeSideFBO(), cubeMappingCreated(false),
      cubeVertexBuffer(QOpenGLBuffer::VertexBuffer), cubeIndexBuffer(QOpenGLBuffer::IndexBuffer), cubeIndexCount(0),
      lightOrthoNear(0.1f), lightOrthoFar(1000.0f), parallaxScale(0.015f)
{
	qDebug()<<"Scenery3d constructor...";

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

	sceneBoundingBox = AABB(Vec3f(0.0f), Vec3f(0.0f));

	debugTextFont.setFamily("Courier");
	debugTextFont.setPixelSize(16);


	qDebug()<<"Scenery3d constructor...done";
}

Scenery3d::~Scenery3d()
{
	if (heightmap) {
		delete heightmap;
		heightmap = NULL;
	}

	if(heightmapLoad)
	{
		delete heightmapLoad;
		heightmapLoad = NULL;
	}

	cubeVertexBuffer.destroy();
	cubeIndexBuffer.destroy();

	deleteShadowmapping();
	deleteCubemapping();

#ifndef QT_OPENGL_ES_2
	//delete extension functions
	delete glExtFuncs;
#endif
}

bool Scenery3d::loadScene(const SceneInfo &scene)
{
	loadingScene = scene;

	if(loadCancel)
		return false;

	//setup some state
	QMatrix4x4 zRot2Grid = (loadingScene.zRotateMatrix*loadingScene.obj2gridMatrix).convertToQMatrix();

	OBJ::vertexOrder objVertexOrder=OBJ::XYZ;
	if (loadingScene.vertexOrder.compare("XZY") == 0) objVertexOrder=OBJ::XZY;
	else if (loadingScene.vertexOrder.compare("YXZ") == 0) objVertexOrder=OBJ::YXZ;
	else if (loadingScene.vertexOrder.compare("YZX") == 0) objVertexOrder=OBJ::YZX;
	else if (loadingScene.vertexOrder.compare("ZXY") == 0) objVertexOrder=OBJ::ZXY;
	else if (loadingScene.vertexOrder.compare("ZYX") == 0) objVertexOrder=OBJ::ZYX;

	parent->updateProgress(q_("Loading model..."),1,0,6);

	//load model
	objModelLoad.reset(new OBJ());
	QString modelFile = StelFileMgr::findFile( loadingScene.fullPath+ "/" + loadingScene.modelScenery);
	qDebug()<<"[Scenery3d] Loading scene from "<<modelFile;
	if(!objModelLoad->load(modelFile, objVertexOrder, loadingScene.sceneryGenerateNormals))
	{
	    qCritical()<<"[Scenery3d] Failed to load OBJ file.";
	    return false;
	}

	if(loadCancel)
		return false;

	parent->updateProgress(q_("Transforming model..."),2,0,6);

	//transform the vertices of the model to match the grid
	objModelLoad->transform( zRot2Grid );

	if(loadCancel)
		return false;

	if(loadingScene.modelGround.isEmpty())
		groundModelLoad = objModelLoad;
	else if (loadingScene.modelGround != "NULL")
	{
		parent->updateProgress(q_("Loading ground..."),3,0,6);

		groundModelLoad.reset(new OBJ());
		modelFile = StelFileMgr::findFile(loadingScene.fullPath + "/" + loadingScene.modelGround);
		qDebug()<<"[Scenery3d] Loading ground from"<<modelFile;
		if(!groundModelLoad->load(modelFile, objVertexOrder, loadingScene.groundGenerateNormals))
		{
			qCritical()<<"[Scenery3d] Failed to load OBJ file.";
			return false;
		}

		parent->updateProgress(q_("Transforming ground..."),4,0,6);
		if(loadCancel)
			return false;

		groundModelLoad->transform( zRot2Grid );
	}

	if(loadCancel)
		return false;

	if(loadingScene.hasLocation())
	{
		if(loadingScene.altitudeFromModel)
		{
			loadingScene.location->altitude=static_cast<int>(0.5*(objModelLoad->getBoundingBox().min[2]+objModelLoad->getBoundingBox().max[2])+loadingScene.modelWorldOffset[2]);
		}
	}

	if(scene.groundNullHeightFromModel)
	{
		loadingScene.groundNullHeight = ((!groundModelLoad.isNull() && groundModelLoad->isLoaded()) ? groundModelLoad->getBoundingBox().min[2] : objModelLoad->getBoundingBox().min[2]);
		qDebug() << "[Scenery3d] Ground outside model is " << loadingScene.groundNullHeight  << "m high (in model coordinates)";
	}
	else qDebug() << "[Scenery3d] Ground outside model stays " << loadingScene.groundNullHeight  << "m high (in model coordinates)";

	//calculate heightmap
	if(loadCancel)
		return false;
	parent->updateProgress(q_("Calculating collision map..."),5,0,6);

	if(heightmapLoad)
	{
		delete heightmapLoad;
	}

	if( !groundModelLoad.isNull() && groundModelLoad->isLoaded())
	{
		heightmapLoad = new Heightmap(groundModelLoad.data());
		heightmapLoad->setNullHeight(loadingScene.groundNullHeight);
	}
	else
		heightmapLoad = NULL;

	parent->updateProgress(q_("Finalizing load..."),6,0,6);

	return true;
}

void Scenery3d::finalizeLoad()
{
	currentScene = loadingScene;

	//move load data to current one
	objModel = objModelLoad;
	objModelLoad.clear();
	groundModel = groundModelLoad;
	groundModelLoad.clear();

	//upload GL
	objModel->uploadBuffersGL();
	objModel->uploadTexturesGL();
	//call this after texture load
	objModel->finalizeForRendering();

	//the ground model needs no opengl uploads, so we skip them

	//delete old heightmap
	if(heightmap)
	{
		delete heightmap;
	}

	heightmap = heightmapLoad;
	heightmapLoad = NULL;

	if(currentScene.startPositionFromModel)
	{
		absolutePosition.v[0] = -(objModel->getBoundingBox().max[0]+objModel->getBoundingBox().min[0])/2.0;
		qDebug() << "Setting Easting  to BBX center: " << objModel->getBoundingBox().min[0] << ".." << objModel->getBoundingBox().max[0] << ": " << absolutePosition.v[0];
		absolutePosition.v[1] = -(objModel->getBoundingBox().max[1]+objModel->getBoundingBox().min[1])/2.0;
		qDebug() << "Setting Northing to BBX center: " << objModel->getBoundingBox().min[1] << ".." << objModel->getBoundingBox().max[1] << ": " << -absolutePosition.v[1];
	}
	else
	{
		absolutePosition[0] = currentScene.relativeStartPosition[0];
		absolutePosition[1] = currentScene.relativeStartPosition[1];
	}
	eye_height = currentScene.eyeLevel;

	//TODO: maybe introduce a switch in scenery3d.ini that allows the "ground" bounding box to be used for shadow calculations
	//this would allow some scenes to have better shadows
	OBJ* cur = objModel.data();
	//Set the scene's AABB
	setSceneAABB(cur->getBoundingBox());

	//Find a good splitweight based on the scene's size
	float maxSize = -std::numeric_limits<float>::max();
	maxSize = std::max(sceneBoundingBox.max.v[0], maxSize);
	maxSize = std::max(sceneBoundingBox.max.v[1], maxSize);


	if(currentScene.shadowSplitWeight<0)
	{
		//qDebug() << "MAXSIZE:" << maxSize;
		if(maxSize < 100.0f)
			currentScene.shadowSplitWeight = 0.5f;
		else if(maxSize < 200.0f)
			currentScene.shadowSplitWeight = 0.60f;
		else if(maxSize < 400.0f)
			currentScene.shadowSplitWeight = 0.70f;
		else
			currentScene.shadowSplitWeight = 0.99f;
	}

	//reset the cubemap time so that is ensured it is immediately rerendered
	invalidateCubemap();
}

void Scenery3d::handleKeys(QKeyEvent* e)
{
	//TODO FS maybe move this to Mgr, so that input is separate from rendering and scene management?

	if ((e->type() == QKeyEvent::KeyPress) && (e->modifiers() & Qt::ControlModifier))
	{
		// Pressing CTRL+ALT: 5x, CTRL+SHIFT: 10x speedup; CTRL+SHIFT+ALT: 50x!
		float speedup=((e->modifiers() & Qt::ShiftModifier)? 10.0f : 1.0f);
		speedup *= ((e->modifiers() & Qt::AltModifier)? 5.0f : 1.0f);
		switch (e->key())
		{
			case Qt::Key_PageUp:    movement[2] = -1.0f * speedup; e->accept(); break;
			case Qt::Key_PageDown:  movement[2] =  1.0f * speedup; e->accept(); break;
			case Qt::Key_Up:        movement[1] = -1.0f * speedup; e->accept(); break;
			case Qt::Key_Down:      movement[1] =  1.0f * speedup; e->accept(); break;
			case Qt::Key_Right:     movement[0] =  1.0f * speedup; e->accept(); break;
			case Qt::Key_Left:      movement[0] = -1.0f * speedup; e->accept(); break;
#ifdef QT_DEBUG
				//leave this out on non-debug builds to reduce conflict chance
			case Qt::Key_P:         saveFrusts(); e->accept(); break;
#endif
		}
	}
	else if ((e->type() == QKeyEvent::KeyRelease) && (e->modifiers() & Qt::ControlModifier))
	{
		if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown ||
				e->key() == Qt::Key_Up     || e->key() == Qt::Key_Down     ||
				e->key() == Qt::Key_Left   || e->key() == Qt::Key_Right     )
		{
			movement[0] = movement[1] = movement[2] = 0.0f;
			e->accept();
		}
	}
}

void Scenery3d::saveFrusts()
{
	fixShadowData = !fixShadowData;

	camFrustShadow.saveDrawingCorners();

	for(int i=0; i<shaderParameters.frustumSplits; i++)
	{
		if(fixShadowData) frustumArray[i].saveDrawingCorners();
		else frustumArray[i].resetCorners();
	}
}

void Scenery3d::setSceneAABB(const AABB& bbox)
{
	sceneBoundingBox = bbox;
}

void Scenery3d::update(double deltaTime)
{
	if (core != NULL)
	{
		StelMovementMgr *stelMovementMgr = GETSTELMODULE(StelMovementMgr);

		Vec3d viewDirection = core->getMovementMgr()->getViewDirectionJ2000();
		Vec3d viewDirectionAltAz=core->j2000ToAltAz(viewDirection);
		double alt, az;
		StelUtils::rectToSphe(&az, &alt, viewDirectionAltAz);

		//if we were moving in the last update
		bool wasMoving = moveVector.lengthSquared()>0.0;

		moveVector = Vec3d(( movement[0] * std::cos(az) + movement[1] * std::sin(az)),
				( movement[0] * std::sin(az) - movement[1] * std::cos(az)),
				movement[2]);

		//get current time
		double curTime = core->getJD();

		if(lazyDrawing)
		{
			needsMovementUpdate = false;

			//check if cubemap requires redraw
			if(qAbs(curTime-lastCubemapUpdate) > lazyInterval * StelCore::JD_SECOND || reinitCubemapping)
			{
				needsCubemapUpdate = true;
				needsMovementEndUpdate = false;
			}
			else if (moveVector.lengthSquared() > 0.0 )
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
				if(wasMoving)
					lastMovementEndRealTime = QDateTime::currentMSecsSinceEpoch();

				if(needsMovementEndUpdate && (QDateTime::currentMSecsSinceEpoch() - lastMovementEndRealTime)  > 700)
				{
					//if the last movement was some time ago, update the whole cubemap
					needsCubemapUpdate = true;
					needsMovementEndUpdate = false;
				}
				else
					needsCubemapUpdate = false;
			}
		}
		else
		{
			needsCubemapUpdate = true;
		}

		moveVector *= deltaTime * 0.01 * qMax(5.0, stelMovementMgr->getCurrentFov());

		//Bring move into world-grid space
		currentScene.zRotateMatrix.transfo(moveVector);

		absolutePosition.v[0] += moveVector.v[0];
		absolutePosition.v[1] += moveVector.v[1];
		eye_height -= moveVector.v[2];
		absolutePosition.v[2] = -groundHeight()-eye_height;


		//View Up in our case always pointing positive up
		mainViewUp.v[0] = 0;
		mainViewUp.v[1] = 0;
		mainViewUp.v[2] = 1;


		viewPos = -absolutePosition;

		//View Direction
		mainViewDir = core->getMovementMgr()->getViewDirectionJ2000();
		mainViewDir = core->j2000ToAltAz(mainViewDir);

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
}

float Scenery3d::groundHeight()
{
	if (heightmap == NULL) {
		return currentScene.groundNullHeight;
	} else {
		return heightmap->getHeight(-absolutePosition.v[0],-absolutePosition.v[1]);
	}
}

void Scenery3d::setupPassUniforms(QOpenGLShaderProgram *shader)
{
	//send projection matrix
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MAT_PROJECTION, projectionMatrix);

	//set alpha test threshold (this is scene-global for now)
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_FLOAT_ALPHA_THRESH,currentScene.transparencyThreshold);

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
			glActiveTexture(GL_TEXTURE4+i);
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
			shader->setUniformValueArray(loc,shadowFrustumSize.constData(),4);
		}
	}

	loc = shaderManager.uniformLocation(shader,ShaderMgr::UNIFORM_MAT_CUBEMVP);
	if(loc>=0)
	{
		//upload cube mvp matrices
		shader->setUniformValueArray(loc,cubeMVP,6);
	}
}

void Scenery3d::setupFrameUniforms(QOpenGLShaderProgram *shader)
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

void Scenery3d::setupMaterialUniforms(QOpenGLShaderProgram* shader, const OBJ::Material &mat)
{
	//ambient is calculated depending on illum model
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_AMBIENT,mat.ambient * lightInfo.ambient);

	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_DIFFUSE, mat.diffuse * lightInfo.directional);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_TORCHDIFFUSE, mat.diffuse * lightInfo.torchDiffuse);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_EMISSIVE,mat.emission * lightInfo.emissive);
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MIX_SPECULAR,mat.specular * lightInfo.specular);

	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MTL_SHININESS,mat.shininess);
	//force alpha to 1 here for non-translucent mats (fixes incorrect blending in cubemap)
	SET_UNIFORM(shader,ShaderMgr::UNIFORM_MTL_ALPHA,mat.hasTransparency ? mat.alpha : 1.0f);

	if(mat.texture)
	{
		mat.texture->bind(0); //this already sets glActiveTexture(0)
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_DIFFUSE,0);
	}
	if(mat.emissive_texture)
	{
		mat.emissive_texture->bind(1);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_EMISSIVE,1);
	}
	if(shaderParameters.bump && mat.bump_texture)
	{
		mat.bump_texture->bind(2);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_BUMP,2);
	}
	if(shaderParameters.bump && mat.height_texture)
	{
		mat.height_texture->bind(3);
		SET_UNIFORM(shader,ShaderMgr::UNIFORM_TEX_HEIGHT,3);
	}
}

bool Scenery3d::drawArrays(bool shading, bool blendAlphaAdditive)
{
	QOpenGLShaderProgram* curShader = NULL;
	QSet<QOpenGLShaderProgram*> initialized;
	GLenum indexDataType = OBJ::getIndexBufferType();
	size_t indexDataTypeSize = OBJ::getIndexBufferTypeSize();

	//override some shader Params
	GlobalShaderParameters pm = shaderParameters;
	switch(lightInfo.lightSource)
	{
		case Venus:
			//turn shadow filter off to get sharper shadows
			pm.shadowFilterQuality = S3DEnum::SFQ_OFF;
			break;
		case None:
			//disable shadow rendering to speed things up
			pm.shadows = false;
			break;
		default:
			break;
	}

	//bind VAO
	objModel->bindGL();

	//assume backfaceculling is on
	bool backfaceCullState = true;
	bool success = true;

	//TODO optimize: clump models with same material together when first loading to minimize state changes
	const OBJ::Material* lastMaterial = NULL;
	bool blendEnabled = false;
	for(int i=0; i<objModel->getNumberOfStelModels(); i++)
	{

		const OBJ::StelModel* pStelModel = &objModel->getStelModel(i);
		const OBJ::Material* pMaterial = pStelModel->pMaterial;
		Q_ASSERT(pMaterial);

		++drawnModels;

		if(lastMaterial!=pMaterial)
		{
			++materialSwitches;
			lastMaterial = pMaterial;

			//get a shader from shadermgr that fits the current state + material combo
			QOpenGLShaderProgram* newShader = shaderManager.getShader(pm,pMaterial);
			if(!newShader)
			{
				//shader invalid, can't draw
				parent->showMessage(q_("Scenery3d shader error, can't draw. Check debug output for details."));
				success = false;
				break;
			}
			if(newShader!=curShader)
			{
				curShader = newShader;
				curShader->bind();
				if(!initialized.contains(curShader))
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
						SET_UNIFORM(curShader,ShaderMgr::UNIFORM_FLOAT_ALPHA_THRESH,currentScene.transparencyThreshold);
					}

					//we remember if we have initialized this shader already, so we can skip "global" initialization later if we encounter it again
					initialized.insert(curShader);
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
				if( ! pMaterial->texture.isNull())
				{
					pMaterial->texture->bind(0);
					SET_UNIFORM(curShader,ShaderMgr::UNIFORM_TEX_DIFFUSE,0);
				}
			}

			if(pMaterial->hasTransparency )
			{
				//TODO provide Z-sorting for transparent objects (center of bounding box should be fine)
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
			else
			{
				if(blendEnabled)
				{
					glDisable(GL_BLEND);
					blendEnabled=false;
				}
			}

			if(backfaceCullState && !pMaterial->backfacecull)
			{
				glDisable(GL_CULL_FACE);
				backfaceCullState = false;
			}
			else if(!backfaceCullState && pMaterial->backfacecull)
			{
				glEnable(GL_CULL_FACE);
				backfaceCullState = true;
			}
		}


		glDrawElements(GL_TRIANGLES, pStelModel->triangleCount * 3, indexDataType, reinterpret_cast<const void*>(pStelModel->startIndex * indexDataTypeSize));
		drawnTriangles+=pStelModel->triangleCount;
	}

	if(!backfaceCullState)
		glEnable(GL_CULL_FACE);

	if(curShader)
		curShader->release();

	if(blendEnabled)
		glDisable(GL_BLEND);

	//release VAO
	objModel->unbindGL();

	return success;
}

void Scenery3d::computeFrustumSplits(const Vec3d viewPos, const Vec3d viewDir, const Vec3d viewUp)
{
	//the frustum arrays all already contain the same adjusted frustum from adjustFrustum
	float zNear = frustumArray[0].zNear;
	float zFar = frustumArray[0].zFar;
	float zRatio = zFar / zNear;
	float zRange = zFar - zNear;

	//Compute the z-planes for the subfrusta
	for(int i=1; i<shaderParameters.frustumSplits; i++)
	{
		float s_i = i/static_cast<float>(shaderParameters.frustumSplits);

		frustumArray[i].zNear = currentScene.shadowSplitWeight*(zNear*powf(zRatio, s_i)) + (1.0f-currentScene.shadowSplitWeight)*(zNear + (zRange)*s_i);
		//Set the previous zFar to the newly computed zNear
		//use a small overlap for robustness
		frustumArray[i-1].zFar = frustumArray[i].zNear * 1.005f;

		frustumArray[i-1].calcFrustum(viewPos,viewDir,viewUp);
	}

	//last zFar is already the zFar of the adjusted frustum
	frustumArray[shaderParameters.frustumSplits-1].calcFrustum(viewPos,viewDir,viewUp);
}

void Scenery3d::computePolyhedron(Polyhedron& body,const Frustum& frustum,const Vec3f& shadowDir)
{
	//Building a convex body for directional lights according to Wimmer et al. 2006


	//Add the Frustum to begin with
	body.add(frustum);
	//Intersect with the scene AABB
	body.intersect(sceneBoundingBox);
	//Extrude towards light direction
	body.extrude(shadowDir, sceneBoundingBox);
}

void Scenery3d::computeOrthoProjVals(const Vec3f shadowDir,float& orthoExtent,float& orthoNear,float& orthoFar)
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

	for(unsigned int i=0; i<AABB::CORNERCOUNT; i++)
	{
		Vec3f v = sceneBoundingBox.getCorner(static_cast<AABB::Corner>(i));
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

void Scenery3d::computeCropMatrix(QMatrix4x4& cropMatrix, QVector4D& orthoScale, Polyhedron& focusBody,const QMatrix4x4& lightProj, const QMatrix4x4& lightMVP)
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
	AABB deb(Vec3f(minX,minY,minZ),Vec3f(maxX,maxY,maxZ));
	focusBody.debugBox = deb.toBox();
	focusBody.debugBox.transform(lightMVP.inverted());
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

void Scenery3d::adjustShadowFrustum(const Vec3d viewPos, const Vec3d viewDir, const Vec3d viewUp, const float fov, const float aspect)
{
	if(fixShadowData)
		return;
	//calc cam frustum for shadowing range
	//note that this is only correct in the perspective projection case, cubemapping WILL introduce shadow artifacts in most cases

	//TODO make shadows in cubemapping mode better by projecting the frusta, more closely estimating the required shadow extents
	camFrustShadow.setCamInternals(fov,aspect,currentScene.camNearZ,currentScene.shadowFarZ);
	camFrustShadow.calcFrustum(viewPos, viewDir, viewUp);

	//Compute H = V intersect S according to Zhang et al.
	Polyhedron p;
	p.add(camFrustShadow);
	p.intersect(sceneBoundingBox);
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

void Scenery3d::calculateShadowCaster()
{
	//shadow source and direction has been calculated in calculateLightSource
	static const QVector3D vZero = QVector3D();
	static const QVector3D vZeroZeroOne = QVector3D(0,0,1);

	//calculate lights modelview matrix
	lightInfo.shadowModelView.setToIdentity();
	lightInfo.shadowModelView.lookAt(lightInfo.lightDirectionWorld,vZero,vZeroZeroOne);
}

bool Scenery3d::renderShadowMaps()
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

		if(lightInfo.lightSource != None && focusBodies[i].getVertCount())
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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

void Scenery3d::calculateLighting()
{
	//calculate which light source we need + intensity
	float ambientBrightness, directionalBrightness,emissiveFactor;
	lightInfo.lightSource = calculateLightSource(ambientBrightness, directionalBrightness, lightInfo.lightDirectionV3f, emissiveFactor);

	lightInfo.lightDirectionWorld = QVector3D(lightInfo.lightDirectionV3f.v[0],lightInfo.lightDirectionV3f.v[1],lightInfo.lightDirectionV3f.v[2]);

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

Scenery3d::ShadowCaster  Scenery3d::calculateLightSource(float &ambientBrightness, float &directionalBrightness, Vec3f &lightsourcePosition, float &emissiveFactor)
{
	Vec3d sunPosition = sun->getAltAzPosAuto(core);
	sunPosition.normalize();
	Vec3d moonPosition = moon->getAltAzPosAuto(core);
	float moonPhaseAngle = moon->getPhase(core->getObserverHeliocentricEclipticPos());
	moonPosition.normalize();
	Vec3d venusPosition = venus->getAltAzPosAuto(core);
	float venusPhaseAngle = venus->getPhase(core->getObserverHeliocentricEclipticPos());
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

	float sinSunAngle  = sunPosition[2];
	float sinMoonAngle = moonPosition[2];
	float sinVenusAngle = venusPosition[2];

	//set the minimum ambient brightness
	//this uses the LandscapeMgr values
	Landscape* l = landscapeMgr->getCurrentLandscape();
	if (landscapeMgr->getFlagLandscapeUseMinimalBrightness())
	{
		// Setting from landscape.ini has priority if enabled
		if (landscapeMgr->getFlagLandscapeSetsMinimalBrightness() && l && l->getLandscapeMinimalBrightness()>=0)
			ambientBrightness = l->getLandscapeMinimalBrightness();
		else
			ambientBrightness = landscapeMgr->getDefaultMinimalBrightness();
	}
	else
	{
		ambientBrightness = 0.0f;
	}

	directionalBrightness=0.0f;
	ShadowCaster shadowcaster = None;
	// DEBUG AIDS: Helper strings to be displayed
	//TODO move these string manipulations to drawDebug, it is a bit dumb to do this every frame, even if not needed
	QString sunAmbientString;
	QString moonAmbientString;
	QString backgroundAmbientString=QString("%1").arg(ambientBrightness, 6, 'f', 4);
	QString directionalSourceString;

	//assume light=sun for a start.
	Vec3d lightPosition = sunPosition;
	directionalSourceString="(Sun, below horiz.)";

	//calculate emissive factor
	if(l!=NULL)
	{
		if(requiresCubemap && lazyDrawing)
		{
			emissiveFactor = l->getTargetLightscapeBrightness();
		}
		else
		{
			//use an interpolated value for smooth fade in/out
			emissiveFactor = l->getEffectiveLightscapeBrightness();
		}
	}
	else
	{
		// I don't know if this can ever happen, but in this case,
		// directly use the same model as LandscapeMgr::update uses for the lightscapeBrightness
		emissiveFactor = 0.0f;
		if (sunPosition[2]<-0.14f) emissiveFactor=1.0f;
		else if (sunPosition[2]<-0.05f) emissiveFactor = 1.0f-(sunPosition[2]+0.14)/(-0.05+0.14);
	}

	// calculate ambient light
	if(sinSunAngle > -0.3f) // sun above -18 deg?
	{
		ambientBrightness += qMin(0.3f, sinSunAngle+0.3f);
		sunAmbientString=QString("%1").arg(qMin(0.3f, sinSunAngle+0.3f), 6, 'f', 4);
	}
	else
		sunAmbientString=QString("0.0");

	if ((sinMoonAngle>0.0f) && (sinSunAngle<0.0f))
	{
		ambientBrightness += sqrt(sinMoonAngle * ((std::cos(moonPhaseAngle)+1)/2)) * LUNAR_BRIGHTNESS_FACTOR;
		moonAmbientString=QString("%1").arg(sqrt(sinMoonAngle * ((std::cos(moonPhaseAngle)+1)/2)) * LUNAR_BRIGHTNESS_FACTOR);
	}
	else
		moonAmbientString=QString("0.0");

	// Now find shadow caster + directional light, if any:
	if (sinSunAngle>-0.1f)
	{
		directionalBrightness=qMin(0.7f, std::sqrt(sinSunAngle+0.1f)); // limit to 0.7 in order to keep total below 1.
		//redundant
		//lightPosition = sunPosition;
		if (shaderParameters.shadows) shadowcaster = Sun;
		directionalSourceString="Sun";
	}
	/*   else if (sinSunAngle> -0.3f) // sun above -18: create shadowless directional pseudo-light from solar azimuth
    {
	directionalBrightness=qMin(0.7, sinSunAngle+0.3); // limit to 0.7 in order to keep total below 1.
	lightsourcePosition.set(sunPosition.v[0], sunPosition.v[1], sinSunAngle+0.3);
	directionalSourceString="(Sun, below hor.)";
    }*/
	// "else" is required now, else we have lunar shadow with sun above horizon...
	else if (sinMoonAngle>0.0f)
	{
		float moonBrightness = std::sqrt(sinMoonAngle) * ((std::cos(moonPhaseAngle)+1.0f)/2.0f) * LUNAR_BRIGHTNESS_FACTOR;
		moonBrightness -= (ambientBrightness-0.05f)/2.0f;
		moonBrightness = qMax(0.0f,moonBrightness);
		if(sinSunAngle<0.0f && sinSunAngle >-0.1f)
		{
			//interpolate directional brightness between sun and moon
			float t = sinSunAngle/-0.1f;
			directionalBrightness = (1.0f-t) * directionalBrightness + t*moonBrightness;
			/*
		    //uncomment to also move the light direction linearly to avoid possible jarring transitions
		    //but that does not seem to have much of an effect
		    if(moonBrightness>0)
		    {
			    lightPosition = (1.0-t) * sunPosition + (double)t * moonPosition;
			    lightPosition.normalize();
		    }
		    */
		}
		else if (moonBrightness >0)
		{
			directionalBrightness = moonBrightness;
			lightPosition = moonPosition;
			if (shaderParameters.shadows) shadowcaster = Moon;
			directionalSourceString="Moon";
		} else directionalSourceString="Moon";
		//Alternately, construct a term around lunar brightness, like
		// directionalBrightness=(mag/-10)
	}
	else if (sinVenusAngle>0.0f)
	{
		float venusBrightness = std::sqrt(sinVenusAngle)*((std::cos(venusPhaseAngle)+1)/2) * VENUS_BRIGHTNESS_FACTOR;
		venusBrightness -= (ambientBrightness-0.05)/2.0f;
		venusBrightness = qMax(0.0f, venusBrightness);

		if(sinSunAngle<0.0f && sinSunAngle >-0.1f)
		{
			//interpolate directional brightness between sun and venus
			float t = sinSunAngle/-0.1f;
			directionalBrightness = (1.0f-t) * directionalBrightness + t*venusBrightness;
			/*
		    //uncomment to also move the light direction linearly to avoid possible jarring transitions
		    //but that does not seem to have much of an effect
		    if(venusBrightness>0)
		    {
			    lightPosition = (1.0-t) * sunPosition + (double)t * venusPosition;
			    lightPosition.normalize();
		    }
		    */
		}
		else if (venusBrightness > 0.0f)
		{
			directionalBrightness = venusBrightness;
			lightPosition = venusPosition;
			if (shaderParameters.shadows) shadowcaster = Venus;
			directionalSourceString="Venus";
		} else directionalSourceString="(Venus, flooded by ambient)";
		//Alternately, construct a term around Venus brightness, like
		// directionalBrightness=(mag/-100)
	}
	else if(sinSunAngle<0.0f && sinSunAngle >-0.1f)
	{
		//let sunlight fall off to zero
		float t = sinSunAngle/-0.1f;
		directionalBrightness = (1.0f - t) * directionalBrightness;
	}

	//convert to float
	lightsourcePosition.set(lightPosition.v[0], lightPosition.v[1], lightPosition.v[2]);

	float landscapeOpacity = 0.0f;

	//check landscape occlusion, modify directional if needed
	if(directionalBrightness>0)
	{
		if(l)
		{
			//TODO the changes are currently rather harsh, find a better method (like angular distance of light source to horizon, or bitmap interpolation for the alpha values)
			landscapeOpacity = l->getOpacity(lightPosition);

			//lerp between the determined opacity and 1.0, depending on landscape fade (visibility)
			float fadeValue = 1.0f + l->getEffectiveLandFadeValue() * (-landscapeOpacity);
			directionalBrightness *= fadeValue;
		}
	}

	//TODO remove the string stuff from this method...
	// DEBUG: Prepare output message
	QString shadowCasterName;
	switch (shadowcaster) {
		case None:  shadowCasterName="None";  break;
		case Sun:   shadowCasterName="Sun";   break;
		case Moon:  shadowCasterName="Moon";  break;
		case Venus: shadowCasterName="Venus"; break;
		default: shadowCasterName="Error!!!";
	}
	lightMessage=QString("Ambient: %1 Directional: %2. Shadows cast by: %3 from %4/%5/%6")
			.arg(ambientBrightness, 6, 'f', 4).arg(directionalBrightness, 6, 'f', 4)
			.arg(shadowCasterName).arg(lightsourcePosition.v[0], 6, 'f', 4)
			.arg(lightsourcePosition.v[1], 6, 'f', 4).arg(lightsourcePosition.v[2], 6, 'f', 4);
	lightMessage2=QString("Contributions: Ambient     Sun: %1, Moon: %2, Background+^L: %3").arg(sunAmbientString).arg(moonAmbientString).arg(backgroundAmbientString);
	lightMessage3=QString("               Directional %1 by: %2, emissive factor: %3, landscape opacity: %4").arg(directionalBrightness, 6, 'f', 4).arg(directionalSourceString).arg(emissiveFactor).arg(landscapeOpacity);

	return shadowcaster;
}

void Scenery3d::calcCubeMVP()
{
	QMatrix4x4 tmp;
	for(int i = 0;i<6;++i)
	{
		tmp = cubeRotation[i];
		tmp.translate(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
		cubeMVP[i] = projectionMatrix * tmp;
	}
}

void Scenery3d::renderIntoCubemapGeometryShader()
{
	//single FBO
	glBindFramebuffer(GL_FRAMEBUFFER,cubeFBO);

	//Hack: because the modelviewmatrix is used for lighting in shader, but we dont want to perform MV transformations 6 times,
	// we just set the position because that currently is all that is needeed for correct lighting
	modelViewMatrix.setToIdentity();
	modelViewMatrix.translate(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render all 6 faces at once
	shaderParameters.geometryShader = true;
	//calculate the final required matrices for each face
	calcCubeMVP();
	drawArrays(true,true);
	shaderParameters.geometryShader = false;
}

void Scenery3d::renderShadowMapsForFace(int face)
{
	//extract view dir from the MV matrix
	QVector4D viewDir = -cubeRotation[face].row(2);

	//somewhere, there are problems when the view direction points exactly up or down, causing missing shadows
	//this is NOT fixed by choosing a different up vector here as could be expected
	//the problem seems to occur during final rendering because shadowmap textures look alright and the scaling values seem valid
	//for now, fix this by adding a tiny value to X in these cases
	adjustShadowFrustum(viewPos,Vec3d(face>3?viewDir[0]+0.000001:viewDir[0],viewDir[1],viewDir[2]),Vec3d(0,0,1),90.0f,1.0f);
	//render shadowmap
	if(!renderShadowMaps())
		return;

	//gl state + viewport must be reset
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glViewport(0, 0, cubemapSize, cubemapSize);
}

void Scenery3d::renderIntoCubemapSixPasses()
{
	//store current projection (= 90° cube projection)
	QMatrix4x4 squareProjection = projectionMatrix;

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
		modelViewMatrix.translate(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
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
			modelViewMatrix.translate(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
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
			modelViewMatrix.translate(absolutePosition.v[0], absolutePosition.v[1], absolutePosition.v[2]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			drawArrays(true,true);
		}
	}
}

void Scenery3d::generateCubeMap()
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
			float aspect = (float)altAzProjector->getViewportWidth() / (float)altAzProjector->getViewportHeight();

			adjustShadowFrustum(viewPos,mainViewDir,mainViewUp,fov,aspect);
			if(!renderShadowMaps())
				return; //shadow map rendering failed, do an early abort
		}
	}

	//setup projection matrix - this is a 90-degree perspective with aspect 1.0
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(90.0f,1.0f,currentScene.camNearZ,currentScene.camFarZ);

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
	glBindFramebuffer(GL_FRAMEBUFFER,0);

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

void Scenery3d::drawFromCubeMap()
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
		cubeShader->setAttributeBuffer(ShaderMgr::ATTLOC_TEXCOORD,GL_FLOAT,0,3);
	else // 2D tex coords are stored in the same buffer, but with an offset
		cubeShader->setAttributeBuffer(ShaderMgr::ATTLOC_TEXCOORD,GL_FLOAT,cubeVertices.size() * sizeof(Vec3f),2);
	cubeVertexBuffer.release();
	cubeShader->enableAttributeArray(ShaderMgr::ATTLOC_TEXCOORD);
	cubeShader->setAttributeArray(ShaderMgr::ATTLOC_VERTEX, reinterpret_cast<const GLfloat*>(transformedCubeVertices.constData()),3);
	cubeShader->enableAttributeArray(ShaderMgr::ATTLOC_VERTEX);

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
		glDrawElements(GL_TRIANGLES,cubeIndexCount,GL_UNSIGNED_SHORT, NULL);
	}
	else
	{
		//use 6 drawcalls
		int faceIndexCount = cubeIndexCount / 6;
		for(int i =0;i<6;++i)
		{
			glBindTexture(GL_TEXTURE_2D, cubeMapTex[i]);
			glDrawElements(GL_TRIANGLES,faceIndexCount, GL_UNSIGNED_SHORT, (const GLvoid*)(i * faceIndexCount * sizeof(short)));
		}
	}
	cubeIndexBuffer.release();

	cubeShader->disableAttributeArray(ShaderMgr::ATTLOC_TEXCOORD);
	cubeShader->disableAttributeArray(ShaderMgr::ATTLOC_VERTEX);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	cubeShader->release();
}

void Scenery3d::drawDirect() // for Perspective Projection only!
{
    //calculate standard perspective projection matrix, use QMatrix4x4 for that
    float fov = altAzProjector->getFov();
    float aspect = (float)altAzProjector->getViewportWidth() / (float)altAzProjector->getViewportHeight();

    //calc modelview transform
    QMatrix4x4 mvMatrix = altAzProjector->getModelViewTransform()->getApproximateLinearTransfo().convertToQMatrix();
    mvMatrix.optimize(); //may make inversion faster?

    //recalculate lighting info
    calculateLighting();

    //do shadow pass
    //only calculate shadows if enabled
    if(shaderParameters.shadows)
    {
	    calculateShadowCaster();

	    //no need to extract view information, use the direction from stellarium
	    adjustShadowFrustum(viewPos,mainViewDir,mainViewUp,fov,aspect);

	    //this call modifies projection + mv matrices, so we have to set them afterwards
	    if(!renderShadowMaps())
		    return; //shadow map rendering failed, do an early abort
    }

    mvMatrix.translate(absolutePosition.v[0],absolutePosition.v[1],absolutePosition.v[2]);

    //set final rendering matrices
    modelViewMatrix = mvMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(fov,aspect,currentScene.camNearZ,currentScene.camFarZ);

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

void Scenery3d::drawWithCubeMap()
{
	if(needsCubemapUpdate || needsMovementUpdate)
	{
		//lazy redrawing: update cubemap in slower intervals
		generateCubeMap();
	}
	drawFromCubeMap();
}

Vec3d Scenery3d::getCurrentGridPosition() const
{
	// this is the observer position (camera eye position) in model-grid coordinates, relative to the origin
	Vec3d pos=currentScene.zRotateMatrix.inverse()* (-absolutePosition);
	// this is the observer position (camera eye position) in grid coordinates, e.g. Gauss-Krueger or UTM.
	pos+= currentScene.modelWorldOffset;

	//subtract the eye_height to get the foot position
	pos[2]-=eye_height;
	return pos;
}

void Scenery3d::setGridPosition(Vec3d pos)
{
	//this is basically the same as getCurrentGridPosition(), but in reverse
	pos[2]+=eye_height;
	pos-=currentScene.modelWorldOffset;

	//calc opengl position
	absolutePosition = - (currentScene.zRotateMatrix * pos);

	//reset cube map time
	invalidateCubemap();
}

void Scenery3d::drawCoordinatesText()
{
    StelPainter painter(altAzProjector);
    painter.setFont(debugTextFont);
    painter.setColor(1.0f,0.0f,1.0f);
    float screen_x = altAzProjector->getViewportWidth()  - 240.0f;
    float screen_y = altAzProjector->getViewportHeight() -  60.0f;
    QString str;

    Vec3d gridPos = getCurrentGridPosition();

    // problem: long grid names!
    painter.drawText(altAzProjector->getViewportWidth()-10-qMax(240, painter.getFontMetrics().boundingRect(currentScene.gridName).width()),
		     screen_y, currentScene.gridName);
    screen_y -= 17.0f;
    str = QString("East:   %1m").arg(gridPos[0], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("North:  %1m").arg(gridPos[1], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("Height: %1m").arg(gridPos[2], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("Eye:    %1m").arg(eye_height, 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);

    /*// DEBUG AIDS:
    screen_y -= 15.0f;
    str = QString("model_X:%1m").arg(model_pos[0], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("model_Y:%1m").arg(model_pos[1], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("model_Z:%1m").arg(model_pos[2], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("abs_X:  %1m").arg(absolutePosition.v[0], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("abs_Y:  %1m").arg(absolutePosition.v[1], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("abs_Z:  %1m").arg(absolutePosition.v[2], 10, 'f', 2);
    painter.drawText(screen_x, screen_y, str);screen_y -= 15.0f;
    str = QString("groundNullHeight: %1m").arg(groundNullHeight, 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    //*/
}

void Scenery3d::drawDebug()
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
			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(1.0f,1.0f,1.0f,1.0f));

			sceneBoundingBox.render();

			SET_UNIFORM(debugShader,ShaderMgr::UNIFORM_VEC_COLOR,QVector4D(0.4f,0.4f,0.4f,1.0f));
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
			qWarning()<<"[Scenery3d] Cannot use debug shader, probably on OpenGL ES context";
		}
	}
#endif


    StelPainter painter(altAzProjector);
    painter.setFont(debugTextFont);
    painter.setColor(1,0,1,1);
    // For now, these messages print light mixture values.
    painter.drawText(20, 160, lightMessage);
    painter.drawText(20, 145, lightMessage2);
    painter.drawText(20, 130, lightMessage3);
    painter.drawText(20, 115, QString("Torch range %1, brightness %2/%3/%4").arg(torchRange).arg(lightInfo.torchDiffuse[0]).arg(lightInfo.torchDiffuse[1]).arg(lightInfo.torchDiffuse[2]));
    QString str = QString("BB: %1/%2/%3 %4/%5/%6").arg(sceneBoundingBox.min.v[0], 7, 'f', 2).arg(sceneBoundingBox.min.v[1], 7, 'f', 2).arg(sceneBoundingBox.min.v[2], 7, 'f', 2)
		    .arg(sceneBoundingBox.max.v[0], 7, 'f', 2).arg(sceneBoundingBox.max.v[1], 7, 'f', 2).arg(sceneBoundingBox.max.v[2], 7, 'f', 2);
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

			int tmp = screen_y - debugTextureSize-30;
			painter.drawText(screen_x-125, tmp, QString("cam n/f: %1/%2").arg(frustumArray[i].zNear, 7, 'f', 2).arg(frustumArray[i].zFar, 7, 'f', 2));
			painter.drawText(screen_x-125, tmp-15.0f, QString("uv scale: %1/%2").arg(shadowFrustumSize[i].x(), 7, 'f', 2).arg(shadowFrustumSize[i].y(),7,'f',2));
			painter.drawText(screen_x-125, tmp-30.0f, QString("ortho n/f: %1/%2").arg(shadowFrustumSize[i].z(), 7, 'f', 2).arg(shadowFrustumSize[i].w(),7,'f',2));

			screen_x -= 290;
		}
		painter.drawText(screen_x+165.0f, screen_y-215.0f, QString("Splitweight: %1").arg(currentScene.shadowSplitWeight, 3, 'f', 2));
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
    str = QString("%1 %2 %3").arg(viewPos.v[0], 7, 'f', 2).arg(viewPos.v[1], 7, 'f', 2).arg(viewPos.v[2], 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = "View Dir, dominant faces";
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("%1 %2 %3, %4/%5").arg(mainViewDir.v[0], 7, 'f', 2).arg(mainViewDir.v[1], 7, 'f', 2).arg(mainViewDir.v[2], 7, 'f', 2).arg(dominantFace).arg(secondDominantFace);
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = "View Up";
    painter.drawText(screen_x, screen_y, str);
    screen_y -= 15.0f;
    str = QString("%1 %2 %3").arg(mainViewUp.v[0], 7, 'f', 2).arg(mainViewUp.v[1], 7, 'f', 2).arg(mainViewUp.v[2], 7, 'f', 2);
    painter.drawText(screen_x, screen_y, str);
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
    str = QString("Venus: %1").arg(lightInfo.lightSource == Venus);
    painter.drawText(screen_x, screen_y, str);
}

void Scenery3d::determineFeatureSupport()
{
	QOpenGLContext* ctx = QOpenGLContext::currentContext();

	// graphics hardware without FrameBufferObj extension cannot use the cubemap rendering and shadow mapping.
	// In this case, set cubemapSize to 0 to signal auto-switch to perspective projection.
	// OpenGL ES2 has framebuffers in the Spec
	if ( !ctx->hasExtension("GL_EXT_framebuffer_object") && !ctx->isOpenGLES() ) {

		//TODO FS: it seems like the current stellarium requires a working framebuffer extension anyway, so skip this check?
		qWarning() << "[Scenery3d] Your hardware does not support EXT_framebuffer_object.";
		qWarning() << "[Scenery3d] Shadow mapping disabled, and display limited to perspective projection.";

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

		qDebug()<<"[Scenery3d] Maximum texture size:"<<texSize;
		qDebug()<<"[Scenery3d] Maximum viewport dims:"<<viewportSize[0]<<viewportSize[1];
		qDebug()<<"[Scenery3d] Maximum renderbuffer size:"<<rbSize;

		maximumFramebufferSize = qMin(texSize,qMin(rbSize,qMin(viewportSize[0],viewportSize[1])));
		qDebug()<<"[Scenery3d] Maximum framebuffer size:"<<maximumFramebufferSize;
	}

	QString renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	isANGLE = renderer.contains("ANGLE");

	//check if GS cubemapping is possible
	if(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry,ctx)) //this checks if version >= 3.2
	{
		this->supportsGSCubemapping = true;
		qDebug()<<"[Scenery3d] Geometry shader supported";
	}
	else
		qDebug()<<"[Scenery3d] Geometry shader not supported on this hardware";

	//Query how many texture units we have at disposal in a fragment shader
	//we currently need 8 in the worst case: diffuse, emissive, bump, height + 4x shadowmap
	GLint texUnits,combUnits;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texUnits);
	qDebug() << "[Scenery3d] GL_MAX_TEXTURE_IMAGE_UNITS:" << texUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &combUnits);
	qDebug() << "[Scenery3d] GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:" << combUnits;
	if(texUnits < 8 || combUnits < 8)
	{
		qWarning()<<"Insufficient texture units available for all effects, should have at least 8!";
	}

	if(shaderParameters.openglES)
	{
		//shadows in our implementation require depth textures
		if(ctx->hasExtension("GL_OES_depth_texture") ||
			ctx->hasExtension("GL_ANGLE_depth_texture"))
		{
			supportsShadows = true;
			qDebug()<<"[Scenery3d] Shadows are supported";
		}
		else
		{
			supportsShadows = false;
			qDebug()<<"[Scenery3d] Shadows are not supported on this hardware";
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

void Scenery3d::init()
{
	OBJ::setupGL();

	QOpenGLContext* ctx = QOpenGLContext::currentContext();

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
	cubeIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	cubeIndexBuffer.create();

	//enable seamless cubemapping if HW supports it
	if(ctx->hasExtension("GL_ARB_seamless_cube_map"))
	{
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		qDebug()<<"[Scenery3d] Seamless cubemap filtering enabled";
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

void Scenery3d::deleteCubemapping()
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

bool Scenery3d::initCubemapping()
{
	GET_GLERROR()

	bool ret = false;
	qDebug()<<"[Scenery3d] Initializing cubemap...";

	//remove old cubemap objects if they exist
	deleteCubemapping();

	GET_GLERROR()

	if(cubemapSize<=0)
	{
		qWarning()<<"[Scenery3d] Cubemapping not supported or disabled";
		parent->showMessage(q_("Your hardware does not support cubemapping, please switch to 'Perspective' projection!"));
		return false;
	}

	cubeMappingCreated = true;

	//last compatibility check before possible crash
	if( !isGeometryShaderCubemapSupported() && cubemappingMode == S3DEnum::CM_CUBEMAP_GSACCEL)
	{
		parent->showMessage(q_("Geometry shader is not supported. Falling back to '6 Textures' mode."));
		qWarning()<<"[Scenery3d] GS not supported, fallback to '6 Textures'";
		cubemappingMode = S3DEnum::CM_TEXTURES;
	}

	//TODO the ANGLE version included with Qt 5.4 includes a bug that prevents Cubemapping to be used
	//Remove this if this is ever fixed
	if(isANGLEContext() && cubemappingMode >= S3DEnum::CM_CUBEMAP)
	{
		//Fall back to "6 Textures" mode
		parent->showMessage(q_("Falling back to '6 Textures' because of ANGLE bug"));
		qWarning()<<"[Scenery3d] On ANGLE, fallback to '6 Textures'";
		cubemappingMode = S3DEnum::CM_TEXTURES;
	}


#ifndef QT_OPENGL_ES_2
	//if we are on an ES context, it may not be possible to specify texture bitdepth
	bool isEs = QOpenGLContext::currentContext()->isOpenGLES();
	GLenum colorFormat = isEs ? GL_RGBA : GL_RGBA8;
	GLenum depthFormat = isEs ? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT24;
	GLenum rbDepth = isEs ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24;
#else
	GLenum colorFormat = GL_RGBA;
	GLenum depthFormat = GL_DEPTH_COMPONENT;
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
		for (int i=0;i<6;++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,colorFormat,
				     cubemapSize,cubemapSize,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
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
				     cubemapSize,cubemapSize,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);

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
		for (int i=0;i<6;++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,depthFormat,
				     cubemapSize,cubemapSize,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,NULL);

			GET_GLERROR()
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
	else
	{
		//gen renderbuffer for single-face depth, reused for all faces to save some memory
		int val = 0;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&val);
		qDebug()<<"[Scenery3d] Max Renderbuffer size"<<val;

		glGenRenderbuffers(1,&cubeRB);
		glBindRenderbuffer(GL_RENDERBUFFER,cubeRB);
		glRenderbufferStorage(GL_RENDERBUFFER, rbDepth, cubemapSize,cubemapSize);
		GLenum err=glGetError();

		switch(err){
			case GL_NO_ERROR:
					break;
			case GL_INVALID_ENUM:
					qWarning()<<"Scenery3D: RB: invalid depth format?";
					break;
			case GL_INVALID_VALUE:
					qWarning()<<"Scenery3D: RB: invalid renderbuffer size";
					break;
			case GL_OUT_OF_MEMORY:
					qWarning()<<"Scenery3D: RB: out of memory. Cannot create renderbuffer.";
					break;
				default:
				qWarning()<<"Scenery3D: RB: unexpected OpenGL error:" << err;
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
			qWarning() << "[Scenery3D] glCheckFramebufferStatus failed, probably can't use cube map";
		}
		else
			ret = true;
	}
	else
	{
		//6 FBOs used
		glGenFramebuffers(6,cubeSideFBO);

		GET_GLERROR()

		for(int i=0;i<6;++i)
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
				qWarning() << "[Scenery3D] glCheckFramebufferStatus failed, probably can't use cube map";
				ret = false;
				break;
			}
			else
				ret = true;
		}
	}

	//unbind last framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER,0);

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
	const double d_sub_v = 2.0 / sub;
	const double d_sub_tex = 1.0 / sub;

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
			float xp = -1.0 + x * d_sub_v;
			float yp = -1.0 + y * d_sub_v;

			float tx = x * d_sub_tex;
			float ty = y * d_sub_tex;

			cubePlaneFront<< Vec3f(xp, 1.0f, yp);
			cubePlaneFrontTex<<Vec2f(tx,ty);

			vertexIdx[y][x] = y*(sub+1)+x;
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

	qDebug()<<"[Scenery3d] Using cube with"<<cubeVertices.size()<<"vertices and" <<cubeIndexCount<<"indices";

	//create the other cube faces by rotating the front face
#define PLANE(_PLANEID_, _MAT_) for(int i=_PLANEID_ * vtxCount;i < (_PLANEID_ + 1)*vtxCount;i++){ _MAT_.transfo(cubeVertices[i]); }\
	for(int i =_PLANEID_ * idxCount; i < (_PLANEID_+1)*idxCount;++i) { cubeIndices[i] = cubeIndices[i] + _PLANEID_ * vtxCount; }

	PLANE(0, Mat4f::zrotation(-M_PI_2)); //S
	PLANE(1, Mat4f::zrotation(M_PI_2));  //N
	PLANE(2, Mat4f::identity());  //E
	PLANE(3, Mat4f::zrotation(M_PI)); //W
	PLANE(4, Mat4f::xrotation(M_PI_2)); //U
	PLANE(5, Mat4f::xrotation(-M_PI_2)); //D
#undef PLANE

	//upload original cube vertices + indices to GL
	cubeVertexBuffer.bind();
	//store original vertex pos (=3D vertex coords) + 2D tex coords in same buffer
	cubeVertexBuffer.allocate(cubeVertices.size() * (sizeof(Vec3f) + sizeof(Vec2f)) );
	cubeVertexBuffer.write(0, cubeVertices.constData(), cubeVertices.size() * sizeof(Vec3f));
	cubeVertexBuffer.write(cubeVertices.size() * sizeof(Vec3f), cubeTexcoords.constData(), cubeTexcoords.size() * sizeof(Vec2f));
	cubeVertexBuffer.release();

	cubeIndexBuffer.bind();
	cubeIndexBuffer.allocate(cubeIndices.constData(),cubeIndices.size() * sizeof(unsigned short));
	cubeIndexBuffer.release();

	//reset cubemap timer to make sure it is rerendered immediately after re-init
	invalidateCubemap();

	qDebug()<<"[Scenery3d] Initializing cubemap...done!";

	if(!ret)
	{
		parent->showMessage("Cannot use cubemapping with current settings");
		deleteCubemapping();
	}
	return ret;
}

void Scenery3d::deleteShadowmapping()
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

		qDebug()<<"[Scenery3d] Shadowmapping objects cleaned up";
	}
}

bool Scenery3d::initShadowmapping()
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
		qWarning()<<"[Scenery3d] Tried to initialize shadows without shadow support!";
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
			glActiveTexture(GL_TEXTURE4+i);

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
			glTexImage2D(GL_TEXTURE_2D, 0, (pcssEnabled ? depthPcss : depthNormal), shadowmapSize, shadowmapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

			GLint filter = (shaderParameters.shadowFilterQuality == S3DEnum::SFQ_HARDWARE
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
			//we use hardware-accelerated depth compare mode, unless pcss is used
			//NOTE: cant use depth compare mode on ES2
			if(!pcssEnabled)
			{
#ifndef QT_OPENGL_ES_2
				if(!isEs)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				}
#endif
			}

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
				qWarning() << "[Scenery3D] glCheckFramebufferStatus failed, can't use FBO";
				break;
			}
			else if (i==shaderParameters.frustumSplits-1)
			{
				valid = true;
			}
		}

		//Done. Unbind and switch to normal texture unit 0
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(GL_TEXTURE0);

		qDebug()<<"[Scenery3D] shadowmapping initialized";
	}
	else
	{
		qWarning()<<"[Scenery3D] shadowmapping not supported or disabled";
	}

	if(!valid)
	{
		deleteShadowmapping();
		parent->showMessage(q_("Shadow mapping can not be used on your hardware, check logs for details"));
	}
	return valid;
}

void Scenery3d::draw(StelCore* core)
{
	//cant draw if no models
	if(!objModel || !objModel->hasStelModels())
		return;

	//reset render statistic
	drawnTriangles = drawnModels = materialSwitches = shaderSwitches = 0;

	requiresCubemap = core->getCurrentProjectionType() != StelCore::ProjectionPerspective;
	//update projector from core
	altAzProjector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	//perform Z-sorting for correct transparency
	//this uses the object's centroids for sorting, so the OBJ must be created correctly
	objModel->transparencyDepthSort(-absolutePosition.toVec3f());

	if(requiresCubemap)
	{
		if(!cubeMappingCreated || reinitCubemapping)
		{
			//init cubemaps
			if(!initCubemapping())
				return;
			reinitCubemapping = false;
		}
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

	cubemappingUsedLastFrame = requiresCubemap;
}
