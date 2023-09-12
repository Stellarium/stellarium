/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2011 Bogdan Marinov
 * Copyright (C) 2014-17 Georg Zotti
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
#include "StelLocation.hpp"
#include "StelLocationMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"

#include <QDebug>
#include <QSettings>
#include <QVarLengthArray>
#include <QFile>
#include <QDir>
#include <QtAlgorithms>
#include <QOpenGLBuffer>
#include <QRegularExpression>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

namespace
{
constexpr int FS_QUAD_COORDS_PER_VERTEX = 2;
constexpr int SKY_VERTEX_ATTRIB_INDEX = 0;
}

Landscape::Landscape(float _radius)
	: radius(static_cast<double>(_radius))
	, id("uninitialized")
	, minBrightness(-1.)
	, landscapeBrightness(1.)
	, lightScapeBrightness(0.)
	, validLandscape(false)
	, rows(20)
	, cols(40)
	, angleRotateZ(0.)
	, angleRotateZOffset(0.)
	, sinMinAltitudeLimit(-0.035) //sin(-2 degrees))
	, defaultFogSetting(-1)
	, defaultExtinctionCoefficient(-1.)
	, defaultTemperature(-1000.)
	, defaultPressure(-2.)
	, horizonPolygon(Q_NULLPTR)
	, fontSize(18)
	, memorySize(sizeof(Landscape))
	, multisamplingEnabled_(StelApp::getInstance().getSettings()->value("video/multisampling", 0).toUInt() != 0)
{
}

Landscape::~Landscape()
{}

