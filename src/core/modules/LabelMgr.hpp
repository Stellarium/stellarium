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

#ifndef _SKYLABELMGR_HPP_
#define _SKYLABELMGR_HPP_


#include "StelFader.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp"
#include "StelObjectType.hpp"
#include "VecMath.hpp"

#include <QVector>
#include <QString>

class StelCore;

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
	Q_OBJECT

public:
	//! Construct a LabelMgr object.
	LabelMgr(); 
	virtual ~LabelMgr();
 
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the LabelMgr object.
	virtual void init();
	
	//! Draw user labels.
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	
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
	//! @param labelDistance Distance of the label from the object
	//! @param style Label style
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
	//! @param text the text to display
	//! @param x the horizontal position on the screen, in pixels, from the left of the screen
	//! @param y the vertical position on the screen, in pixels, from the top of the screen
	//! @param visible if true, the label starts displayed, else it starts hidden
	//! @param fontSize size of the font to use
	//! @param fontColor HTML-like color spec, e.g. "#ffff00" for yellow
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
	//! set text of label identified by id to be newText
	void setLabelText(int id, const QString& newText);
	//! Delete a label by the ID which was returned from addLabel...
	//! @return true if the id existed and was deleted, else false
	bool deleteLabel(int id);
	//! Delete all labels.
	//! @return the number of labels deleted
	int deleteAllLabels(void);

private:
	QVector<class StelLabel*> allLabels;
};

#endif // _SKYLABELMGR_HPP_
