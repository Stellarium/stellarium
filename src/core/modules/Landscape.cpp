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
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocation.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"

#include <QDebug>
#include <QSettings>
#include <QVarLengthArray>

Landscape::Landscape(float _radius) : radius(_radius), skyBrightness(1.), angleRotateZOffset(0.)
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

LandscapeOldStyle::LandscapeOldStyle(float _radius) : Landscape(_radius), sideTexs(NULL), sides(NULL), tanMode(false), calibrated(false)
{}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (sideTexs)
	{
		delete [] sideTexs;
		sideTexs = NULL;
	}

	if (sides) delete [] sides;
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
	sideTexs = new StelTextureSP[nbSideTexs];
	for (int i=0; i<nbSideTexs; ++i)
	{
		QString textureKey = QString("landscape/tex%1").arg(i);
		QString textureName = landscapeIni.value(textureKey).toString();
		const QString texturePath = getTexturePath(textureName, landscapeId);
		sideTexs[i] = StelApp::getInstance().getTextureManager().createTexture(texturePath);
	}

	QMap<int, int> texToSide;
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
		sides[i].tex = sideTexs[texnum];
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
	QString groundTexPath = getTexturePath(groundTexName, landscapeId);
	groundTex = StelApp::getInstance().getTextureManager().createTexture(groundTexPath, StelTexture::StelTextureParams(true));
	QString description = landscapeIni.value("landscape/ground").toString();
	//sscanf(description.toLocal8Bit(),"groundtex:%f:%f:%f:%f",&a,&b,&c,&d);
	QStringList parameters = description.split(':');
	groundTexCoord.tex = groundTex;
	groundTexCoord.texCoords[0] = parameters.at(1).toFloat();
	groundTexCoord.texCoords[1] = parameters.at(2).toFloat();
	groundTexCoord.texCoords[2] = parameters.at(3).toFloat();
	groundTexCoord.texCoords[3] = parameters.at(4).toFloat();

	QString fogTexName = landscapeIni.value("landscape/fogtex").toString();
	QString fogTexPath = getTexturePath(fogTexName, landscapeId);
	fogTex = StelApp::getInstance().getTextureManager().createTexture(fogTexPath, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
	description = landscapeIni.value("landscape/fog").toString();
	//sscanf(description.toLocal8Bit(),"fogtex:%f:%f:%f:%f",&a,&b,&c,&d);
	parameters = description.split(':');
	fogTexCoord.tex = fogTex;
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

	// Precompute the vertex arrays for ground display
	// Make slices_per_side=(3<<K) so that the innermost polygon of the fandisk becomes a triangle:
	int slices_per_side = 3*64/(nbDecorRepeat*nbSide);
	if (slices_per_side<=0)
		slices_per_side = 1;

	// draw a fan disk instead of a ordinary disk to that the inner slices
	// are not so slender. When they are too slender, culling errors occur
	// in cylinder projection mode.
	int slices_inside = nbSide*slices_per_side*nbDecorRepeat;
	int level = 0;
	while ((slices_inside&1)==0 && slices_inside > 4)
	{
		++level;
		slices_inside>>=1;
	}
	StelPainter::computeFanDisk(radius, slices_inside, level, groundVertexArr, groundTexCoordArr);


	// Precompute the vertex arrays for side display
	static const int stacks = (calibrated ? 16 : 8); // GZ: 8->16, I need better precision.
	const double z0 = calibrated ?
	// GZ: For calibrated, we use z=decorAngleShift...(decorAltAngle-decorAngleShift), but we must compute the tan in the loop.
	decorAngleShift : (tanMode ? radius * std::tan(decorAngleShift*M_PI/180.f) : radius * std::sin(decorAngleShift*M_PI/180.f));
	// GZ: The old formula is completely meaningless for photos with opening angle >90,
	// and most likely also not what was intended for other images.
	// Note that GZ fills this value with a different meaning!
	const double d_z = calibrated ? decorAltAngle/stacks : (tanMode ? radius*std::tan(decorAltAngle*M_PI/180.f)/stacks : radius*std::sin(decorAltAngle*M_PI/180.0)/stacks);

	const float alpha = 2.f*M_PI/(nbDecorRepeat*nbSide*slices_per_side);
	const float ca = std::cos(alpha);
	const float sa = std::sin(alpha);
	float y0 = radius*std::cos(angleRotateZ*M_PI/180.f);
	float x0 = radius*std::sin(angleRotateZ*M_PI/180.f);

	LOSSide precompSide;
	precompSide.arr.primitiveType=StelVertexArray::Triangles;
	for (int n=0;n<nbDecorRepeat;n++)
	{
		for (int i=0;i<nbSide;i++)
		{
			int ti;
			if (texToSide.contains(i))
				ti = texToSide[i];
			else
			{
				qDebug() << QString("LandscapeOldStyle::load ERROR: found no corresponding tex value for side%1").arg(i);
				break;
			}
			precompSide.arr.vertex.resize(0);
			precompSide.arr.texCoords.resize(0);
			precompSide.arr.indices.resize(0);
			precompSide.tex=sideTexs[ti];

			float tx0 = sides[ti].texCoords[0];
			const float d_tx0 = (sides[ti].texCoords[2]-sides[ti].texCoords[0]) / slices_per_side;
			const float d_ty = (sides[ti].texCoords[3]-sides[ti].texCoords[1]) / stacks;
			for (int j=0;j<slices_per_side;j++)
			{
				const float y1 = y0*ca - x0*sa;
				const float x1 = y0*sa + x0*ca;
				const float tx1 = tx0 + d_tx0;
				float z = z0;
				float ty0 = sides[ti].texCoords[1];
				for (int k=0;k<=stacks*2;k+=2)
				{
					precompSide.arr.texCoords << Vec2f(tx0, ty0) << Vec2f(tx1, ty0);
					if (calibrated)
					{
						float tanZ=radius * std::tan(z*M_PI/180.f);
						precompSide.arr.vertex << Vec3d(x0, y0, tanZ) << Vec3d(x1, y1, tanZ);
					} else
					{
						precompSide.arr.vertex << Vec3d(x0, y0, z) << Vec3d(x1, y1, z);
					}
					z += d_z;
					ty0 += d_ty;
				}
				unsigned int offset = j*(stacks+1)*2;
				for (int k = 2;k<stacks*2+2;k+=2)
				{
					precompSide.arr.indices << offset+k-2 << offset+k-1 << offset+k;
					precompSide.arr.indices << offset+k << offset+k-1 << offset+k+1;
				}
				y0 = y1;
				x0 = x1;
				tx0 = tx1;
			}
			precomputedSides.append(precompSide);
		}
	}
}