void Landscape::initGL()
{
	vbo.reset(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
	vbo->create();
	vbo->bind();
	const GLfloat vertices[]=
	{
		// full screen quad
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	auto& gl = *QOpenGLContext::currentContext()->functions();
	gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	vbo->release();

	vao.reset(new QOpenGLVertexArrayObject);
	vao->create();
	bindVAO();
	setupCurrentVAO();
	releaseVAO();

	initialized = true;
}

void Landscape::setupCurrentVAO()
{
	auto& gl = *QOpenGLContext::currentContext()->functions();
	vbo->bind();
	gl.glVertexAttribPointer(0, FS_QUAD_COORDS_PER_VERTEX, GL_FLOAT, false, 0, 0);
	vbo->release();
	gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
}

void Landscape::bindVAO()
{
	if(vao->isCreated())
		vao->bind();
	else
		setupCurrentVAO();
}

void Landscape::releaseVAO()
{
	if(vao->isCreated())
	{
		vao->release();
	}
	else
	{
		auto& gl = *QOpenGLContext::currentContext()->functions();
		gl.glDisableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
	}
}

// Load attributes common to all landscapes
void Landscape::loadCommon(const QSettings& landscapeIni, const QString& landscapeId)
{
	static const QRegularExpression spaceRe("\\\\n\\s*\\\\n");
	id = landscapeId;
	name = landscapeIni.value("landscape/name").toString();
	author = landscapeIni.value("landscape/author").toString();
	description = landscapeIni.value("landscape/description").toString();
	description = description.replace(spaceRe, "<br />");
	description = description.replace("\\n", " ");
	if (name.isEmpty())
	{
		qWarning() << "No valid landscape definition (no name) found for landscape ID "
			<< landscapeId << ". No landscape in use." << StelUtils::getEndLineChar();
		validLandscape = false;
		return;
	}
	else
	{
		validLandscape = true;
	}

	// Optional data
	rows = landscapeIni.value("landscape/tesselate_rows", 20).toUInt();
	cols = landscapeIni.value("landscape/tesselate_cols", 40).toUInt();

	if (landscapeIni.childGroups().contains("location"))
	{
		if (landscapeIni.contains("location/planet"))
			location.planetName = landscapeIni.value("location/planet").toString();
		else
			location.planetName = "Earth";
		// Tolerate decimal values in .ini file, but round to nearest integer
		if (landscapeIni.contains("location/altitude"))
			location.altitude = qRound(landscapeIni.value("location/altitude").toDouble());
		if (landscapeIni.contains("location/latitude"))
			location.setLatitude(static_cast<float>(StelUtils::getDecAngle(landscapeIni.value("location/latitude").toString())*M_180_PI));
		if (landscapeIni.contains("location/longitude"))
			location.setLongitude(static_cast<float>(StelUtils::getDecAngle(landscapeIni.value("location/longitude").toString())*M_180_PI));
		if (landscapeIni.contains("location/country"))
			location.region = StelLocationMgr::pickRegionFromCountry(landscapeIni.value("location/country").toString());
		if (landscapeIni.contains("location/state"))
			location.state = landscapeIni.value("location/state").toString();
		if (landscapeIni.contains("location/name"))
			location.name = landscapeIni.value("location/name").toString();
		else
			location.name = name;
		location.landscapeKey = name;

		QString tzString=landscapeIni.value("location/timezone", "").toString();
		if (!tzString.isEmpty())
			location.ianaTimeZone=StelLocationMgr::sanitizeTimezoneStringFromLocationDB(tzString);

		auto defaultBortleIndex = landscapeIni.value("location/light_pollution", -1).toInt();
		if (defaultBortleIndex<=0) defaultBortleIndex=-1; // neg. values in ini file signal "no change".
		if (defaultBortleIndex>9) defaultBortleIndex=9; // correct bad values.
		const auto lum = landscapeIni.value("location/light_pollution_luminance");
		if (lum.isValid())
		{
			defaultLightPollutionLuminance = lum;

			if (defaultBortleIndex>=0)
			{
				qWarning() << "Landscape light pollution is specified both as luminance and as Bortle "
				              "scale index. Only one value should be specified, preferably luminance.";
			}
		}
		else if (defaultBortleIndex>=0)
			defaultLightPollutionLuminance = StelCore::bortleScaleIndexToLuminance(defaultBortleIndex);

		defaultFogSetting = landscapeIni.value("location/display_fog", -1).toInt();
		defaultExtinctionCoefficient = landscapeIni.value("location/atmospheric_extinction_coefficient", -1.0).toDouble();
		defaultTemperature = landscapeIni.value("location/atmospheric_temperature", -1000.0).toDouble();
		defaultPressure = landscapeIni.value("location/atmospheric_pressure", -2.0).toDouble(); // -2=no change! [-1=computeFromAltitude]
	}

	// Set minimal brightness for landscape
	minBrightness = landscapeIni.value("landscape/minimal_brightness", -1.0f).toFloat();

	// set a minimal altitude which the landscape covers. (new in 0.14)
	// This is to allow landscapes with "holes" in the ground (space station?) (Bug lp:1469407)
	sinMinAltitudeLimit = std::sin(M_PI_180 * landscapeIni.value("landscape/minimal_altitude", -2.0).toDouble());

	// This is now optional for all classes, for mixing with a photo horizon:
	// they may have different offsets, like a south-centered pano and a grid-aligned polygon.
	// In case they are aligned, we can use one value angle_rotatez, or define the polygon rotation individually.
	if (landscapeIni.contains("landscape/polygonal_horizon_list"))
	{
		createPolygonalHorizon(
					StelFileMgr::findFile("landscapes/" + landscapeId + "/" +
							       landscapeIni.value("landscape/polygonal_horizon_list").toString()),
					landscapeIni.value("landscape/polygonal_angle_rotatez", 0.f).toFloat(),
					landscapeIni.value("landscape/polygonal_horizon_list_mode", "azDeg_altDeg").toString()
					);
		// This line can then be drawn in all classes with the color specified here.
		// If not specified, don't draw it! (flagged by negative red)
		horizonPolygonLineColor=Vec3f(landscapeIni.value("landscape/horizon_line_color", "-1,0,0" ).toString());
	}
	loadLabels(landscapeId);
}

void Landscape::createPolygonalHorizon(const QString& lineFileName, const float polyAngleRotateZ, const QString &listMode)
{
	// qDebug() << _name << " " << _fullpath << " " << _lineFileName ;

	const QStringList horizonModeList = { "azDeg_altDeg", "azDeg_zdDeg", "azRad_altRad",
					      "azRad_zdRad", "azGrad_zdGrad", "azGrad_zdGrad"};
	const horizonListMode coordMode=static_cast<horizonListMode>(horizonModeList.indexOf(listMode));

	QVector<Vec3d> horiPoints(0);
	QFile file(lineFileName);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Landscape Horizon line data file" << QDir::toNativeSeparators(lineFileName) << "not found.";
		return;
	}
	static const QRegularExpression emptyLine("^\\s*$");
	static const QRegularExpression spaceRe("\\s+");
	QTextStream in(&file);
	while (!in.atEnd())
	{
		// Build list of vertices. The checks can certainly become more robust.
		QString line = in.readLine();
		if (line.length()==0) continue;
		if (emptyLine.match(line).hasMatch()) continue;
		if (line.at(0)=='#') continue; // skip comment lines.
		const QStringList list = line.trimmed().split(spaceRe);
		if (list.count() < 2)
		{
			qWarning() << "Landscape polygon file" << QDir::toNativeSeparators(lineFileName)
				   << "has bad line:" << line << "with" << list.count() << "elements";
			continue;
		}
		//if (list.count() > 2) // use first two elements but give warning.
		//{
		//	qWarning() << "Landscape polygon file" << QDir::toNativeSeparators(lineFileName) << "has excessive elements in line:" << line << " (" << list.count() << ", not 2 elements)";
		//}
		Vec3d point;
		//qDebug() << "Creating point for az=" << list.at(0) << " alt/zd=" << list.at(1);
		float az = 0.f, alt = 0.f;

		switch (coordMode)
		{
			case azDeg_altDeg:
				az=(180.0f - polyAngleRotateZ - list.at(0).toFloat())*M_PI_180f;
				alt=list.at(1).toFloat()*M_PI_180f;
				break;
			case azDeg_zdDeg:
				az=(180.0f - polyAngleRotateZ - list.at(0).toFloat())*M_PI_180f;
				alt=(90.0f-list.at(1).toFloat())*M_PI_180f;
				break;
			case azRad_altRad:
				az=(M_PIf - polyAngleRotateZ*M_PI_180f - list.at(0).toFloat());
				alt=list.at(1).toFloat();
				break;
			case azRad_zdRad:
				az=(M_PIf - polyAngleRotateZ*M_PI_180f - list.at(0).toFloat());
				alt=M_PI_2f-list.at(1).toFloat();
				break;
			case azGrad_altGrad:
				az=(200.0f  - list.at(0).toFloat())*M_PIf/200.f    - polyAngleRotateZ*M_PI_180f;
				alt=list.at(1).toFloat()*M_PIf/200.f;
				break;
			case azGrad_zdGrad:
				az=(200.0f  - list.at(0).toFloat())*M_PIf/200.f    - polyAngleRotateZ*M_PI_180f;
				alt=(100.0f-list.at(1).toFloat())*M_PIf/200.f;
				break;
			case invalid:
				qWarning() << "invalid polygonal_horizon_list_mode while reading horizon line.";
				return;
		}

		StelUtils::spheToRect(az, alt, point);
		if (horiPoints.isEmpty() || horiPoints.last() != point)
			horiPoints.append(point);
	}
	file.close();
	//horiPoints.append(horiPoints.at(0)); // close loop? Apparently not necessary.

	//qDebug() << "created horiPoints with " << horiPoints.count() << "points:";
	//for (int i=0; i<horiPoints.count(); ++i)
	//	qDebug() << horiPoints.at(i)[0] << "/" << horiPoints.at(i)[1] << "/" << horiPoints.at(i)[2] ;
	AllSkySphericalRegion allskyRegion;
	SphericalPolygon aboveHorizonPolygon(horiPoints);
	horizonPolygon = allskyRegion.getSubtraction(aboveHorizonPolygon);

	// If this now contains the zenith, invert the solution:
	if (horizonPolygon->contains(Vec3d(0.,0.,1.)))
	{
		//qDebug() << "Must invert polygon vector!";
		std::reverse(horiPoints.begin(), horiPoints.end());
		AllSkySphericalRegion allskyRegion;
		SphericalPolygon aboveHorizonPolygon(horiPoints);
		horizonPolygon = allskyRegion.getSubtraction(aboveHorizonPolygon);
		AllSkySphericalRegion allskyRegion2;
		horizonPolygon = allskyRegion2.getSubtraction(horizonPolygon);
	}
}

void Landscape::drawHorizonLine(StelCore* core, StelPainter& painter)
{
	if (!horizonPolygon || horizonPolygonLineColor == Vec3f(-1.f,0.f,0.f))
		return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(static_cast<double>(-angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);
	painter.setProjector(prj);
	painter.setLineSmooth(true);
	painter.setBlending(true);
	painter.setColor(horizonPolygonLineColor, landFader.getInterstate());
	const float lineWidth=painter.getLineWidth();
	const float ppx = static_cast<float>(prj->getDevicePixelsPerPixel());
	painter.setLineWidth(GETSTELMODULE(LandscapeMgr)->getPolyLineThickness()*ppx);
	painter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeBoundary);
	painter.setLineWidth(lineWidth);
	painter.setLineSmooth(false);
}

#include <iostream>
const QString Landscape::getTexturePath(const QString& basename, const QString& landscapeId)
{
	// look in the landscape directory first, and if not found default to global textures directory
	QString path = StelFileMgr::findFile("landscapes/" + landscapeId + "/" + basename);
	if (path.isEmpty())
		path = StelFileMgr::findFile("textures/" + basename);
	if (path.isEmpty())
		qWarning() << "Warning: Landscape" << landscapeId << ": File" << basename << "does not exist.";
	return path;
}

// find optional file and fill landscapeLabels list.
void Landscape::loadLabels(const QString& landscapeId)
{
	// in case we have labels and this is called for a retranslation, clean list first.
	landscapeLabels.clear();

	QString lang, descFileName, locLabelFileName, engLabelFileName;

	lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	locLabelFileName = StelFileMgr::findFile("landscapes/" + landscapeId,
						 StelFileMgr::Directory) + "/gazetteer." + lang + ".utf8";
	engLabelFileName = StelFileMgr::findFile("landscapes/" + landscapeId,
						 StelFileMgr::Directory) + "/gazetteer.en.utf8";

	// Check the file with full name of locale
	if (!QFileInfo::exists(locLabelFileName))
	{
		// File not found. What about short name of locale?
		lang = lang.split("_").at(0);
		locLabelFileName = StelFileMgr::findFile("landscapes/" + landscapeId,
							 StelFileMgr::Directory) + "/gazetteer." + lang + ".utf8";
	}

	// Get localized or at least English description for landscape
	if (QFileInfo::exists(locLabelFileName))
		descFileName = locLabelFileName;
	else if (QFileInfo::exists(engLabelFileName))
		descFileName = engLabelFileName;
	else
		return;

	// We have found some file now.
	QFile file(descFileName);
	if(file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream in(&file);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		in.setEncoding(QStringConverter::Utf8);
#else
		in.setCodec("UTF-8");
#endif
		while (!in.atEnd())
		{
			QString line=in.readLine();

			// Skip comments and all-empty lines (space allowed and ignored)
			if (line.startsWith('#') || line.trimmed().isEmpty() )
				continue;
			// Read entries, construct vectors, put in list.
			const QStringList parts=line.split('|');
			if (parts.count() != 5)
			{
				qWarning() << "Invalid line in landscape gazetteer" << descFileName << ":" << line;
				continue;
			}
			LandscapeLabel newLabel;
			newLabel.name=parts.at(4).trimmed();
			StelUtils::spheToRect((180.0f-parts.at(0).toFloat()) *M_PI_180f,
					      parts.at(1).toFloat()*M_PI_180f, newLabel.featurePoint);
			StelUtils::spheToRect((180.0f-parts.at(0).toFloat() - parts.at(3).toFloat())*M_PI_180f,
					      (parts.at(1).toFloat() + parts.at(2).toFloat())*M_PI_180f, newLabel.labelPoint);
			landscapeLabels.append(newLabel);
			//qDebug() << "Added landscape label " << newLabel.name;
		}
		file.close();
	}
}

void Landscape::drawLabels(StelCore* core, StelPainter *painter)
{
	if (landscapeLabels.length()==0) // no labels
		return;
	if (labelFader.getInterstate() < 0.0001f) // switched off
		return;

	// We must reset painter to pure altaz coordinates without pano-based rotation
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	painter->setProjector(prj);
	QFont font;
	font.setPixelSize(fontSize);
	painter->setFont(font);
	QFontMetrics fm(font);
	painter->setColor(labelColor, labelFader.getInterstate()*landFader.getInterstate());

	painter->setBlending(true);
	painter->setLineSmooth(true);

	for (int i = 0; i < landscapeLabels.size(); ++i)
	{
		// in case of gravityLabels, we cannot shift-adjust centered placename, sorry!
		if (prj->getFlagGravityLabels())
		{
			painter->drawText(landscapeLabels.at(i).labelPoint, landscapeLabels.at(i).name, 0, 0, 0, false);
		}
		else
		{
			int textWidth=fm.boundingRect(landscapeLabels.at(i).name).width();
			painter->drawText(landscapeLabels.at(i).labelPoint, landscapeLabels.at(i).name, 0, -textWidth/2, 2, true);
		}
		painter->drawGreatCircleArc(landscapeLabels.at(i).featurePoint, landscapeLabels.at(i).labelPoint, Q_NULLPTR);
	}

	painter->setLineSmooth(false);
	painter->setBlending(false);
}


LandscapeOldStyle::LandscapeOldStyle(float _radius)
	: Landscape(_radius)
	, sideTexs(Q_NULLPTR)
	, nbSideTexs(0)
	, nbSide(0)
	, sides(Q_NULLPTR)
	, nbDecorRepeat(0)
	, fogAltAngle(0.)
	, fogAngleShift(0.)
	, decorAltAngle(0.)
	, decorAngleShift(0.)
	, groundAngleShift(0.)
	, groundAngleRotateZ(0.)
	, drawGroundFirst(false)
	, tanMode(false)
	, calibrated(false)  // start with just the known entries.
{
	memorySize=sizeof(LandscapeOldStyle);
}

LandscapeOldStyle::~LandscapeOldStyle()
{
	if (sideTexs)
	{
		delete [] sideTexs;
		sideTexs = Q_NULLPTR;
	}

	if (sides) delete [] sides;
	if (!sidesImages.isEmpty())
	{
		qDeleteAll(sidesImages);
		sidesImages.clear();
	}
	landscapeLabels.clear();
}

void LandscapeOldStyle::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// TODO: put values into hash and call create() method to consolidate code
	loadCommon(landscapeIni, landscapeId);
	// rows, cols have been loaded already, but with different defaults.
	// GZ Hey, they are not used altogether! Resolution is constant, below!
	//rows = landscapeIni.value("landscape/tesselate_rows", 8).toInt();
	//cols = landscapeIni.value("landscape/tesselate_cols", 16).toInt();
	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "old_style")
	{
		qWarning() << "Landscape type mismatch for landscape " << landscapeId
				   << ", expected old_style, found " << type << ".  No landscape in use.";
		validLandscape = false;
		return;
	}

	nbDecorRepeat      = static_cast<unsigned short>(landscapeIni.value("landscape/nb_decor_repeat", 1).toUInt());
	fogAltAngle        = landscapeIni.value("landscape/fog_alt_angle", 0.).toFloat();
	fogAngleShift      = landscapeIni.value("landscape/fog_angle_shift", 0.).toFloat();
	decorAltAngle      = landscapeIni.value("landscape/decor_alt_angle", 0.).toFloat();
	decorAngleShift    = landscapeIni.value("landscape/decor_angle_shift", 0.).toFloat();
	angleRotateZ       = landscapeIni.value("landscape/decor_angle_rotatez", 0.).toFloat()  * M_PI_180f;
	groundAngleShift   = landscapeIni.value("landscape/ground_angle_shift", 0.).toFloat()   * M_PI_180f;
	groundAngleRotateZ = landscapeIni.value("landscape/ground_angle_rotatez", 0.).toDouble() * M_PI_180;
	drawGroundFirst    = landscapeIni.value("landscape/draw_ground_first", false).toBool();
	tanMode            = landscapeIni.value("landscape/tan_mode", false).toBool();
	calibrated         = landscapeIni.value("landscape/calibrated", false).toBool();

	auto& texMan = StelApp::getInstance().getTextureManager();
	// Load sides textures
	nbSideTexs = static_cast<unsigned short>(landscapeIni.value("landscape/nbsidetex", 0).toUInt());
	sideTexs = new StelTextureSP[static_cast<size_t>(nbSideTexs)*2]; // 0.14: allow upper half for light textures!
	const auto texParams = StelTexture::StelTextureParams(true, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
	for (unsigned int i=0; i<nbSideTexs; ++i)
	{
		QString textureKey = QString("landscape/tex%1").arg(i);
		QString textureName = landscapeIni.value(textureKey).toString();
		const QString texturePath = getTexturePath(textureName, landscapeId);
		sideTexs[i] = texMan.createTexture(texturePath, texParams);
		// GZ: To query the textures, also keep an array of QImage*, but only
		// if that query is not going to be prevented by the polygon that already has been loaded at that point...
		if ( (!horizonPolygon) && calibrated ) { // for uncalibrated landscapes the texture is currently never queried, so no need to store.
			QImage *image = new QImage(texturePath);
			sidesImages.append(image); // indices identical to those in sideTexs
			memorySize+=(image->sizeInBytes());
		}
		// Also allow light textures. The light textures must cover the same geometry as the sides. It is allowed that not all or even any light textures are present!
		textureKey = QString("landscape/light%1").arg(i);
		textureName = landscapeIni.value(textureKey).toString();
		if (textureName.length())
		{
			const QString lightTexturePath = getTexturePath(textureName, landscapeId);
			sideTexs[nbSideTexs+i] = texMan.createTexture(lightTexturePath, texParams);
			if(sideTexs[nbSideTexs+i])
				memorySize+=sideTexs[nbSideTexs+i]->getGlSize();
		}
		else
			sideTexs[nbSideTexs+i].clear();
	}
	if ( (!horizonPolygon) && calibrated )
	{
		Q_ASSERT(sidesImages.size()==nbSideTexs);
	}
	QMap<unsigned int, unsigned int> texToSide;
	// Init sides parameters
	nbSide = static_cast<unsigned short>(landscapeIni.value("landscape/nbside", 0).toUInt());
	sides = new landscapeTexCoord[static_cast<size_t>(nbSide)];
	unsigned int texnum;
	for (unsigned int i=0;i<nbSide;++i)
	{
		const QString key = QString("landscape/side%1").arg(i);                             // e.g. side0
		//sscanf(s.toLocal8Bit(),"tex%d:%f:%f:%f:%f",&texnum,&a,&b,&c,&d);
		const QStringList parameters = landscapeIni.value(key).toString().split(':');  // e.g. tex0:0:0:1:1
		//TODO: How should be handled an invalid texture description?
		QString textureName = parameters.value(0);                                    // tex0
		texnum = textureName.right(textureName.length() - 3).toUInt();             // 0
		sides[i].tex = sideTexs[texnum];
		sides[i].tex_illum = sideTexs[nbSide+texnum];
		sides[i].texCoords[0] = parameters.at(1).toFloat();
		sides[i].texCoords[1] = parameters.at(2).toFloat();
		sides[i].texCoords[2] = parameters.at(3).toFloat();
		sides[i].texCoords[3] = parameters.at(4).toFloat();
		//qDebug() << i << texnum << sides[i].texCoords[0] << sides[i].texCoords[1] << sides[i].texCoords[2] << sides[i].texCoords[3];

		// Prior to precomputing the sides, we used to match E to side0.
		// In r4598 the precomputing was put in place and caused a problem for
		// old_style landscapes which had a z rotation on the side textures
		// and where side0 did not map to tex0
		// texToSide is a nasty hack to replace the old behaviour.
		// GZ for V0.13: I put the zrotation to the draw call (like for all other landscapes).
		// Maybe this can be again simplified?
		texToSide[i] = texnum;
	}
	const QString groundTexName = landscapeIni.value("landscape/groundtex").toString();
	const QString groundTexPath = getTexturePath(groundTexName, landscapeId);
	groundTex = texMan.createTexture(groundTexPath, texParams);
	if (groundTex)
		memorySize+=groundTex->getGlSize();

	const QString fogTexName = landscapeIni.value("landscape/fogtex").toString();
	const QString fogTexPath = getTexturePath(fogTexName, landscapeId);
	fogTex = texMan.createTexture(fogTexPath, StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
	if (fogTex)
		memorySize+=fogTex->getGlSize();

	// Precompute the vertex arrays for ground display
	// Make slices_per_side=(3<<K) so that the innermost polygon of the fandisk becomes a triangle:
	//const int slices_per_side = 3*64/(nbDecorRepeat*nbSide);
	//if (slices_per_side<=0) // GZ: How can negative ever happen?
	//	slices_per_side = 1;
	const auto slices_per_side = static_cast<unsigned short>(qMax(3u*64u/(nbDecorRepeat*nbSide), 1u));

	// draw a fan disk instead of a ordinary disk to that the inner slices
	// are not so slender. When they are too slender, culling errors occur
	// in cylinder projection mode.
	unsigned short int slices_inside = nbSide*slices_per_side*nbDecorRepeat;
	uint level = 0;
	while ((slices_inside&1)==0 && slices_inside > 4)
	{
		++level;
		slices_inside>>=1;
	}
	StelPainter::computeFanDisk(static_cast<float>(radius), slices_inside, level, groundVertexArr, groundTexCoordArr); //comaVertexArr, comaTexCoordArr);


	// Precompute the vertex arrays for side display. The geometry of the sides is always a cylinder.
	// The texture is split into regular quads.

	// GZ: the original code for vertical placement makes unfortunately no sense. There are many approximately-fitted landscapes, though.
	// I added a switch "calibrated" for the ini file. If true, it works as this landscape apparently was originally intended,
	// if false (or missing) it uses the original code.
	// I corrected the texture coordinates so that decorAltAngle is the total vertical angle, decorAngleShift the lower angle,
	// and the texture in between is correctly stretched.
	// I located an undocumented switch tan_mode, maybe tan_mode=true means cylindrical panorama projection instead of equirectangular.
	// Since V0.13, calibrated&&tanMode also works!
	// In calibrated && !tan_mode, the vertical position is computed correctly, so that quads off the horizon are larger.
	// in calibrated &&  tan_mode, d_z can become a constant because the texture is already predistorted in cylindrical projection.
	const unsigned short int stacks = (calibrated ? 16u : 8u); // GZ: 8->16, I need better precision.
	float z0, d_z;
	if (calibrated)
	{
		if (tanMode) // cylindrical pano: linear in d_z, simpler.
		{
			z0=  static_cast<float>(radius)*std::tan(decorAngleShift*M_PI_180f);
			d_z=(static_cast<float>(radius)*std::tan((decorAltAngle+decorAngleShift)*M_PI_180f) - z0)/stacks;
		}
		else // equirectangular pano: angular z, requires more work in the loop below!
		{
			z0=decorAngleShift;
			d_z=decorAltAngle/stacks;
		}
	}
	else // buggy code, but legacy.
	{
		z0 =static_cast<float>(radius)*(tanMode ? std::tan(decorAngleShift*M_PI_180f)
							: std::sin(decorAngleShift*M_PI_180f));
		d_z=static_cast<float>(radius)*(tanMode ? std::tan(decorAltAngle  *M_PI_180f)/stacks
							: std::sin(decorAltAngle  *M_PI_180f)/stacks);
	}

	const float alpha = 2.f*static_cast<float>(M_PI)/(nbDecorRepeat*nbSide*slices_per_side); //delta_azimuth
	const float ca = std::cos(alpha);
	const float sa = std::sin(alpha);
	float y0 = static_cast<float>(radius);
	float x0 = 0.0f;
	unsigned short int limit;

	LOSSide precompSide;
	precompSide.arr.primitiveType=StelVertexArray::Triangles;	
	for (unsigned int n=0;n<nbDecorRepeat;n++)
	{
		for (unsigned int i=0;i<nbSide;i++)
		{
			unsigned int ti;
			if (texToSide.contains(i))
				ti = texToSide[i];
			else
			{
				qDebug() << QString("LandscapeOldStyle::load ERROR: found no "
						    "corresponding tex value for side%1").arg(i);
				break;
			}
			precompSide.arr.vertex.resize(0);
			precompSide.arr.texCoords.resize(0);
			precompSide.arr.indices.resize(0);
			precompSide.tex=sideTexs[ti];
			precompSide.light=false;

			float tx0 = sides[ti].texCoords[0];
			const float d_tx = (sides[ti].texCoords[2]-sides[ti].texCoords[0]) / slices_per_side;
			const float d_ty = (sides[ti].texCoords[3]-sides[ti].texCoords[1]) / stacks;			
			for (unsigned short int j=0;j<slices_per_side;j++)
			{
				const float y1 = y0*ca - x0*sa;
				const float x1 = y0*sa + x0*ca;
				const float tx1 = tx0 + d_tx;
				float z = z0;
				float ty0 = sides[ti].texCoords[1];
				limit = static_cast<unsigned short int>(stacks*2u);
				for (unsigned short int k=0u;k<=limit;k+=2u)
				{
					precompSide.arr.texCoords << Vec2f(tx0, ty0) << Vec2f(tx1, ty0);
					if (calibrated && !tanMode)
					{
						double tanZ=radius * static_cast<double>(std::tan(z*M_PI_180f));
						precompSide.arr.vertex << Vec3d(static_cast<double>(x0),
										static_cast<double>(y0), tanZ)
								       << Vec3d(static_cast<double>(x1),
										static_cast<double>(y1), tanZ);
					} else
					{
						precompSide.arr.vertex << Vec3d(static_cast<double>(x0),
										static_cast<double>(y0), static_cast<double>(z))
								       << Vec3d(static_cast<double>(x1),
										static_cast<double>(y1), static_cast<double>(z));
					}
					z += d_z;
					ty0 += d_ty;
				}
				unsigned short int offset = j*(stacks+1u)*2u;
				limit = static_cast<unsigned short int>(stacks*2u+2u);
				for (unsigned short int k = 2;k<limit;k+=2u)
				{
					precompSide.arr.indices << offset+k-2 << offset+k-1 << offset+k;
					precompSide.arr.indices << offset+k   << offset+k-1 << offset+k+1;
				}
				y0 = y1;
				x0 = x1;
				tx0 = tx1;
			}
			precomputedSides.append(precompSide);
			if (sideTexs[ti+nbSide])
			{
				precompSide.light=true;
				precompSide.tex=sideTexs[ti+nbSide];
				precomputedSides.append(precompSide);	// These sides are not called by strict index!
									// May be 9 for 8 sidetexs plus 1-only light panel
			}
		}
	}
	//qDebug() << "OldStyleLandscape" << landscapeId << "loaded, mem size:" << memorySize;
}

void LandscapeOldStyle::draw(StelCore* core, bool onlyPolygon)
{
	if(!StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER).contains("textureGrad_SUPPORTED"))
	{
		drawLowGL(core, onlyPolygon);
		return;
	}

	if(!initialized) initGL();
	if(!validLandscape) return;
	if(landFader.getInterstate()==0.f) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);

	if(!renderProgram || !prevProjector || !prj->isSameProjection(*prevProjector))
	{
		renderProgram.reset(new QOpenGLShaderProgram);
		prevProjector = prj;

		const auto vert =
			StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
			R"(
ATTRIBUTE highp vec3 vertex;
VARYING highp vec3 ndcPos;
void main()
{
	gl_Position = vec4(vertex, 1.);
	ndcPos = vertex;
}
)";
		bool ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "LandscapeSpherical: Warnings while compiling vertex shader:\n"
					     << renderProgram->log();
		if(!ok) return;

		const auto frag =
			StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
			prj->getUnProjectShader() +
			R"(
