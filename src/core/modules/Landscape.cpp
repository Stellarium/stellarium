/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2011 Bogdan Marinov
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

#include "Landscape.hpp"
#include "StelApp.hpp"
#include "renderer/StelGeometryBuilder.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocation.hpp"
#include "StelCore.hpp"

#include <QDebug>
#include <QSettings>
#include <QVarLengthArray>

Landscape::Landscape(float _radius) : radius(_radius), skyBrightness(1.), nightBrightness(0.8), angleRotateZOffset(0.)
{
	validLandscape = 0;
}

Landscape::~Landscape()
{
}


// Load attributes common to all landscapes
void Landscape::loadCommon(const QSettings& landscapeIni, const QString& landscapeId)
{
	name = landscapeIni.value("landscape/name").toString();
	author = landscapeIni.value("landscape/author").toString();
	description = landscapeIni.value("landscape/description").toString();
	description = description.replace(QRegExp("\\\\n\\s*\\\\n"), "<br />");
	description = description.replace("\\n", " ");
	if (name.isEmpty())
	{
		qWarning() << "No valid landscape definition found for landscape ID "
			<< landscapeId << ". No landscape in use." << endl;
		validLandscape = 0;
		return;
	}
	else
	{
		validLandscape = 1;
	}

	// Optional data
	// Patch GZ:
	if (landscapeIni.contains("landscape/tesselate_rows"))
		rows = landscapeIni.value("landscape/tesselate_rows").toInt();
	else rows=20;
	if (landscapeIni.contains("landscape/tesselate_cols"))
		cols = landscapeIni.value("landscape/tesselate_cols").toInt();
	else cols=40;


	if (landscapeIni.contains("location/planet"))
		location.planetName = landscapeIni.value("location/planet").toString();
	else
		location.planetName = "Earth";
	if (landscapeIni.contains("location/altitude"))
		location.altitude = landscapeIni.value("location/altitude").toInt();
	if (landscapeIni.contains("location/latitude"))
		location.latitude = StelUtils::getDecAngle(landscapeIni.value("location/latitude").toString())*180./M_PI;
	if (landscapeIni.contains("location/longitude"))
		location.longitude = StelUtils::getDecAngle(landscapeIni.value("location/longitude").toString())*180./M_PI;
	if (landscapeIni.contains("location/country"))
		location.country = landscapeIni.value("location/country").toString();
	if (landscapeIni.contains("location/state"))
		location.state = landscapeIni.value("location/state").toString();
	if (landscapeIni.contains("location/name"))
		location.name = landscapeIni.value("location/name").toString();
	else
		location.name = name;
	location.landscapeKey = name;
	// New entries by GZ.
	if (landscapeIni.contains("location/light_pollution"))
		defaultBortleIndex = landscapeIni.value("location/light_pollution").toInt();
	else defaultBortleIndex=-1; // mark "invalid/no change".
	if (defaultBortleIndex<=0) defaultBortleIndex=-1; // also allow neg. values in ini file, signalling "no change".
	if (defaultBortleIndex>9) defaultBortleIndex=9; // correct bad values.
	if (landscapeIni.contains("location/display_fog"))
		defaultFogSetting = landscapeIni.value("location/display_fog").toInt();
	else defaultFogSetting=-1;
	if (landscapeIni.contains("location/atmospheric_extinction_coefficient"))
		defaultExtinctionCoefficient = landscapeIni.value("location/atmospheric_extinction_coefficient").toDouble();
	else defaultExtinctionCoefficient=-1.0;
	if (landscapeIni.contains("location/atmospheric_temperature"))
		defaultTemperature = landscapeIni.value("location/atmospheric_temperature").toDouble();
	else defaultTemperature=-1000.0;
	if (landscapeIni.contains("location/atmospheric_pressure"))
		defaultPressure = landscapeIni.value("location/atmospheric_pressure").toDouble();
	else defaultPressure=-2.0; // "no change"
}