void LandscapeOldStyle::draw(StelCore* core)
{
	StelPainter painter(core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	painter.enableTexture2d(true);
	glEnable(GL_CULL_FACE);

	if (!validLandscape)
		return;
	if (drawGroundFirst)
		drawGround(core, painter);
	drawDecor(core, painter);
	if (!drawGroundFirst)
		drawGround(core, painter);
	drawFog(core, painter);
}


// Draw the horizon fog
void LandscapeOldStyle::drawFog(StelCore* core, StelPainter& sPainter) const
{
	if (!fogFader.getInterstate())
		return;

	const float vpos = (tanMode||calibrated) ? radius*std::tan(fogAngleShift*M_PI/180.) : radius*std::sin(fogAngleShift*M_PI/180.);
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::translation(Vec3d(0.,0.,vpos)));
	sPainter.setProjector(core->getProjection(transfo));
	glBlendFunc(GL_ONE, GL_ONE);
	const float nightModeFilter = StelApp::getInstance().getVisionModeNight() ? 0.f : 1.f;
	sPainter.setColor(fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
			  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness)*nightModeFilter,
			  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness)*nightModeFilter);
	fogTex->bind();
	const float height = (tanMode||calibrated) ? radius*std::tan(fogAltAngle*M_PI/180.) : radius*std::sin(fogAltAngle*M_PI/180.);
	sPainter.sCylinder(radius, height, 64, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Draw the mountains with a few pieces of texture
void LandscapeOldStyle::drawDecor(StelCore* core, StelPainter& sPainter) const
{

	// Patched by Georg Zotti: I located an undocumented switch tan_mode, maybe tan_mode=true means cylindrical panorama projection.
	// anyway, the old code makes unfortunately no sense.
	// I added a switch "calibrated" for the ini file. If true, it works as this landscape apparently was originally intended.
	// So I corrected the texture coordinates so that decorAltAngle is the total angle, decorAngleShift the lower angle,
	// and the texture in between is correctly stretched.
	// TODO: (1) Replace fog cylinder by similar texture, which could be painted as image layer in Photoshop/Gimp.
	//       (2) Implement calibrated && tan_mode
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-angleRotateZOffset*M_PI/180.f));

	sPainter.setProjector(core->getProjection(transfo));

	if (!landFader.getInterstate())
		return;
	const float nightModeFilter = StelApp::getInstance().getVisionModeNight() ? 0.f : 1.f;
	sPainter.setColor(skyBrightness, skyBrightness*nightModeFilter, skyBrightness*nightModeFilter, landFader.getInterstate());

	foreach (const LOSSide& side, precomputedSides)
	{
		side.tex->bind();
		sPainter.drawSphericalTriangles(side.arr, true, NULL, false);
	}
}


