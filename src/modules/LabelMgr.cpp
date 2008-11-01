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


#include "LabelMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelApp.hpp"
#include "StarMgr.hpp"
#include "StelCore.hpp"
#include "SFont.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp"
#include "StelObjectType.hpp"
#include "StelUtils.hpp"
#include "vecmath.h"

#include <vector>
#include <QString>
#include <QDebug>

/////////////////////
// StelLabel class //
/////////////////////
StelLabel::StelLabel(const QString& text, SFont* font, const Vec3f& color)
	: labelText(text),
	  labelFont(font),
	  labelColor(color)
{
}

void StelLabel::update(double deltaTime)
{
	labelFader.update((int)(deltaTime*1000));
}

void StelLabel::setFadeDuration(float duration)
{
	labelFader.setDuration(duration);
}

void StelLabel::setFontColor(const Vec3f& color)
{
	labelColor = color;
}

void StelLabel::setFlagShow(bool b)
{
	labelFader = b;
}

bool StelLabel::getFlagShow(void)
{
	return labelFader;
}

////////////////////
// SkyLabel class //
////////////////////
SkyLabel::SkyLabel(const QString& text, StelObjectP bindObject, SFont* font,
                   Vec3f color, QString side, double distance, SkyLabel::Style style,
                   double enclosureSize)
	: StelLabel(text, font, color),
	  labelObject(bindObject),
	  labelSide(side),
	  labelDistance(distance),
	  labelStyle(style),
	  labelEnclosureSize(enclosureSize)
{
}

SkyLabel::~SkyLabel()
{
}

bool SkyLabel::draw(const StelCore* core)
{
	if(labelFader.getInterstate() <= 0.0)
		return false;

	Vec3d objectPos = labelObject->getJ2000EquatorialPos(core->getNavigation());
	Vec3d labelXY;
	core->getProjection()->project(objectPos,labelXY);

	double xOffset(0.);
	double yOffset(0.);
	char hJustify = 'c';
	char vJustify = 'c';

	if (labelSide.toUpper().contains("N"))
	{
		yOffset = 1.0;
		vJustify = 'b'; // bottom justufy text
	}
	else if (labelSide.toUpper().contains("S"))
	{
		yOffset = -1.0;
		vJustify = 't'; // top justufy text
	}

	if (labelSide.toUpper().contains("E"))
	{
		xOffset = 1.0;
		hJustify = 'l'; // right justify text
	}
	else if (labelSide.toUpper().contains("W"))
	{
		xOffset = -1.0;
		hJustify = 'r'; // left justify text
	}

	if (labelDistance >= 0.0)
	{
		xOffset *= labelDistance;
		yOffset *= labelDistance;
	}
	else
	{
		float shift = 4.f + labelObject->getOnScreenSize(core)/1.8f;
		// use the object size
		xOffset *= shift;
		yOffset *= shift;
	}

	double jxOffset(0.);
	double jyOffset(0.); 
	if (hJustify == 'r')
		jxOffset = labelFont->getStrLen(labelText);
	else if (hJustify == 'c')
		jxOffset = labelFont->getStrLen(labelText) / 2.;

	if (vJustify == 't')
		jyOffset = labelFont->getLineHeight();
	else if (vJustify == 'c')
		jyOffset = labelFont->getLineHeight() / 2.;

	glColor4f(labelColor[0], labelColor[1], labelColor[2], labelFader.getInterstate());
	core->getProjection()->drawText(labelFont, labelXY[0]+xOffset-jxOffset, labelXY[1]+yOffset-jyOffset, labelText, 0, 0, 0, false);

	if (labelStyle == SkyLabel::Line)
	{
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// screen coordinates of object
		Vec3d objXY;
		core->getProjection()->project(objectPos,objXY);

		double lineEndX = labelXY[0]+xOffset;
		double lineEndY = labelXY[1]+yOffset;

		if (vJustify == 'b')
			lineEndY -= 5;
		else if (vJustify == 't')
			lineEndY += 5;
				
		if (hJustify == 'l')
			lineEndX -= 5;
		else if (hJustify == 'r')
			lineEndX += 5;
				
		glColor4f(labelColor[0], labelColor[1], labelColor[2], labelFader.getInterstate());
		glBegin(GL_LINES);
			glVertex2f(lineEndX,lineEndY);
			glVertex2f(objXY[0], objXY[1]);
		glEnd();
	}

	return true;
}