VARYING highp vec3 ndcPos;
uniform int whatToRender;
uniform int totalNumberOfSides;
uniform int firstSideInBatch;
uniform int numberOfSidesInBatch;
uniform int sidePresenceMask;
uniform vec4 perSideTexCoords[8];
uniform sampler2D sideTex0;
uniform sampler2D sideTex1;
uniform sampler2D sideTex2;
uniform sampler2D sideTex3;
uniform sampler2D sideTex4;
uniform sampler2D sideTex5;
uniform sampler2D sideTex6;
uniform sampler2D sideTex7;
uniform sampler2D mapTex;
uniform mat4 projectionMatrixInverse;
uniform float vshift; // tan (or sin) of {ground,fog}_angle_shift
uniform float decorAngleShift;
uniform float sideAngularHeight;
uniform float fogCylinderHeight;
uniform bool calibrated;
uniform bool tanMode;
uniform vec4 brightness;

vec4 sampleSideTexture(int sideIndex, const vec2 texc, const vec2 texDx, const vec2 texDy)
{
	sideIndex -= firstSideInBatch;
	if(sideIndex==0 && (sidePresenceMask&  1)!=0) return textureGrad(sideTex0, texc, texDx, texDy);
	if(sideIndex==1 && (sidePresenceMask&  2)!=0) return textureGrad(sideTex1, texc, texDx, texDy);
	if(sideIndex==2 && (sidePresenceMask&  4)!=0) return textureGrad(sideTex2, texc, texDx, texDy);
	if(sideIndex==3 && (sidePresenceMask&  8)!=0) return textureGrad(sideTex3, texc, texDx, texDy);
	if(sideIndex==4 && (sidePresenceMask& 16)!=0) return textureGrad(sideTex4, texc, texDx, texDy);
	if(sideIndex==5 && (sidePresenceMask& 32)!=0) return textureGrad(sideTex5, texc, texDx, texDy);
	if(sideIndex==6 && (sidePresenceMask& 64)!=0) return textureGrad(sideTex6, texc, texDx, texDy);
	if(sideIndex==7 && (sidePresenceMask&128)!=0) return textureGrad(sideTex7, texc, texDx, texDy);
	return vec4(0);
}