#include <iostream>
const QString Landscape::getTexturePath(const QString& basename, const QString& landscapeId)
{
	// look in the landscape directory first, and if not found default to global textures directory
	QString path;
	try
	{
		path = StelFileMgr::findFile("landscapes/" + landscapeId + "/" + basename);
		return path;
	}
	catch (std::runtime_error& e)
	{
#ifdef BUILD_FOR_MAEMO
		if (!basename.endsWith(".pvr"))
		{
			QString tmp = basename;
			tmp.replace(".png", ".pvr");
			try
			{
				tmp = getTexturePath(tmp, landscapeId);
				tmp.replace(".pvr", ".png");
				return tmp;
			}
			catch (std::runtime_error& e)
			{;}
		}
#endif
		path = StelFileMgr::findFile("textures/" + basename);
		return path;
	}
}

LandscapeOldStyle::LandscapeOldStyle(float _radius)
	: Landscape(_radius)
	, sideTexs(NULL)
	, texturesInitialized(false)
	, sides(NULL)
	, fogTex(NULL)
	, groundTex(NULL)
	, tanMode(false)
	, calibrated(false)
	, fogCylinderBuffer(NULL)
	// Way off to ensure the height is detected as "changed" on the first drawFog() call.
	, previousFogHeight(-1000.0f)
	, groundFanDisk(NULL)
	, groundFanDiskIndices(NULL)
{}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (NULL != sideTexs)
	{
		if(texturesInitialized)
		{
			for(int i = 0; i < nbSideTexs; ++i)
			{
				delete sideTexs[i].texture;
			}
		}
		delete [] sideTexs;
		sideTexs = NULL;
	}

	if(NULL != fogTex)
	{
		delete fogTex;
		fogTex = NULL;
	}

	if(NULL != groundTex)
	{
		delete groundTex;
		groundTex = NULL;
	}

	if (NULL != fogCylinderBuffer) 
	{
		delete fogCylinderBuffer;
		fogCylinderBuffer = NULL;
	}

	if (NULL != groundFanDisk) 
	{
		Q_ASSERT_X(NULL != groundFanDiskIndices, Q_FUNC_INFO, 
		           "Vertex buffer is generated but index buffer is not");
		delete groundFanDisk;
		groundFanDisk = NULL;
		delete groundFanDiskIndices;
		groundFanDiskIndices = NULL;
	}

	if (NULL != sides) 
	{
		delete [] sides;
		sides = NULL;
	}

	for(int side = 0; side < precomputedSides.length(); ++side)
	{
		delete precomputedSides[side].vertices;
		delete precomputedSides[side].indices;
	}
	precomputedSides.clear();
}

