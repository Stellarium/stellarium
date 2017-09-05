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

#include "StelTexture.hpp"

class StelPainter;
class HipsTile;
class QNetworkReply;
class SphericalCap;

class HipsSurvey : public QObject
{
	Q_OBJECT
public:
	HipsSurvey(const QString& url);
	virtual ~HipsSurvey();
	void draw(StelPainter* sPainter);

private:
	QString url;
	QCache<long int, HipsTile> tiles;
	// reply to the initial download of the properties file.
	QNetworkReply *networkReply = NULL;

	StelTextureSP allsky;

	struct {
		int hips_order;
		int hips_order_min;
		int hips_tile_width;
		QString hips_tile_format;
	} properties;
	bool propertiesParsed = false;

	bool parseProperties();
	bool getAllsky();
	HipsTile* getTile(int order, int pix);
	void drawTile(int order, int pix, int drawOrder, const SphericalCap& viewportShape, StelPainter* sPainter);
};

#endif // _STELHIPS_
