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

#include "S3DScene.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelUtils.hpp"

#include <QVector3D>

Q_LOGGING_CATEGORY(s3dscene, "stel.plugin.scenery3d.s3dscene")

void S3DScene::Material::loadTexturesAsync()
{
	StelTextureMgr& mgr = StelApp::getInstance().getTextureManager();
	/*if(!map_Ka.isEmpty())
		tex_Ka = mgr.createTextureThread(map_Ka, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true), false);*/
	if(!map_Kd.isEmpty())
		tex_Kd = mgr.createTextureThread(map_Kd, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true), false);
	if(!map_Ke.isEmpty())
		tex_Ke = mgr.createTextureThread(map_Ke, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true), false);
	/*if(!map_Ks.isEmpty())
		tex_Ks = mgr.createTextureThread(map_Ks, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true), false);*/
	if(!map_bump.isEmpty())
		tex_bump = mgr.createTextureThread(map_bump, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true), false);
	if(!map_height.isEmpty())
		tex_height = mgr.createTextureThread(map_height, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT, true), false);
}

void S3DScene::Material::fixup()
{
	//traits.hasAmbientTexture = tex_Ka && tex_Ka->canBind();
	traits.hasDiffuseTexture = tex_Kd && tex_Kd->canBind();
	//traits.hasSpecularTexture = tex_Ks && tex_Ks->canBind();
	traits.hasEmissiveTexture = tex_Ke && tex_Ke->canBind();
	traits.hasBumpTexture = tex_bump && tex_bump->canBind();
	traits.hasHeightTexture = tex_height && tex_height->canBind();

	//test if specular coefficient and shininess is non-zero
	traits.hasSpecularity = Ks[0]<0.0f ? false : Ks.lengthSquared()>0.0001f  && Ns > 0.0001f;

	bool alphaChannel = tex_Kd && tex_Kd->canBind() && tex_Kd->hasAlphaChannel();
	//test if we require blending
	if(d< .0f)
	{
		//no alpha value set, no transparency
		traits.hasTransparency = false;
	}
	else if (d <1.0f)
	{
		//alpha value set, between 0 and 1
		traits.hasTransparency = true;
	}
	else
	{
		//alpha = 1, check if texture has alpha channel, otherwise it makes no sense enabling transparency
		traits.hasTransparency = alphaChannel;
	}

	//find out if Kd is valid
	//this is probably the minimum we should expect, but there are exporters not writing Kd when map_Kd is given. Log a warning if there is also no texture.
	if(Kd[0]< .0f)
	{
		if (!traits.hasDiffuseTexture)
			qCWarning(s3dscene)<<"Material"<<name<<"has no Kd defined";
		Kd = QVector3D(0.8f,0.8f,0.8f);
	}

	//support for "legacy" illumination using the illum statement
	//Illum definitions are used by a lot of exporters, but their use is rather inconsistent
	//Here we try to make something useful out of it
	if(illum>I_NONE)
	{
		if(Ka[0]< .0f) //set to old default value
			Ka = QVector3D(0.2f,0.2f,0.2f);

		switch(illum)
		{
			case I_DIFFUSE:
				//explicitely set Ka to Kd
				Ka = Kd;
				//explicitly disable specularity and transparency
				traits.hasSpecularity = false;
				traits.hasTransparency = false;
				break;
			case I_DIFFUSE_AND_AMBIENT:
				//explicitly disable specularity and transparency
				traits.hasSpecularity = false;
				traits.hasTransparency = false;
				break;
			case I_SPECULAR:
				//here, this follows the same logic as if no Illum was set
				//only when there is a valid Ks and Ns, specularity is used
				//this fixes problems with some exporters where this illum
				//is set but Ns = 0
				//traits.hasSpecularity = true;
				traits.hasTransparency = false;
				break;
			case I_TRANSLUCENT:
				//also follow the "no illum" logic here
				//traits.hasSpecularity = false;
				if(d<.0f)
				{
					d = 1.0f;
					//here, transparancy is only enabled if
					//there is actually something transparent!
					//would be a waste of computing power otherwise
					traits.hasTransparency = alphaChannel;
				}
				else
				{
					traits.hasTransparency = true;
				}
				break;
			default:
				qCWarning(s3dscene)<<"Unknown illum model encountered"<<illum;
				break;
		}
	}
	else
	{
		if(Ka[0]< .0f)
		{
			//ambient was not set, old "illum 0" behaviour, set Ka to Kd
			Ka = Kd;
		}

		if(d <.0f)
		{
			d = 1.0f;
		}
	}

	if(qIsNull(d))
	{
		traits.isFullyTransparent = true;
	}

	//finally, find out if Ks and Ke are valid
	if(Ks[0]< .0f)
	{
		//no specularity
		Ks = QVector3D();
	}
	if(Ke[0]< .0f)
	{
		//no emission
		Ke = QVector3D();
	}
}