void LandscapeOldStyle::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// TODO: put values into hash and call create method to consolidate code
	loadCommon(landscapeIni, landscapeId);

	if (landscapeIni.contains("landscape/tesselate_rows"))
		rows = landscapeIni.value("landscape/tesselate_rows").toInt();
	else rows=8;
	if (landscapeIni.contains("landscape/tesselate_cols"))
		cols = landscapeIni.value("landscape/tesselate_cols").toInt();
	else cols=16;

	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "old_style")
	{
		qWarning() << "Landscape type mismatch for landscape " << landscapeId
				   << ", expected old_style, found " << type << ".  No landscape in use.";
		validLandscape = 0;
		return;
	}

	// Load sides textures
	nbSideTexs = landscapeIni.value("landscape/nbsidetex", 0).toInt();
	sideTexs = new SideTexture[nbSideTexs];
	for (int i=0; i<nbSideTexs; ++i)
	{
		QString textureKey = QString("landscape/tex%1").arg(i);
		QString textureName = landscapeIni.value(textureKey).toString();
		sideTexs[i].path = getTexturePath(textureName, landscapeId);
		// Will be lazily initialized
		sideTexs[i].texture = NULL;
	}

	// Init sides parameters
	nbSide = landscapeIni.value("landscape/nbside", 0).toInt();
	sides = new landscapeTexCoord[nbSide];
	int texnum;

	for (int i=0;i<nbSide;++i)
	{
		QString key = QString("landscape/side%1").arg(i);
		QString description = landscapeIni.value(key).toString();
		//sscanf(s.toLocal8Bit(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		QStringList parameters = description.split(':');
		//TODO: How should be handled an invalid texture description?
		QString textureName = parameters.value(0);
		texnum = textureName.right(textureName.length() - 3).toInt();
		sides[i].texCoords[0] = parameters.at(1).toFloat();
		sides[i].texCoords[1] = parameters.at(2).toFloat();
		sides[i].texCoords[2] = parameters.at(3).toFloat();
		sides[i].texCoords[3] = parameters.at(4).toFloat();
		//qDebug() << i << texnum << sides[i].texCoords[0] << sides[i].texCoords[1] << sides[i].texCoords[2] << sides[i].texCoords[3];

		// Prior to precomputing the sides, we used to match E to side0
		// in r4598 the precomputing was put in place and caused a problem for
		// old_style landscapes which had a z rotation on the side textures
		// and where side0 did not map to tex0
		// texToSide is a nasty hack to replace the old behaviour
		texToSide[i] = texnum;
	}

	nbDecorRepeat = landscapeIni.value("landscape/nb_decor_repeat", 1).toInt();

	QString groundTexName = landscapeIni.value("landscape/groundtex").toString();
	groundTexPath = getTexturePath(groundTexName, landscapeId);
	QString description = landscapeIni.value("landscape/ground").toString();
	//sscanf(description.toLocal8Bit(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	QStringList parameters = description.split(':');
	groundTexCoord.texCoords[0] = parameters.at(1).toFloat();
	groundTexCoord.texCoords[1] = parameters.at(2).toFloat();
	groundTexCoord.texCoords[2] = parameters.at(3).toFloat();
	groundTexCoord.texCoords[3] = parameters.at(4).toFloat();

	QString fogTexName = landscapeIni.value("landscape/fogtex").toString();
	fogTexPath = getTexturePath(fogTexName, landscapeId);
	description = landscapeIni.value("landscape/fog").toString();
	//sscanf(description.toLocal8Bit(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	parameters = description.split(':');
	fogTexCoord.texCoords[0] = parameters.at(1).toFloat();
	fogTexCoord.texCoords[1] = parameters.at(2).toFloat();
	fogTexCoord.texCoords[2] = parameters.at(3).toFloat();
	fogTexCoord.texCoords[3] = parameters.at(4).toFloat();

	fogAltAngle        = landscapeIni.value("landscape/fog_alt_angle", 0.).toFloat();
	fogAngleShift      = landscapeIni.value("landscape/fog_angle_shift", 0.).toFloat();
	decorAltAngle      = landscapeIni.value("landscape/decor_alt_angle", 0.).toFloat();
	decorAngleShift    = landscapeIni.value("landscape/decor_angle_shift", 0.).toFloat();
	angleRotateZ       = landscapeIni.value("landscape/decor_angle_rotatez", 0.).toFloat();
	groundAngleShift   = landscapeIni.value("landscape/ground_angle_shift", 0.).toFloat();
	groundAngleRotateZ = landscapeIni.value("landscape/ground_angle_rotatez", 0.).toFloat();
	drawGroundFirst    = landscapeIni.value("landscape/draw_ground_first", 0).toInt();
	tanMode            = landscapeIni.value("landscape/tan_mode", false).toBool();
	calibrated         = landscapeIni.value("landscape/calibrated", false).toBool();
}

