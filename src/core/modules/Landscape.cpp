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
#include <QFile>
#include <QDir>

Landscape::Landscape(float _radius) : radius(_radius), skyBrightness(1.), nightBrightness(0.8), angleRotateZOffset(0.), horizonPolygon(NULL)
{
	validLandscape = 0;
}

Landscape::~Landscape()
{}


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
		qWarning() << "No valid landscape definition (no name) found for landscape ID "
			<< landscapeId << ". No landscape in use." << endl;
		validLandscape = 0;
		return;
	}
	else
	{
		validLandscape = 1;
	}

	// Optional data
	rows = landscapeIni.value("landscape/tesselate_rows", 20).toInt();
	cols = landscapeIni.value("landscape/tesselate_cols", 40).toInt();

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
	defaultBortleIndex = landscapeIni.value("location/light_pollution", -1).toInt();
	if (defaultBortleIndex<=0) defaultBortleIndex=-1; // neg. values in ini file signal "no change".
	if (defaultBortleIndex>9) defaultBortleIndex=9; // correct bad values.

	defaultFogSetting = landscapeIni.value("location/display_fog", -1).toInt();
	defaultExtinctionCoefficient = landscapeIni.value("location/atmospheric_extinction_coefficient", -1.0).toDouble();
	defaultTemperature = landscapeIni.value("location/atmospheric_temperature", -1000.0).toDouble();
	defaultPressure = landscapeIni.value("location/atmospheric_pressure", -2.0).toDouble(); // -2=no change! [-1=computeFromAltitude]
	// Set minimal brightness for landscape
	minBrightness = landscapeIni.value("landscape/minimal_brightness", -1.0).toDouble();

	// This is now optional for all classes, for mixing with a photo horizon:
	// they may have different offsets, like a south-centered pano and a geographically-oriented polygon.
	// In case they are aligned, we can use one value angle_rotatez, or define the polygon rotation individually.
	if (landscapeIni.contains("landscape/polygonal_horizon_list"))
	{
		createPolygonalHorizon(
					StelFileMgr::findFile("landscapes/" + landscapeId + "/" + landscapeIni.value("landscape/polygonal_horizon_list").toString()),
					landscapeIni.value("landscape/polygonal_angle_rotatez", 0.f).toFloat());
		// This line can then be drawn in all classes with the color specified here. If not specified, don't draw it! (flagged by negative red)
		horizonPolygonLineColor=StelUtils::strToVec3f( landscapeIni.value("landscape/horizon_line_color", "-1,0,0" ).toString());
	}
}

void Landscape::createPolygonalHorizon(const QString& _lineFileName, const float _polyAngleRotateZ)
{
	// qDebug() << _name << " " << _fullpath << " " << _lineFileName ;

	QVector<Vec3d> horiPoints(0);
	QFile file(_lineFileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Landscape Horizon line data file" << QDir::toNativeSeparators(_lineFileName) << "not found.";
		return;
	}
	QRegExp emptyLine("^\\s*$");
	QTextStream in(&file);
	while (!in.atEnd())
	{
		// Build list of vertices. The checks can certainly become more robust.
		QString line = in.readLine();
		//line=line.trimmed(); // crash?
		if (line.length()==0) continue;
		if (emptyLine.exactMatch((line))) continue;
		if (line.at(0)=='#') continue; // skip comment lines.
		//if (line.isEmpty()) continue;
		QStringList list = line.split(QRegExp("\\b\\s+\\b"));
		if (list.count() != 2)
		{
			qWarning() << "Landscape polygon file" << QDir::toNativeSeparators(_lineFileName) << "has bad line:" << line << "with" << list.count() << "elements";
			continue;
		}
		Vec3d point;
		//qDebug() << "Creating point for az=" << list.at(0) << " alt=" << list.at(1);
		StelUtils::spheToRect((180.0f - _polyAngleRotateZ - list.at(0).toFloat())*M_PI/180.f, list.at(1).toFloat()*M_PI/180.f, point);
		horiPoints.append(point);
	}
	qDebug() << "closing file" ;
	file.close();
	horiPoints.append(horiPoints.at(0)); // close loop? Apparently not necessary.

	//qDebug() << "created horiPoints with " << horiPoints.count() << "points:";
	//for (int i=0; i<horiPoints.count(); ++i)
	//	qDebug() << horiPoints.at(i)[0] << "/" << horiPoints.at(i)[1] << "/" << horiPoints.at(i)[2] ;
	AllSkySphericalRegion allskyRegion;
	SphericalPolygon aboveHorizonPolygon;
	aboveHorizonPolygon.setContour(horiPoints);
	horizonPolygon = allskyRegion.getSubtraction(aboveHorizonPolygon);
}

