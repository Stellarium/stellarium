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
#include "StelUtils.hpp"
#include "StelProgressController.hpp"

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
	StelTextureSP texture = StelTextureSP(Q_NULLPTR);
	StelTextureSP allsky = StelTextureSP(Q_NULLPTR); // allsky low res version of the texture.

	// Used for smooth fade in
	QTimeLine texFader;
};

static QString getExt(const QString& format)
{
	for (auto ext : format.split(' '))
	{
		if (ext == "jpeg") return "jpg";
		if (ext == "png") return "png";
	}
	return QString();
}

QUrl HipsSurvey::getUrlFor(const QString& path) const
{
	QUrl base = url;
	QString args = "";
	if (base.scheme().isEmpty()) base.setScheme("file");
	if (base.scheme() != "file")
		args += QString("?v=%1").arg(static_cast<int>(releaseDate));
	return QString("%1/%2%3").arg(base.url()).arg(path).arg(args);
}

HipsSurvey::HipsSurvey(const QString& url_, double releaseDate_):
	url(url_),
	releaseDate(releaseDate_),
	planetarySurvey(false),
	tiles(1000 * 512 * 512), // Cache max cost in pixels (enough for 1000 512x512 tiles).
	nbVisibleTiles(0),
	nbLoadedTiles(0)
{
	// Immediatly download the properties.
	QNetworkRequest req = QNetworkRequest(getUrlFor("properties"));
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
	req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
	QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, &QNetworkReply::finished, [&, networkReply] {
		QByteArray data = networkReply->readAll();
		for (QString line : data.split('\n'))
		{
			if (line.startsWith("#")) continue;
			QString key = line.section("=", 0, 0).trimmed();
			if (key.isEmpty()) continue;
			QString value = line.section("=", 1, -1).trimmed();
			properties[key] = value;
		}
		if (properties.contains("hips_release_date"))
		{
			// XXX: StelUtils::getJulianDayFromISO8601String does not work
			// without the seconds!
			QDateTime date = QDateTime::fromString(properties["hips_release_date"].toString(), Qt::ISODate);
			releaseDate = StelUtils::qDateTimeToJd(date);
		}
		if (properties.contains("hips_frame"))
			hipsFrame = properties["hips_frame"].toString().toLower();

		QStringList DSSSurveys;
		DSSSurveys << "equatorial" << "galactic" << "ecliptic"; // HiPS frames for DSS surveys
		if (DSSSurveys.contains(hipsFrame, Qt::CaseInsensitive) && !(properties["creator_did"].toString().contains("moon", Qt::CaseInsensitive)) && !(properties["client_category"].toString().contains("solar system", Qt::CaseInsensitive)))
			planetarySurvey = false;
		else
			planetarySurvey = true;

		emit propertiesChanged();
		emit statusChanged();
		networkReply->deleteLater();
	});
}

HipsSurvey::~HipsSurvey()
{
}

bool HipsSurvey::isVisible() const
{
	return static_cast<bool>(fader);
}

