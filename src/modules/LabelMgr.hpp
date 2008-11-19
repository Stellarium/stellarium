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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _SKYLABELMGR_HPP_
#define _SKYLABELMGR_HPP_


#include "Fader.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp"
#include "StelObjectType.hpp"
#include "vecmath.h"

#include <QVector>
#include <QString>

class StelCore;
class SFont;
class StelPainter;

// Base class from which other label types inherit
class StelLabel
{
public:
	StelLabel(const QString& text, SFont* font, const Vec3f& color);
	virtual ~StelLabel() {;}

	//! draw the label on the sky
	//! @param core the StelCore object
	virtual bool draw(StelCore* core, const StelPainter& sPainter) = 0;
	//! update fade for on/off action
	virtual void update(double deltaTime);
	//! Set the duration used for the fade in / fade out of the label.
	virtual void setFadeDuration(float duration);
	//! Set the font color used for the font
	virtual void setFontColor(const Vec3f& color);
	//! Show or hide the label.  It will fade in/out.
	virtual void setFlagShow(bool b);
	//! Get value of flag used to turn on and off the label
	virtual bool getFlagShow(void);

protected:
	QString labelText;
	SFont* labelFont;
	Vec3f labelColor;
	LinearFader labelFader;

};

//! @class SkyLabel is used to create user labels which are bound to some object
//! on the celestial sphere.  The object in question can be any existing StelObject
//! or celestial coordinates.
class SkyLabel : public StelLabel
{
public:
	//! @enum Style determined the way the object to which the label is bound
	//! is indicated. 
	enum Style {
		TextOnly,   //!< Just put the label near the object
		Line        //!< Draw a line from the label text to the object
	};

	//! Constructor of a SkyLabel which is attached to an existing object
	//! @param text the text which will be displayed
	//! @param bindObject a pointer to an existing object to which the label will be attached
	//! @param font a pointer the font to use for this label
	//! @param fontColor choose a color for the label
	//! @param side which side of the object to draw the label, values N, S, E, W, NE, NW, SE, SW, C (C is centred on the object)
	//! @param distance the distance from the object to draw the label.  If < 0.0, placement is automatic.
	//! @param style determines how the label is drawn
	//! @param enclosureSize determines the size of the enclosure for styles Box and Circle
	SkyLabel(const QString& text, StelObjectP bindObject, SFont* font, Vec3f color,
	         QString side="NE", double distance=-1.0, SkyLabel::Style style=TextOnly, 
	         double enclosureSize=0.0);

	virtual ~SkyLabel();
	// SkyLabel(const QString& text, Vec3d coords, QString side="NE", double distance=-1.0, SkyLabel::Style style=TextOnly, double enclosureSize=-1.0);

	//! draw the label on the sky
	//! @param core the StelCore object
	virtual bool draw(StelCore* core, const StelPainter& sPainter);

private:
	StelObjectP labelObject;
	QString labelSide;
	double labelDistance;
	SkyLabel::Style labelStyle;
	double labelEnclosureSize;
	
};

//! @class ScreenLabel is used to create user labels which are bound to a fixed point
//! on the screen.
class ScreenLabel : public StelLabel
{
public:
	//! Constructor of a SkyLabel which is to be displayed at a fixed position on the screen.
	//! @param text the text for the label
	//! @param x the x-position on the screen (pixels from the left side)
	//! @param y the y-position on the screen (pixels from the bottom side)
	//! @param font the font to use
	//! @param color the color for the label
	ScreenLabel(const QString& text, int x, int y, SFont* font, Vec3f color);
	virtual ~ScreenLabel();

	//! draw the label on the sky
	//! @param core the StelCore object
	virtual bool draw(StelCore* core, const StelPainter& sPainter);

private:
	int screenX;
	int screenY;

};

//! @class LabelMgr
//! Allows for creation of custom labels on objects or coordinates.
//! Because this class is intended for use in scripting (although
//! other uses are also fine), all label types and so on are specified 
//! by QString descriptions.
//! TODO: when QT4.5 is out, change implementation to use QGraphicsTextItem.
//! (QT4.5 should allow for opacity changes for fades, but it is not currently
//! implemented.
class LabelMgr : public StelModule
{
	Q_OBJECT;

public:
	//! Construct a LabelMgr object.
	LabelMgr(); 
	virtual ~LabelMgr();
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the LabelMgr object.
	virtual void init();
	
	//! Draw user labels.
	virtual void draw(StelCore* core);
	
	//! Update time-dependent parts of the module.
	virtual void update(double deltaTime);

	//! Defines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	//! Create a label which is attached to a StelObject.
	//! @param text the text to display
	//! @param objectName the English name of the object to attach to
	//! @param visible if true, the label starts displayed, else it starts hidden
	//! @param fontSize size of the font to use
	//! @param fontColor HTML-like color spec, e.g. "#ffff00" for yellow
	//! @param side where the label appears in relation to object:
	//! - "N" = above object on screen
	//! - "S" = below object on screen
	//! - "E" = to the right of the object on screen
	//! - "W" = to the left of the object on screen
	//! - "NE", "NW", "SE", "SW" work too.
	//! @return a unique ID which can be used to refer to the label.
	//! returns -1 if the label could not be created (e.g. object not found)
	int labelObject(const QString& text, 
	                const QString& objectName,
	                bool visible=true,
	                float fontSize=14,
	                const QString& fontColor="#999999",
	                const QString& side="E",
	                double labelDistance=-1.0,
	                const QString& style="TextOnly");

	//! Create a label at fixed screen coordinates
	int labelScreen(const QString& text,
	                int x,
	                int y,
	                bool visible=true,
	                float fontSize=14,
	                const QString& fontColor="#999999");

	//! find out if a label identified by id is presently shown
	bool getLabelShow(int id); 
	//! set a label identified by id to be shown or not
	void setLabelShow(int id, bool show); 
	//! Delete a label by the ID which was returned from addLabel...
	//! @return true if the id existed and was deleted, else false
	bool deleteLabel(int id);
	//! Delete all labels.
	//! @return the number of labels deleted
	int deleteAllLabels(void);

private:
	SkyLabel::Style stringToStyle(const QString& s);
	QVector<StelLabel*> allLabels;
};

#endif // _SKYLABELMGR_HPP_