void LandscapeOldStyle::draw(StelCore* core, StelRenderer* renderer)
{
	if (!validLandscape) {return;}

	lazyInitTextures(renderer);
	renderer->setBlendMode(BlendMode_Alpha);
	StelProjectorP projector =
		core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	renderer->setCulledFaces(CullFace_Back);
	if (drawGroundFirst) {drawGround(core, renderer);}
	drawDecor(core, renderer);
	if (!drawGroundFirst) {drawGround(core, renderer);}
	drawFog(core, renderer);
	renderer->setCulledFaces(CullFace_None);
}

// Draw the horizon fog
void LandscapeOldStyle::drawFog(StelCore* core, StelRenderer* renderer)
{
	if (!fogFader.getInterstate()) {return;}

	const float vpos = radius * ((tanMode || calibrated) 
	                          ? std::tan(fogAngleShift * M_PI / 180.0) 
	                          : std::sin(fogAngleShift * M_PI / 180.0));

	StelProjector::ModelViewTranformP transform =
		core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transform->combine(Mat4d::translation(Vec3d(0.,0.,vpos)));

	StelProjectorP projector = core->getProjection(transform);
	renderer->setBlendMode(BlendMode_Add);

	const float nightModeFilter = 
		StelApp::getInstance().getVisionModeNight() ? 0.f : 1.f;

	const float intensity = fogFader.getInterstate() * (0.1f + 0.1f * skyBrightness);
	const float filteredIntensity = intensity * nightModeFilter;
	renderer->setGlobalColor(intensity, filteredIntensity, filteredIntensity);
	fogTex->bind();
	const float height = (tanMode || calibrated) 
	                   ? radius * std::tan(fogAltAngle * M_PI / 180.f) 
	                   : radius * std::sin(fogAltAngle * M_PI / 180.f);

	if(std::fabs(height - previousFogHeight) > 0.01)
	{
		// Height has changed, need to regenerate the buffer.
		delete fogCylinderBuffer;
		fogCylinderBuffer = NULL;
		previousFogHeight = height;
	}

	if(NULL == fogCylinderBuffer)
	{
		fogCylinderBuffer = renderer->createVertexBuffer<VertexP3T2>(PrimitiveType_TriangleStrip);
		StelGeometryBuilder().buildCylinder(fogCylinderBuffer, radius, height, 64, true);
	}

	renderer->drawVertexBuffer(fogCylinderBuffer, NULL, projector);

	renderer->setBlendMode(BlendMode_Alpha);
}

void LandscapeOldStyle::lazyInitTextures(StelRenderer* renderer)
{
	if(texturesInitialized){return;}
	for (int i = 0; i < nbSideTexs; ++i)
	{
		sideTexs[i].texture = renderer->createTexture(sideTexs[i].path);
	}
	for (int i=0;i<nbSide;++i)
	{
		sides[i].tex = sideTexs[i].texture;
	}
	fogTex = renderer->createTexture(fogTexPath, 
		                              TextureParams().generateMipmaps().wrap(TextureWrap_Repeat));
	fogTexCoord.tex = fogTex;
	groundTex = renderer->createTexture(groundTexPath, TextureParams().generateMipmaps());
	groundTexCoord.tex = groundTex;
	texturesInitialized = true;
}

