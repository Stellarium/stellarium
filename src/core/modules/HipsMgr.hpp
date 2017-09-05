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

#ifndef _HIPSMGR_HPP_
#define _HIPSMGR_HPP_

#include "StelModule.hpp"

class HipsMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool surveyDisplayed
			READ getFlagSurveyShow
			WRITE setFlagSurveyShow
			NOTIFY surveyDisplayedChanged)

	Q_PROPERTY(QString surveyUrl
			READ getSurveyUrl
			WRITE setSurveyUrl
			NOTIFY surveyUrlChanged)

public:
	HipsMgr();
	virtual ~HipsMgr();
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	QString getSurveyUrl() const;
	void setSurveyUrl(const QString& url);

signals:
	void surveyUrlChanged(const QString& url);

public slots:
	void setFlagSurveyShow(bool displayed);
	bool getFlagSurveyShow(void) const;

signals:
	void surveyDisplayedChanged(const bool displayed) const;

private:
	class HipsSurvey* survey = NULL;
	class LinearFader* fader = NULL;
};

#endif // _HIPSMGR_HPP_