// Draw the ground
void LandscapeOldStyle::drawGround(StelCore* core, StelPainter& sPainter) const
{
	if (!landFader.getInterstate())
		return;
	const float vshift = (tanMode || calibrated) ?
	  radius*std::tan(groundAngleShift*M_PI/180.) :
	  radius*std::sin(groundAngleShift*M_PI/180.);
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation((groundAngleRotateZ-angleRotateZOffset)*M_PI/180.f) * Mat4d::translation(Vec3d(0,0,vshift)));

	sPainter.setProjector(core->getProjection(transfo));
	float nightModeFilter = StelApp::getInstance().getVisionModeNight() ? 0.f : 1.f;
	sPainter.setColor(skyBrightness, skyBrightness*nightModeFilter, skyBrightness*nightModeFilter, landFader.getInterstate());
	groundTex->bind();
	sPainter.setArrays((Vec3d*)groundVertexArr.constData(), (Vec2f*)groundTexCoordArr.constData());
	sPainter.drawFromArray(StelPainter::Triangles, groundVertexArr.size()/3);
}

LandscapeFisheye::LandscapeFisheye(float _radius) : Landscape(_radius)
{}

LandscapeFisheye::~LandscapeFisheye()
{
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
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex, StelTexture::StelTextureParams(true));
	texFov = atexturefov*M_PI/180.f;
	angleRotateZ = aangleRotateZ*M_PI/180.f;
}


void LandscapeFisheye::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+(angleRotateZOffset*M_PI/180.))));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float nightModeFilter = StelApp::getInstance().getVisionModeNight() ? 0.f : 1.f;
	sPainter.setColor(skyBrightness, skyBrightness*nightModeFilter, skyBrightness*nightModeFilter, landFader.getInterstate());

	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	mapTex->bind();
	// Patch GZ: (40,20)->(cols,rows)
	sPainter.sSphereMap(radius,cols,rows,texFov,1);

	glDisable(GL_CULL_FACE);
}


// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius) : Landscape(_radius)
{}

LandscapeSpherical::~LandscapeSpherical()
{
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
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex, StelTexture::StelTextureParams(true));
	angleRotateZ = _angleRotateZ*M_PI/180.f;
}


void LandscapeSpherical::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+(angleRotateZOffset*M_PI/180.))));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float nightModeFilter = StelApp::getInstance().getVisionModeNight() ? 0. : 1.;
	sPainter.setColor(skyBrightness, skyBrightness*nightModeFilter, skyBrightness*nightModeFilter, landFader.getInterstate());

	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	mapTex->bind();

	// TODO: verify that this works correctly for custom projections
	// seam is at East
	//sPainter.sSphere(radius, 1.0, 40, 20, 1, true);
	// GZ: Want better angle resolution, optional!
	sPainter.sSphere(radius, 1.0, cols, rows, 1, true);

	glDisable(GL_CULL_FACE);
}
