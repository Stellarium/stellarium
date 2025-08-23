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
	//! @param group A string that identifies a group of color/normals/horizons
	//!              surveys that this survey belongs to.
	//! @param frame The reference frame from the survey's \c hips_frame property.
	//! @param type Survey type from the survey's \c type property.
	//! @param releaseDate If known the UTC JD release date of the survey.  Used for cache busting.
	HipsSurvey(const QString& url, const QString& group, const QString& frame, const QString& type,
	           const QMap<QString, QString>& hipslistProps, double releaseDate=0.0);
	~HipsSurvey() override;

	static QString frameToPlanetName(const QString& frame);

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

	//! Return the frame name of the survey (its \c hips_frame property).
	QString getFrame() const { return hipsFrame; }

	//! Return the type of the survey (its \c type property).
	QString getType() const { return type; }

	//! Return the group of of color/normals/horizons surveys that this survey belongs to.
	QString getGroup() const { return group; }

	//! Get whether the survey is still loading.
	bool isLoading(void) const;

	bool isPlanetarySurvey(void) const { return planetarySurvey; }

	void setNormalsSurvey(const HipsSurveyP& normals);
	void setHorizonsSurvey(const HipsSurveyP& horizons);

	//! Parse a hipslist file into a list of surveys.
	static QList<HipsSurveyP> parseHipslist(const QString& hipslistURL, const QString& data);

signals:
	void propertiesChanged(void);
	void statusChanged(void);
	void visibleChanged(bool);

private:
	void checkForPlanetarySurvey();

private:
	LinearFader fader;
	QString url;
	QString group;
	QString type;
	QString hipsFrame;
	QString planet;
	HipsSurveyP normals;
	HipsSurveyP horizons;
	double releaseDate; // As UTC Julian day.
	int order = -1;
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
	/*! @brief Bind textures for drawing

	    If @p tile is not ready for drawing (e.g. not fully loaded), alter
	    @p texCoordShift and @p texCoordScale so that they let us address
	    the corresponding part of a parent texture that will be bound.

	    @param tile The tile to draw
	    @param orderMin Smallest available HiPS order of the current survey
	    @param texCoordShift The UV coordinates shift to be applied to the
	     texture coordinates to address a subtexture in the parent. Must be
	     set to 0 before the initial call to this function.
	    @param texCoordScale The UV coordinates scale to be applied to the
	     texture coordinates to address a subtexture in the parent. Must be
	     set to 1 before the initial call to this function.
	    @param tileIsLoaded Gets set to \c false if it was the requested
	     HiPS level isn't fully loaded. Must be set to \c true before the
	     initial call to this function.

	    @return Whether a tile has been successfully bound.
	 */
	bool bindTextures(HipsTile& tile, int orderMin, Vec2f& texCoordShift, float& texCoordScale, bool& tileIsLoaded);
	// draw a single tile. observerVelocity (in the correct hipsFrame) is necessary for aberration correction. Set to 0 for no aberration correction.
	void drawTile(int order, int pix, int drawOrder, int splitOrder, bool outside,
	              const SphericalCap& viewportShape, StelPainter* sPainter, Vec3d observerVelocity, DrawCallback callback);

	// Fill the array for a given tile.
	int fillArrays(int order, int pix, int drawOrder, int splitOrder,
	               bool outside, StelPainter* sPainter, Vec3d observerVelocity,
	               const Vec2f& texCoordShift, const float texCoordScale,
	               QVector<Vec3d>& verts, QVector<Vec2f>& tex, QVector<uint16_t>& indices);

	void updateProgressBar(int nb, int total);
};

#endif // STELHIPS_
