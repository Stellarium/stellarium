/*
 * Stellarium
 * This file Copyright (C) 2008 Matthew Gates
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

#ifndef _SCREENIMAGEMGR_HPP_
#define _SCREENIMAGEMGR_HPP_


#include "StelModule.hpp"
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
	//! @param y the screen x-position for the texture (in pixels), measured from the bottom of the screen.
	//! @param show the initial displayed status of the image (false == hidden).
	//! @param scale scale factor for the image. 1 = original size, 0.5 = 50% size etc.
	//! @param alpha Alpha (opaqueness) of the image.
	//! @param fadeDuration the time it takes for screen images to fade in/out/change alpha in seconds.
	ScreenImage(const QString& filename, float x, float y, bool show=false, float scale=1., float alpha=1., float fadeDuration=1.);
	virtual ~ScreenImage();

	//! Draw the image.
	//! @param core the StelCore object
	virtual bool draw(const StelCore* core);
	//! update fade for on/off action
	virtual void update(double deltaTime);
	//! Set the duration used for the fade in / fade out of the image
	virtual void setFadeDuration(float duration);
	//! Show or hide the image (it will fade in/out)
	//! @param b if true, the image will be shown, else it will be hidden
	virtual void setFlagShow(bool b);
	//! Get the displayed status of the image
	virtual bool getFlagShow(void);
	//! Set the image alpha for when it is in full "on" (after fade in).
	//! @param a the new alpha (transparency) for the image.  1.0 = totally transparent, 0.0 = fully opaque.
	virtual void setAlpha(float a);
	//! Set the x, y position of the image.
	//! @param x new x position
	//! @param y new y position
	//! @param duration how long for the movement to take in seconds
	virtual void setXY(float x, float y, float duration=0.);
	//! Set the x, y position of the image relative to the current position
	//! @param x the offset in the x-axis
	//! @param y the offset in the y-axis
	//! @param duration how long for the movement to take in seconds
	virtual void addXY(float x, float y, float duration=0.);
	virtual int imageHeight(void);
	virtual int imageWidth(void);

protected:
	QGraphicsPixmapItem* tex;
	QTimeLine* moveTimer;
	QTimeLine* fadeTimer;
	QGraphicsItemAnimation* anim;

private slots:
	void setOpacity(qreal alpha);

private:
	float maxAlpha;

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
	//! Construct a LabelMgr object.
	ScreenImageMgr(); 
	virtual ~ScreenImageMgr();
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	//! Update time-dependent parts of the module.
	virtual void update(double deltaTime);
	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	//! Create an image from a file and display on the screen at x,y coordinates.
	//! @param id the ID to use when referring to this image (an arbitrary string).
	//! @param filename the partial path of the file to load.  This will be searched
	//! for using StelFileMgr, with "scripts/" prefixed to the filename.
	//! @param x The x-coordinate for the image (0 = left of screen)
	//! @param y The y-coordinate for the image (0 = bottom of screen)
	//! @param scale Image scale multiplier.
	//! @param visible The initial visible state of the image
	//! @param alpha The initial alpha (transparancy) value for the image (range 0.0 to 1.0)
	//! @param fadeDuration the time it takes for screen images to fade in/out/change alpha in seconds.
	void createScreenImage(const QString& id,
	                       const QString& filename,
	                       float x,
	                       float y,
	                       float scale=1.,
	                       bool visible=true,
	                       float alpha=1.,
	                       float fadeDuration=1.);

	//! Find out if an image is currently visible.
	//! @param id the ID for the desired image.
	bool getShowImage(const QString& id); 
	//! Set an image's visible status.
	//! @param id the ID for the desired image.
	int getImageWidth(const QString& id);
	int getImageHeight(const QString& id);
	void showImage(const QString& id, bool show); 
	//! Set an image's alpha value when visible
	//! @param id the ID for the desired image.
	//! @param alpha the new alpha value to set.
	void setImageAlpha(const QString& id, float alpha); 
	//! Set the x and y coordinates for the specified image
	//! @param id the ID for the desired image.
	//! @param x The new x-coordinate for the image.
	//! @param y The new y-coordinate for the image.
	//! @param duration The time for the change to take place, in seconds.
	void setImageXY(const QString& id, float x, float y, float duration=0.);
	//! Delete an image.
	//! @param id the ID for the desired image.
	void deleteImage(const QString& id);
	//! Delete all images currently managed by ScreenImageMgr.
	void deleteAllImages(void);
	//! Get a list of currently loaded image IDs.
	QStringList getAllImageIDs(void);

private:
	QMap<QString, ScreenImage*> allScreenImages;
};

#endif // _SCREENIMAGEMGR_HPP_