bool S3DScene::Material::updateFadeInfo(double currentJD)
{
	//find out if we have to fade in or out
	if(currentJD >= vis_fadeIn[0] && currentJD <= vis_fadeIn[1])
	{
		vis_fadeValue = static_cast<float>((currentJD - vis_fadeIn[0]) / (vis_fadeIn[1] - vis_fadeIn[0]));
		traits.isFading = true;
	}
	else if(currentJD >= vis_fadeOut[0] && currentJD <= vis_fadeOut[1])
	{
		vis_fadeValue = 1.0f - static_cast<float>((currentJD - vis_fadeOut[0]) / (vis_fadeOut[1] - vis_fadeOut[0]));
		traits.isFading = true;
	}
	else if (currentJD > vis_fadeIn[1] && currentJD < vis_fadeOut[0])
	{
		vis_fadeValue = 1.0f;
		traits.isFading = false;
	}
	else
	{
		vis_fadeValue = 0.0f;
		traits.isFading = false;
		traits.isFullyTransparent = true;
		//we skip drawing this object entirely!
		return false;
	}
	traits.isFullyTransparent = d <= 0.0f;
	return true;
}

S3DScene::S3DScene(const SceneInfo &info)
	: glReady(false), info(info),
	  viewDirection(1.0,0.0,0.0), position(0.0,0.0,0.0), eye_height(1.65), eyePosition(0.0,0.0,1.65)
{
	//setup main load transform matrix
	zRot2Grid = (info.zRotateMatrix*info.obj2gridMatrix).convertToQMatrix();
}

void S3DScene::setModel(const StelOBJ &model)
{
	modelData = model;
	//transform the model
	modelData.transform(zRot2Grid);
	sceneAABB = modelData.getAABBox();

	//copy materials
	const StelOBJ::MaterialList& objMats = modelData.getMaterialList();
	materials.reserve(objMats.size());
	for(int i=0;i<objMats.size();++i)
	{
		materials.append(objMats[i]);
		//start loading textures
		materials[i].loadTexturesAsync();
	}

	//copy objects
	objects = modelData.getObjectList();

	if(info.hasLocation())
	{
		if(info.altitudeFromModel)
		{
			info.location->altitude=static_cast<int>(0.5*static_cast<double>(sceneAABB.min[2]+sceneAABB.max[2])+info.modelWorldOffset[2]);
		}
	}

	if(info.startPositionFromModel)
	{
		//position at the XY center of the model
		position.v[0] = static_cast<double>(sceneAABB.max[0]+sceneAABB.min[0])/2.0;
		qCDebug(s3dscene) << "Setting Easting  to BBX center: " << sceneAABB.min[0] << ".." << sceneAABB.max[0] << ": " << -position.v[0];
		position.v[1] = static_cast<double>(sceneAABB.max[1]+sceneAABB.min[1])/2.0;
		qCDebug(s3dscene) << "Setting Northing to BBX center: " << sceneAABB.min[1] << ".." << sceneAABB.max[1] << ": " << position.v[1];
	}
	else
	{
		position[0] = info.relativeStartPosition[0];
		position[1] = info.relativeStartPosition[1];
	}

	eye_height = info.eyeLevel;
	recalcEyePos();

	//Find a good splitweight based on the scene's size
	float maxSize = -std::numeric_limits<float>::max();
	maxSize = std::max(sceneAABB.max.v[0], maxSize);
	maxSize = std::max(sceneAABB.max.v[1], maxSize);


	if(info.shadowSplitWeight<0)
	{
		//qDebug() << "MAXSIZE:" << maxSize;
		if(maxSize < 100.0f)
			info.shadowSplitWeight = 0.5f;
		else if(maxSize < 200.0f)
			info.shadowSplitWeight = 0.60f;
		else if(maxSize < 400.0f)
			info.shadowSplitWeight = 0.70f;
		else
			info.shadowSplitWeight = 0.99f;
	}
}

