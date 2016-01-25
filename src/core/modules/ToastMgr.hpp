/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _TOASTMGR_HPP_
#define _TOASTMGR_HPP_

#include "StelModule.hpp"

class ToastMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool surveyDisplayed
			READ getFlagSurveyShow
			WRITE setFlagSurveyShow
			NOTIFY surveyDisplayedChanged)
public:
	ToastMgr();
	virtual ~ToastMgr();
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName) const {return 2.0;}

public slots:
	void setFlagSurveyShow(const bool displayed);
	bool getFlagSurveyShow(void) const;

signals:
	void surveyDisplayedChanged(const bool displayed) const;

private:
	class ToastSurvey* survey;

	bool flagSurveyDisplayed;
};

#endif // _TOASTMGR_HPP_