void HipsSurvey::setVisible(bool value)
{
	if (value == isVisible()) return;
	fader = value;
	if (!value && progressBar)
	{
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
	emit visibleChanged(value);
}

int HipsSurvey::getPropertyInt(const QString& key, int fallback)
{
	if (!properties.contains(key)) return fallback;
	QJsonValue val = properties[key];
	if (val.isDouble()) return val.toInt();
	if (val.isString()) return val.toString().toInt();
	return 0;
}

bool HipsSurvey::getAllsky()
{
	if (!allsky.isNull() || noAllsky) return true;
	if (properties.isEmpty()) return false;

	// Allsky is deprecated after version 1.4.
	if (properties.contains("hips_version")) {
		QStringList version = properties["hips_version"].toString().split(".");
		if ((version.size() >= 2) && (version[0].toInt() * 100 + version[1].toInt() >= 104)) {
			noAllsky = true;
			return true;
		}
	}

	if (!networkReply)
	{
		QString ext = getExt(properties["hips_tile_format"].toString());
		QUrl path = getUrlFor(QString("Norder%1/Allsky.%2").arg(getPropertyInt("hips_order_min", 3)).arg(ext));
		qDebug() << "Load allsky" << path;
		QNetworkRequest req = QNetworkRequest(path);
		req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
		networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
		emit statusChanged();

		updateProgressBar(0, 100);
		connect(networkReply, &QNetworkReply::downloadProgress, [this](qint64 received, qint64 total) {
			updateProgressBar(static_cast<int>(received), static_cast<int>(total));
		});
	}
	if (networkReply->isFinished())
	{
		if (networkReply->error() == QNetworkReply::NoError) {
			qDebug() << "got allsky";
			QByteArray data = networkReply->readAll();
			allsky = QImage::fromData(data);
		} else {
			noAllsky = true;
		}
		networkReply->deleteLater();
		networkReply = Q_NULLPTR;
		emit statusChanged();
	};
	return !allsky.isNull();
}

bool HipsSurvey::isLoading(void) const
{
	return (networkReply != Q_NULLPTR);
}

void HipsSurvey::draw(StelPainter* sPainter, double angle, HipsSurvey::DrawCallback callback)
{
	// We don't draw anything until we get the properties file and the
	// allsky texture (if available).
	bool outside = (angle == 2.0 * M_PI);
	if (properties.isEmpty()) return;
	if (!getAllsky()) return;
	if (fader.getInterstate() == 0.0f) return;
	sPainter->setColor(1, 1, 1, fader.getInterstate());

	// Set the projection.
	StelCore* core = StelApp::getInstance().getCore();
	StelCore::FrameType frame = StelCore::FrameUninitialized;
	if (hipsFrame == "galactic")
		frame = StelCore::FrameGalactic;
	else if (hipsFrame == "equatorial")
		frame = StelCore::FrameJ2000;
	if (frame)
		sPainter->setProjector(core->getProjection(frame));

	// Compute the maximum visible level for the tiles according to the view resolution.
	// We know that each tile at level L represents an angle of 90 / 2^L
	// The maximum angle we want to see is the size of a tile in pixels time the angle for one visible pixel.
	double px = static_cast<double>(sPainter->getProjector()->getPixelPerRadAtCenter()) * angle;
	int tileWidth = getPropertyInt("hips_tile_width");

	int orderMin = getPropertyInt("hips_order_min", 3);
	int order = getPropertyInt("hips_order");
	int drawOrder = qRound(ceil(log2(px / (4.0 * std::sqrt(2.0) * tileWidth))));
	drawOrder = qBound(orderMin, drawOrder, order);
	int splitOrder = qMax(drawOrder, 4);

	nbVisibleTiles = 0;
	nbLoadedTiles = 0;

	// Draw the 12 root tiles and their children.
	const SphericalCap& viewportRegion = sPainter->getProjector()->getBoundingCap();
	for (int i = 0; i < 12; i++)
	{
		drawTile(0, i, drawOrder, splitOrder, outside, viewportRegion, sPainter, callback);
	}

	updateProgressBar(nbLoadedTiles, nbVisibleTiles);
}

void HipsSurvey::updateProgressBar(int nb, int total)
{
	if (nb == total && progressBar) {
		StelApp::getInstance().removeProgressBar(progressBar);
		progressBar = Q_NULLPTR;
	}
	if (nb == total) return;

	if (!progressBar)
	{
		progressBar = StelApp::getInstance().addProgressBar();
		progressBar->setFormat(getTitle());
		progressBar->setRange(0, 100);
	}
	progressBar->setValue(100 * nb / total);
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
		QString ext = getExt(properties["hips_tile_format"].toString());
		QUrl path = getUrlFor(QString("Norder%1/Dir%2/Npix%3.%4").arg(order).arg((pix / 10000) * 10000).arg(pix).arg(ext));
		tile->texture = texMgr.createTextureThread(path.url(), StelTexture::StelTextureParams(true), false);

		// Use the allsky image until we load the full texture.
		if (order == orderMin && !allsky.isNull())
		{
			int nbw = static_cast<int>(std::sqrt(12 * (1 << (2 * order))));
			int x = (pix % nbw) * allsky.width() / nbw;
			int y = (pix / nbw) * allsky.width() / nbw;
			int s = allsky.width() / nbw;
			QImage image = allsky.copy(x, y, s, s);
			tile->allsky = texMgr.createTexture(image, StelTexture::StelTextureParams(true));
		}
		int tileWidth = getPropertyInt("hips_tile_width", 512);
		tiles.insert(uid, tile, tileWidth * tileWidth);
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


void HipsSurvey::drawTile(int order, int pix, int drawOrder, int splitOrder, bool outside,
						  const SphericalCap& viewportShape, StelPainter* sPainter, DrawCallback callback)
{
	Vec3d pos;
	Mat3d mat3;
	const Vec2d uv[4] = {Vec2d(0, 0), Vec2d(0, 1), Vec2d(1, 0), Vec2d(1, 1)};
	HipsTile *tile;
	int orderMin = getPropertyInt("hips_order_min", 3);
	QVector<Vec3d> vertsArray;
	QVector<Vec2f> texArray;
	QVector<uint16_t> indicesArray;
	int nb;
	Vec4f color = sPainter->getColor();
	float alpha;

	healpix_pix2vec(1 << order, pix, pos.v);

	// Check if the tile is visible.  For outside survey (fullsky), we
	// use bounding cap, otherwise we use proper tile clipping test.
	if (outside)
	{
		SphericalCap boundingCap;
		boundingCap.n = pos;
		boundingCap.d = cos(M_PI / 2.0 / (1 << order));
		if (!viewportShape.intersects(boundingCap)) return;
	}
	else
	{
		double clip_pos[4][4];
		healpix_get_mat3(1 << order, pix, reinterpret_cast<double(*)[3]>(mat3.r));
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
	}

	if (order < orderMin)
		goto skip_render;

	nbVisibleTiles++;
	tile = getTile(order, pix);
	if (!tile) return;
	if (!tile->texture->bind() && (!tile->allsky || !tile->allsky->bind()))
		return;
	if (tile->texFader.state() == QTimeLine::NotRunning && tile->texFader.currentValue() == 0.0)
		tile->texFader.start();
	nbLoadedTiles++;

	if (order < drawOrder)
	{
		// If all the children tiles are loaded, we can skip the parent.
		int i;
		for (i = 0; i < 4; i++)
		{
			HipsTile* child = getTile(order + 1, pix * 4 + i);
			if (!child || child->texFader.currentValue() < 1.0) break;
		}
		if (i == 4) goto skip_render;
	}

	// Actually draw the tile, as a single quad.
	alpha = color[3] * static_cast<float>(tile->texFader.currentValue());
	if (alpha < 1.0f)
	{
		sPainter->setBlending(true);
		sPainter->setColor(color[0], color[1], color[2], alpha);
	}
	else
	{
		sPainter->setBlending(false);
		sPainter->setColor(1, 1, 1, 1);
	}
	sPainter->setCullFace(true);
	nb = fillArrays(order, pix, drawOrder, splitOrder, outside, sPainter,
					vertsArray, texArray, indicesArray);
	if (!callback) {
		sPainter->setArrays(vertsArray.constData(), texArray.constData());
		sPainter->drawFromArray(StelPainter::Triangles, nb, 0, true, indicesArray.constData());
	} else {
		callback(vertsArray, texArray, indicesArray);
	}

skip_render:
	// Draw the children.
	if (order < drawOrder)
	{
		for (int i = 0; i < 4; i++)
		{
			drawTile(order + 1, pix * 4 + i, drawOrder, splitOrder, outside,
					 viewportShape, sPainter, callback);
		}
	}
	// Restore the painter color.
	sPainter->setColor(color);
}

int HipsSurvey::fillArrays(int order, int pix, int drawOrder, int splitOrder,
						   bool outside, StelPainter* sPainter,
						   QVector<Vec3d>& verts, QVector<Vec2f>& tex, QVector<uint16_t>& indices)
{
	Q_UNUSED(sPainter)
	Mat3d mat3;
	Vec3d pos;
	Vec2f texPos;
	uint16_t gridSize = static_cast<uint16_t>(1 << (splitOrder - drawOrder));
	uint16_t n = gridSize + 1;
	const uint16_t INDICES[2][6][2] = {
		{{0, 0}, {0, 1}, {1, 0}, {1, 1}, {1, 0}, {0, 1}},
		{{0, 0}, {1, 0}, {1, 1}, {1, 1}, {0, 1}, {0, 0}},
	};

	healpix_get_mat3(1 << order, pix, reinterpret_cast<double(*)[3]>(mat3.r));

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			texPos = Vec2f(static_cast<float>(i) / gridSize, static_cast<float>(j) / gridSize);
			pos = mat3 * Vec3d(1.0 - static_cast<double>(j) / gridSize, static_cast<double>(i) / gridSize, 1.0);
			healpix_xy2vec(pos.v, pos.v);
			verts << pos;
			tex << texPos;
		}
	}
	for (uint16_t i = 0; i < gridSize; i++)
	{
		for (uint16_t j = 0; j < gridSize; j++)
		{
			for (uint16_t k = 0; k < 6; k++)
			{
				indices << (INDICES[outside ? 1 : 0][k][1] + i) * n +
					        INDICES[outside ? 1 : 0][k][0] + j;
			}
		}
	}
	return gridSize * gridSize * 6;
}

