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

#ifndef STELHIPS_HPP
#define STELHIPS_HPP

#include <QObject>
#include <QCache>
#include <QImage>
#include <QJsonObject>
#include <QUrl>
#include <functional>

#include "StelTexture.hpp"
#include "VecMath.hpp"
#include "StelFader.hpp"

class StelPainter;
class HipsTile;
class QNetworkReply;
class SphericalCap;
class HipsSurvey;
class StelProgressController;

typedef QSharedPointer<HipsSurvey> HipsSurveyP;
Q_DECLARE_METATYPE(HipsSurveyP)

class HipsSurvey : public QObject
{
	friend class HipsMgr;
	Q_OBJECT

	Q_PROPERTY(QString url MEMBER url CONSTANT)
	Q_PROPERTY(QJsonObject properties MEMBER properties NOTIFY propertiesChanged)
	Q_PROPERTY(bool isLoading READ isLoading NOTIFY statusChanged)
	Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
	//! The name of the planet the survey is attached to, or empty if this is a skysurvey.
	Q_PROPERTY(QString planet MEMBER planet)

public:
	typedef std::function<void(const QVector<Vec3d>& verts, const QVector<Vec2f>& tex,
							   const QVector<uint16_t>& indices)> DrawCallback;
	//! Create a new HipsSurvey from its url.
	//! @param url The location of the survey.
	//! @param releaseDate If known the UTC JD release date of the survey.  Used for cache busting.
	HipsSurvey(const QString& url, double releaseDate=0.0);
	virtual ~HipsSurvey();

	//! Get whether the survey is visible.
	bool isVisible() const;

	//! Define whether the survey should be visible.
	void setVisible(bool value);
	float getInterstate() const {return fader.getInterstate();}

	//! Render the survey.
	//! @param sPainter the painter to use.
	//! @param angle total visible angle of the survey in radians. This is used to optimize the rendering of planet
	//!         surveys.  Should be set to 2 pi for sky surveys.
	//! @param callback if set this will be called for each visible tile, and the callback should do it rendering
	//!         itself.  If set to Q_NULLPTR, the function will draw the tiles using the default shader.
	void draw(StelPainter* sPainter, double angle = 2.0 * M_PI, DrawCallback callback = Q_NULLPTR);

	//! Return the source URL of the survey.
	const QString& getUrl() const {return url;}

	//! Get whether the survey is still loading.
	bool isLoading(void) const;

	bool isPlanetarySurvey(void) const { return planetarySurvey; }

	//! Parse a hipslist file into a list of surveys.
	static QList<HipsSurveyP> parseHipslist(const QString& data);

signals:
	void propertiesChanged(void);
	void statusChanged(void);
	void visibleChanged(bool);

private:
	LinearFader fader;
	QString url;
	QString hipsFrame = "equatorial";
	QString planet;
	double releaseDate; // As UTC Julian day.
	bool planetarySurvey;
	QCache<long int, HipsTile> tiles;
	// reply to the initial download of the properties file and to the
	// allsky texture.
	QNetworkReply *networkReply = Q_NULLPTR;

	QImage allsky = QImage();
	bool noAllsky = false;

	// Values from the property file.
	QJsonObject properties;

	// Used to show the loading progress.
	StelProgressController* progressBar = Q_NULLPTR;
	int nbVisibleTiles;
	int nbLoadedTiles;


	QString getTitle(void) const;
	QUrl getUrlFor(const QString& path) const;
	int getPropertyInt(const QString& key, int fallback = 0);
	bool getAllsky();
	HipsTile* getTile(int order, int pix);
	void drawTile(int order, int pix, int drawOrder, int splitOrder, bool outside,
				  const SphericalCap& viewportShape, StelPainter* sPainter, DrawCallback callback);

	// Fill the array for a given tile.
	int fillArrays(int order, int pix, int drawOrder, int splitOrder,
				   bool outside, StelPainter* sPainter,
				   QVector<Vec3d>& verts, QVector<Vec2f>& tex, QVector<uint16_t>& indices);

	void updateProgressBar(int nb, int total);
};

#endif // STELHIPS_