void main(void)
{
	const float PI = 3.14159265;
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool unprojectSuccess = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, unprojectSuccess).xyz;
	// First we must compute the derivatives of texture coordinates, and only
	// then decide whether we want to display the !unprojectSuccess case.

	vec3 viewDir = normalize(modelPos);

	vec4 color;
	if(whatToRender == -1) // ground
	{
		vec2 centeredTexCoords = viewDir.z!=0. ? vshift / viewDir.z * viewDir.xy
											   : vec2(0);
		vec2 texc = (centeredTexCoords + 1.) / 2.;
		vec2 texDx = dFdx(texc);
		vec2 texDy = dFdy(texc);

		// Now that all dFdx/dFdy are computed, we can early return if needed.
		if(!unprojectSuccess)
		{
			FRAG_COLOR = vec4(0);
			return;
		}
		if(viewDir.z > 0. || length(centeredTexCoords) > 1.)
		{
			FRAG_COLOR = vec4(0);
			return;
		}

		color = textureGrad(mapTex, texc, texDx, texDy);
	}
	else if(whatToRender == -2) // fog
	{
		float azimuth = atan(viewDir.x, viewDir.y);
		if(azimuth<0.) azimuth += 2.*PI;
		float s = azimuth/(2.*PI);

		float tanElevation = viewDir.z / sqrt(1. - viewDir.z*viewDir.z);
		float t = (tanElevation - vshift) / fogCylinderHeight;

		vec2 texc = vec2(s,t);
		vec2 texDx = dFdx(texc);
		vec2 texDy = dFdy(texc);

		// Now that all dFdx/dFdy are computed, we can early return if needed.
		if(!unprojectSuccess)
		{
			FRAG_COLOR = vec4(0);
			return;
		}
		if(t<0. || t>1.)
		{
			FRAG_COLOR = vec4(0);
			return;
		}
		color = textureGrad(mapTex, texc, texDx, texDy);
	}
	else
	{
		float azimuth = atan(viewDir.x, viewDir.y);
		if(azimuth<0.) azimuth += 2.*PI;
		float sideAngularWidth = 2.*PI/float(totalNumberOfSides);
		int currentSide = int(azimuth/sideAngularWidth);
		int currentSideInBatch = currentSide%numberOfSidesInBatch;
		float leftBorderAzimuth = float(currentSide)*sideAngularWidth;
		float texCoordLeft  = perSideTexCoords[currentSideInBatch][0];
		float texCoordRight = perSideTexCoords[currentSideInBatch][2];
		float deltaS = texCoordRight - texCoordLeft;
		float s = texCoordLeft + (azimuth-leftBorderAzimuth)/sideAngularWidth * deltaS;

		float elevation = asin(viewDir.z);
		float texCoordBottom  = perSideTexCoords[currentSideInBatch][1];
		float texCoordTop     = perSideTexCoords[currentSideInBatch][3];
		float deltaT = texCoordTop - texCoordBottom;
		float texCoordInUnitRange;
		if(calibrated)
		{
			if(tanMode)
			{
				float tanShift = tan(decorAngleShift);
				texCoordInUnitRange = (tan(elevation)-tanShift)/(tan(decorAngleShift+sideAngularHeight)-tanShift);
			}
			else
			{
				texCoordInUnitRange = (elevation-decorAngleShift)/sideAngularHeight;
			}
		}
		else
		{
			if(tanMode)
			{
				texCoordInUnitRange = (tan(elevation)-tan(decorAngleShift))/tan(sideAngularHeight);
			}
			else
			{
				texCoordInUnitRange = (tan(elevation)-sin(decorAngleShift))/sin(sideAngularHeight);
			}
		}
		float t = texCoordBottom + texCoordInUnitRange * deltaT;

		// The usual automatic computation of derivatives of texture coordinates
		// breaks down at the discontinuity of atan, resulting in choosing the most
		// minified mip level instead of the correct one, which looks as a seam on
		// the screen. Thus, we need to compute them in a custom way, treating atan
		// as a (continuous) multivalued function. We differentiate
		// atan(modelPosX(x,y), modelPosY(x,y)) with respect to x and y and yield
		// gradAzimuth vector.
		vec2 gradModelPosX = vec2(dFdx(modelPos.x), dFdy(modelPos.x));
		vec2 gradModelPosY = vec2(dFdx(modelPos.y), dFdy(modelPos.y));
		float texTdx = dFdx(t);
		float texTdy = dFdy(t);

		// Now that all dFdx/dFdy are computed, we can early return if needed.
		int lastSideInBatch = firstSideInBatch+numberOfSidesInBatch-1;
		if(currentSide < firstSideInBatch || currentSide > lastSideInBatch)
		{
			FRAG_COLOR = vec4(0);
			return;
		}
		if(s<texCoordLeft || s>texCoordRight || t<texCoordBottom || t>texCoordTop)
		{
			FRAG_COLOR = vec4(0);
			return;
		}
		if(!unprojectSuccess)
		{
			FRAG_COLOR = vec4(0);
			return;
		}

		vec2 gradAzimuth   = vec2(modelPos.y*gradModelPosX.s-modelPos.x*gradModelPosY.s,
								  modelPos.y*gradModelPosX.t-modelPos.x*gradModelPosY.t)
															/
												 dot(modelPos, modelPos);
		vec2 texDx = vec2(gradAzimuth.s/sideAngularWidth*deltaS, texTdx);
		vec2 texDy = vec2(gradAzimuth.t/sideAngularWidth*deltaS, texTdy);
		color = sampleSideTexture(currentSide, vec2(s,t), texDx, texDy);
	}

	FRAG_COLOR = color * brightness;
}
)";
		ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "LandscapeSpherical: Warnings while compiling fragment shader:\n"
					     << renderProgram->log();

		if(!ok) return;

		renderProgram->bindAttributeLocation("vertex", SKY_VERTEX_ATTRIB_INDEX);

		if(!StelPainter::linkProg(renderProgram.get(), "Spherical landscape render program"))
			return;

		renderProgram->bind();
		shaderVars.mapTex                  = renderProgram->uniformLocation("mapTex");
		shaderVars.vshift                  = renderProgram->uniformLocation("vshift");
		shaderVars.tanMode                 = renderProgram->uniformLocation("tanMode");
		shaderVars.calibrated              = renderProgram->uniformLocation("calibrated");
		shaderVars.brightness              = renderProgram->uniformLocation("brightness");
		shaderVars.whatToRender            = renderProgram->uniformLocation("whatToRender");
		shaderVars.decorAngleShift         = renderProgram->uniformLocation("decorAngleShift");
		shaderVars.firstSideInBatch        = renderProgram->uniformLocation("firstSideInBatch");
		shaderVars.sidePresenceMask        = renderProgram->uniformLocation("sidePresenceMask");
		shaderVars.sideAngularHeight       = renderProgram->uniformLocation("sideAngularHeight");
		shaderVars.fogCylinderHeight       = renderProgram->uniformLocation("fogCylinderHeight");
		shaderVars.totalNumberOfSides      = renderProgram->uniformLocation("totalNumberOfSides");
		shaderVars.numberOfSidesInBatch    = renderProgram->uniformLocation("numberOfSidesInBatch");
		shaderVars.projectionMatrixInverse = renderProgram->uniformLocation("projectionMatrixInverse");
		for(unsigned n = 0; n < std::size(shaderVars.sideTexN); ++n)
		{
			shaderVars.sideTexN[n] = renderProgram->uniformLocation(("sideTex"+std::to_string(n)).c_str());
			shaderVars.perSideTexCoords[n] =
				renderProgram->uniformLocation(("perSideTexCoords["+std::to_string(n)+"]").c_str());
		}
		renderProgram->release();
	}
	if(!renderProgram || !renderProgram->isLinked())
		return;

	auto& gl = *QOpenGLContext::currentContext()->functions();

	if (!onlyPolygon || !horizonPolygon) // Make sure to draw the regular pano when there is no polygon
	{
		renderProgram->bind();
		bindVAO();

		const int firstFreeTexSampler = 0;

		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(drawGroundFirst)
			drawGround(core, firstFreeTexSampler);
		drawDecor(core, firstFreeTexSampler, false);
		if(!drawGroundFirst)
			drawGround(core, firstFreeTexSampler);

		drawFog(core, firstFreeTexSampler);

		// Self-luminous layer (Light pollution etc). This looks striking!
		if (lightScapeBrightness>0.0f && (illumFader.getInterstate()>0.f))
		{
			gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			drawDecor(core, firstFreeTexSampler, true);
		}

		releaseVAO();
		gl.glDisable(GL_BLEND);
		renderProgram->release();
	}
	StelPainter painter(prj);
	drawHorizonLine(core, painter);
}


// Draw the horizon fog
void LandscapeOldStyle::drawFog(StelCore*const core, const int firstFreeTexSampler) const
{
	if (fogFader.getInterstate()==0.f)
		return;
	if(landFader.getInterstate()==0.f)
		return;
	if (!(core->getSkyDrawer()->getFlagHasAtmosphere()))
		return;

	const float vpos = tanMode || calibrated ? std::tan(fogAngleShift*M_PI_180f) : std::sin(fogAngleShift*M_PI_180f);
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);

	if (calibrated) // new in V0.13: take proper care of the fog layer. This will work perfectly only for calibrated&&tanMode.
		transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZ+angleRotateZOffset)));

	const StelProjectorP prj = core->getProjection(transfo);

	auto& gl = *QOpenGLContext::currentContext()->functions();
	gl.glBlendFunc(GL_ONE, GL_ONE);

	const float height = calibrated ? std::tan((fogAltAngle+fogAngleShift)*M_PI_180f) - std::tan(fogAngleShift*M_PI_180f)
					: tanMode ? std::tan(fogAltAngle*M_PI_180f)
						  : std::sin(fogAltAngle*M_PI_180f);

	renderProgram->setUniformValue(shaderVars.whatToRender, -2);
	renderProgram->setUniformValue(shaderVars.fogCylinderHeight, height);
	renderProgram->setUniformValue(shaderVars.vshift, vpos);

	const float brightness = landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness);
	renderProgram->setUniformValue(shaderVars.brightness, brightness, brightness, brightness,
				       (1.f-landscapeTransparency)*landFader.getInterstate());
	renderProgram->setUniformValue(shaderVars.projectionMatrixInverse, prj->getProjectionMatrix().toQMatrix().inverted());
	prj->setUnProjectUniforms(*renderProgram);

	fogTex->bind(firstFreeTexSampler);
	renderProgram->setUniformValue(shaderVars.mapTex, firstFreeTexSampler);
	gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw the side textures