#include <iostream>
const QString Landscape::getTexturePath(const QString& basename, const QString& landscapeId)
{
	// look in the landscape directory first, and if not found default to global textures directory
	QString path = StelFileMgr::findFile("landscapes/" + landscapeId + "/" + basename);
	if (path.isEmpty())
		path = StelFileMgr::findFile("textures/" + basename);
	return path;
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

	rows = landscapeIni.value("landscape/tesselate_rows", 8).toInt();
	cols = landscapeIni.value("landscape/tesselate_cols", 16).toInt();

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
	angleRotateZ       = landscapeIni.value("landscape/decor_angle_rotatez", 0.).toFloat() * M_PI/180.f;
	groundAngleShift   = landscapeIni.value("landscape/ground_angle_shift", 0.).toFloat() * M_PI/180.f;
	groundAngleRotateZ = landscapeIni.value("landscape/ground_angle_rotatez", 0.).toFloat() * M_PI/180.f;
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
	float y0 = radius*std::cos(angleRotateZ);
	float x0 = radius*std::sin(angleRotateZ);

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
	sPainter.setColor(fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
			  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
			  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness));
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
	// note that angleRotateZOffset is always zero unless changed during runtime by LandscapeMgr::setZRotation().
	transfo->combine(Mat4d::zrotation(-angleRotateZOffset));

	sPainter.setProjector(core->getProjection(transfo));

	if (!landFader.getInterstate())
		return;
	sPainter.setColor(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());

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
	  radius*std::tan(groundAngleShift) :
	  radius*std::sin(groundAngleShift);
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(groundAngleRotateZ-angleRotateZOffset) * Mat4d::translation(Vec3d(0,0,vshift)));

	sPainter.setProjector(core->getProjection(transfo));
	sPainter.setColor(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());

	groundTex->bind();
	sPainter.setArrays((Vec3d*)groundVertexArr.constData(), (Vec2f*)groundTexCoordArr.constData());
	sPainter.drawFromArray(StelPainter::Triangles, groundVertexArr.size()/3);
}

float LandscapeOldStyle::getOpacity(const Vec3d azalt) const
{
	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)	) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...

	const float alt_rad = std::asin(azalt[2]);  // sampled altitude, radians
	if (alt_rad < decorAngleShift*M_PI/180.0f) return 1.0f; // below decor, i.e. certainly soil.
	if (alt_rad > (decorAltAngle+decorAngleShift)*M_PI/180.0f) return 0.0f; // above decor, i.e. certainly sky.

	// GZ TODO: Implement properly...

	if (alt_rad < 0) return 1.0f; else return 0.0f; // signal for now: better use math. horizon.

//	float az=atan2(azalt[0], azalt[1]) + M_PI/2; // -pi/2..+3pi/2, real azimuth. NESW



}

////////////////////////////////////////////////////////////////////////////////////////////////////

LandscapePolygonal::LandscapePolygonal(float _radius) : Landscape(_radius)
{}

LandscapePolygonal::~LandscapePolygonal()
{}

void LandscapePolygonal::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// loading the polygon has been moved to Landscape::loadCommon(), so that all Landscape classes can use a polygon line.

	loadCommon(landscapeIni, landscapeId);
	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "polygonal")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId << ", expected polygonal, found " << type << ".  No landscape in use.\n";
		validLandscape = 0;
		return;
	}

	if (horizonPolygon.isNull())
	{
		qWarning() << "Landscape " << landscapeId << " does not declare a valid polygonal_horizon_list.  No landscape in use.\n";
		validLandscape = 0;
		return;
	}

	horizonPolygonGroundColor=StelUtils::strToVec3f( landscapeIni.value("landscape/ground_color", "0,0,0" ).toString() );
	validLandscape = 1;  // assume ok...
}

