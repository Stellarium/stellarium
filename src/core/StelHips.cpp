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
	if (!properties.isEmpty()) return true;
	if (!networkReply)
	{
		QNetworkRequest req = QNetworkRequest(url + "/properties");
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	}
	if (networkReply->isFinished())
	{
		// XXX: check for errors.
		QByteArray data = networkReply->readAll();
		foreach(QString line, data.split('\n'))
		{
			QString key = line.section("=", 0, 0).trimmed();
			QString value = line.section("=", 1, -1).trimmed();
			properties[key] = value;
		}
		delete networkReply;
		networkReply = NULL;
	}
	return !properties.isEmpty();
}

int HipsSurvey::getPropertyInt(const QString& key, int fallback)
{
	if (!properties.contains(key)) return fallback;
	return properties[key].toInt();
}

bool HipsSurvey::getAllsky()
{
	if (!allsky.isNull() || noAllsky) return true;
	if (properties.isEmpty()) return false;
	if (!networkReply)
	{
		QString ext = getExt(properties["hips_tile_format"]);
		QString path = QString("%1/Norder%2/Allsky.%3").arg(url).arg(getPropertyInt("hips_order_min", 3)).arg(ext);
		QNetworkRequest req = QNetworkRequest(path);
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	}
	if (networkReply->isFinished())
	{
		QByteArray data = networkReply->readAll();
		allsky = QImage::fromData(data);
		delete networkReply;
		networkReply = NULL;
	};
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
	StelCore::FrameType frame = StelCore::FrameUninitialized;
	if (properties["hips_frame"] == "galactic")
		frame = StelCore::FrameGalactic;
	else if (properties["hips_frame"] == "equatorial")
		frame = StelCore::FrameJ2000;
	if (frame)
		sPainter->setProjector(core->getProjection(frame));

	// Compute the maximum visible level for the tiles according to the view resolution.
	// We know that each tile at level L represents an angle of 90 / 2^L
	// The maximum angle we want to see is the size of a tile in pixels time the angle for one visible pixel.
	const double anglePerPixel = 1./sPainter->getProjector()->getPixelPerRadAtCenter()*180./M_PI;
	int tileWidth = getPropertyInt("hips_tile_width");
	int orderMin = getPropertyInt("hips_order_min", 3);
	int order = getPropertyInt("hips_order");
	const double maxAngle = anglePerPixel * tileWidth;
	int drawOrder = (int)(log2(90. / maxAngle));
	drawOrder = qBound(orderMin, drawOrder, order);
	drawOrder = 1; // Test.

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
	int orderMin = getPropertyInt("hips_order_min", 3);
	HipsTile* tile = tiles[uid];
	if (!tile)
	{
		StelTextureMgr& texMgr = StelApp::getInstance().getTextureManager();
		tile = new HipsTile();
		tile->order = order;
		tile->pix = pix;
		QString ext = getExt(properties["hips_tile_format"]);
		QString path = QString("%1/Norder%2/Dir%3/Npix%4.%5").arg(url).arg(order).arg((pix / 10000) * 10000).arg(pix).arg(ext);
		tile->texture = texMgr.createTextureThread(path, StelTexture::StelTextureParams(true), false);
		tiles.insert(uid, tile);

		// Use the allsky image until we load the full texture.
		if (order == orderMin && !allsky.isNull())
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

// Test if a shape in clipping coordinate is clipped or not.
static bool isClipped(int n, double (*pos)[4])
{
    // The six planes equations:
    const int P[6][4] = {
        {-1, 0, 0, -1}, {1, 0, 0, -1},
        {0, -1, 0, -1}, {0, 1, 0, -1},
        {0, 0, -1, -1}, {0, 0, 1, -1}
    };
    int i, p;
    for (p = 0; p < 6; p++) {
        for (i = 0; i < n; i++) {
            if (    P[p][0] * pos[i][0] +
                    P[p][1] * pos[i][1] +
                    P[p][2] * pos[i][2] +
                    P[p][3] * pos[i][3] <= 0) {
                break;
            }
        }
        if (i == n) // All the points are outside a clipping plane.
            return true;
    }
    return false;
}


void HipsSurvey::drawTile(int order, int pix, int drawOrder, const SphericalCap& viewportShape, StelPainter* sPainter)
{
	Vec3d pos;
	Mat3d mat3;
	Vec3d verts[4];
	// Vec2f uv[4] = {Vec2f(0, 0), Vec2f(1, 0), Vec2f(0, 1), Vec2f(1, 1)};
	Vec2f uv[4] = {Vec2f(0, 0), Vec2f(0, 1), Vec2f(1, 0), Vec2f(1, 1)};
	unsigned short indices[6] = {0, 2, 1, 3, 1, 2};
	HipsTile *tile;
	int orderMin = getPropertyInt("hips_order_min", 3);

	healpix_pix2vec(1 << order, pix, pos.v);

	// Check if the tile is visible.
	if (0)
	{
		SphericalCap boundingCap;
		boundingCap.n = pos;
		boundingCap.d = cos(M_PI / 2.0 / (1 << order));
		if (!viewportShape.intersects(boundingCap)) return;
	}

	double clip_pos[4][4];
	healpix_get_mat3(1 << order, pix, (double(*)[3])mat3.r);
	auto proj = sPainter->getProjector();
	for (int i = 0; i < 4; i++)
	{
		pos = mat3 * Vec3d(1 - uv[i][1], uv[i][0], 1.0);
		healpix_xy2vec(pos.v, pos.v);
		proj->projectInPlace(pos);
		pos[0] = (pos[0] - proj->getViewportCenter()[0]) / proj->getViewportWidth() * 2.0;
		pos[1] = (pos[1] - proj->getViewportCenter()[1]) / proj->getViewportHeight() * 2.0;
		clip_pos[i][0] = pos[0];
		clip_pos[i][1] = pos[1];
		clip_pos[i][2] = 0.0;
		clip_pos[i][3] = 1.0;
	}
	if (isClipped(4, clip_pos)) return;

	// Also check the culling.
	if (order > 0)
	{
		Vec2d u(clip_pos[1][0] - clip_pos[0][0], clip_pos[1][1] - clip_pos[0][1]);
		Vec2d v(clip_pos[2][0] - clip_pos[0][0], clip_pos[2][1] - clip_pos[0][1]);
		u.normalize();
		v.normalize();
		// XXX: the error (0.5) depends on the order: the higher the order
		// the lower the error should be.
		if (u[0] * v[1] - u[1] * v[0] > 0.5) return;
	}

	if (order < orderMin)
		goto skip_render;

	tile = getTile(order, pix);
	if (!tile) return;
	if (!tile->texture->bind() && (!tile->allsky || !tile->allsky->bind()))
		return;
	if (tile->texFader.state() == QTimeLine::NotRunning && tile->texFader.currentValue() == 0.0)
		tile->texFader.start();

	// XXX: we should be able to render the children tiles on top of the
	// parent tiles, but the depth function doesn't work well then!
	if (order < drawOrder)
		goto skip_render;

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