void LandscapeOldStyle::generatePrecomputedSides(StelRenderer* renderer)
{
	// Make slicesPerSide=(3<<K) so that the innermost polygon of the fandisk becomes a triangle:
	const int slicesPerSide = std::max(1, 3 * 64 / (nbDecorRepeat * nbSide));

	// Precompute the vertex arrays for side display
	static const int stacks = (calibrated ? 16 : 8); // GZ: 8->16, I need better precision.
	const double z0 = calibrated ?
	// GZ: For calibrated, we use z=decorAngleShift...(decorAltAngle-decorAngleShift), but we must compute the tan in the loop.
	decorAngleShift : (tanMode ? radius * std::tan(decorAngleShift*M_PI/180.f) : radius * std::sin(decorAngleShift*M_PI/180.f));
	// GZ: The old formula is completely meaningless for photos with opening angle >90,
	// and most likely also not what was intended for other images.
	// Note that GZ fills this value with a different meaning!
	const double d_z = calibrated ? decorAltAngle/stacks : (tanMode ? radius*std::tan(decorAltAngle*M_PI/180.f)/stacks : radius*std::sin(decorAltAngle*M_PI/180.0)/stacks);

	const float alpha = 2.f * M_PI / (nbDecorRepeat * nbSide * slicesPerSide);
	const float ca = std::cos(alpha);
	const float sa = std::sin(alpha);
	float y0 = radius*std::cos(angleRotateZ*M_PI/180.f);
	float x0 = radius*std::sin(angleRotateZ*M_PI/180.f);

	LOSSide precompSide;
	for (int n = 0; n < nbDecorRepeat; n++)
	{
		for (int i = 0; i < nbSide; i++)
		{
			if (!texToSide.contains(i))
			{
				qDebug() << QString("LandscapeOldStyle::load ERROR: found no corresponding tex value for side%1").arg(i);
				break;
			}

			const int ti = texToSide[i];
			precompSide.vertices = 
				renderer->createVertexBuffer<VertexP3T2>(PrimitiveType_Triangles);
			precompSide.indices = renderer->createIndexBuffer(IndexType_U16);
			precompSide.tex = sideTexs[ti].texture;

			float tx0 = sides[ti].texCoords[0];
			const float d_tx0 = (sides[ti].texCoords[2] - sides[ti].texCoords[0]) / slicesPerSide;
			const float d_ty  = (sides[ti].texCoords[3] - sides[ti].texCoords[1]) / stacks;
			for (int j = 0; j < slicesPerSide; j++)
			{
				const float y1  = y0 * ca - x0 * sa;
				const float x1  = y0 * sa + x0 * ca;
				const float tx1 = tx0 + d_tx0;

				float z   = z0;
				float ty0 = sides[ti].texCoords[1];

				for (int k = 0; k <= stacks * 2; k += 2)
				{

					const float calibratedZ =
						calibrated ? radius * std::tan(z * M_PI / 180.0f) : z;
					precompSide.vertices->addVertex
						(VertexP3T2(Vec3f(x0, y0, calibratedZ), Vec2f(tx0, ty0)));
					precompSide.vertices->addVertex
						(VertexP3T2(Vec3f(x1, y1, calibratedZ), Vec2f(tx1, ty0)));
					z   += d_z;
					ty0 += d_ty;
				}
				const uint offset = j*(stacks+1)*2;
				for (int k = 2; k < stacks * 2 + 2; k += 2)
				{
					precompSide.indices->addIndex(offset + k - 2);
					precompSide.indices->addIndex(offset + k - 1);
					precompSide.indices->addIndex(offset + k);
					precompSide.indices->addIndex(offset + k);
					precompSide.indices->addIndex(offset + k - 1);
					precompSide.indices->addIndex(offset + k + 1);
				}
				y0  = y1;
				x0  = x1;
				tx0 = tx1;
			}
			precompSide.vertices->lock();
			precompSide.indices->lock();
			precomputedSides.append(precompSide);
		}
	}
}

// Draw the mountains with a few pieces of texture
void LandscapeOldStyle::drawDecor(StelCore* core, StelRenderer* renderer) 
{
	if (!landFader.getInterstate()) {return;}

	// Patched by Georg Zotti: I located an undocumented switch tan_mode, maybe tan_mode=true means cylindrical panorama projection.
	// anyway, the old code makes unfortunately no sense.
	// I added a switch "calibrated" for the ini file. If true, it works as this landscape apparently was originally intended.
	// So I corrected the texture coordinates so that decorAltAngle is the total angle, decorAngleShift the lower angle,
	// and the texture in between is correctly stretched.
	// TODO: (1) Replace fog cylinder by similar texture, which could be painted as image layer in Photoshop/Gimp.
	//       (2) Implement calibrated && tan_mode
	StelProjector::ModelViewTranformP transform =
		core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transform->combine(Mat4d::zrotation(-angleRotateZOffset*M_PI/180.f));
	StelProjectorP projector = core->getProjection(transform);

	const Vec4f color = StelApp::getInstance().getVisionModeNight() 
	                  ? Vec4f(skyBrightness*nightBrightness, 0.0, 0.0, landFader.getInterstate())
	                  : Vec4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());
	renderer->setGlobalColor(color);

	// Lazily generate decoration sides.
	if(precomputedSides.empty())
	{
		generatePrecomputedSides(renderer);
	}

	// Draw decoration sides.
	foreach (const LOSSide& side, precomputedSides)
	{
		side.tex->bind();
		renderer->drawVertexBuffer(side.vertices, side.indices, projector);
	}
}