void LandscapePolygonal::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-angleRotateZOffset));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode for the transition blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	sPainter.setColor(skyBrightness*horizonPolygonGroundColor[0], skyBrightness*horizonPolygonGroundColor[1], skyBrightness*horizonPolygonGroundColor[2], landFader.getInterstate());
	sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeFill);

	// This line may be available in all landscape types if horizonPolygon is present. Currently only here:
	// TODO: remove spurious line at begin of the octahedronPolygon's cachedVertexArray.
	if (horizonPolygonLineColor[0] >= 0)
	{
		sPainter.setColor(horizonPolygonLineColor[0], horizonPolygonLineColor[1], horizonPolygonLineColor[2], landFader.getInterstate());
		sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeBoundary);
	}
	glDisable(GL_CULL_FACE);
}

float LandscapePolygonal::getOpacity(const Vec3d azalt) const
{
	//	qDebug() << "Landscape sampling: az=" << (az+angleRotateZ)/M_PI*180.0f << "° alt=" << alt_rad/M_PI*180.f
	//			 << "°, w=" << mapImage->width() << " h=" << mapImage->height()
	//			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
	if (horizonPolygon->contains(azalt)	) return 1.0f; else return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
// LandscapeFisheye
//

LandscapeFisheye::LandscapeFisheye(float _radius) : Landscape(_radius)
{}

LandscapeFisheye::~LandscapeFisheye()
{
	if (mapImage) delete mapImage;
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
	create(name,
		   landscapeIni.value("landscape/texturefov", 360).toFloat(),
		   getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
			getTexturePath(landscapeIni.value("landscape/maptex_fog").toString(), landscapeId),
			getTexturePath(landscapeIni.value("landscape/maptex_illum").toString(), landscapeId),
			landscapeIni.value("landscape/angle_rotatez", 0.f).toFloat());
}


void LandscapeFisheye::create(const QString _name, float _texturefov, const QString& _maptex, const QString &_maptexFog, const QString& _maptexIllum, const float _angleRotateZ)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = 1;  // assume ok...
	name = _name;
	texFov = _texturefov*M_PI/180.f;
	angleRotateZ = _angleRotateZ*M_PI/180.f;
	mapImage = new QImage(_maptex);
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex, StelTexture::StelTextureParams(true));

	if (_maptexIllum.length())
		mapTexIllum = StelApp::getInstance().getTextureManager().createTexture(_maptexIllum, StelTexture::StelTextureParams(true));
	if (_maptexFog.length())
		mapTexFog = StelApp::getInstance().getTextureManager().createTexture(_maptexFog, StelTexture::StelTextureParams(true));

}


void LandscapeFisheye::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.setColor(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());
	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	mapTex->bind();
	sPainter.sSphereMap(radius,cols,rows,texFov,1);
	// GZ: NEW PARTS: Fog also for fisheye...
	if (mapTexFog)
	{
		//glBlendFunc(GL_ONE, GL_ONE); // GZ: Take blending mode as found in the old_style landscapes...
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR); // GZ: better?
		sPainter.setColor(fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
						  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
						  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness), fogFader.getInterstate());
		mapTexFog->bind();
		sPainter.sSphereMap(radius,cols,rows,texFov,1);
	}

	// GZ new:
	if (mapTexIllum && lightScapeBrightness>0.0f)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		sPainter.setColor(lightScapeBrightness, lightScapeBrightness, lightScapeBrightness, landFader.getInterstate());
		mapTexIllum->bind();
		sPainter.sSphereMap(radius, cols, rows, texFov, 1);
	}

	glDisable(GL_CULL_FACE);
}

float LandscapeFisheye::getOpacity(const Vec3d azalt) const
{
	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)	) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...

	// QImage has pixel 0/0 in top left corner.
	// The texture is taken from the center circle in the square texture.
	// It is possible that sample position is outside. in this case, assume full opacity and exit early.
	const float alt_rad = std::asin(azalt[2]);  // sampled altitude, radians
	if (M_PI/2-alt_rad > texFov/2.0 ) return 1.0; // outside fov, in the clamped texture zone: always opaque.

	float radius=(M_PI/2-alt_rad)*2.0f/texFov; // radius in units of mapImage.height/2

	float az=atan2(azalt[0], azalt[1]) + M_PI/2 - angleRotateZ; // -pi/2..+3pi/2, real azimuth. NESW
	//  The texture map has south on top, east at right (if anglerotateZ=0)
	int x= mapImage->height()/2*(1 + radius*std::sin(az));
	int y= mapImage->height()/2*(1 + radius*std::cos(az));

	QRgb pixVal=mapImage->pixel(x, y);
	qDebug() << "Landscape sampling: az=" << (az+angleRotateZ)/M_PI*180.0f << "° alt=" << alt_rad/M_PI*180.f
			 << "°, w=" << mapImage->width() << " h=" << mapImage->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
	return qAlpha(pixVal)/255.0f;


}
/////////////////////////////////////////////////////////////////////////////////////////////////
// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius) : Landscape(_radius)
{}

