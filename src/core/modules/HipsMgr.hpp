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
#include "StelHips.hpp"
#include <QJsonArray>

class HipsMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(QList<HipsSurveyP> surveys
			MEMBER surveys
			NOTIFY surveysChanged)

	//! Special property to quicky show the DSS hips survey.
	Q_PROPERTY(bool showDSS READ getShowDSS WRITE setShowDSS NOTIFY showDSSChanged)

public:
	HipsMgr();
	virtual ~HipsMgr();
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	//! Return the hips survey that has a given url.
	Q_INVOKABLE
	HipsSurveyP getSurveyByUrl(const QString &url);

	//! Return whether the DSS survey is visible.
	bool getShowDSS() const;
	//! Set whether the DSS survey is visible.
	void setShowDSS(bool value);

signals:
	void surveyDisplayedChanged(const bool displayed) const;
	void surveysChanged() const;
	//! Emitted when a new survey has been loaded.
	void gotNewSurvey(HipsSurveyP survey) const;
	//! Emitted when the DSS survey visible status has changed.
	void showDSSChanged() const;

private:
	QList<HipsSurveyP> surveys;
};

#endif // _HIPSMGR_HPP_
