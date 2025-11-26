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
#include "Planet.hpp"
#include "StelPainter.hpp"
#include "StelTextureMgr.hpp"
#include "StelUtils.hpp"
#include "StelProgressController.hpp"
#include "StelHealpix.hpp"

#include <QNetworkReply>
#include <QTimeLine>

class HipsTile
{
public:
	int order;
	int pix;
	StelTextureSP texture;
	StelTextureSP normalTexture;
	StelTextureSP horizonTexture;
	StelTextureSP allsky; // allsky low res version of the texture.
	StelTextureSP normalAllsky; // allsky low res version of the texture.
	StelTextureSP horizonAllsky; // allsky low res version of the texture.
};

static QString getExt(const QString& format)
{
	for (auto &ext : format.split(' '))
	{
		if (ext == "jpeg") return "jpg";
		if (ext == "png") return "png";
		if (ext == "webp") return "webp";
		if (ext == "tiff") return "tiff";
		if (ext == "bmp") return "bmp";
	}
	return QString();
}

static int shiftPix180deg(const int order, const int origPix)
{
	const int scale = 1 << (2 * order);
	const int baseSide = origPix / scale; // 0..11
	Q_ASSERT(baseSide < 12);
	const int newBaseSide = baseSide / 4 * 4 + (baseSide + 2) % 4;
	const int newPix = origPix + (newBaseSide - baseSide) * scale;
	Q_ASSERT(newPix >= 0);
	return newPix;
}

QUrl HipsSurvey::getUrlFor(const QString& path) const
{
	QUrl base = url;
	QString args = "";
	if (base.scheme().isEmpty()) base.setScheme("file");
	if (base.scheme() != "file")
		args += QString("?v=%1").arg(static_cast<int>(releaseDate));
	return QString("%1/%2%3").arg(base.url(), path, args);
}

HipsSurvey::HipsSurvey(const QString& url_, const QString& group, const QString& frame, const QString& type,
                       const QMap<QString, QString>& hipslistProps, const double releaseDate_) :
	url(url_),
	group(group),
	type(type),
	hipsFrame(frame),
	releaseDate(releaseDate_),
	planetarySurvey(false),
	tiles(1000 * 512 * 512), // Cache max cost in pixels (enough for 1000 512x512 tiles).
	nbVisibleTiles(0),
	nbLoadedTiles(0)
{
	// First save properties from hipslist
	for (auto it = hipslistProps.begin(); it != hipslistProps.end(); ++it)
		properties[it.key()] = it.value();
	checkForPlanetarySurvey();

	// Immediately download the properties and replace the hipslist data with them.
	QNetworkRequest req = QNetworkRequest(getUrlFor("properties"));
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
	req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
	QNetworkReply* networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, &QNetworkReply::finished, this, [&, networkReply] {
		QByteArray data = networkReply->readAll();
		for (const QByteArray &line : data.split('\n'))
		{
			if (line.startsWith("#")) continue;
			QStringList list=QString(line).split("=");
			if (list.length()!=2) continue;
			QString key = list.at(0).trimmed();
			if (key.isEmpty()) continue;
			QString value = list.at(1).trimmed();
			properties[key] = value;
		}
		if (properties.contains("hips_release_date"))
		{
			// XXX: StelUtils::getJulianDayFromISO8601String does not work
			// without the seconds!
			QDateTime date = QDateTime::fromString(properties["hips_release_date"].toString(), Qt::ISODate);
			date.setTimeSpec(Qt::UTC);
			releaseDate = StelUtils::qDateTimeToJd(date);
		}
		if (properties.contains("hips_frame"))
			hipsFrame = properties["hips_frame"].toString().toLower();

		checkForPlanetarySurvey();
		emit propertiesChanged();
		emit statusChanged();
		networkReply->deleteLater();
	});
}

HipsSurvey::~HipsSurvey()
{
}

