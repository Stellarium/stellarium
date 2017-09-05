/*
 * Stellarium
 * Copyright (C) 2017 Guillaume Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelHips.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelTextureMgr.hpp"

#include <QNetworkReply>
#include <QTimeLine>

// Declare functions defined in healpix.c
extern "C" {
	void healpix_pix2vec(int nside, int pix, double out[3]);
	void healpix_get_mat3(int nside, int pix, double out[3][3]);
	void healpix_xy2vec(const double xy[2], double out[3]);
}

class HipsTile
{
public:
	int order;
	int pix;
	StelTextureSP texture = NULL;
	StelTextureSP allsky = NULL; // allsky low res version of the texture.

	// Used for smooth fade in
	QTimeLine texFader;
};

static QString getExt(const QString& format)
{
	foreach(QString ext, format.split(' '))
	{
		if (ext == "jpeg") return "jpg";
		if (ext == "png") return "png";
	}
	return QString();
}

HipsSurvey::HipsSurvey(const QString& url):
	url(url),
	tiles(1000)
{
}

HipsSurvey::~HipsSurvey()
{

}

bool HipsSurvey::parseProperties()
{
	if (propertiesParsed) return true;
	if (networkReply) return false;
	QNetworkRequest req = QNetworkRequest(url + "/properties");
	networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, &QNetworkReply::finished, [=] {
		// Default properties
		properties.hips_order_min = 3;
		QByteArray data = networkReply->readAll();
		foreach(QString line, data.split('\n'))
		{
			QString key = line.section("=", 0, 0).trimmed();
			QString value = line.section("=", 1, -1).trimmed();
			if (key == "hips_order")
				properties.hips_order = value.toInt();
			if (key == "hips_order_min")
				properties.hips_order_min = value.toInt();
			if (key == "hips_tile_format")
				properties.hips_tile_format = value;
			if (key == "hips_tile_width")
				properties.hips_tile_width = value.toInt();
			if (key == "hips_frame")
				properties.hips_frame = value;
		}
		propertiesParsed = true;
		delete networkReply;
		networkReply = NULL;
	});
	return propertiesParsed;
}

bool HipsSurvey::getAllsky()
{
	if (!allsky.isNull()) return true;
	if (!propertiesParsed || networkReply || noAllsky)
		return false;
	QString ext = getExt(properties.hips_tile_format);
	QString path = QString("%1/Norder%2/Allsky.%3").arg(url).arg(properties.hips_order_min).arg(ext);
	QNetworkRequest req = QNetworkRequest(path);
	networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, &QNetworkReply::finished, [=] {
		QByteArray data = networkReply->readAll();
		allsky = QImage::fromData(data);
		propertiesParsed = true;
		delete networkReply;
		networkReply = NULL;
	});
	return !allsky.isNull();
}

void HipsSurvey::draw(StelPainter* sPainter)
{
	// We don't draw anything until we get the properties file and the
	// allsky texture (if available).
	if (!parseProperties()) return;
	if (!getAllsky()) return;

	// Set the projection.
	StelCore* core = StelApp::getInstance().getCore();
	StelCore::FrameType frame;
	if (properties.hips_frame == "galactic")
		frame = StelCore::FrameGalactic;
	else
		frame = StelCore::FrameJ2000;
	sPainter->setProjector(core->getProjection(frame));

	// Compute the maximum visible level for the tiles according to the view resolution.
	// We know that each tile at level L represents an angle of 90 / 2^L
	// The maximum angle we want to see is the size of a tile in pixels time the angle for one visible pixel.
	const double anglePerPixel = 1./sPainter->getProjector()->getPixelPerRadAtCenter()*180./M_PI;
	const double maxAngle = anglePerPixel * properties.hips_tile_width;
	int drawOrder = (int)(log2(90. / maxAngle));
	drawOrder = qBound(properties.hips_order_min, drawOrder, properties.hips_order);

	// Draw the 12 root tiles and their children.
	const SphericalCap& viewportRegion = sPainter->getProjector()->getBoundingCap();
	for (int i = 0; i < 12; i++)
	{
		drawTile(0, i, drawOrder, viewportRegion, sPainter);
	}
}

HipsTile* HipsSurvey::getTile(int order, int pix)
{
	int nside = 1 << order;
	long int uid = pix + 4L * nside * nside;
	HipsTile* tile = tiles[uid];
	if (!tile)
	{
		StelTextureMgr& texMgr = StelApp::getInstance().getTextureManager();
		tile = new HipsTile();
		tile->order = order;
		tile->pix = pix;
		QString ext = getExt(properties.hips_tile_format);
		QString path = QString("%1/Norder%2/Dir%3/Npix%4.%5").arg(url).arg(order).arg((pix / 10000) * 10000).arg(pix).arg(ext);
		tile->texture = texMgr.createTextureThread(path, StelTexture::StelTextureParams(true), false);
		tiles.insert(uid, tile);

		// Use the allsky image until we load the full texture.
		if (order == properties.hips_order_min && !allsky.isNull())
		{
			int nbw = sqrt(12 * 1 << (2 * order));
			int x = (pix % nbw) * allsky.width() / nbw;
			int y = (pix / nbw) * allsky.width() / nbw;
			int s = allsky.width() / nbw;
			QImage image = allsky.copy(x, y, s, s);
			tile->allsky = texMgr.createTexture(image, StelTexture::StelTextureParams(true));
		}
	}
	return tile;
}

void HipsSurvey::drawTile(int order, int pix, int drawOrder, const SphericalCap& viewportShape, StelPainter* sPainter)
{
	// Check if the tile is visible.
	SphericalCap boundingCap;
	Vec3d pos;
	Mat3d mat3;
	Vec3d verts[4];
	Vec2f uv[4] = {Vec2f(0, 0), Vec2f(1, 0), Vec2f(0, 1), Vec2f(1, 1)};
	unsigned short indices[6] = {0, 2, 1, 3, 1, 2};
	HipsTile *tile;

	healpix_pix2vec(1 << order, pix, pos.v);
	boundingCap.n = pos;
	boundingCap.d = cos(M_PI / 2.0 / (1 << order));

	if (!viewportShape.intersects(boundingCap)) return;

	if (order < properties.hips_order_min)
		goto skip_render;

	tile = getTile(order, pix);
	if (!tile) return;
	if (!tile->texture->bind() && (!tile->allsky || !tile->allsky->bind()))
		return;
	if (tile->texFader.state() == QTimeLine::NotRunning && tile->texFader.currentValue() == 0.0)
		tile->texFader.start();

	// Actually draw the tile, as a single quad.
	healpix_get_mat3(1 << order, pix, (double(*)[3])mat3.r);
	for (int i = 0; i < 4; i++)
	{
		pos = mat3 * Vec3d(1 - uv[i][1], uv[i][0], 1.0);
		healpix_xy2vec(pos.v, verts[i].v);
	}
	if (tile->texFader.state() == QTimeLine::Running)
	{
		sPainter->setBlending(true);
		sPainter->setColor(1, 1, 1, tile->texFader.currentValue());
	}
	else
	{
		sPainter->setBlending(false);
		sPainter->setColor(1, 1, 1, 1);
	}

	sPainter->setCullFace(true);
	sPainter->setArrays(verts, uv);
	sPainter->drawFromArray(StelPainter::Triangles, 6, 0, true, indices);

skip_render:
	// Draw the children.
	if (order < drawOrder)
	{
		for (int i = 0; i < 4; i++)
			drawTile(order + 1, pix * 4 + i, drawOrder, viewportShape, sPainter);
		return;
	}
}