///////////////////////
// ScreenLabel class //
///////////////////////
ScreenLabel::ScreenLabel(const QString& text, int x, int y, SFont* font, Vec3f color)
	: StelLabel(text, font, color),
	  screenX(x),
	  screenY(y)
{
}

ScreenLabel::~ScreenLabel()
{
}

bool ScreenLabel::draw(const StelCore* core)
{
	if(labelFader.getInterstate() <= 0.0)
		return false;

	glColor4f(labelColor[0], labelColor[1], labelColor[2], labelFader.getInterstate());
	core->getProjection()->drawText(labelFont, screenX, screenY, labelText, 0, 0, 0, false);
	return true;
}

///////////////////////
// LabelMgr class //
///////////////////////
LabelMgr::LabelMgr()
{
	setObjectName("LabelMgr");
}
 
LabelMgr::~LabelMgr()
{
}

void LabelMgr::init()
{
}

void LabelMgr::draw(StelCore* core)
{
	core->setCurrentFrame(StelCore::FrameJ2000);
	for(std::vector<StelLabel*>::iterator i=allLabels.begin(); i!=allLabels.end(); i++)
	{
		if (*i != NULL)
			(*i)->draw(core);
	}
}
	
int LabelMgr::labelObject(const QString& text,
                             const QString& objectName,
                             bool visible,
                             float fontSize,
                             const QString& fontColor,
                             const QString& side,
                             double labelDistance,
                             const QString& style)
{
	SFont* font = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);
	Q_ASSERT(font);
	StelObjectP obj = StelApp::getInstance().getStelObjectMgr().searchByName(objectName);
	if (!obj)
	{
		qWarning() << "LabelMgr::labelObject object not found: " << objectName;
		return -1;
	}
	
	StelLabel* l = new SkyLabel(text, obj, font, StelUtils::htmlColorToVec3f(fontColor), side, labelDistance, stringToStyle(style));
	if (l==NULL)
		return -1;

	if (visible)
		l->setFlagShow(true);

	allLabels.push_back(l);
	return allLabels.size()-1;
}

int LabelMgr::labelScreen(const QString& text,
                             int x,
                             int y,
                             bool visible,
                             float fontSize,
                             const QString& fontColor)
{
	SFont* font = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);
	Q_ASSERT(font);
	ScreenLabel* l = new ScreenLabel(text, x, y, font, StelUtils::htmlColorToVec3f(fontColor));
	if (l==NULL)
		return -1;

	if (visible)
		l->setFlagShow(true);

	allLabels.push_back(l);
	return allLabels.size()-1;
}

bool LabelMgr::getLabelShow(int id)
{
	if (allLabels.at(id) == NULL)
		return false;
	else
		return allLabels.at(id)->getFlagShow();
}
	
void LabelMgr::setLabelShow(int id, bool show)
{
	if (allLabels.at(id) != NULL)
		allLabels.at(id)->setFlagShow(show);
}
	
bool LabelMgr::deleteLabel(int id)
{
	if (allLabels.at(id) != NULL)
	{
		delete allLabels.at(id);
		allLabels.at(id) = NULL;
		return true;
	}
	else
		return false;
}
	
void LabelMgr::update(double deltaTime)
{
	for(std::vector<StelLabel*>::iterator i=allLabels.begin(); i!=allLabels.end(); i++)
	{
		if (*i != NULL)
			(*i)->update(deltaTime);
	}
}
	
double LabelMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+11;
        return 0;
}

int LabelMgr::deleteAllLabels(void)
{
	int count=0;
	for(std::vector<StelLabel*>::iterator i=allLabels.begin(); i!=allLabels.end(); i++)
	{
		if (*i != NULL)
		{
			delete *i;
			count++;
		}
	}
	allLabels.clear();
	return count;
}

SkyLabel::Style LabelMgr::stringToStyle(const QString& s)
{
	if (s=="Line")
		return SkyLabel::Line;
	else
		return SkyLabel::TextOnly;
}