void LandscapeOldStyle::drawDecor(StelCore*const core, const int firstFreeTexSampler, const bool drawLight) const
{
	if (landFader.getInterstate()==0.f)
		return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);

	if (drawLight)
	{
		const auto brightness = illumFader.getInterstate()*lightScapeBrightness;
		renderProgram->setUniformValue(shaderVars.brightness,
					       brightness, brightness, brightness,
					       (1.f-landscapeTransparency)*landFader.getInterstate());
	}
	else
	{
		renderProgram->setUniformValue(shaderVars.brightness,
					       landscapeBrightness, landscapeBrightness, landscapeBrightness,
					       (1.f-landscapeTransparency)*landFader.getInterstate());
	}

	renderProgram->setUniformValue(shaderVars.tanMode, tanMode);
	renderProgram->setUniformValue(shaderVars.calibrated, calibrated);
	renderProgram->setUniformValue(shaderVars.projectionMatrixInverse, prj->getProjectionMatrix().toQMatrix().inverted());
	renderProgram->setUniformValue(shaderVars.totalNumberOfSides, static_cast<GLint>(nbSide));
	renderProgram->setUniformValue(shaderVars.decorAngleShift, decorAngleShift*M_PI_180f);
	renderProgram->setUniformValue(shaderVars.sideAngularHeight, decorAltAngle*M_PI_180f);
	prj->setUnProjectUniforms(*renderProgram);

	auto& gl = *QOpenGLContext::currentContext()->functions();
	GLint globalSideNumber = 0, firstSideInBatch = 0;
	const GLint numberOfSidesInBatch = std::size(shaderVars.sideTexN);
	renderProgram->setUniformValue(shaderVars.numberOfSidesInBatch, numberOfSidesInBatch);
	renderProgram->setUniformValue(shaderVars.firstSideInBatch, firstSideInBatch);
	GLint sidePresenceMask = 0;
	for(unsigned repetition = 0; repetition < nbDecorRepeat; ++repetition)
	{
		for(unsigned sideN = 0; sideN < nbSide; ++sideN, ++globalSideNumber)
		{
			auto sideNumberInBatch = globalSideNumber-firstSideInBatch;
			if(sideNumberInBatch >= numberOfSidesInBatch)
			{
				if(sidePresenceMask)
				{
					renderProgram->setUniformValue(shaderVars.sidePresenceMask, sidePresenceMask);
					gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				}

				// Start a new batch
				sidePresenceMask = 0;
				sideNumberInBatch = 0;
				firstSideInBatch += numberOfSidesInBatch;
				renderProgram->setUniformValue(shaderVars.firstSideInBatch, firstSideInBatch);
			}

			const auto& side = sides[sideN];

			const int texSampler = firstFreeTexSampler + sideNumberInBatch;
			if(drawLight)
			{
				if(side.tex_illum)
				{
					side.tex_illum->bind(texSampler);
					sidePresenceMask |= 1u<<sideNumberInBatch;
					renderProgram->setUniformValue(shaderVars.sideTexN[sideNumberInBatch], texSampler);
				}
			}
			else
			{
				side.tex->bind(texSampler);
				sidePresenceMask |= 1u<<sideNumberInBatch;
				renderProgram->setUniformValue(shaderVars.sideTexN[sideNumberInBatch], texSampler);
			}

			renderProgram->setUniformValue(shaderVars.perSideTexCoords[sideNumberInBatch],
						       side.texCoords[0], side.texCoords[1],
						       side.texCoords[2], side.texCoords[3]);
			renderProgram->setUniformValue(shaderVars.whatToRender, 0);
		}
	}
	// Draw the rest
	if(sidePresenceMask)
	{
		renderProgram->setUniformValue(shaderVars.sidePresenceMask, sidePresenceMask);
		gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

// Draw the ground, assuming full-screen-quad VAO is bound and the common uniforms are configured
void LandscapeOldStyle::drawGround(StelCore*const core, const int firstFreeTexSampler) const
{
	if (landFader.getInterstate()==0.f)
		return;

	if (!groundTex)
	{
		qWarning() << "LandscapeOldStyle groundTex is invalid!";
		return;
	}

	const float vshift = (tanMode || calibrated) ? std::tan(groundAngleShift) : std::sin(groundAngleShift);
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(groundAngleRotateZ-static_cast<double>(angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);

	auto& gl = *QOpenGLContext::currentContext()->functions();

	renderProgram->setUniformValue(shaderVars.whatToRender, -1);
	const int texSampler = firstFreeTexSampler;
	groundTex->bind(texSampler);
	renderProgram->setUniformValue(shaderVars.mapTex, texSampler);
	renderProgram->setUniformValue(shaderVars.vshift, vshift);
	renderProgram->setUniformValue(shaderVars.projectionMatrixInverse, prj->getProjectionMatrix().toQMatrix().inverted());
	renderProgram->setUniformValue(shaderVars.brightness,
				       landscapeBrightness, landscapeBrightness, landscapeBrightness,
				       (1.f-landscapeTransparency)*landFader.getInterstate());
	prj->setUnProjectUniforms(*renderProgram);
	gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void LandscapeOldStyle::drawLowGL(StelCore* core, bool onlyPolygon)
{
	if (!validLandscape)
		return;

	StelPainter painter(core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff));
	painter.setBlending(true);
	painter.setCullFace(true);

	if (!onlyPolygon || !horizonPolygon) // Make sure to draw the regular pano when there is no polygon
	{
#ifdef GL_MULTISAMPLE
		const auto gl = painter.glFuncs();
		if (multisamplingEnabled_)
			gl->glEnable(GL_MULTISAMPLE);
#endif

		if (drawGroundFirst)
			drawGroundLowGL(core, painter);
		drawDecorLowGL(core, painter, false);
		if (!drawGroundFirst)
			drawGroundLowGL(core, painter);
		drawFogLowGL(core, painter);

		// Self-luminous layer (Light pollution etc). This looks striking!
		if (lightScapeBrightness>0.0f && (illumFader.getInterstate()>0.f))
		{
			painter.setBlending(true, GL_SRC_ALPHA, GL_ONE);
			drawDecorLowGL(core, painter, true);
		}

#ifdef GL_MULTISAMPLE
		if (multisamplingEnabled_)
			gl->glDisable(GL_MULTISAMPLE);
#endif
	}
	drawHorizonLine(core, painter);
	drawLabels(core, &painter);
}


// Draw the horizon fog
void LandscapeOldStyle::drawFogLowGL(StelCore* core, StelPainter& sPainter) const
{
	if (fogFader.getInterstate()==0.f)
		return;
	if(landFader.getInterstate()==0.f)
		return;
	if (!(core->getSkyDrawer()->getFlagHasAtmosphere()))
		return;

	const float vpos = static_cast<float>(radius) * ((tanMode||calibrated) ? std::tan(fogAngleShift*M_PI_180f) : std::sin(fogAngleShift*M_PI_180f));
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);

	if (calibrated) // new in V0.13: take proper care of the fog layer. This will work perfectly only for calibrated&&tanMode.
		transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZ+angleRotateZOffset)));

	transfo->combine(Mat4d::translation(Vec3d(0.,0.,static_cast<double>(vpos))));
	sPainter.setProjector(core->getProjection(transfo));
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setColor(Vec3f(landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness)),
			  (1.f-landscapeTransparency)*landFader.getInterstate());
	fogTex->bind();
	const double height = radius * static_cast<double>(calibrated?
				(std::tan((fogAltAngle+fogAngleShift)*M_PI_180f) - std::tan(fogAngleShift*M_PI_180f))
				: ((tanMode) ? std::tan(fogAltAngle*M_PI_180f) : std::sin(fogAltAngle*M_PI_180f)));
	sPainter.sCylinder(radius, height, 64, 1);
	sPainter.setBlending(true);
}

// Draw the side textures
void LandscapeOldStyle::drawDecorLowGL(StelCore* core, StelPainter& sPainter, const bool drawLight) const
{
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZ+angleRotateZOffset)));
	sPainter.setProjector(core->getProjection(transfo));

	if (landFader.getInterstate()==0.f)
		return;
	if (drawLight)
		sPainter.setColor(Vec3f(illumFader.getInterstate()*lightScapeBrightness),
				  (1.f-landscapeTransparency)*landFader.getInterstate());
	else
		sPainter.setColor(Vec3f(landscapeBrightness), (1.f-landscapeTransparency)*landFader.getInterstate());

	for (const auto& side : precomputedSides)
	{
		if (side.light==drawLight)
		{
			side.tex->bind();
			sPainter.drawSphericalTriangles(side.arr, true, false, Q_NULLPTR, false);
		}
	}
}

// Draw the ground
void LandscapeOldStyle::drawGroundLowGL(StelCore* core, StelPainter& sPainter) const
{
	if (landFader.getInterstate()==0.f)
		return;
	const float vshift = static_cast<float>(radius) * ((tanMode || calibrated) ? std::tan(groundAngleShift)
							   			   : std::sin(groundAngleShift));
	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(groundAngleRotateZ-static_cast<double>(angleRotateZOffset)) *
			  Mat4d::translation(Vec3d(0,0,static_cast<double>(vshift))));

	sPainter.setProjector(core->getProjection(transfo));
	sPainter.setColor(landscapeBrightness, landscapeBrightness, landscapeBrightness,
			  (1.f-landscapeTransparency)*landFader.getInterstate());

	if(groundTex.isNull())
	{
		qWarning()<<"LandscapeOldStyle groundTex is invalid!";
	}
	else
	{
		groundTex->bind();
	}
	StelVertexArray va(static_cast<const QVector<Vec3d> >(groundVertexArr), StelVertexArray::Triangles,
			   static_cast<const QVector<Vec2f> >(groundTexCoordArr));

	sPainter.drawStelVertexArray(va, true);
}