void LandscapeOldStyle::generateGroundFanDisk(StelRenderer* renderer)
{
	// Precompute the vertex buffer for ground display
	// Make slicesPerSide = (3<<K) so that the 
	// innermost polygon of the fandisk becomes a triangle:
	int slicesPerSide = std::max(1, 3 * 64 / (nbDecorRepeat * nbSide));

	// draw a fan disk instead of a ordinary disk so that the inner slices
	// are not so slender. When they are too slender, culling errors occur
	// in cylinder projection mode.
	int slicesInside = nbSide * slicesPerSide * nbDecorRepeat;
	int level = 0;
	while ((slicesInside & 1) == 0 && slicesInside > 4)
	{
		++level;
		slicesInside >>= 1;
	}

	groundFanDisk = renderer->createVertexBuffer<VertexP3T2>(PrimitiveType_Triangles);
	groundFanDiskIndices = renderer->createIndexBuffer(IndexType_U16);
	StelGeometryBuilder()
		.buildFanDisk(groundFanDisk, groundFanDiskIndices, radius, slicesInside, level);
}

// Draw the ground
void LandscapeOldStyle::drawGround(StelCore* core, StelRenderer* renderer)
{
	if (!landFader.getInterstate()) {return;}

	const float vshift = (tanMode || calibrated) 
	                   ? radius * std::tan(groundAngleShift * M_PI / 180.0f) 
	                   : radius * std::sin(groundAngleShift * M_PI / 180.0f);

	StelProjector::ModelViewTranformP transform =
		core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transform->combine
		(Mat4d::zrotation((groundAngleRotateZ - angleRotateZOffset) * M_PI / 180.0f) *
		 Mat4d::translation(Vec3d(0.0, 0.0, vshift)));

	StelProjectorP projector = core->getProjection(transform);

	const Vec4f color = StelApp::getInstance().getVisionModeNight()
	                  ? Vec4f(skyBrightness*nightBrightness, 0.0, 0.0, landFader.getInterstate())
	                  : Vec4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());
	renderer->setGlobalColor(color);

	groundTex->bind();

	// Lazily generate the ground fan disk.
	if(NULL == groundFanDisk)
	{
		Q_ASSERT_X(NULL == groundFanDiskIndices, Q_FUNC_INFO, 
		           "Vertex buffer is NULL but index buffer is already generated");
		generateGroundFanDisk(renderer);
	}

	// Draw the ground.
	renderer->drawVertexBuffer(groundFanDisk, groundFanDiskIndices, projector);
}

LandscapeFisheye::LandscapeFisheye(float _radius) 
	: Landscape(_radius)
	, mapTex(NULL)
	, fisheyeSphere(NULL)
{}

LandscapeFisheye::~LandscapeFisheye()
{
	if(NULL != fisheyeSphere)
	{
		delete fisheyeSphere;
	}
	if(NULL != mapTex)
	{
		delete mapTex;
	}
}

void LandscapeFisheye::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);

	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "fisheye")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId << ", expected fisheye, found " << type << ".  No landscape in use.\n";
		validLandscape = 0;
		return;
	}
	create(name, getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
		landscapeIni.value("landscape/texturefov", 360).toFloat(),
		landscapeIni.value("landscape/angle_rotatez", 0.).toFloat());
}


