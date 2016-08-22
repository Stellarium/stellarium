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

#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelApp.hpp"

#include <QVector3D>

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

void S3DScene::Material::calculateTraits()
{
	//traits.hasAmbientTexture = tex_Ka && tex_Ka->canBind();
	traits.hasDiffuseTexture = tex_Kd && tex_Kd->canBind();
	//traits.hasSpecularTexture = tex_Ks && tex_Ks->canBind();
	traits.hasEmissiveTexture = tex_Ke && tex_Ke->canBind();
	traits.hasBumpTexture = tex_bump && tex_bump->canBind();
	traits.hasHeightTexture = tex_height && tex_height->canBind();

	//test if specular coefficient and shininess is non-zero
	traits.hasSpecularity = Ks.lengthSquared()>0.0001f  && Ns > 0.0001f;

	bool alphaChannel = tex_Kd && tex_Kd->canBind() && tex_Kd->hasAlphaChannel();
	//test if we require blending
	if(d< .0f)
	{
		//no alpha value set, no transparency
		traits.hasTransparency = false;
		d = 1.0f;
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
}

S3DScene::S3DScene(const SceneInfo &info)
	: info(info)
{
	//setup main load transform matrix
	zRot2Grid = (info.zRotateMatrix*info.obj2gridMatrix).convertToQMatrix();
}

void S3DScene::setModel(const StelOBJ &model)
{
	modelData = model;
	//transform the model
	modelData.transform(zRot2Grid);
	sceneAABB = model.getAABBox();

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
			info.location->altitude=static_cast<int>(0.5*(sceneAABB.min[2]+sceneAABB.max[2])+info.modelWorldOffset[2]);
		}
	}

	if(info.startPositionFromModel)
	{
		//position at the XY center of the model
		position.v[0] = (sceneAABB.max[0]+sceneAABB.min[0])/2.0;
		qDebug() << "Setting Easting  to BBX center: " << sceneAABB.min[0] << ".." << sceneAABB.max[0] << ": " << -position.v[0];
		position.v[1] = (sceneAABB.max[1]+sceneAABB.min[1])/2.0;
		qDebug() << "Setting Northing to BBX center: " << sceneAABB.min[1] << ".." << sceneAABB.max[1] << ": " << position.v[1];
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
	StelOBJ::V3Vec groundPositionList;
	ground.splitVertexData(&groundPositionList);
	for(int i=0;i<groundPositionList.size();++i)
	{
		Vec3f& pos = groundPositionList[i];
		QVector3D trans = zRot2Grid * QVector3D(pos[0], pos[1], pos[2]);
		pos.set(trans.x(),trans.y(),trans.z());
	}

	if(info.groundNullHeightFromModel)
	{
		info.groundNullHeight = ground.getAABBox().min[2];
	}

	heightmap.setNullHeight(info.groundNullHeight);
	heightmap.setMeshData(ground.getIndexList(), groundPositionList);
}

float S3DScene::getGroundHeightAtViewer() const
{
	return heightmap.getHeight(position.v[0],position.v[1]);
}

bool S3DScene::glLoad()
{
	bool ok = glArray.load(&modelData);
	modelData.clear();

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

		mat.calculateTraits();
	}

	return ok;
}

void S3DScene::setViewerPosition(const Vec3d &pos)
{
	position = pos;
	recalcEyePos();
}

void S3DScene::setViewerPositionOnHeightmap(const Vec2d &pos)
{
	position = Vec3d(pos[0], pos[1], heightmap.getHeight(pos[0],pos[1]));
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
			qWarning()<<"Error loading texture"<<tex->getFullPath()<<tex->getErrorMessage();
			tex.clear();
		}
		else
		{
			//clean up after ourselves
			tex->release();
		}
	}
}