float LandscapeOldStyle::getOpacity(Vec3d azalt) const
{
	if(!validLandscape) return (azalt[2]>0.0 ? 0.0f : 1.0f);

	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(static_cast<double>(angleRotateZOffset)));

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
	}
	// Else, sample the images...
	float az, alt_rad;
	StelUtils::rectToSphe(&az, &alt_rad, azalt);

	if (alt_rad < decorAngleShift*M_PI_180f) return 1.0f; // below decor, i.e. certainly opaque ground.
	if (alt_rad > (decorAltAngle+decorAngleShift)*M_PI_180f) return 0.0f; // above decor, i.e. certainly free sky.
	if (!calibrated) // the result of this function has no real use here: just complain and return result for math. horizon.
	{
		static QString lastLandscapeName;
		if (lastLandscapeName != name)
		{
			qWarning() << "Dubious result: Landscape " << name
				   << " not calibrated. Opacity test represents mathematical horizon only.";
			lastLandscapeName=name;
		}
		return (azalt[2] > 0 ? 0.0f : 1.0f);
	}
	az = (M_PIf-az) / M_PIf;                             //  0..2 = N.E.S.W.N
	// we go to 0..1 domain, it's easier to think.
	const float xShift=angleRotateZ /(2.0f*M_PIf); // shift value in -1..1 domain
	Q_ASSERT(xShift >= -1.0f);
	Q_ASSERT(xShift <=  1.0f);
	float az_phot=az*0.5f - 0.25f - xShift;      // The 0.25 is caused by regular pano left edge being East. The xShift compensates any configured angleRotateZ
	az_phot=fmodf(az_phot, 1.0f);
	if (az_phot<0) az_phot+=1.0f;                                //  0..1 = image-X for a non-repeating pano photo
	float az_panel =  nbSide*nbDecorRepeat * az_phot; // azimuth in "panel space". Ex for nbS=4, nbDR=3: [0..[12, say 11.4
	float x_in_panel=fmodf(az_panel, 1.0f);
	int currentSide = static_cast<int>(floor(fmodf(az_panel, nbSide)));
	Q_ASSERT(currentSide>=0);
	Q_ASSERT(currentSide<static_cast<int>(nbSideTexs));
	if (sidesImages[currentSide]->isNull()) return 0.0f; // can happen if image is misconfigured and failed to load.
	int x= static_cast<int>(sides[currentSide].texCoords[0] + x_in_panel*(sides[currentSide].texCoords[2]-sides[currentSide].texCoords[0]))
			* sidesImages[currentSide]->width(); // pixel X from left.

	// QImage has pixel 0/0 in top left corner. We must find image Y for optionally cropped images.
	// It should no longer be possible that sample position is outside cropped texture. in this case, assert(0) but again assume full transparency and exit early.

	float y_img_1; // y of the sampled altitude in 0..1 visible image height from bottom
	if (tanMode)
	{
		const float tanAlt=std::tan(alt_rad);
		const float tanTop=std::tan((decorAltAngle+decorAngleShift)*M_PI_180f);
		const float tanBot=std::tan(decorAngleShift*M_PI_180f);
		y_img_1=(tanAlt-tanBot)/(tanTop-tanBot); // Y position 0..1 in visible image height from bottom
	}
	else
	{ // adapted from spherical...
		const float alt_pm1 = 2.0f * alt_rad  / M_PIf;  // sampled altitude, -1...+1 linear in altitude angle
		const float img_top_pm1 = 1.0f-( (90.0f-decorAltAngle-decorAngleShift) / 90.0f); // the top line in -1..+1 (angular)
		if (alt_pm1>img_top_pm1) { Q_ASSERT(0); return 0.0f; } // should have been caught above with alt_rad tests
		const float img_bot_pm1 = 1.0f-((90.0f-decorAngleShift) / 90.0f); // the bottom line in -1..+1 (angular)
		if (alt_pm1<img_bot_pm1) { Q_ASSERT(0); return 1.0f; } // should have been caught above with alt_rad tests

		y_img_1=(alt_pm1-img_bot_pm1)/(img_top_pm1-img_bot_pm1); // the sampled altitude in 0..1 visible image height from bottom
		Q_ASSERT(y_img_1<=1.f);
		Q_ASSERT(y_img_1>=0.f);
	}
	// x0/y0 is lower left, x1/y1 upper right corner.
	float y_baseImg_1 = sides[currentSide].texCoords[1]+ y_img_1*(sides[currentSide].texCoords[3]-sides[currentSide].texCoords[1]);
	int y=static_cast<int>((1.0f-y_baseImg_1)*static_cast<float>(sidesImages[currentSide]->height())); // pixel Y from top.
	QRgb pixVal=sidesImages[currentSide]->pixel(x, y);
/*
#ifndef NDEBUG
	// GZ: please leave the comment available for further development!
	qDebug() << "Oldstyle Landscape sampling: az=" << az*180.0 << "° alt=" << alt_rad*180.0f/M_PI
			 << "°, xShift[-1..+1]=" << xShift << " az_phot[0..1]=" << az_phot
			 << " --> current side panel " << currentSide
			 << ", w=" << sidesImages[currentSide]->width() << " h=" << sidesImages[currentSide]->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
#endif
*/
	return qAlpha(pixVal)/255.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LandscapePolygonal::LandscapePolygonal(float _radius) : Landscape(_radius)
{
	memorySize=(sizeof(LandscapePolygonal));
}

LandscapePolygonal::~LandscapePolygonal()
{
	landscapeLabels.clear();
}

void LandscapePolygonal::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	// loading the polygon has been moved to Landscape::loadCommon(), so
	// that all Landscape classes can use a polygon line.
	loadCommon(landscapeIni, landscapeId);
	const QString type = landscapeIni.value("landscape/type").toString();
	if(type != "polygonal")
	{
		qWarning() << "Landscape type mismatch for landscape "
			   << landscapeId << ", expected polygonal, found "
			   << type << ".  No landscape in use.\n";
		validLandscape = false;
		return;
	}
	if (horizonPolygon.isNull())
	{
		qWarning() << "Landscape " << landscapeId
			   << " does not declare a valid polygonal_horizon_list.  No landscape in use.\n";
		validLandscape = false;
		return;
	}
	groundColor=Vec3f( landscapeIni.value("landscape/ground_color", "0,0,0" ).toString() );
	//flagDrawInFront=landscapeIni.value("landscape/draw_in_foreground", false).toBool(); // manually configured override for a polygonal line plotted into the foreground.
	validLandscape = true;  // assume ok...
	//qDebug() << "PolygonalLandscape" << landscapeId << "loaded, mem size:" << getMemorySize();
}

void LandscapePolygonal::draw(StelCore* core, bool onlyPolygon)
{
	if(!validLandscape) return;
	if(landFader.getInterstate()==0.f) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);
	StelPainter sPainter(prj);

	// Normal transparency mode for the transition blending
	sPainter.setBlending(true);
	sPainter.setCullFace(true);

	if (!onlyPolygon) // The only useful application of the onlyPolygon is a demo which does not fill the polygon
	{
		sPainter.setColor(landscapeBrightness*groundColor, (1.f-landscapeTransparency)*landFader.getInterstate());
#ifdef GL_MULTISAMPLE
		const auto gl = sPainter.glFuncs();
		if (multisamplingEnabled_)
			gl->glEnable(GL_MULTISAMPLE);
#endif

		sPainter.drawSphericalRegion(horizonPolygon.data(), StelPainter::SphericalPolygonDrawModeFill);

#ifdef GL_MULTISAMPLE
		if (multisamplingEnabled_)
			gl->glDisable(GL_MULTISAMPLE);
#endif
	}

	drawHorizonLine(core, sPainter);
	drawLabels(core, &sPainter);
}

float LandscapePolygonal::getOpacity(Vec3d azalt) const
{
	if(!validLandscape) return (azalt[2]>0.0 ? 0.0f : 1.0f);
	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(static_cast<double>(angleRotateZOffset)));

	if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
}

void LandscapePolygonal::setGroundColor(const Vec3f &color)
{
	groundColor=color;
}

////////////////////////////////////////////////////////////////////////////////////////
// LandscapeFisheye
//

LandscapeFisheye::LandscapeFisheye(float _radius)
	: Landscape(_radius)
	, mapTex(StelTextureSP())
	, mapTexFog(StelTextureSP())
	, mapTexIllum(StelTextureSP())
	, mapImage(Q_NULLPTR)
	, texFov(360.)
{
	memorySize=sizeof(LandscapeFisheye);
}

LandscapeFisheye::~LandscapeFisheye()
{
	if (mapImage) delete mapImage;
	landscapeLabels.clear();
}

void LandscapeFisheye::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);

	QString type = landscapeIni.value("landscape/type").toString();
	if(type != "fisheye")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId
			   << ", expected fisheye, found " << type << ".  No landscape in use.\n";
		validLandscape = false;
		return;
	}
	create(name,
	       landscapeIni.value("landscape/texturefov", 360).toFloat(),
	       getTexturePath(landscapeIni.value("landscape/maptex").toString(), landscapeId),
	       getTexturePath(landscapeIni.value("landscape/maptex_fog").toString(), landscapeId),
	       getTexturePath(landscapeIni.value("landscape/maptex_illum").toString(), landscapeId),
	       landscapeIni.value("landscape/angle_rotatez", 0.f).toFloat());
	//qDebug() << "FisheyeLandscape" << landscapeId << "loaded, mem size:" << memorySize;
}


void LandscapeFisheye::create(const QString _name, float _texturefov, const QString& _maptex,
			      const QString &_maptexFog, const QString& _maptexIllum, const float _angleRotateZ)
{
	// qDebug() << _name << " " << _fullpath << " " << _maptex << " " << _texturefov;
	validLandscape = true;  // assume ok...
	name = _name;
	texFov = _texturefov*M_PI_180f;
	angleRotateZ = _angleRotateZ*M_PI_180f;

	auto& texMan = StelApp::getInstance().getTextureManager();
	if (!horizonPolygon)
	{
		mapImage = new QImage(_maptex);
		memorySize+=(mapImage->sizeInBytes());
	}
	mapTex = texMan.createTexture(_maptex, StelTexture::StelTextureParams(true));
	memorySize+=mapTex->getGlSize();

	if (_maptexIllum.length() && (!_maptexIllum.endsWith("/")))
	{
		mapTexIllum = texMan.createTexture(_maptexIllum, StelTexture::StelTextureParams(true));
		if (mapTexIllum)
			memorySize+=mapTexIllum->getGlSize();
	}
	if (_maptexFog.length() && (!_maptexFog.endsWith("/")))
	{
		mapTexFog = texMan.createTexture(_maptexFog, StelTexture::StelTextureParams(true));
		if (mapTexFog)
			memorySize+=mapTexFog->getGlSize();
	}
}


void LandscapeFisheye::draw(StelCore* core, bool onlyPolygon)
{
	if(!initialized) initGL();
	if(!validLandscape) return;
	if(landFader.getInterstate()==0.f) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-static_cast<double>(angleRotateZ+angleRotateZOffset)));
	const StelProjectorP prj = core->getProjection(transfo);

	if(!renderProgram || !prevProjector || !prj->isSameProjection(*prevProjector))
	{
		renderProgram.reset(new QOpenGLShaderProgram);
		prevProjector = prj;

		const auto vert =
			StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
			R"(
ATTRIBUTE highp vec3 vertex;
VARYING highp vec3 ndcPos;
void main()
{
	gl_Position = vec4(vertex, 1.);
	ndcPos = vertex;
}
)";
		bool ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "LandscapeSpherical: Warnings while compiling vertex shader:\n"
					     << renderProgram->log();
		if(!ok) return;

		const auto frag =
			StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
			prj->getUnProjectShader() +
			R"(