QString HipsSurvey::frameToPlanetName(const QString& frame)
{
	if (frame == "ceres")
		return "(1) ceres";
	return frame;
}

void HipsSurvey::checkForPlanetarySurvey()
{
	planetarySurvey = !QStringList{"equatorial","galactic","ecliptic"}.contains(hipsFrame, Qt::CaseInsensitive) ||
	                  std::as_const(properties)["creator_did"].toString().contains("moon", Qt::CaseInsensitive) ||
	                  std::as_const(properties)["client_category"].toString().contains("solar system", Qt::CaseInsensitive);

	// Assume that all the planetary HiPS describe color maps by default
	if (planetarySurvey && type.isEmpty())
		type = "planet";
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
		connect(networkReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
			updateProgressBar(static_cast<int>(received), static_cast<int>(total));
		});
	}
	if (networkReply->isFinished())
	{
		updateProgressBar(100, 100);
		if (networkReply->error() == QNetworkReply::NoError) {
			QByteArray data = networkReply->readAll();
			qDebug().nospace() << "Got allsky for " << getTitle() << ", " << data.size() << " bytes";
			allsky = QImage::fromData(data);
			if (allsky.isNull())
			{
				qWarning() << "Failed to decode allsky image data";
				noAllsky = true;
			}
		} else {
			noAllsky = true;
		}
		networkReply->deleteLater();
		networkReply = Q_NULLPTR;
		emit statusChanged();
	}
	return !allsky.isNull();
}

bool HipsSurvey::isLoading(void) const
{
	return (networkReply != Q_NULLPTR);
}

void HipsSurvey::setNormalsSurvey(const HipsSurveyP& normals)
{
	if (type != "planet")
	{
		qWarning() << "Attempted to add normals survey to a non-planet survey";
		return;
	}
	this->normals = normals;
	// Resetting normals should result in clearing normal maps from all
	// the tiles. The easiest way to do this is to remove the tiles.
	if (!normals) tiles.clear();
}

void HipsSurvey::setHorizonsSurvey(const HipsSurveyP& horizons)
{
	if (type != "planet")
	{
		qWarning() << "Attempted to add horizons survey to a non-planet survey";
		return;
	}
	this->horizons = horizons;
	// Resetting horizons should result in clearing horizon maps from all
	// the tiles. The easiest way to do this is to remove the tiles.
	if (!horizons) tiles.clear();
}

