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

// Support for HiPS surveys.

#ifndef _STELHIPS_HPP_
#define _STELHIPS_HPP_

#include <QObject>
#include <QCache>
#include <QImage>
#include <functional>

#include "StelTexture.hpp"
#include "VecMath.hpp"

class StelPainter;
class HipsTile;
class QNetworkReply;
class SphericalCap;

class HipsSurvey : public QObject
{
	Q_OBJECT
public:
	typedef std::function<void(const QVector<Vec3d>& verts, const QVector<Vec2f>& tex, const QVector<uint16_t>& indices)> DrawCallback;
	HipsSurvey(const QString& url);
	virtual ~HipsSurvey();
	void draw(StelPainter* sPainter, double angle = 2 * M_PI, DrawCallback callback = NULL);
	const QString& getUrl() const {return url;}

private:
	QString url;
	QCache<long int, HipsTile> tiles;
	// reply to the initial download of the properties file and to the
	// allsky texture.
	QNetworkReply *networkReply = NULL;

	QImage allsky = QImage();
	bool noAllsky = false;

	// Values from the property file.
	QMap<QString, QString> properties;

	bool parseProperties();
	int getPropertyInt(const QString& key, int fallback = 0);
	bool getAllsky();
	HipsTile* getTile(int order, int pix);
	void drawTile(int order, int pix, int drawOrder, int splitOrder, bool outside,
				  const SphericalCap& viewportShape, StelPainter* sPainter, DrawCallback callback);

	// Fill the array for a given tile.
	int fillArrays(int order, int pix, int drawOrder, int splitOrder,
				   bool outside, StelPainter* sPainter,
				   QVector<Vec3d>& verts, QVector<Vec2f>& tex, QVector<uint16_t>& indices);
};

#endif // _STELHIPS_
