/*
 * Stellarium
 * This file Copyright (C) 2008 Matthew Gates
 * Parts copyright (C) 2016 Georg Zotti (added size transitions)
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

#ifndef SCREENIMAGEMGR_HPP
#define SCREENIMAGEMGR_HPP


#include "StelModule.hpp"
#include "StelTextureTypes.hpp"
#include "VecMath.hpp"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QSize>

class StelCore;
class QGraphicsPixmapItem;
class QTimeLine;
class QGraphicsItemAnimation;

// base class for different image types
class ScreenImage : public QObject
{
	Q_OBJECT

public:
	//! Load an image
	//! @param filename the partial path of the file to load.  This will be searched for in the
	//! scripts directory using StelFileMgr.
	//! @param x the screen x-position for the texture (in pixels), measured from the left side of the screen.
	//! @param y the screen x-position for the texture (in pixels), measured from the top of the screen.
	//! @param show the initial display status of the image (false == hidden).
	//! @param scale scale factor for the image. 1 = original size, 0.5 = 50% size etc.
	//!        Note that this also controls the final resolution of the image! Scale smaller that 1 leads to reduced resolution,
	//!        larger than 1 of course creates upsampling artifacts. The scaling that happens after loading is a simple stretch of this loaded pixmap.
	//!        In order to get a small image on screen which you might want to grow later, load with this scale=1 and setScale() later.
	//! @param fadeDuration the time it takes for screen images to fade in/out/change alpha in seconds.
	ScreenImage(const QString& filename, qreal x, qreal y, bool show=false, qreal scale=1., qreal alpha=1., float fadeDuration=1.);
	virtual ~ScreenImage();

	//! Draw the image.
	//! @param core the StelCore object
	virtual bool draw(const StelCore* core);
	//! Empty dummy. The various animations are updated by other means.
	virtual void update(double deltaTime);
	//! Set the duration used for the fade in / fade out of the image
	virtual void setFadeDuration(float duration);
	//! Show or hide the image (it will fade in/out)
	//! @param b if true, the image will be shown, else it will be hidden
	virtual void setFlagShow(bool b);
	//! Get the display status of the image
	virtual bool getFlagShow(void) const;

	//! Set the image alpha for when it is in full "on" (after fade in).
	//! @param a the new alpha (transparency) for the image.  1.0 = totally transparent, 0.0 = fully opaque.
	//! @param duration the time for the change in alpha to take effect.
	virtual void setAlpha(qreal a);

	//! Set the x, y position of the image.
	//! @param x new x position
	//! @param y new y position
	//! @param duration how long for the movement to take in seconds
	virtual void setXY(qreal x, qreal y, float duration=0.);

	//! Set the x, y position of the image relative to the current position
	//! @param x the offset in the x-axis
	//! @param y the offset in the y-axis
	//! @param duration how long for the movement to take in seconds
	virtual void addXY(qreal x, qreal y, float duration=0.);
	virtual int imageHeight(void) const;
	virtual int imageWidth(void) const;

	//! Set the image scale relative to the size originally loaded.
	//! @param scale new (target) horizontal and vertical scale factor. Native size=1.
	virtual void setScale(qreal scale);

	//! Set the image scale relative to the size originally loaded.
	//! @param scaleX new (target) horizontal scale factor. Native size=1.
	//! @param scaleY new (target) vertical scale factor. Native size=1.
	//! @param duration how long for the resize to take in seconds
	virtual void setScale(qreal scaleX, qreal scaleY, float duration=0.);
	virtual qreal imageScaleX(void) const;
	virtual qreal imageScaleY(void) const;

protected:
	QGraphicsPixmapItem* tex;
	QTimeLine* moveTimer; // for position changes
	QTimeLine* fadeTimer; // for transparency
	QTimeLine* scaleTimer; // for grow/shrink
	QGraphicsItemAnimation* anim;
	QGraphicsItemAnimation* scaleAnim;

private slots:
	void setOpacity(qreal alpha);

private:
	qreal maxAlpha;
};

//! @class ScreenImageMgr
//! Module for managing images for scripting.  Images are identified by a string ID which is
//! passed to ScreenImageMgr members when it is necessary to specify an image to work with.
//! Member functions in this class which modify the state of the class are all mediated
//! through the signal/slot mechanism to ensure such operations happen in the main thread
//! where images are drawn, and not in the script thread.
class ScreenImageMgr : public StelModule
{
	Q_OBJECT

public:
	//! Construct a ScreenImageMgr object.
	ScreenImageMgr(); 
	virtual ~ScreenImageMgr();
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void draw(StelCore* core);
	//! Update time-dependent parts of the module.
	virtual void update(double deltaTime);
	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	//! Create an image from a file and display on the screen at x,y coordinates.
	//! @param id the ID to use when referring to this image (an arbitrary string).
	//!        If an image with this id exists already, it will be deleted first.
	//! @param filename the partial path of the file to load.  This will be searched
	//!        for using StelFileMgr, with "scripts/" prefixed to the filename.
	//! @param x The x-coordinate for the image (0 = left of screen)
	//! @param y The y-coordinate for the image (0 = top of screen)
	//! @param scale scale factor for the image. 1 = original size, 0.5 = 50% size etc.
	//! @param visible The initial visible state of the image
	//! @param alpha The initial alpha (opacity) value for the image (range 0.0=transparent to 1.0=opaque)
	//! @param fadeDuration the time it takes for screen images to fade in/out/change alpha in seconds.
	void createScreenImage(const QString& id,
				const QString& filename,
				qreal x,
				qreal y,
				qreal scale=1.,
				bool visible=true,
				qreal alpha=1.,
				float fadeDuration=1.);

	//! Find out if an image is currently visible.
	//! @param id the ID for the desired image.
	//! @return true if visible, false if not visible or not loaded.
	bool getShowImage(const QString& id) const;
	//! Set an image's visible status.
	//! @param id the ID for the desired image.
	//! @param show the new visible state to set.
	void showImage(const QString& id, bool show);
	//! @param id the ID for the desired image.
	//! @return the currently scaled width, in pixels.
	int getImageWidth(const QString& id) const;
	//! @param id the ID for the desired image.
	//! @return the currently scaled height in pixels.
	int getImageHeight(const QString& id) const;

	//! Set the x and y scale for the specified image, relative to size given at load time.
	//! @param id the ID for the desired image.
	//! @param scaleX The new x-scale for the image.
	//! @param scaleY The new y-scale for the image.
	//! @param duration The time for the change to take place, in seconds.
	void setImageScale(const QString& id, qreal scaleX, qreal scaleY, float duration=0.);
	//! @param id the ID for the desired image.
	//! @return current X scaling factor, relative to loaded size.
	qreal getImageScaleX(const QString& id) const;
	//! @param id the ID for the desired image.
	//! @return current Y scaling factor, relative to loaded size.
	qreal getImageScaleY(const QString& id) const;
	//! Set an image's alpha value when visible
	//! @param id the ID for the desired image.
	//! @param alpha the new alpha value to set.
	void setImageAlpha(const QString& id, qreal alpha);
	//! Set the x and y coordinates for the specified image
	//! @param id the ID for the desired image.
	//! @param x The new x-coordinate for the image, pixels.
	//! @param y The new y-coordinate for the image, pixels.
	//! @param duration The time for the change to take place, in seconds.
	void setImageXY(const QString& id, qreal x, qreal y, float duration=0.);
	//! Add x and y coordinate offsets to the specified image
	//! @param id the ID for the desired image.
	//! @param x The x-coordinate shift for the image, pixels.
	//! @param y The y-coordinate shift for the image, pixels.
	//! @param duration The time for the change to take place, in seconds.
	void addImageXY(const QString& id, qreal x, qreal y, float duration=0.);
	//! Delete an image.
	//! @param id the ID for the desired image.
	void deleteImage(const QString& id);
	//! Delete all images currently managed by ScreenImageMgr.
	void deleteAllImages(void);
	//! Get a list of currently loaded image IDs.
	QStringList getAllImageIDs(void) const;

private:
	QMap<QString, ScreenImage*> allScreenImages;
};

#endif // _SCREENIMAGEMGR_HPP