VARYING highp vec3 ndcPos;
uniform sampler2D mapTex;
uniform mat4 projectionMatrixInverse;
uniform float texFov;
uniform vec4 brightness;
void main(void)
{
	const float PI = 3.14159265;
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool unprojectSuccess = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, unprojectSuccess).xyz;
	// First we must compute the derivatives of texture coordinates, and only
	// then decide whether we want to display the !unprojectSuccess case.

	modelPos = normalize(modelPos);
	float modelZenithAngle = acos(modelPos.z);

	float r = min(modelZenithAngle / texFov, 0.5);
	vec2 posFromCenter = dot(modelPos,modelPos)==0 ? vec2(0) : r*normalize(modelPos.yx);
	vec2 texc = posFromCenter+vec2(0.5);

	vec4 color = texture2D(mapTex, texc);

	// This "early" return must be done *after* derivatives of
	// texture coordinates are computed above (indirectly by texture2D()).
	if(!unprojectSuccess)
	{
		FRAG_COLOR = vec4(0);
		return;
	}
	FRAG_COLOR = color * brightness;
}
)";
		ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "LandscapeSpherical: Warnings while compiling fragment shader:\n" << renderProgram->log();

		if(!ok) return;

		renderProgram->bindAttributeLocation("vertex", SKY_VERTEX_ATTRIB_INDEX);

		if(!StelPainter::linkProg(renderProgram.get(), "Spherical landscape render program"))
			return;

		renderProgram->bind();
		shaderVars.texFov           = renderProgram->uniformLocation("texFov");
		shaderVars.mapTex           = renderProgram->uniformLocation("mapTex");
		shaderVars.brightness       = renderProgram->uniformLocation("brightness");
		shaderVars.projectionMatrixInverse = renderProgram->uniformLocation("projectionMatrixInverse");
		renderProgram->release();
	}
	if(!renderProgram || !renderProgram->isLinked())
		return;

	auto& gl = *QOpenGLContext::currentContext()->functions();

	if (!onlyPolygon || !horizonPolygon) // Make sure to draw the regular pano when there is no polygon
	{
		renderProgram->bind();
		renderProgram->setUniformValue(shaderVars.brightness,
					       landscapeBrightness, landscapeBrightness,
					       landscapeBrightness, landFader.getInterstate());
		const int mainTexSampler = 0;
		mapTex->bind(mainTexSampler);
		renderProgram->setUniformValue(shaderVars.mapTex, mainTexSampler);
		renderProgram->setUniformValue(shaderVars.texFov, texFov);
		renderProgram->setUniformValue(shaderVars.projectionMatrixInverse,
					       prj->getProjectionMatrix().toQMatrix().inverted());
		prj->setUnProjectUniforms(*renderProgram);

		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		bindVAO();

		gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// NEW since 0.13: Fog also for fisheye...
		if ((mapTexFog) && (core->getSkyDrawer()->getFlagHasAtmosphere()))
		{
			gl.glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			const float brightness = landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness);

			renderProgram->setUniformValue(shaderVars.brightness, brightness, brightness, brightness,
						       landFader.getInterstate());
			mapTexFog->bind();
			gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		if (mapTexIllum && lightScapeBrightness>0.0f && (illumFader.getInterstate()>0.f))
		{
			gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			const float brightness = lightScapeBrightness*illumFader.getInterstate();
			renderProgram->setUniformValue(shaderVars.brightness, brightness, brightness, brightness,
						       landFader.getInterstate());
			mapTexIllum->bind();
			gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		releaseVAO();
		gl.glDisable(GL_BLEND);
		renderProgram->release();
	}

	StelPainter painter(prj);
	drawHorizonLine(core, painter);
	drawLabels(core, &painter);
}

float LandscapeFisheye::getOpacity(Vec3d azalt) const
{
	if(!validLandscape || (!horizonPolygon && (!mapImage || mapImage->isNull())))
		return (azalt[2]>0.0 ? 0.0f : 1.0f); // can happen if image is misconfigured and failed to load.

	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(static_cast<double>(angleRotateZOffset)));

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...
	float az, alt_rad;
	StelUtils::rectToSphe(&az, &alt_rad, azalt);

	// QImage has pixel 0/0 in top left corner.
	// The texture is taken from the center circle in the square texture.
	// It is possible that sample position is outside. in this case, assume full opacity and exit early.
	if (M_PI_2f-alt_rad > texFov/2.0f ) return 1.0f; // outside fov, in the clamped texture zone: always opaque.

	float radius=(M_PI_2f-alt_rad)*2.0f/texFov; // radius in units of mapImage.height/2

	az = (M_PIf-az) - static_cast<float>(angleRotateZ); // 0..+2pi -angleRotateZ, real azimuth. NESW
	//  The texture map has south on top, east at right (if anglerotateZ=0)
	int x= static_cast<int>(mapImage->height()/2*(1 + radius*std::sin(az)));
	int y= static_cast<int>(mapImage->height()/2*(1 + radius*std::cos(az)));

	QRgb pixVal=mapImage->pixel(x, y);
/*
#ifndef NDEBUG
	// GZ: please leave the comment available for further development!
	qDebug() << "Landscape sampling: az=" << (az+angleRotateZ)/M_PI*180.0f << "° alt=" << alt_rad/M_PI*180.f
			 << "°, w=" << mapImage->width() << " h=" << mapImage->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
#endif
*/
	return qAlpha(pixVal)/255.0f;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// spherical panoramas

LandscapeSpherical::LandscapeSpherical(float _radius)
	: Landscape(_radius)
	, mapTex(StelTextureSP())
	, mapTexFog(StelTextureSP())
	, mapTexIllum(StelTextureSP())
	, bottomCap(Vec3d(0.0,0.0,-1.0), 0.0)
	, mapTexTop(0.)
	, mapTexBottom(0.)
	, fogTexTop(0.)
	, fogTexBottom(0.)
	, illumTexTop(0.)
	, illumTexBottom(0.)
	, mapImage(Q_NULLPTR)
	, bottomCapColor(-1.0f, 0.0f, 0.0f)
{
	memorySize=sizeof(LandscapeSpherical);
}

LandscapeSpherical::~LandscapeSpherical()
{
	if (mapImage) delete mapImage;
	landscapeLabels.clear();
}

void LandscapeSpherical::load(const QSettings& landscapeIni, const QString& landscapeId)
{
	loadCommon(landscapeIni, landscapeId);
//	if (horizonPolygon)
//		qDebug() << "This landscape, " << landscapeId << ", has a polygon defined!" ;
//	else
//		qDebug() << "This landscape, " << landscapeId << ", has no polygon defined!" ;

	QString type = landscapeIni.value("landscape/type").toString();
	if (type != "spherical")
	{
		qWarning() << "Landscape type mismatch for landscape "<< landscapeId
			   << ", expected spherical, found " << type
			   << ".  No landscape in use.\n";
		validLandscape = false;
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
	       landscapeIni.value("landscape/maptex_illum_bottom", -90.f).toFloat(),
	       Vec3f(landscapeIni.value("landscape/bottom_cap_color", "-1.0,0.0,0.0").toString()));
	//qDebug() << "SphericalLandscape" << landscapeId << "loaded, mem size:" << memorySize;
}


//// create a spherical landscape from basic parameters (no ini file needed)
void LandscapeSpherical::create(const QString _name, const QString& _maptex, const QString& _maptexFog, const QString& _maptexIllum,
				const float _angleRotateZ,
				const float _mapTexTop, const float _mapTexBottom,
				const float _fogTexTop, const float _fogTexBottom,
				const float _illumTexTop, const float _illumTexBottom, const Vec3f _bottomCapColor)
{
	//qDebug() << "LandscapeSpherical::create():"<< _name << " : " << _maptex << " : " << _maptexFog << " : " << _maptexIllum << " : " << _angleRotateZ;
	validLandscape = true;  // assume ok...
	name = _name;
	angleRotateZ  = _angleRotateZ         *M_PI_180f; // Defined in ini --> internal prg value
	mapTexTop     = (90.f-_mapTexTop)     *M_PI_180f; // top     90     -->   0
	mapTexBottom  = (90.f-_mapTexBottom)  *M_PI_180f; // bottom -90     -->  pi
	fogTexTop     = (90.f-_fogTexTop)     *M_PI_180f;
	fogTexBottom  = (90.f-_fogTexBottom)  *M_PI_180f;
	illumTexTop   = (90.f-_illumTexTop)   *M_PI_180f;
	illumTexBottom= (90.f-_illumTexBottom)*M_PI_180f;
	if (!horizonPolygon)
	{
		mapImage = new QImage(_maptex);
		memorySize+=(mapImage->sizeInBytes());
	}

	auto& gl = *QOpenGLContext::currentContext()->functions();
	auto& texMan = StelApp::getInstance().getTextureManager();

	mapTex = texMan.createTexture(_maptex, StelTexture::StelTextureParams(true));
	memorySize+=mapTex->getGlSize();
	mapTex->bind(0);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	if(!StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER).contains("textureGrad_SUPPORTED"))
	{
		// Avoid mip mapping when we can't supply custom dFdx/dFdy to texture sampler.
		// The noise that may appear due to linear filter is must less distracting than
		// the seam that will appear due to wrong mip level computed from
		// default-computed derivatives.
		gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	mapTex->release();

	if (_maptexIllum.length() && (!_maptexIllum.endsWith("/")))
	{
		mapTexIllum = texMan.createTexture(_maptexIllum, StelTexture::StelTextureParams(true));
		if (mapTexIllum)
		{
			memorySize+=mapTexIllum->getGlSize();
			mapTexIllum->bind(0);
			gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			mapTexIllum->release();
		}
	}
	if (_maptexFog.length() && (!_maptexFog.endsWith("/")))
	{
		mapTexFog = texMan.createTexture(_maptexFog, StelTexture::StelTextureParams(true));
		if (mapTexFog)
		{
			memorySize+=mapTexFog->getGlSize();
			mapTexFog->bind(0);
			gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			mapTexFog->release();
		}
	}	

	// Add a bottom cap in case of maptex_bottom.
	if ((mapTexBottom>-90.f*M_PI_180f) && (_bottomCapColor != Vec3f(-1.0f, 0.0f, 0.0f)))
	{
		bottomCap = SphericalCap(Vec3d(0.0, 0.0, -1.0), cos(M_PI-static_cast<double>(mapTexBottom)));
		bottomCapColor = _bottomCapColor;
	}
}

