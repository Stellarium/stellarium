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

#ifndef HIPSMGR_HPP
#define HIPSMGR_HPP

#include "StelModule.hpp"
#include "StelHips.hpp"
#include <QLoggingCategory>
#include <QJsonArray>


//! @class HiPSMgr
//! This class controls display of Hierarchical Progressive Surveys (HiPS), all-sky surveys in a particular format
//! defined by the International Virtual Observatory Alliance. A full description of the format can be found on the
//! IVOA website. (https://www.ivoa.net/documents/HiPS/20170519/REC-HIPS-1.0-20170519.pdf)
//! Stellarium supports only image-based surveys.
//!
//! @note You can finetune the amount of HiPS-related messages in the logfile by configuring the logging category stel.HiPS.
//! For this, e.g. set environment variable QT_LOGGING_RULES="*.debug=false;stel.HiPS.debug=true;".
//! By default, only Info and more severe messages are displayed.

Q_DECLARE_LOGGING_CATEGORY(HiPS)

class HipsMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(QList<HipsSurveyP> surveys
			MEMBER surveys
			NOTIFY surveysChanged)
	Q_PROPERTY(bool flagShow READ getFlagShow WRITE setFlagShow NOTIFY showChanged)
	Q_PROPERTY(State state READ getState NOTIFY stateChanged)
	Q_PROPERTY(bool loaded READ isLoaded NOTIFY stateChanged)

public:
	//! @enum State The loading state of the survey sources.
	enum State
	{
		Created,
		Loading,
		Loaded,
	};
	Q_ENUM(State)

	HipsMgr();
	~HipsMgr() override;
	void init() override;
	void deinit() override;
	void update(double deltaTime) override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	//! Return the hips survey that has a given url.
	Q_INVOKABLE
	HipsSurveyP getSurveyByUrl(const QString &url);

	State getState() const {return state;}
	bool isLoaded() const {return state == Loaded;}

signals:
	void showChanged(bool value) const;
	void surveysChanged() const;
	void stateChanged(State value) const;
	//! Emitted when a new survey has been loaded.
	void gotNewSurvey(HipsSurveyP survey) const;
	void toggleDialog() const;

public slots:
	//! Start to load the default sources.
	void loadSources();

	//! Get whether the surveys are displayed.
	bool getFlagShow(void) const;
	//! Set whether the surveys are displayed.
	void setFlagShow(bool b);

private slots:
	// after loading survey list from network, restore the visible surveys from config.ini.
	void restoreVisibleSurveys();

private:
	QList<HipsSurveyP> surveys;
	bool visible = true;
	State state = Created;
	//! Used internally to keep track of the loading state.
	int nbSourcesLoaded = 0;
};

#endif // HIPSMGR_HPP
