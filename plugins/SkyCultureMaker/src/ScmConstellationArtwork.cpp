/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScmConstellationArtwork.hpp"
#include "ConstellationMgr.hpp"
#include "StarMgr.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelTextureMgr.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>

scm::ScmConstellationArtwork::ScmConstellationArtwork(const std::array<Anchor, 3> &anchors, const QImage &artwork)
	: anchors(anchors)
	, artwork(artwork)
	, hasArt(true)
{
	initValues();
}

scm::ScmConstellationArtwork::ScmConstellationArtwork()
	: hasArt(false)
{
	initValues();
}

void scm::ScmConstellationArtwork::initValues()
{
	ConstellationMgr *constMgr = GETSTELMODULE(ConstellationMgr);
	artIntensity = constMgr->getArtIntensity();
	QObject::connect(constMgr, &ConstellationMgr::artIntensityChanged, [this](float v) { artIntensity = v; });
}

void scm::ScmConstellationArtwork::setupArt()
{
	if (hasArt == false)
	{
		qWarning() << "SkyCultureMaker: Failed to setup the artwork, because it has no art";
		return;
	}

	StelApp &app     = StelApp::getInstance();
	StarMgr *starMgr = GETSTELMODULE(StarMgr);
	StelCore *core   = app.getCore();

	if (starMgr == nullptr)
	{
		qWarning() << "SkyCultureMaker: Failed to setup the artwork, because the starMgr is not available";
		return;
	}

	artTexture = app.getTextureManager().createTexture(artwork, StelTexture::StelTextureParams(true));

	// Coded is copied from src/modules/ConstellationMgr.cpp from loadlinesNamesAndArt
	// This is done because the functions are public and not separable to only showing the Art
	StelObjectP s1obj = starMgr->searchHP(anchors[0].hip);
	StelObjectP s2obj = starMgr->searchHP(anchors[1].hip);
	StelObjectP s3obj = starMgr->searchHP(anchors[2].hip);

	// check for null pointers
	if (s1obj.isNull() || s2obj.isNull() || s3obj.isNull())
	{
		qWarning() << "SkyCultureMaker: could not find stars:" << anchors[0].hip << ", " << anchors[1].hip
			   << "or " << anchors[2].hip;
		return;
	}

	const Vec3d s1 = s1obj->getJ2000EquatorialPos(core);
	const Vec3d s2 = s2obj->getJ2000EquatorialPos(core);
	const Vec3d s3 = s3obj->getJ2000EquatorialPos(core);

	const Vec2i &xy1 = anchors[0].position;
	const Vec2i &xy2 = anchors[1].position;
	const Vec2i &xy3 = anchors[2].position;
	const int x1     = xy1[0];
	const int y1     = xy1[1];
	const int x2     = xy2[0];
	const int y2     = xy2[1];
	const int x3     = xy3[0];
	const int y3     = xy3[1];

	const int texSizeX = artwork.width();
	const int texSizeY = artwork.height();

	// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
	// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate space
	// We need 3 stars and the 4th point is deduced from the others to get a normal base
	// X = B inv(A)
	Vec3d s4 = s1 + ((s2 - s1) ^ (s3 - s1));
	Mat4d B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
	Mat4d A(x1, texSizeY - static_cast<int>(y1), 0., 1., x2, texSizeY - static_cast<int>(y2), 0., 1., x3,
	        texSizeY - static_cast<int>(y3), 0., 1., x1, texSizeY - static_cast<int>(y1), texSizeX, 1.);
	Mat4d X = B * A.inverse();

	// Tessellate on the plane assuming a tangential projection for the image
	static const int nbPoints = 5;
	QVector<Vec2f> texCoords;
	texCoords.reserve(nbPoints * nbPoints * 6);
	for (int j = 0; j < nbPoints; ++j)
	{
		for (int i = 0; i < nbPoints; ++i)
		{
			texCoords << Vec2f((static_cast<float>(i)) / nbPoints, (static_cast<float>(j)) / nbPoints);
			texCoords << Vec2f((static_cast<float>(i) + 1.f) / nbPoints, (static_cast<float>(j)) / nbPoints);
			texCoords << Vec2f((static_cast<float>(i)) / nbPoints, (static_cast<float>(j) + 1.f) / nbPoints);
			texCoords << Vec2f((static_cast<float>(i) + 1.f) / nbPoints, (static_cast<float>(j)) / nbPoints);
			texCoords << Vec2f((static_cast<float>(i) + 1.f) / nbPoints,
			                   (static_cast<float>(j) + 1.f) / nbPoints);
			texCoords << Vec2f((static_cast<float>(i)) / nbPoints, (static_cast<float>(j) + 1.f) / nbPoints);
		}
	}

	QVector<Vec3d> contour;
	contour.reserve(texCoords.size());
	for (const auto &v : std::as_const(texCoords))
	{
		Vec3d vertex = X *
		               Vec3d(static_cast<double>(v[0]) * texSizeX, static_cast<double>(v[1]) * texSizeY, 0.);
		// Originally the projected texture plane remained as tangential plane.
		// The vertices should however be reduced to the sphere for correct aberration:
		vertex.normalize();
		contour << vertex;
	}

	artPolygon.vertex        = contour;
	artPolygon.texCoords     = texCoords;
	artPolygon.primitiveType = StelVertexArray::Triangles;

	Vec3d tmp(X * Vec3d(0.5 * texSizeX, 0.5 * texSizeY, 0.));
	tmp.normalize();
	Vec3d tmp2(X * Vec3d(0., 0., 0.));
	tmp2.normalize();
	boundingCap.n = tmp;
	boundingCap.d = tmp * tmp2;

	isSetup = true;
}