void S3DScene::setGround(const StelOBJ &ground)
{
	//we only need to retain the position data for the ground
	StelOBJ groundTmp = ground;
	groundTmp.transform(zRot2Grid,true);
	StelOBJ::V3Vec groundPositionList;
	groundTmp.splitVertexData(&groundPositionList);

	heightmap.setMeshData(groundTmp.getIndexList(), groundPositionList, &groundTmp.getAABBox());
	if(info.groundNullHeightFromModel)
	{
		info.groundNullHeight = static_cast<double>(ground.getAABBox().min[2]);
	}
}

float S3DScene::getGroundHeightAtViewer() const
{
	return heightmap.getHeight(static_cast<float>(position.v[0]),static_cast<float>(position.v[1]));
}

Vec3d S3DScene::getGridPosition() const
{
	Vec3d pos = getViewerPosition();
	// this is the observer position (camera eye position) in model-grid coordinates, relative to the origin
	pos=info.zRotateMatrix.inverse()* pos;
	// this is the observer position (camera eye position) in grid coordinates, e.g. Gauss-Krueger or UTM.
	pos+= info.modelWorldOffset;

	return pos;
}

void S3DScene::setGridPosition(const Vec3d &gridPos)
{
	Vec3d pos = gridPos;
	//this is basically the same as getCurrentGridPosition(), but in reverse
	pos-=info.modelWorldOffset;

	//calc opengl position
	setViewerPosition(info.zRotateMatrix * pos);
}

bool S3DScene::glLoad()
{
	bool ok = glArray.load(&modelData);
	modelData.clear();

	//set this here, to respect models without ground OBJ
	heightmap.setNullHeight(static_cast<float>(info.groundNullHeight));

	//move the viewer to the ground height
	position[2] = static_cast<double>(getGroundHeightAtViewer());
	recalcEyePos();

	double currentJD = StelApp::getInstance().getCore()->getJD();

	//make sure textures are loaded
	for(int i =0; i< materials.size();++i)
	{
		S3DScene::Material& mat = materials[i];
		//ambient and specular textures currently unused
		//finalizeTexture(mat.tex_Ka);
		//finalizeTexture(mat.tex_Ks);
		finalizeTexture(mat.tex_Kd);
		finalizeTexture(mat.tex_Ke);
		finalizeTexture(mat.tex_bump);
		finalizeTexture(mat.tex_height);

		mat.fixup();
		//make sure fade value is current
		if(mat.traits.hasTimeFade)
			mat.updateFadeInfo(currentJD);
	}

	glReady = ok;
	return ok;
}

void S3DScene::moveViewer(const Vec3d &moveView)
{
	//get the azimuth angle of the current view vector
	double alt, az;
	StelUtils::rectToSphe(&az, &alt, viewDirection);

	//calculate the move vector in the world space
	Vec3d moveWorld(  moveView[1] * std::cos(az) + moveView[0] * std::sin(az),
			- moveView[0] * std::cos(az) + moveView[1] * std::sin(az),
			  moveView[2]);

	eye_height+= moveWorld[2];
	position[0]+= moveWorld[0];
	position[1]+= moveWorld[1];
	position[2] = static_cast<double>(heightmap.getHeight(static_cast<float>(position[0]),static_cast<float>(position[1])));
	recalcEyePos();
}

void S3DScene::setViewerPosition(const Vec3d &pos)
{
	position = pos;
	recalcEyePos();
}

void S3DScene::setViewerPositionOnHeightmap(const Vec2d &pos)
{
	position = Vec3d(pos[0], pos[1], static_cast<double>(heightmap.getHeight(static_cast<float>(pos[0]),static_cast<float>(pos[1]))));
	recalcEyePos();
}

void S3DScene::finalizeTexture(StelTextureSP &tex)
{
	if(tex)
	{
		tex->waitForLoaded();

		//load it into GL
		if(!tex->bind())
		{
			qCWarning(s3dscene)<<"Error loading texture"<<tex->getFullPath()<<tex->getErrorMessage();
			tex.clear();
		}
		else
		{
			//clean up after ourselves
			tex->release();
		}
	}
}