void LandscapeSpherical::draw(StelCore* core, bool onlyPolygon)
{
	if(!initialized) initGL();
	if(!validLandscape) return;
	if(landFader.getInterstate()==0.f) return;

	StelProjector::ModelViewTranformP transfo = core->getAltAzModelViewTransform(StelCore::RefractionOff);
	transfo->combine(Mat4d::zrotation(-(static_cast<double>(angleRotateZ+angleRotateZOffset))));
	const StelProjectorP prj = core->getProjection(transfo);

	if(!renderProgram || !prevProjector || !prj->isSameProjection(*prevProjector))
	{
		renderProgram.reset(new QOpenGLShaderProgram);
		prevProjector = prj;

		const auto vert =
			StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
			R"(
ATTRIBUTE highp vec3 vertex;
VARYING highp vec3 ndcPos;
void main()
{
	gl_Position = vec4(vertex, 1.);
	ndcPos = vertex;
}
)";
		bool ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "LandscapeSpherical: Warnings while compiling vertex shader:\n"
					     << renderProgram->log();
		if(!ok) return;

		const auto frag =
			StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
			prj->getUnProjectShader() +
			R"(
VARYING highp vec3 ndcPos;
uniform sampler2D mapTex;
uniform mat4 projectionMatrixInverse;
uniform float mapTexTop;
uniform float mapTexBottom;
uniform vec4 bottomCapColor;
uniform vec4 brightness;
void main(void)
{
	const float PI = 3.14159265;
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool unprojectSuccess = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, unprojectSuccess).xyz;
	// First we must compute the derivatives of texture coordinates, and only
	// then decide whether we want to display the !unprojectSuccess case.
	modelPos = normalize(modelPos);
	float modelZenithAngle = acos(modelPos.z);
	float modelLongitude = atan(modelPos.x, modelPos.y);

	vec2 texc = vec2(modelLongitude/(2.*PI),
					 1.-(modelZenithAngle-mapTexTop)/(mapTexBottom-mapTexTop));

#ifdef textureGrad_SUPPORTED
	// The usual automatic computation of derivatives of texture coordinates
	// breaks down at the discontinuity of atan, resulting in choosing the most
	// minified mip level instead of the correct one, which looks as a seam on
	// the screen. Thus, we need to compute them in a custom way, treating atan
	// as a (continuous) multivalued function. We differentiate
	// atan(modelPosX(x,y), modelPosY(x,y)) with respect to x and y and yield
	// gradLongitude vector.
	vec2 gradModelPosX = vec2(dFdx(modelPos.x), dFdy(modelPos.x));
	vec2 gradModelPosY = vec2(dFdx(modelPos.y), dFdy(modelPos.y));
	vec2 gradLongitude = vec2(modelPos.y*gradModelPosX.s-modelPos.x*gradModelPosY.s,
							  modelPos.y*gradModelPosX.t-modelPos.x*gradModelPosY.t)
														/
											 dot(modelPos, modelPos);
	float texTdx = dFdx(texc.t);
	float texTdy = dFdy(texc.t);
	vec2 texDx = vec2(gradLongitude.s/(2.*PI), texTdx);
	vec2 texDy = vec2(gradLongitude.t/(2.*PI), texTdy);
	vec4 color = textureGrad(mapTex, texc, texDx, texDy);
#else
	vec4 color = texture2D(mapTex, texc);
#endif

	// These "early" returns must be done *after* dFdx/dFdy are computed above
	if(!unprojectSuccess)
	{
		FRAG_COLOR = vec4(0);
		return;
	}
	if(modelZenithAngle > mapTexBottom)
	{
		FRAG_COLOR = bottomCapColor;
		return;
	}
	if(modelZenithAngle < mapTexTop)
	{
		FRAG_COLOR = vec4(0);
		return;
	}

	FRAG_COLOR = color * brightness;
}
)";
		ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "LandscapeSpherical: Warnings while compiling fragment shader:\n"
					     << renderProgram->log();

		if(!ok) return;

		renderProgram->bindAttributeLocation("vertex", SKY_VERTEX_ATTRIB_INDEX);

		if(!StelPainter::linkProg(renderProgram.get(), "Spherical landscape render program"))
			return;

		renderProgram->bind();
		shaderVars.mapTex           = renderProgram->uniformLocation("mapTex");
		shaderVars.mapTexTop        = renderProgram->uniformLocation("mapTexTop");
		shaderVars.brightness       = renderProgram->uniformLocation("brightness");
		shaderVars.mapTexBottom     = renderProgram->uniformLocation("mapTexBottom");
		shaderVars.bottomCapColor   = renderProgram->uniformLocation("bottomCapColor");
		shaderVars.projectionMatrixInverse = renderProgram->uniformLocation("projectionMatrixInverse");
		renderProgram->release();
	}
	if(!renderProgram || !renderProgram->isLinked())
		return;

	auto& gl = *QOpenGLContext::currentContext()->functions();

	if (!onlyPolygon || !horizonPolygon) // Make sure to draw the regular pano when there is no polygon
	{
		renderProgram->bind();
		renderProgram->setUniformValue(shaderVars.bottomCapColor,
					       landscapeBrightness*bottomCapColor[0],
					       landscapeBrightness*bottomCapColor[1],
					       landscapeBrightness*bottomCapColor[2],
					       bottomCapColor[0] < 0 ? 0 : landFader.getInterstate());

		renderProgram->setUniformValue(shaderVars.brightness,
					       landscapeBrightness, landscapeBrightness,
					       landscapeBrightness, (1.f-landscapeTransparency)*landFader.getInterstate());
		const int mainTexSampler = 0;
		mapTex->bind(mainTexSampler);
		renderProgram->setUniformValue(shaderVars.mapTex, mainTexSampler);
		renderProgram->setUniformValue(shaderVars.mapTexTop, mapTexTop);
		renderProgram->setUniformValue(shaderVars.mapTexBottom, mapTexBottom);

		renderProgram->setUniformValue(shaderVars.projectionMatrixInverse,
					       prj->getProjectionMatrix().toQMatrix().inverted());
		prj->setUnProjectUniforms(*renderProgram);

		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		bindVAO();

		gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Since 0.13: Fog also for sphericals...
		if ((mapTexFog) && (core->getSkyDrawer()->getFlagHasAtmosphere()))
		{
			gl.glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			const float brightness =
				landFader.getInterstate()*fogFader.getInterstate()*(0.1f+0.1f*landscapeBrightness);

			renderProgram->setUniformValue(shaderVars.bottomCapColor, 0.f, 0.f, 0.f, 0.f);
			renderProgram->setUniformValue(shaderVars.brightness,
						       brightness, brightness, brightness,
						       (1.f-landscapeTransparency)*landFader.getInterstate());
			mapTexFog->bind();
			renderProgram->setUniformValue(shaderVars.mapTexTop, fogTexTop);
			renderProgram->setUniformValue(shaderVars.mapTexBottom, fogTexBottom);
			gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		// Self-luminous layer (Light pollution etc). This looks striking!
		if (mapTexIllum && (lightScapeBrightness>0.0f) && (illumFader.getInterstate()>0.0f))
		{
			gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			const float brightness = lightScapeBrightness*illumFader.getInterstate();
			renderProgram->setUniformValue(shaderVars.bottomCapColor, 0.f, 0.f, 0.f, 0.f);
			renderProgram->setUniformValue(shaderVars.brightness,
						       brightness, brightness, brightness,
						       (1.f-landscapeTransparency)*landFader.getInterstate());
			mapTexIllum->bind();
			renderProgram->setUniformValue(shaderVars.mapTexTop, illumTexTop);
			renderProgram->setUniformValue(shaderVars.mapTexBottom, illumTexBottom);
			gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		releaseVAO();
		gl.glDisable(GL_BLEND);
		renderProgram->release();
	}

	StelPainter sPainter(prj);
	drawHorizonLine(core, sPainter);
	drawLabels(core, &sPainter);
}

//! Sample landscape texture for transparency. May be used for advanced
//! visibility computation like sunrise on the visible horizon etc.
//! @param azalt: normalized direction in alt-az frame
//! @retval alpha (0..1), where 0=fully transparent.
float LandscapeSpherical::getOpacity(Vec3d azalt) const
{
	if(!validLandscape || (!horizonPolygon && (!mapImage || mapImage->isNull())))
		return (azalt[2]>0.0 ? 0.0f : 1.0f); // can happen if image is misconfigured and failed to load.

	if (angleRotateZOffset!=0.0f)
		azalt.transfo4d(Mat4d::zrotation(static_cast<double>(angleRotateZOffset)));

	// in case we also have a horizon polygon defined, this is trivial and fast.
	if (horizonPolygon)
	{
		if (horizonPolygon->contains(azalt)) return 1.0f; else return 0.0f;
	}
	// Else, sample the image...
	float az, alt_rad;
	StelUtils::rectToSphe(&az, &alt_rad, azalt);

	// QImage has pixel 0/0 in top left corner. We must first find image Y for optionally cropped images.
	// It is possible that sample position is outside cropped texture. in this case, assume full transparency and exit early.

	const float alt_pm1 = 2.0f * alt_rad / M_PIf;               // sampled altitude, -1...+1 linear in altitude angle
	const float img_top_pm1 = 1.0f-2.0f*(mapTexTop    / M_PIf); // the top    line in -1..+1
	if (alt_pm1>img_top_pm1) return 0.0f;
	const float img_bot_pm1 = 1.0f-2.0f*(mapTexBottom / M_PIf); // the bottom line in -1..+1
	if (alt_pm1<img_bot_pm1) return 1.0f; // rare case of a hole in the ground. Even though there is a visible hole, play opaque.

	float y_img_1=(alt_pm1-img_bot_pm1)/(img_top_pm1-img_bot_pm1); // the sampled altitude in 0..1 image height from bottom
	Q_ASSERT(y_img_1<=1.f);
	Q_ASSERT(y_img_1>=0.f);

	int y=static_cast<int>((1.0f-y_img_1)*mapImage->height());           // pixel Y from top.

	az = (M_PIf-az) / M_PIf;                            //  0..2 = N.E.S.W.N

	const float xShift=static_cast<float>(angleRotateZ) /M_PIf; // shift value in -2..2
	float az_phot=az - 0.5f - xShift;      // The 0.5 is caused by regular pano left edge being East. The xShift compensates any configured angleRotateZ
	az_phot=fmodf(az_phot, 2.0f);
	if (az_phot<0) az_phot+=2.0f;                                //  0..2 = image-X

	int x=static_cast<int>((az_phot*0.5f) * mapImage->width()); // pixel X from left.

	QRgb pixVal=mapImage->pixel(x, y);
/*
#ifndef NDEBUG
	// GZ: please leave the comment available for further development!
	qDebug() << "Landscape sampling: az=" << az*180.0 << "° alt=" << alt_pm1*90.0f
			 << "°, xShift[-2..+2]=" << xShift << " az_phot[0..2]=" << az_phot
			 << ", w=" << mapImage->width() << " h=" << mapImage->height()
			 << " --> x:" << x << " y:" << y << " alpha:" << qAlpha(pixVal)/255.0f;
#endif
*/
	return qAlpha(pixVal)/255.0f;
}