// create a fisheye landscape from basic parameters (no ini file needed)
void LandscapeFisheye::create(const QString _name, const QString& _maptex, float atexturefov, float aangleRotateZ)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = 1;  // assume ok...
	name = _name;
	mapTexPath = _maptex;
	texFov = atexturefov*M_PI/180.f;
	angleRotateZ = aangleRotateZ*M_PI/180.f;

	const SphereParams params = SphereParams(radius).resolution(cols, rows).orientInside();
	fisheyeSphere = StelGeometryBuilder().buildSphereFisheye(params, texFov);
}

void LandscapeFisheye::draw(StelCore* core, StelRenderer* renderer)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	if(NULL == mapTex)
	{
		mapTex = renderer->createTexture(mapTexPath, TextureParams().generateMipmaps());
	}

	StelProjector::ModelViewTranformP transform = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transform->combine(Mat4d::zrotation(-(angleRotateZ+(angleRotateZOffset*M_PI/180.))));
	const StelProjectorP projector = core->getProjection(transform);
	const Vec4f color = StelApp::getInstance().getVisionModeNight()
	                  ? Vec4f(skyBrightness*nightBrightness, 0.0, 0.0, landFader.getInterstate())
	                  : Vec4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());
	renderer->setGlobalColor(color);
	renderer->setCulledFaces(CullFace_Back);
	renderer->setBlendMode(BlendMode_Alpha);
	mapTex->bind();

	fisheyeSphere->draw(renderer, projector);

	renderer->setCulledFaces(CullFace_None);
}


// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius) 
	: Landscape(_radius)
	, mapTex(NULL)
{
}

LandscapeSpherical::~LandscapeSpherical()
{
	if(NULL != landscapeSphere)
	{
		delete landscapeSphere;
		landscapeSphere = NULL;
	}
	if(NULL != mapTex)
	{
		delete mapTex;
	}
}

void LandscapeSpherical::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);

	QString type = landscapeIni.value("landscape/type").toString();
	if (type != "spherical")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId
			<< ", expected spherical, found " << type
			<< ".  No landscape in use.\n";
		validLandscape = 0;
		return;
	}

	create(name, getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
		landscapeIni.value("landscape/angle_rotatez", 0.f).toFloat());
}


// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const QString _name, const QString& _maptex, float _angleRotateZ)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = 1;  // assume ok...
	name = _name;
	angleRotateZ = _angleRotateZ*M_PI/180.f;
	mapTexPath = _maptex;

	const SphereParams params 
		= SphereParams(radius).resolution(64, 48).orientInside().flipTexture();
	landscapeSphere = StelGeometryBuilder().buildSphereUnlit(params);
}


void LandscapeSpherical::draw(StelCore* core, StelRenderer* renderer)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;
	if(NULL == mapTex)
	{
		mapTex = renderer->createTexture(mapTexPath, TextureParams().generateMipmaps());
	}

	StelProjector::ModelViewTranformP transform = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transform->combine(Mat4d::zrotation(-(angleRotateZ+(angleRotateZOffset*M_PI/180.))));
	const StelProjectorP projector = core->getProjection(transform);

	renderer->setBlendMode(BlendMode_Alpha);

	const Vec4f color = StelApp::getInstance().getVisionModeNight()
	                  ? Vec4f(skyBrightness*nightBrightness, 0.0, 0.0, landFader.getInterstate())
	                  : Vec4f(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());
	renderer->setGlobalColor(color);
	renderer->setCulledFaces(CullFace_Back);
	mapTex->bind();

	landscapeSphere->draw(renderer, projector);
	// TODO: verify that this works correctly for custom projections
	// seam is at East
	// GZ: Want better angle resolution, optional!
	renderer->setCulledFaces(CullFace_None);
}