LandscapeSpherical::~LandscapeSpherical()
{
	if (mapImage) delete mapImage;
}

void LandscapeSpherical::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);
	if (horizonPolygon)
		qDebug() << "This landscape, " << landscapeId << ", has a polygon defined!" ;
	else
		qDebug() << "This landscape, " << landscapeId << ", has no polygon defined!" ;

	QString type = landscapeIni.value("landscape/type").toString();
	if (type != "spherical")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId
			<< ", expected spherical, found " << type
			<< ".  No landscape in use.\n";
		validLandscape = 0;
		return;
	}

	create(name,
		   getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
			getTexturePath(landscapeIni.value("landscape/maptex_fog").toString(), landscapeId),
			getTexturePath(landscapeIni.value("landscape/maptex_illum").toString(), landscapeId),
			landscapeIni.value("landscape/angle_rotatez"      ,   0.f).toFloat(),
			landscapeIni.value("landscape/maptex_top"         ,  90.f).toFloat(),
			landscapeIni.value("landscape/maptex_bottom"      , -90.f).toFloat(),
			landscapeIni.value("landscape/maptex_fog_top"     ,  90.f).toFloat(),
			landscapeIni.value("landscape/maptex_fog_bottom"  , -90.f).toFloat(),
			landscapeIni.value("landscape/maptex_illum_top"   ,  90.f).toFloat(),
			landscapeIni.value("landscape/maptex_illum_bottom", -90.f).toFloat());
}


//// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const QString _name, const QString& _maptex, const QString& _maptexFog, const QString& _maptexIllum, const float _angleRotateZ,
								const float _mapTexTop, const float _mapTexBottom,
								const float _fogTexTop, const float _fogTexBottom,
								const float _illumTexTop, const float _illumTexBottom)
{
	//qDebug() << "LandscapeSpherical::create():"<< _name << " : " << _maptex << " : " << _maptexFog << " : " << _maptexIllum << " : " << _angleRotateZ;
	validLandscape = 1;  // assume ok...
	name = _name;
	angleRotateZ  = _angleRotateZ         *M_PI/180.f; // Defined in ini --> internal prg value
	mapTexTop     = (90.f-_mapTexTop)     *M_PI/180.f; // top     90     -->   0
	mapTexBottom  = (90.f-_mapTexBottom)  *M_PI/180.f; // bottom -90     -->  pi
	fogTexTop     = (90.f-_fogTexTop)     *M_PI/180.f;
	fogTexBottom  = (90.f-_fogTexBottom)  *M_PI/180.f;
	illumTexTop   = (90.f-_illumTexTop)   *M_PI/180.f;
	illumTexBottom= (90.f-_illumTexBottom)*M_PI/180.f;
	mapImage = new QImage(_maptex);
	mapTex = StelApp::getInstance().getTextureManager().createTexture(_maptex, StelTexture::StelTextureParams(true));

	if (_maptexIllum.length())
		mapTexIllum = StelApp::getInstance().getTextureManager().createTexture(_maptexIllum, StelTexture::StelTextureParams(true));
	if (_maptexFog.length())
		mapTexFog = StelApp::getInstance().getTextureManager().createTexture(_maptexFog, StelTexture::StelTextureParams(true));
}