void scm::ScmConstellationArtwork::setAnchor(int index, const Anchor &anchor)
{
	if (index < 0 || index >= static_cast<int>(anchors.size()))
	{
		qDebug() << "SkyCultureMaker: Index ouf of bounds for setting an anchor.";
		return;
	}

	anchors[index] = anchor;
}

const std::array<scm::Anchor, 3> &scm::ScmConstellationArtwork::getAnchors() const
{
	return anchors;
}

void scm::ScmConstellationArtwork::setArtwork(const QImage &artwork)
{
	ScmConstellationArtwork::artwork = artwork;
	hasArt                           = true;
}

const QImage &scm::ScmConstellationArtwork::getArtwork() const
{
	return artwork;
}

bool scm::ScmConstellationArtwork::getHasArt() const
{
	return hasArt;
}

void scm::ScmConstellationArtwork::reset()
{
	hasArt  = false;
	isSetup = false;
}

void scm::ScmConstellationArtwork::draw(StelCore *core) const
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	draw(core, painter);
}

void scm::ScmConstellationArtwork::draw(StelCore *core, StelPainter &painter) const
{
	if (hasArt == false)
	{
		return;
	}

	if (isSetup == false)
	{
		qWarning() << "SkyCultureMaker: Failed to draw the artwork: call setup first";
		return;
	}

	painter.setBlending(true, GL_ONE, GL_ONE);
	painter.setCullFace(true);

	Vec3d vel(0.);
	if (core->getUseAberration())
	{
		vel = core->getAberrationVec(core->getJDE());
	}

	SphericalRegionP region = painter.getProjector()->getViewportConvexPolygon();
	drawOptimized(painter, *region, vel);
}

QJsonObject scm::ScmConstellationArtwork::toJson(const QString &relativePath) const
{
	QJsonObject json;

	json["file"] = relativePath;

	QJsonArray artworkSizeJson;
	artworkSizeJson.append(artwork.width());
	artworkSizeJson.append(artwork.height());
	json["size"] = artworkSizeJson;

	QJsonArray anchorsJson;

	for (const auto &anchor : anchors)
	{
		QJsonObject anchorJson;

		auto &pos = anchor.position;
		QJsonArray posAnchorJson;
		posAnchorJson.append(pos[0]);
		posAnchorJson.append(pos[1]);
		anchorJson["pos"] = posAnchorJson;

		anchorJson["hip"] = anchor.hip;

		anchorsJson.append(anchorJson);
	}

	json["anchors"] = anchorsJson;

	return json;
}

bool scm::ScmConstellationArtwork::save(const QString &filepath) const
{
	QFileInfo fileInfo(filepath);

	// Create folder structure if not existing
	bool success = fileInfo.absoluteDir().mkpath(fileInfo.absolutePath());
	if (success == false)
	{
		qWarning() << "SkyCultureMaker: Failed to create the directory structure for: '"
			   << fileInfo.absolutePath() << "'";
		return false;
	}

	success = artwork.save(fileInfo.absoluteFilePath());
	if (success == false)
	{
		qWarning() << "SkyCultureMaker: Failed to save the image to the given path: '"
			   << fileInfo.absoluteFilePath() << "'";
		return false;
	}

	return true;
}

void scm::ScmConstellationArtwork::drawOptimized(StelPainter &sPainter, const SphericalRegion &region,
                                                 const Vec3d &obsVelocity) const
{
	const float intensity = artIntensity * artIntensityFovScale;
	if (artTexture && intensity > 0.0f && region.intersects(boundingCap))
	{
		sPainter.setColor(intensity, intensity, intensity);

		// The texture is not fully loaded
		if (artTexture->bind() == false) return;

#ifdef Q_OS_LINUX
		// Unfortunately applying aberration to the constellation artwork causes ugly artifacts visible on Linux.
		// It is better to disable aberration in this case and have a tiny texture shift where it usually does not need to critically match.
		sPainter.drawStelVertexArray(artPolygon, false, Vec3d(0.));
#else
		sPainter.drawStelVertexArray(artPolygon, false, obsVelocity);
#endif
	}
}