void HipsSurvey::draw(StelPainter* sPainter, double angle, HipsSurvey::DrawCallback callback)
{
	// We don't draw anything until we get the properties file and the
	// allsky texture (if available).
	const bool outside = qFuzzyCompare(angle, 2.0 * M_PI);
	if (properties.isEmpty()) return;
	bool gotAllsky = getAllsky();
	if (normals) gotAllsky &= normals->getAllsky();
	if (horizons) gotAllsky &= horizons->getAllsky();
	if (!gotAllsky) return;
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

	Vec3d obsVelocity(0.);
	// Aberration: retrieve observer velocity to apply, and transform it to frametype-dependent orientation
	if (core->getUseAberration())
	{
		static const Mat4d matVsop87ToGalactic=StelCore::matJ2000ToGalactic*StelCore::matVsop87ToJ2000;
		obsVelocity=core->getCurrentPlanet()->getHeliocentricEclipticVelocity(); // in VSOP87 frame...
		switch (frame){
			case StelCore::FrameJ2000:
				StelCore::matVsop87ToJ2000.transfo(obsVelocity);
				break;
			case StelCore::FrameGalactic:
				matVsop87ToGalactic.transfo(obsVelocity);
				break;
			case StelCore::FrameHeliocentricEclipticJ2000:
				// do nothing. Assume this frame is equal to VSOP87 (which is slightly incorrect!)
				break;
			default:
				if (!planetarySurvey)
					qDebug() << "HiPS: Unexpected Frame: " << hipsFrame;
		}
		obsVelocity *= core->getAberrationFactor() * (AU/(86400.0*SPEED_OF_LIGHT));
	}

	// Compute the maximum visible level for the tiles according to the view resolution.
	// We know that each tile at level L represents an angle of 90 / 2^L
	// The maximum angle we want to see is the size of a tile in pixels time the angle for one visible pixel.
	double px = static_cast<double>(sPainter->getProjector()->getPixelPerRadAtCenter()) * angle;
	int tileWidth = getPropertyInt("hips_tile_width");

	int orderMin = getPropertyInt("hips_order_min", 3);
	if (order < 0)
	{
		order = getPropertyInt("hips_order");
		int normalsOrder = normals ? normals->getPropertyInt("hips_order") : order;
		int horizonsOrder = horizons ? horizons->getPropertyInt("hips_order") : order;
		if (normalsOrder < order)
		{
			qWarning().nospace() << "Normal map survey's hips_order=" << normalsOrder << " is less than that of the color survey: "
			                     << order << ". Will reduce the total order accordingly.";
			order = normalsOrder;
		}
		if (horizonsOrder < order)
		{
			qWarning().nospace() << "Horizon map survey's hips_order=" << horizonsOrder << " is less than that of the color survey: "
			                     << order << ". Will reduce the total order accordingly.";
			order = horizonsOrder;
		}
	}
	int drawOrder;
	if (outside)
	{
		drawOrder = ceil(log2(px / (4.0 * std::sqrt(2.0) * tileWidth)));
	}
	else
	{
		// The divisor approximately accounts for the fraction of the planetary disk
		// most often taken by the most stretched tiles (the ones near the poles).
		drawOrder = ceil(log2(px / 1.5 / tileWidth));
	}
	drawOrder = qBound(orderMin, drawOrder, order);
	int splitOrder = qMax(drawOrder, 4);
	if (tileWidth < 512)
	{
		int w = 512;
		while (tileWidth < w && splitOrder > 0)
		{
			w /= 2;
			--splitOrder;
		}
	}

	nbVisibleTiles = 0;
	nbLoadedTiles = 0;

	// Draw the 12 root tiles and their children.
	const SphericalCap& viewportRegion = sPainter->getProjector()->getBoundingCap();
	for (int i = 0; i < 12; i++)
	{
		drawTile(0, i, drawOrder, splitOrder, outside, viewportRegion, sPainter, obsVelocity, callback);
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
		const bool isShifted = planetarySurvey && properties["type"].toString().isEmpty();
		const int texturePix = isShifted ? shiftPix180deg(order, pix) : pix;
		QUrl path = getUrlFor(QString("Norder%1/Dir%2/Npix%3.%4").arg(order).arg((texturePix / 10000) * 10000).arg(texturePix).arg(ext));
		const StelTexture::StelTextureParams texParams(true, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
		tile->texture = texMgr.createTextureThread(path.url(), texParams, false);

		// Use the allsky image until we load the full texture.
		if (order == orderMin && !allsky.isNull())
		{
			int nbw = static_cast<int>(std::sqrt(12 * (1 << (2 * order))));
			int x = (pix % nbw) * allsky.width() / nbw;
			int y = (pix / nbw) * allsky.width() / nbw;
			int s = allsky.width() / nbw;
			QImage image = allsky.copy(x, y, s, s);
			tile->allsky = texMgr.createTexture(image, texParams);
		}
		int tileWidth = getPropertyInt("hips_tile_width", 512);
		tiles.insert(uid, tile, static_cast<long>(tileWidth) * tileWidth);
	}

	if (tile && normals && !tile->normalTexture)
	{
		const auto normalsTile = normals->getTile(order, pix);
		if (normalsTile && (normalsTile->texture || normalsTile->allsky))
		{
			tile->normalTexture = normalsTile->texture;
			tile->normalAllsky = normalsTile->allsky;
		}
	}

	if (tile && horizons && !tile->horizonTexture)
	{
		const auto horizonsTile = horizons->getTile(order, pix);
		if (horizonsTile && (horizonsTile->texture || horizonsTile->allsky))
		{
			tile->horizonTexture = horizonsTile->texture;
			tile->horizonAllsky = horizonsTile->allsky;
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

bool HipsSurvey::bindTextures(HipsTile& tile, const int orderMin, Vec2f& texCoordShift, float& texCoordScale, bool& tileIsLoaded)
{
	constexpr int colorTexUnit = 0;
	constexpr int normalTexUnit = 2;
	constexpr int horizonTexUnit = 4;

	if (normals && !tile.normalTexture && !tile.normalAllsky) return false;
	if (horizons && !tile.horizonTexture && !tile.horizonAllsky) return false;

	bool ok = tile.texture->bind(colorTexUnit);
	if (!ok) ok = tile.allsky && tile.allsky->bind(colorTexUnit);
	if (ok && normals)
	{
		ok = tile.normalTexture && tile.normalTexture->bind(normalTexUnit);
		if (!ok) ok = tile.normalAllsky && tile.normalAllsky->bind(normalTexUnit);
	}
	if (ok && horizons)
	{
		ok = tile.horizonTexture && tile.horizonTexture->bind(horizonTexUnit);
		if (!ok) ok = tile.horizonAllsky && tile.horizonAllsky->bind(horizonTexUnit);
	}

	if (!ok) tileIsLoaded = false;

	if (!ok && tile.order > orderMin)
	{
		// Current-level textures failed to bind, let's try the previous level
		const auto parentTile = getTile(tile.order - 1, tile.pix / 4);
		if (!parentTile) return false;
		assert(parentTile->order == tile.order - 1);
		assert(parentTile->order >= orderMin);

		static const Vec2f bottomLeftPartUV[] = {Vec2f(0,0.5), Vec2f(0,0), Vec2f(0.5,0.5), Vec2f(0.5,0)};
		const int pos = tile.pix % 4;
		// Like the multiplication of the texture transform by scale and shift matrix on the left:
		// transform = shift(bottomLeftPartUV[pos]) * scale(0.5) * transform
		texCoordShift = texCoordShift * 0.5f + bottomLeftPartUV[pos];
		texCoordScale *= 0.5f;

		ok = bindTextures(*parentTile, orderMin, texCoordShift, texCoordScale, tileIsLoaded);
		if (!ok) return false;
	}

	return ok;
}

void HipsSurvey::drawTile(int order, int pix, int drawOrder, int splitOrder, bool outside,
                          const SphericalCap& viewportShape, StelPainter* sPainter,
                          Vec3d observerVelocity, DrawCallback callback)
{
	Vec3d pos;
	Mat3d mat3;
	const Vec2d uv[4] = {Vec2d(0, 0), Vec2d(0, 1), Vec2d(1, 0), Vec2d(1, 1)};
	bool tileLoaded = true;
	Vec2f texCoordShift(0,0);
	float texCoordScale = 1;
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
	if (!bindTextures(*tile, orderMin, texCoordShift, texCoordScale, tileLoaded))
		return;

	if (tileLoaded)
		nbLoadedTiles++;

	if (order < drawOrder) goto skip_render;

	// Actually draw the tile, as a single quad.
	alpha = color[3];
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
	nb = fillArrays(order, pix, drawOrder, splitOrder, outside, sPainter, observerVelocity,
	                texCoordShift, texCoordScale, vertsArray, texArray, indicesArray);
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
			         viewportShape, sPainter, observerVelocity, callback);
		}
	}
	// Restore the painter color.
	sPainter->setColor(color);
}

int HipsSurvey::fillArrays(int order, int pix, int drawOrder, int splitOrder,
                           bool outside, StelPainter* sPainter, Vec3d observerVelocity,
                           const Vec2f& texCoordShift, const float texCoordScale,
                           QVector<Vec3d>& verts, QVector<Vec2f>& tex, QVector<uint16_t>& indices)
{
	Q_UNUSED(sPainter)
	Mat3d mat3;
	Vec3d pos;
	Vec2f texPos;
	// First of all, min() limits gridSize to <256, because otherwise squaring it will overflow index type, uint16_t.
	// But in practice 32 points per side seems already good enough, and getting to 128 points noticeably affects
	// performance, so the upper limit is lower.
	uint16_t gridSize = static_cast<uint16_t>(1 << std::min(5, splitOrder + drawOrder - order));
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

			// Aberration: Assume pos=normalized vertex position on the sphere in HiPS frame equatorial/ecliptical/galactic. Velocity is already transformed to frame
			if (!planetarySurvey)
			{
				pos+=observerVelocity;
				pos.normalize();
			}

			verts << pos;
			tex << texPos * texCoordScale + texCoordShift;
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

			// Check that the surface is convex. If it isn't, make it convex.

			const auto p0 = verts[indices[indices.size() - 6 + 0]];
			const auto p1 = verts[indices[indices.size() - 6 + 1]];
			const auto p2 = verts[indices[indices.size() - 6 + 2]];

			// Midpoint of the shared edge of two triangles.
			const auto midPoint = outside ? (p0 + p2)/2. : (p1 + p2)/2.;
			// A vertex that's not on the shared edge.
			const auto outerVert = outside ? p1 : p0;
			// The vector from an outer edge towards the midpoint of the shared edge.
			const auto vecFromMidPointToOuterVert = outerVert - midPoint;
			if(vecFromMidPointToOuterVert.dot(midPoint) > 0)
			{
				// The surface is concave. Swap some vertices to make it convex.
				if(outside)
				{
					indices[indices.size() - 4] = indices[indices.size() - 2];
					indices[indices.size() - 1] = indices[indices.size() - 5];
				}
				else
				{
					indices[indices.size() - 4] = indices[indices.size() - 3];
					indices[indices.size() - 1] = indices[indices.size() - 6];
				}
			}
		}
	}
	return gridSize * gridSize * 6;
}

