/*
 * Stellarium
 * This file Copyright (C) 2008 Matthew Gates
 * Horizon system labels (c) 2015 Georg Zotti
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

#ifndef LABELMGR_HPP
#define LABELMGR_HPP

#include "StelModule.hpp"
#include "VecMath.hpp"
#include <QMap>
#include <QString>

class StelCore;
class StelPainter;

//! @class LabelMgr
//! Allows for creation of custom labels on objects or coordinates.
//! Because this class is intended for use in scripting (although
//! other uses are also fine), all label types and so on are specified 
//! by QString descriptions.
//! The labels are painted very late, i.e. also sky object labels will be written over the landscape.
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
	//! @param fontColor either HTML-like color spec, e.g. "#ffff00", or 3-part float vector like Vec3f(1.0f,1.0f,0.0f) for yellow
	//! @param side where the label appears in relation to object:
	//! - "N" = above object on screen
	//! - "S" = below object on screen
	//! - "E" = to the right of the object on screen
	//! - "W" = to the left of the object on screen
	//! - "NE", "NW", "SE", "SW" work too.
	//! @return a unique ID which can be used to refer to the label.
	//! returns -1 if the label could not be created (e.g. object not found)
	//! @param autoDelete the label will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the label will be automatically deleted after
	//! autoDeleteTimeoutMs ms
	int labelObject(const QString& text,
			const QString& objectName,
	                bool visible=true,
	                float fontSize=14,
	                const QString& fontColor="#999999",
	                const QString& side="E",
			double labelDistance=-1.0,
			const QString& style="TextOnly",
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0);
	int labelObject(const QString& text,
			const QString& objectName,
			bool visible,
			float fontSize,
			const Vec3f& fontColor,
			const QString& side="E",
			double labelDistance=-1.0,
			const QString& style="TextOnly",
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0);
	//! Create a label in azimuthal coordinate system. Can be used e.g. to show landscape features
	//! @param text the text to display
	//! @param az azimuth, degrees
	//! @param alt altitude, degrees
	//! @param visible if true, the label starts displayed, else it starts hidden
	//! @param fontSize size of the font to use
	//! @param fontColor either HTML-like color spec, e.g. "#ffff00", or 3-part float vector like Vec3f(1.0f,1.0f,0.0f) for yellow
	//! @param autoDelete the label will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the label will be automatically deleted after
	//! autoDeleteTimeoutMs ms
	int labelHorizon(const QString& text,
			float az,
			float alt,
			bool visible=true,
			float fontSize=14,
			const QString& fontColor="#999999",
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0);
	int labelHorizon(const QString& text,
			float az,
			float alt,
			bool visible,
			float fontSize,
			const Vec3f& fontColor,
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0);

	//! Create a label in equatorial coordinate system
	//! @param text the text to display
	//! @param RA right ascension (e.g. 5h10m31s)
	//! @param Dec declination (e.g. 25d30m30s)
	//! @param visible if true, the label starts displayed, else it starts hidden
	//! @param fontSize size of the font to use
	//! @param fontColor either HTML-like color spec, e.g. "#ffff00", or 3-part float vector like Vec3f(1.0f,1.0f,0.0f) for yellow
	//! @param side where the label appears in relation to coordinates:
	//! - "N" = above object on screen
	//! - "S" = below object on screen
	//! - "E" = to the right of the object on screen
	//! - "W" = to the left of the object on screen
	//! - "NE", "NW", "SE", "SW" work too.
	//! @param autoDelete the label will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the label will be automatically deleted after autoDeleteTimeoutMs ms
	//! @param j2000epoch if true, the label starts displayed in equatorial coordinates for epoch J2000.0
	int labelEquatorial(const QString& text,
			const QString& ra,
			const QString& dec,
			bool visible=true,
			float fontSize=14,
			const QString& fontColor="#999999",
			const QString& side="",
			double labelDistance=-1.0,
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0,
			bool j2000epoch = true);
	int labelEquatorial(const QString& text,
			const QString& ra,
			const QString& dec,
			bool visible,
			float fontSize,
			const Vec3f& fontColor,
			const QString& side="",
			double labelDistance=-1.0,
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0,
			bool j2000epoch = true);

	//! Create a label at fixed screen coordinates
	//! @param text the text to display
	//! @param x the horizontal position on the screen, in pixels, from the left of the screen
	//! @param y the vertical position on the screen, in pixels, from the top of the screen
	//! @param visible if true, the label starts displayed, else it starts hidden
	//! @param fontSize size of the font to use
	//! @param fontColor either HTML-like color spec, e.g. "#ffff00", or 3-part float vector like Vec3f(1.0f,1.0f,0.0f) for yellow
	//! @param autoDelete the label will be automatically deleted after it is displayed once
	//! @param autoDeleteTimeoutMs if not zero, the label will be automatically deleted after
	//! autoDeleteTimeoutMs ms
	int labelScreen(const QString& text,
	                int x,
	                int y,
	                bool visible=true,
			float fontSize=14,
			const QString& fontColor="#999999",
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0);
	int labelScreen(const QString& text,
			int x,
			int y,
			bool visible,
			float fontSize,
			const Vec3f& fontColor,
			bool autoDelete = false,
			int autoDeleteTimeoutMs = 0);

	//! find out if a label identified by id is presently shown
	bool getLabelShow(int id) const;
	//! set a label identified by id to be shown or not
	void setLabelShow(int id, bool show); 
	//! set text of label identified by id to be newText
	void setLabelText(int id, const QString& newText);
	//! Delete a label by the ID which was returned from addLabel...
	//! @return true if the id existed and was deleted, else false
	void deleteLabel(int id);
	//! Delete all labels.
	//! @return the number of labels deleted
	int deleteAllLabels(void);

private slots:
	void messageTimeout1();
	void messageTimeout2();

private:
	QMap<int, class StelLabel*> allLabels;
	int counter;
	int appendLabel(class StelLabel* l, int autoDeleteTimeoutMs);
};

#endif // LABELMGR_HPP