//! Parse a hipslist file into a list of surveys.
QList<HipsSurveyP> HipsSurvey::parseHipslist(const QString& data)
{
	QList<HipsSurveyP> ret;
	QString url;
	double releaseDate = 0;
	for (auto line : data.split('\n'))
	{
		if (line.startsWith('#')) continue;
		QString key = line.section("=", 0, 0).trimmed();
		QString value = line.section("=", 1, -1).trimmed();
		if (key == "hips_service_url") url = value;
		// special case: https://github.com/Stellarium/stellarium/issues/1276
		if (url.contains("data.stellarium.org/surveys/dss")) continue;
		if (key == "hips_release_date")
		{
			// XXX: StelUtils::getJulianDayFromISO8601String does not work
			// without the seconds!
			QDateTime date = QDateTime::fromString(value, Qt::ISODate);
			releaseDate = StelUtils::qDateTimeToJd(date);
		}
		if (key == "hips_status" && value.split(' ').contains("public")) {
			ret.append(HipsSurveyP(new HipsSurvey(url, releaseDate)));
			url = "";
			releaseDate = 0;
		}
	}
	return ret;
}

QString HipsSurvey::getTitle(void) const
{
	// Todo: add a fallback if the properties don't have a title.
	return properties["obs_title"].toString();
}