//! Parse a hipslist file into a list of surveys.
QList<HipsSurveyP> HipsSurvey::parseHipslist(const QString& hipslistURL, const QString& data)
{
	QList<HipsSurveyP> ret;
	static const QString defaultFrame = "equatorial";
	for (const auto& entry : data.split(QRegularExpression("\n\\s*\n")))
	{
		QString url;
		QString type;
		QString frame = defaultFrame;
		QString status;
		QString group = hipslistURL;
		double releaseDate = 0;
		QMap<QString, QString> hipslistProps;
		for (const auto &line : entry.split('\n'))
		{
			if (line.startsWith('#')) continue;
			QString key = line.section("=", 0, 0).trimmed();
			QString value = line.section("=", 1, -1).trimmed();

			hipslistProps[key] = value;

			if (key == "hips_service_url")
			{
				url = value;
				// special case: https://github.com/Stellarium/stellarium/issues/1276
				if (url.contains("data.stellarium.org/surveys/dss")) continue;
			}
			else if (key == "hips_release_date")
			{
				// XXX: StelUtils::getJulianDayFromISO8601String does not work
				// without the seconds!
				QDateTime date = QDateTime::fromString(value, Qt::ISODate);
				date.setTimeSpec(Qt::UTC);
				releaseDate = StelUtils::qDateTimeToJd(date);
			}
			else if (key == "hips_frame")
				frame = value.toLower();
			else if (key == "type")
				type = value.toLower();
			else if (key == "hips_status")
				status = value.toLower();
			else if (key == "group")
				group = value;
		}
		if(status.split(' ').contains("public"))
			ret.append(HipsSurveyP(new HipsSurvey(url, group, frame, type, hipslistProps, releaseDate)));
	}
	return ret;
}

QString HipsSurvey::getTitle(void) const
{
	// Todo: add a fallback if the properties don't have a title.
	return properties["obs_title"].toString();
}