void LandscapeSpherical::draw(StelCore* core)
{
	if(!validLandscape) return;
	if(!landFader.getInterstate()) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.setColor(skyBrightness, skyBrightness, skyBrightness, landFader.getInterstate());

	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	mapTex->bind();

	// TODO: verify that this works correctly for custom projections [comment not by GZ]
	// seam is at East, except if angleRotateZ has been given.
	sPainter.sSphere(radius, 1.0, cols, rows, 1, true, mapTexTop, mapTexBottom);
	// GZ: NEW PARTS:
	// Fog also for sphericals...
	if (mapTexFog)
	{
		//glBlendFunc(GL_ONE, GL_ONE); // GZ: blending mode as found in the old_style landscapes...
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR); // GZ: better?
		sPainter.setColor(fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
						  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness),
						  fogFader.getInterstate()*(0.1f+0.1f*skyBrightness), fogFader.getInterstate());
		mapTexFog->bind();
		sPainter.sSphere(radius, 1.0, cols, (int) ceil(rows*(fogTexTop-fogTexBottom)/(mapTexTop-mapTexBottom)), 1, true, fogTexTop, fogTexBottom);
	}

	// Self-luminous layer (Light pollution etc). This looks striking!
	if (mapTexIllum && lightScapeBrightness>0.0f)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		sPainter.setColor(lightScapeBrightness, lightScapeBrightness, lightScapeBrightness, landFader.getInterstate());
		mapTexIllum->bind();
		sPainter.sSphere(radius, 1.0, cols, (int) ceil(rows*(illumTexTop-illumTexBottom)/(mapTexTop-mapTexBottom)), 1, true, illumTexTop, illumTexBottom);
	}	
	//qDebug() << "before drawing line";

	// If a horizon line also has been defined, draw it.
	if (horizonPolygon && (horizonPolygonLineColor[0] >= 0))
	{
		//qDebug() << "drawing line";
		transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
		transfo->combine(Mat4d::zrotation(-angleRotateZOffset));
		const StelProjectorP prj = core->getProjection(transfo);
		sPainter.setProjector(prj);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// TODO: remove spurious line at begin of the octahedronPolygon's cachedVertexArray.
		sPainter.setColor(horizonPolygonLineColor[0], horizonPolygonLineColor[1], horizonPolygonLineColor[2], landFader.getInterstate());
		sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeBoundary);
	}
	//else qDebug() << "no polygon defined";
	glDisable(GL_CULL_FACE);

}

//! Sample landscape texture for transparency. May be used for advanced visibility, sunrise on the visible horizon etc.
//! @param azalt: normalized direction in alt-az frame
//! @retval alpha (0..1), where 0=fully transparent.
float LandscapeSpherical::getOpacity(const Vec3d azalt) const
{

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)	) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...

	// QImage has pixel 0/0 in top left corner. We must first find image Y for optionally cropped images.
	// It is possible that sample position is outside cropped texture. in this case, assume full transparency and exit early.
	const float alt_pm1 = 2.0f * std::asin(azalt[2])  / M_PI;  // sampled altitude, -1...+1 linear in altitude angle
	const float img_top_pm1 = 1.0f-2.0f*(mapTexTop    / M_PI); // the top    line in -1..+1
	if (alt_pm1>img_top_pm1) return 0.0f;
	const float img_bot_pm1 = 1.0f-2.0f*(mapTexBottom / M_PI); // the bottom line in -1..+1
	if (alt_pm1<img_bot_pm1) return 0.0f;

	float y_img_1=(alt_pm1-img_bot_pm1)/(img_top_pm1-img_bot_pm1); // the sampled altitude in 0..1 image height from bottom

	int y=(1.0-y_img_1)*mapImage->height();           // pixel Y from top.

	float az=atan2(azalt[0], azalt[1]) / M_PI + 0.5f;  // -0.5..+1.5
	if (az<0) az+=2.0f;                                //  0..2 = N.E.S.W.N

	const float xShift=angleRotateZ /M_PI; // shift value in -2..2
	float az_phot=az - 0.5f - xShift;      // The 0.5 is caused by regular pano left edge being East. The xShift compensates any configured angleRotateZ
	az_phot=fmodf(az_phot, 2.0f);
	if (az_phot<0) az_phot+=2.0f;                                //  0..2 = image-X

	int x=(az_phot/2.0f) * mapImage->width(); // pixel X from left.

	QRgb pixVal=mapImage->pixel(x, y);
	qDebug() << "Landscape sampling: az=" << az*180.0 << "° alt=" << alt_pm1*90.0f
			 << "°, xShift[-2..+2]=" << xShift << " az_phot[0..2]=" << az_phot
			 << ", w=" << mapImage->width() << " h=" << mapImage->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
	return qAlpha(pixVal)/255.0f;

}
