/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
 * Copyright (C) 2010 Bogdan Marinov (add/remove landscapes feature)
 * Copyright (C) 2011 Alexander Wolf
 * Copyright (C) 2012 Timothy Reaves
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

#include "StelActionMgr.hpp"
#include "LandscapeMgr.hpp"
#include "Landscape.hpp"
#include "AtmospherePreetham.hpp"
#include "AtmosphereShowMySky.hpp"
#include "StelApp.hpp"
#include "SolarSystem.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "Planet.hpp"
#include "StelIniParser.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"
#include "StelPropertyMgr.hpp"
#include "StelUtils.hpp"

#include <private/qzipreader_p.h>
#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QString>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryFile>
#include <QMouseEvent>
#include <QPainter>
#include <QElapsedTimer>
#include <QOpenGLPaintDevice>

#include <stdexcept>

namespace
{
constexpr char ATMOSPHERE_MODEL_CONFIG_KEY[]="landscape/atmosphere_model";
constexpr char ATMOSPHERE_MODEL_PATH_CONFIG_KEY[]="landscape/atmosphere_model_path";
constexpr char ATMOSPHERE_ECLIPSE_SIM_QUALITY_CONFIG_KEY[]="landscape/atmosphere_eclipse_simulation_quality";
constexpr char ATMOSPHERE_MODEL_CONF_VAL_PREETHAM[]="preetham";
constexpr char ATMOSPHERE_MODEL_CONF_VAL_SHOWMYSKY[]="showmysky";
constexpr char ATMOSPHERE_MODEL_CONF_VAL_DEFAULT[]="preetham";
}

Cardinals::Cardinals()
	: color(0.6f,0.2f,0.2f)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	screenFontSize = StelApp::getInstance().getScreenFontSize();
	propMgr = StelApp::getInstance().getStelPropertyManager();
	// Default font size is 24
	font4WCR.setPixelSize(conf->value("viewing/cardinal_font_size", screenFontSize+11).toInt());
	// Default font size is 18
	font8WCR.setPixelSize(conf->value("viewing/ordinal_font_size", screenFontSize+5).toInt());
	// Draw the principal wind points even smaller.
	font16WCR.setPixelSize(conf->value("viewing/16wcr_font_size", screenFontSize+2).toInt());
	font32WCR.setPixelSize(conf->value("viewing/32wcr_font_size", screenFontSize).toInt());

	// English names for cardinals
	labels = {
		{   dN,  "N" }, {   dS,  "S" }, {   dE,  "E" }, {   dW,  "W" },
		{  dNE, "NE" }, {  dSE, "SE" }, {  dSW, "SW" }, {  dNW, "NW" },
		{ dNNE,"NNE" }, { dENE,"ENE" }, { dESE,"ESE" }, { dSSE,"SSE" },
		{ dSSW,"SSW" }, { dWSW,"WSW" }, { dWNW,"WNW" }, { dNNW,"NNW" },
		{ dNbE,"NbE" }, {dNEbN,"NEbN"}, {dNEbE,"NEbE"}, { dEbN,"EbN" },
		{ dEbS,"EbS" }, {dSEbE,"SEbE"}, {dSEbS,"SEbS"}, { dSbE,"SbE" },
		{ dSbW,"SbW" }, {dSWbS,"SWbS"}, {dSWbW,"SWbW"}, { dWbS,"WbS" },
		{ dWbN,"WbN" }, {dNWbW,"NWbW"}, {dNWbN,"NWbN"}, { dNbW,"NbW" }
	};
}

Cardinals::~Cardinals()
{
}

const float Cardinals::sp8 = sin(M_PIf/8.f); // dimension for intercardinals
const float Cardinals::cp8 = cos(M_PIf/8.f); // dimension for intercardinals
const float Cardinals::s1p16 = sin(M_PIf/16.f);     // dimension for rose32
const float Cardinals::c1p16 = cos(M_PIf/16.f);     // dimension for rose32
const float Cardinals::s3p16 = sin(3.f*M_PIf/16.f); // dimension for rose32
const float Cardinals::c3p16 = cos(3.f*M_PIf/16.f); // dimension for rose32

const QMap<Cardinals::CompassDirection, Vec3f> Cardinals::rose4winds = {
	{ Cardinals::dN, Vec3f(-1.f, 0.f, 0.f) }, { Cardinals::dS, Vec3f(1.f,  0.f, 0.f) },
	{ Cardinals::dE, Vec3f( 0.f, 1.f, 0.f) }, { Cardinals::dW, Vec3f(0.f, -1.f, 0.f) }
};
const QMap<Cardinals::CompassDirection, Vec3f> Cardinals::rose8winds = {
	{ Cardinals::dNE, Vec3f(-q8,  q8, 0.f) }, { Cardinals::dSE, Vec3f( q8,  q8, 0.f) },
	{ Cardinals::dSW, Vec3f( q8, -q8, 0.f) }, { Cardinals::dNW, Vec3f(-q8, -q8, 0.f) }
};
const QMap<Cardinals::CompassDirection, Vec3f> Cardinals::rose16winds = {
	{ Cardinals::dNNE, Vec3f(-cp8,  sp8, 0.f) }, { Cardinals::dENE, Vec3f(-sp8,  cp8, 0.f) },
	{ Cardinals::dESE, Vec3f( sp8,  cp8, 0.f) }, { Cardinals::dSSE, Vec3f( cp8,  sp8, 0.f) },
	{ Cardinals::dSSW, Vec3f( cp8, -sp8, 0.f) }, { Cardinals::dWSW, Vec3f( sp8, -cp8, 0.f) },
	{ Cardinals::dWNW, Vec3f(-sp8, -cp8, 0.f) }, { Cardinals::dNNW, Vec3f(-cp8, -sp8, 0.f) }
};
const QMap<Cardinals::CompassDirection, Vec3f> Cardinals::rose32winds = {
	{ Cardinals::dNbE,  Vec3f(-c1p16, s1p16, 0.f) }, { Cardinals::dNbW,  Vec3f(-c1p16, -s1p16, 0.f) },
	{ Cardinals::dSbE,  Vec3f( c1p16, s1p16, 0.f) }, { Cardinals::dSbW,  Vec3f( c1p16, -s1p16, 0.f) },
	{ Cardinals::dEbS,  Vec3f( s1p16, c1p16, 0.f) }, { Cardinals::dEbN,  Vec3f(-s1p16,  c1p16, 0.f) },
	{ Cardinals::dWbN,  Vec3f(-s1p16,-c1p16, 0.f) }, { Cardinals::dWbS,  Vec3f( s1p16, -c1p16, 0.f) },
	{ Cardinals::dNEbN, Vec3f(-c3p16, s3p16, 0.f) }, { Cardinals::dNWbN, Vec3f(-c3p16, -s3p16, 0.f) },
	{ Cardinals::dSEbS, Vec3f( c3p16, s3p16, 0.f) }, { Cardinals::dSWbS, Vec3f( c3p16, -s3p16, 0.f) },
	{ Cardinals::dSEbE, Vec3f( s3p16, c3p16, 0.f) }, { Cardinals::dNEbE, Vec3f(-s3p16,  c3p16, 0.f) },
	{ Cardinals::dNWbW, Vec3f(-s3p16,-c3p16, 0.f) }, { Cardinals::dSWbW, Vec3f( s3p16, -c3p16, 0.f) }
};

void Cardinals::update(double deltaTime)
{
	fader4WCR.update(static_cast<int>(deltaTime*1000));
	fader8WCR.update(static_cast<int>(deltaTime*1000));
	fader16WCR.update(static_cast<int>(deltaTime*1000));
	fader32WCR.update(static_cast<int>(deltaTime*1000));
}

void Cardinals::setFadeDuration(float duration)
{
	fader4WCR.setDuration(static_cast<int>(duration*1000.f));
	fader8WCR.setDuration(static_cast<int>(duration*1000.f));
	fader16WCR.setDuration(static_cast<int>(duration*1000.f));
	fader32WCR.setDuration(static_cast<int>(duration*1000.f));
}

// Draw the cardinals points : N S E W and the subcardinal and sub-subcardinal.
// Handles special cases at poles
void Cardinals::draw(const StelCore* core, double latitude) const
{
	// fun polar special cases: no cardinals!
	if ((fabs(latitude - 90.0) < 1e-10) || (fabs(latitude + 90.0) < 1e-10))
		return;

	if (fader4WCR.getInterstate()>0.f)
	{
		const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		const float ppx = static_cast<float>(core->getCurrentStelProjectorParams().devicePixelsPerPixel);
		StelPainter sPainter(prj);
		sPainter.setFont(font4WCR);
		float sshift=0.f, bshift=0.f, cshift=0.f, dshift=0.f, vshift=1.f;
		bool flagMask = (core->getProjection(StelCore::FrameJ2000)->getMaskType() != StelProjector::MaskDisk);
		if (propMgr->getProperty("SpecialMarkersMgr.compassMarksDisplayed")->getValue().toBool())
			vshift = static_cast<float>(screenFontSize + 12)*ppx;

		Vec3f xy;
		sPainter.setColor(color, fader4WCR.getInterstate());
		sPainter.setBlending(true);
		QMapIterator<Cardinals::CompassDirection, Vec3f> it4w(rose4winds);
		while(it4w.hasNext())
		{
			it4w.next();
			QString directionLabel = labels.value(it4w.key(), "");

			if (flagMask)
				sshift = ppx*static_cast<float>(sPainter.getFontMetrics().boundingRect(directionLabel).width())*0.5f;

			if (prj->project(it4w.value(), xy))
			{
				Vec3f up(it4w.value()[0], it4w.value()[1], 1.f*M_PI_180f);
				Vec3f upPrj;
				prj->project(up, upPrj);
				float dx=upPrj[0]-xy[0];
				float dy=upPrj[1]-xy[1];
				float textAngle=atan2(dx, dy);
				sPainter.drawText(xy[0], xy[1], directionLabel, -textAngle*M_180_PIf, -sshift, vshift, true);
			}
		}

		if (fader8WCR.getInterstate()>0.f)
		{
			float minFader = qMin(fader4WCR.getInterstate(), fader8WCR.getInterstate());
			sPainter.setColor(color, minFader);
			sPainter.setFont(font8WCR);

			QMapIterator<Cardinals::CompassDirection, Vec3f> it8w(rose8winds);
			while(it8w.hasNext())
			{
				it8w.next();
				QString directionLabel = labels.value(it8w.key(), "");

				if (flagMask)
					bshift = ppx*static_cast<float>(sPainter.getFontMetrics().boundingRect(directionLabel).width())*0.5f;

				if (prj->project(it8w.value(), xy))
				{
					Vec3f up(it8w.value()[0], it8w.value()[1], 1.f*M_PI_180f);
					Vec3f upPrj;
					prj->project(up, upPrj);
					float dx=upPrj[0]-xy[0];
					float dy=upPrj[1]-xy[1];
					float textAngle=atan2(dx, dy);
					sPainter.drawText(xy[0], xy[1], directionLabel, -textAngle*M_180_PIf, -bshift, vshift, true);
				}
			}

			if (fader16WCR.getInterstate()>0.f)
			{
				sPainter.setColor(color, qMin(minFader, fader16WCR.getInterstate()));
				sPainter.setFont(font16WCR);

				QMapIterator<Cardinals::CompassDirection, Vec3f> it16w(rose16winds);
				while(it16w.hasNext())
				{
					it16w.next();
					QString directionLabel = labels.value(it16w.key(), "");

					if (flagMask)
						cshift = ppx*static_cast<float>(sPainter.getFontMetrics().boundingRect(directionLabel).width())*0.5f;

					if (prj->project(it16w.value(), xy))
					{
						Vec3f up(it16w.value()[0], it16w.value()[1], 1.f*M_PI_180f);
						Vec3f upPrj;
						prj->project(up, upPrj);
						float dx=upPrj[0]-xy[0];
						float dy=upPrj[1]-xy[1];
						float textAngle=atan2(dx, dy);
						sPainter.drawText(xy[0], xy[1], directionLabel, -textAngle*M_180_PIf, -cshift, vshift, true);
					}
				}

				if (fader32WCR.getInterstate()>0.f)
				{
					sPainter.setColor(color, qMin(minFader, fader32WCR.getInterstate()));
					sPainter.setFont(font32WCR);

					QMapIterator<Cardinals::CompassDirection, Vec3f> it32w(rose32winds);
					while(it32w.hasNext())
					{
						it32w.next();
						QString directionLabel = labels.value(it32w.key(), "");

						if (flagMask)
							dshift = ppx*static_cast<float>(sPainter.getFontMetrics().boundingRect(directionLabel).width())*0.5f;

						if (prj->project(it32w.value(), xy))
						{
							Vec3f up(it32w.value()[0], it32w.value()[1], 1.f*M_PI_180f);
							Vec3f upPrj;
							prj->project(up, upPrj);
							float dx=upPrj[0]-xy[0];
							float dy=upPrj[1]-xy[1];
							float textAngle=atan2(dx, dy);
							sPainter.drawText(xy[0], xy[1], directionLabel, -textAngle*M_180_PIf, -dshift, vshift, true);
						}
					}
				}
			}
		}
	}
}

// Translate cardinal labels with gettext to current sky language and update font for the language
void Cardinals::updateI18n()
{
	labels = {
		// TRANSLATORS: North
		{ dN,	qc_("N",   "compass direction") },
		// TRANSLATORS: South
		{ dS,	qc_("S",   "compass direction") },
		// TRANSLATORS: East
		{ dE,	qc_("E",   "compass direction") },
		// TRANSLATORS: West
		{ dW,	qc_("W",   "compass direction") },
		// TRANSLATORS: Northeast
		{ dNE,	qc_("NE",  "compass direction") },
		// TRANSLATORS: Southeast
		{ dSE,	qc_("SE",  "compass direction") },
		// TRANSLATORS: Southwest
		{ dSW,	qc_("SW",  "compass direction") },
		// TRANSLATORS: Northwest
		{ dNW,	qc_("NW",  "compass direction") },
		// TRANSLATORS: North-northeast
		{ dNNE,	qc_("NNE", "compass direction") },
		// TRANSLATORS: East-northeast
		{ dENE,	qc_("ENE", "compass direction") },
		// TRANSLATORS: East-southeast
		{ dESE,	qc_("ESE", "compass direction") },
		// TRANSLATORS: South-southeast
		{ dSSE,	qc_("SSE", "compass direction") },
		// TRANSLATORS: South-southwest
		{ dSSW,	qc_("SSW", "compass direction") },
		// TRANSLATORS: West-southwest
		{ dWSW,	qc_("WSW", "compass direction") },
		// TRANSLATORS: West-northwest
		{ dWNW, qc_("WNW", "compass direction") },
		// TRANSLATORS: North-northwest
		{ dNNW,	qc_("NNW", "compass direction") },
		// TRANSLATORS: North by east
		{ dNbE, qc_("NbE", "compass direction") },
		// TRANSLATORS: Northeast by north
		{dNEbN, qc_("NEbN","compass direction") },
		// TRANSLATORS: Northeast by east
		{dNEbE, qc_("NEbE","compass direction") },
		// TRANSLATORS: East by north
		{ dEbN, qc_("EbN", "compass direction") },
		// TRANSLATORS: East by south
		{ dEbS, qc_("EbS", "compass direction") },
		// TRANSLATORS: Southeast by east
		{dSEbE, qc_("SEbE","compass direction") },
		// TRANSLATORS: Southeast by south
		{dSEbS, qc_("SEbS","compass direction") },
		// TRANSLATORS: South by east
		{ dSbE, qc_("SbE", "compass direction") },
		// TRANSLATORS: South by west
		{ dSbW, qc_("SbW", "compass direction") },
		// TRANSLATORS: Southwest by south
		{dSWbS, qc_("SWbS","compass direction") },
		// TRANSLATORS: Southwest by west
		{dSWbW, qc_("SWbW","compass direction") },
		// TRANSLATORS: West by south
		{ dWbS, qc_("WbS", "compass direction") },
		// TRANSLATORS: West by north
		{ dWbN, qc_("WbN", "compass direction") },
		// TRANSLATORS: Northwest by west
		{dNWbW, qc_("NWbW","compass direction") },
		// TRANSLATORS: Northwest by north
		{dNWbN, qc_("NWbN","compass direction") },
		// TRANSLATORS: North by west
		{ dNbW, qc_("NbW", "compass direction") }
	};
}

LandscapeMgr::LandscapeMgr()
	: StelModule()
	, atmosphere(Q_NULLPTR)
	, cardinalPoints(Q_NULLPTR)
	, landscape(Q_NULLPTR)
	, oldLandscape(Q_NULLPTR)
	, messageTimer(new QTimer(this))
	, flagLandscapeSetsLocation(false)
	, flagLandscapeAutoSelection(false)
	, flagLightPollutionFromDatabase(false)
	, atmosphereNoScatter(false)
	, flagPolyLineDisplayedOnly(false)
	, polyLineThickness(1)
	, flagLandscapeUseMinimalBrightness(false)
	, defaultMinimalBrightness(0.01)
	, flagLandscapeSetsMinimalBrightness(false)
	, flagEnvironmentAutoEnabling(false)
	, flagLandscapeUseTransparency(false)
	, landscapeTransparency(0.)
{
	setObjectName("LandscapeMgr"); // should be done by StelModule's constructor.

	//Note: The first entry in the list is used as the default 'default landscape' in removeLandscape().
	packagedLandscapeIDs = (QStringList() << "guereins");
	QDirIterator directories(StelFileMgr::getInstallationDir()+"/landscapes/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while(directories.hasNext())
	{
		directories.next();
		packagedLandscapeIDs << directories.fileName();
	}
	packagedLandscapeIDs.removeDuplicates();
	landscapeCache.clear();

	messageTimer->setInterval(5000);
	messageTimer->setSingleShot(true);
	connect(messageTimer, &QTimer::timeout, this, &LandscapeMgr::clearMessage);
}

LandscapeMgr::~LandscapeMgr()
{
	delete cardinalPoints;
	if (oldLandscape)
	{
		delete oldLandscape;
		oldLandscape=Q_NULLPTR;
	}
	delete landscape;
	landscape = Q_NULLPTR;
	qDebug() << "LandscapeMgr: Clearing cache of" << landscapeCache.size() << "landscapes totalling about " << landscapeCache.totalCost() << "MB.";
	landscapeCache.clear(); // deletes all objects within.
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double LandscapeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SporadicMeteorMgr")->getCallOrder(actionName)+20;
	if (actionName==StelModule::ActionUpdate)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+10;
	// GZ The next 2 lines are only required to test landscape transparency. They should be commented away for releases.
	if (actionName==StelModule::ActionHandleMouseClicks)
		return StelApp::getInstance().getModuleMgr().getModule("StelMovementMgr")->getCallOrder(actionName)-1;
	return 0.;
}

void LandscapeMgr::update(double deltaTime)
{
	if(needToRecreateAtmosphere && !loadingAtmosphere)
		createAtmosphere();

	const auto core = StelApp::getInstance().getCore();
	const auto drawer = core->getSkyDrawer();

	if(loadingAtmosphere && loadingAtmosphere->isLoading())
	{
		try
		{
			// Use no more than 1/60th of a second for this batch of loading
			QElapsedTimer timer;
			timer.start();
			Atmosphere::LoadingStatus status={1,1};
			while(loadingAtmosphere->isLoading() && timer.elapsed() < 1000/60)
				status = loadingAtmosphere->stepDataLoading();
			if(loadingAtmosphere->isLoading())
			{
				setAtmosphereShowMySkyStoppedWithError(false);
				const auto percentDone = std::lround(100.*status.stepsDone/status.stepsToDo);
				setAtmosphereShowMySkyStatusText(QString("%1 %2% %3").arg(q_("Loading..."), QString::number(percentDone), qc_("done","percentage of done")));
				qDebug() << "Finished this batch of loading at" << percentDone << "%, will continue in the next frame";
			}
			else
			{
				setAtmosphereShowMySkyStatusText(q_("Switching models..."));
			}
		}
		catch(Atmosphere::InitFailure const& error)
		{
			qWarning() << "ERROR: Failed to load atmosphere model data:" << error.what();
			qWarning() << "WARNING: Falling back to the Preetham's model";
			setAtmosphereShowMySkyStoppedWithError(true);
			setAtmosphereShowMySkyStatusText(error.what());
			loadingAtmosphere.reset();
		}
	}

	if(loadingAtmosphere && loadingAtmosphere->isReadyToRender())
	{
		bool loaded = false;
		if(drawer->getFlagHasAtmosphere())
		{
			// Fade out current atmosphere, then fade in the new one
			if(atmosphere->getFlagShow())
			{
				atmosphere->setFlagShow(false);
			}
			else if(atmosphere->getFadeIntensity() == 0)
			{
				loadingAtmosphere->setFlagShow(true);
				loadingAtmosphere->setFadeDuration(atmosphere->getFadeDuration());
				loadingAtmosphere->setLightPollutionLuminance(atmosphere->getLightPollutionLuminance());
				loaded = true;
			}
		}
		else
		{
			loaded = true;
		}

		if(loaded)
		{
			atmosphere = std::move(loadingAtmosphere);
#ifdef ENABLE_SHOWMYSKY
			if(dynamic_cast<AtmosphereShowMySky*>(atmosphere.get()))
				setAtmosphereShowMySkyStatusText(q_("Loaded successfully"));
#endif
			emit atmosphereModelChanged(getAtmosphereModel());
		}
	}

	atmosphere->update(deltaTime);

	if (oldLandscape)
	{
		// This is only when transitioning to newly loaded landscape. We must draw the old one until the new one is faded in completely.
		oldLandscape->update(deltaTime);
		if (getIsLandscapeFullyVisible())
		{
			oldLandscape->setFlagShow(false);

			if (oldLandscape->getEffectiveLandFadeValue()< 0.01f)
			{
				// new logic: try to put old landscape to cache.
				//qDebug() << "LandscapeMgr::update: moving oldLandscape " << oldLandscape->getId() << "to Cache. Cost:" << oldLandscape->getMemorySize()/(1024*1024)+1;
				landscapeCache.insert(oldLandscape->getId(), oldLandscape, oldLandscape->getMemorySize()/(1024*1024)+1);
				//qDebug() << "--> LandscapeMgr::update(): cache now contains " << landscapeCache.size() << "landscapes totalling about " << landscapeCache.totalCost() << "MB.";
				oldLandscape=Q_NULLPTR;
			}
		}
	}
	landscape->update(deltaTime);
	cardinalPoints->update(deltaTime);

	// Compute the atmosphere color and intensity
	// Compute the sun position in local coordinate
	SolarSystem* ssystem = static_cast<SolarSystem*>(StelApp::getInstance().getModuleMgr().getModule("SolarSystem"));

	// Compute the moon position in local coordinate
	const auto sun   = ssystem->getSun();
	const auto moon  = ssystem->getMoon();
	const auto earth = ssystem->getEarth();
	const auto currentPlanet = core->getCurrentPlanet();
	const bool currentIsEarth = currentPlanet->getID() == earth->getID();
	// First parameter in next call is used for particularly earth-bound computations in Schaefer's sky brightness model. Difference DeltaT makes no difference here.
	// Temperature = 15°C, relative humidity = 40%
	try
	{
		atmosphere->computeColor(core, core->getJDE(), *currentPlanet, *sun,
								 currentIsEarth ? moon.data() : nullptr, core->getCurrentLocation(),
								 15.f, 40.f, static_cast<float>(drawer->getExtinctionCoefficient()), atmosphereNoScatter);
	}
	catch(Atmosphere::InitFailure const& error)
	{
		qWarning().noquote() << "ShowMySky atmosphere model crashed:" << error.what();
		qWarning() << "Loading Preetham model";
		showMessage(q_("ShowMySky atmosphere model crashed. Loading Preetham model as a fallback."));
		resetToFallbackAtmosphere();
	}

	core->getSkyDrawer()->reportLuminanceInFov(3.75f+atmosphere->getAverageLuminance()*3.5f, true);

	// NOTE: Simple workaround for brightness of landscape when observing from the Sun.
	if (currentPlanet->getID() == sun->getID())
	{
		landscape->setBrightness(1.0, 1.0);
		return;
	}

	// Compute the ground luminance based on every planets around
	// TBD: Reactivate and verify this code!? Source, reference?
//	float groundLuminance = 0;
//	const vector<Planet*>& allPlanets = ssystem->getAllPlanets();
//	for (auto i=allPlanets.begin();i!=allPlanets.end();++i)
//	{
//		Vec3d pos = (*i)->getAltAzPos(core);
//		pos.normalize();
//		if (pos[2] <= 0)
//		{
//			// No need to take this body into the landscape illumination computation
//			// because it is under the horizon
//		}
//		else
//		{
//			// Compute the Illuminance E of the ground caused by the planet in lux = lumen/m^2
//			float E = pow10(((*i)->get_mag(core)+13.988)/-2.5);
//			//qDebug() << "mag=" << (*i)->get_mag(core) << " illum=" << E;
//			// Luminance in cd/m^2
//			groundLuminance += E/0.44*pos[2]*pos[2]; // 1m^2 from 1.5 m above the ground is 0.44 sr.
//		}
//	}
//	groundLuminance*=atmosphere->getFadeIntensity();
//	groundLuminance=atmosphere->getAverageLuminance()/50;
//	qDebug() << "Atmosphere lum=" << atmosphere->getAverageLuminance() << " ground lum=" <<  groundLuminance;
//	qDebug() << "Adapted Atmosphere lum=" << eye->adaptLuminance(atmosphere->getAverageLuminance()) << " Adapted ground lum=" << eye->adaptLuminance(groundLuminance);

	// compute global ground brightness in a simplistic way, directly in RGB

	double landscapeBrightness=0.0;
	if (getFlagLandscapeUseMinimalBrightness())
	{
		// Setting from landscape.ini has priority if enabled
		if (getFlagLandscapeSetsMinimalBrightness() && landscape->getLandscapeMinimalBrightness()>=0)
			landscapeBrightness = landscape->getLandscapeMinimalBrightness();
		else
			landscapeBrightness = getDefaultMinimalBrightness();
	}

	Vec3d sunPos = sun->getAltAzPosAuto(core);
	sunPos.normalize();
	Vec3d moonPos = moon->getAltAzPosAuto(core);
	moonPos.normalize();

	// With atmosphere on, we define the solar brightness contribution zero when the sun is 8 degrees below the horizon.
	// The multiplier of 1.5 just looks better, it somehow represents illumination by scattered sunlight.
	// Else, we should account for sun's diameter but else just apply Lambertian Cos-rule and check with landscape opacity.
	double sinSunAngle = 0.0;
	if(atmosphere->getFlagShow())
	{
		sinSunAngle=sin(qMin(M_PI_2, asin(sunPos[2])+8.*M_PI/180.));
		if(sinSunAngle > -0.1/1.5 )
			landscapeBrightness +=  1.5*(sinSunAngle+0.1/1.5);
	}
	else
	{
		// In case we have exceptionally deep horizons ("Little Prince planet"), the sun will rise somehow over that line and demand light on the landscape.
		sinSunAngle=sin(qMin(M_PI_2, asin(qBound(-1.0, sunPos[2]-landscape->getSinMinAltitudeLimit(), 1.0) ) + (0.25 *M_PI_180)));
		if(sinSunAngle > 0.0)
			landscapeBrightness +=  (1.0-static_cast<double>(landscape->getOpacity(sunPos)))*sinSunAngle;
	}

	// GZ: 2013-09-25 Take light pollution into account!
	const float nelm = StelCore::luminanceToNELM(drawer->getLightPollutionLuminance());
	float pollutionAddonBrightness=(15.5f-2*nelm)*0.025f; // 0..8, so we assume empirical linear brightening 0..0.02
	float lunarAddonBrightness=0.f;
	if (currentIsEarth && moonPos[2] > -0.1/1.5)
		lunarAddonBrightness = qMax(0.2f/-12.f*moon->getVMagnitudeWithExtinction(core),0.f)*static_cast<float>(moonPos[2]);

	landscapeBrightness += static_cast<double>(qMax(lunarAddonBrightness, pollutionAddonBrightness));

	// TODO make this more generic for non-atmosphere planets
	if(atmosphere->getFadeIntensity() > 0.99999f )
	{
		// If the atmosphere is on, a solar eclipse might darken the sky
		// otherwise we just use the sun position calculation above
		landscapeBrightness *= static_cast<double>(atmosphere->getRealDisplayIntensityFactor()+0.1f);
	}
	// TODO: should calculate dimming with solar eclipse even without atmosphere on

	// Brightness can't be over 1.f (see https://bugs.launchpad.net/stellarium/+bug/1115364)
	if (landscapeBrightness>0.95)
		landscapeBrightness = 0.95;

	// GZ's rules and intentions for lightscape brightness:
	// lightscapeBrightness >0 makes sense only for sun below horizon.
	// If atmosphere on, we mix it in with darkening twilight. If atmosphere off, we can switch on more apruptly.
	// Note however that lightscape rendering does not per se depend on atmosphere on/off.
	// This allows for illuminated windows or light panels on spaceships. If a landscape's lightscape
	// contains light smog of a city, it should also be shown if atmosphere is switched off.
	// (Configure another landscape without light smog to avoid, or just switch off lightscape.)
	double lightscapeBrightness=0.0;
	const double sinSunAlt = sunPos[2];
	if (atmosphere->getFlagShow())
	{
		// light pollution layer is mixed in at -3...-8 degrees.
		if (sinSunAlt<-0.14)
			lightscapeBrightness=1.0;
		else if (sinSunAlt<-0.05)
			lightscapeBrightness = 1.0-(sinSunAlt+0.14)/(-0.05+0.14);
	}
	else
	{
		// If we have no atmosphere, we can assume windows and panels on spaceships etc. are switched on whenever the sun does not shine, i.e. when sun is blocked by landscape.
		lightscapeBrightness= static_cast<double>(landscape->getOpacity(sunPos));
	}

	landscape->setBrightness(landscapeBrightness, lightscapeBrightness);
	if (getFlagLandscapeUseTransparency())
		landscape->setTransparency(landscapeTransparency);

	messageFader.update(static_cast<int>(deltaTime*1000));
}

void LandscapeMgr::draw(StelCore* core)
{
	// For observers we never draw anything of landscape, atmosphere, cardinals.
	if (core->getCurrentPlanet()->getPlanetType()==Planet::isObserver)
		return;

	StelSkyDrawer* drawer=core->getSkyDrawer();

	// Draw the atmosphere
	if (!getFlagAtmosphereNoScatter())
	    atmosphere->draw(core);

	// GZ 2016-01: When we draw the atmosphere with a low sun, it is possible that the glaring red ball is overpainted and thus invisible.
	// Attempt to draw the sun only here while not having drawn it by SolarSystem:
	//if (atmosphere->getFlagShow())
	if (drawer->getFlagDrawSunAfterAtmosphere())
	{
		SolarSystem* ssys = GETSTELMODULE(SolarSystem);
		PlanetP sun=ssys->getSun();
		QFont font;
		font.setPixelSize(StelApp::getInstance().getScreenFontSize());
		sun->draw(core, 0, font);
	}

	// Draw the landscape
	if (oldLandscape)
		oldLandscape->draw(core, flagPolyLineDisplayedOnly);
	landscape->draw(core, flagPolyLineDisplayedOnly);

	// Draw the cardinal points
	cardinalPoints->draw(core, static_cast<double>(StelApp::getInstance().getCore()->getCurrentLocation().getLatitude()));

	if(messageFader.getInterstate())
	{
		const StelProjectorP prj = core->getProjection(StelCore::FrameEquinoxEqu);
		StelPainter painter(prj);
		QFont font;
		font.setPixelSize(16);
		painter.setFont(font);
		painter.setColor(1, 0, 0, messageFader.getInterstate());
		painter.drawText(83, 70, messageToShow);
	}
}

// Some element in drawing order behind LandscapeMgr can call this at the end of its own draw() to overdraw with the polygon line and gazetteer.
void LandscapeMgr::drawPolylineOnly(StelCore* core)
{
	// For observers we never draw anything of landscape, atmosphere, cardinals.
	if (core->getCurrentPlanet()->getPlanetType()==Planet::isObserver)
		return;

	// Draw the landscape
	if (oldLandscape && oldLandscape->hasLandscapePolygon())
		oldLandscape->draw(core, true);
	if (landscape->hasLandscapePolygon())
		landscape->draw(core, true);

	// Draw the cardinal points
	cardinalPoints->draw(core, static_cast<double>(StelApp::getInstance().getCore()->getCurrentLocation().getLatitude()));
}

void LandscapeMgr::createAtmosphere()
{
	const auto modelName=getAtmosphereModel();
	const auto modelConfig=modelName.toLower();
	bool needResetConfig=false;
	if(modelConfig==ATMOSPHERE_MODEL_CONF_VAL_PREETHAM)
	{
		loadingAtmosphere.reset(new AtmospherePreetham(skylight));
	}
#ifdef ENABLE_SHOWMYSKY
	else if(modelConfig==ATMOSPHERE_MODEL_CONF_VAL_SHOWMYSKY)
	{
		try
		{
			// Clear status so that if a repeated error happens, we do emit a signal that will update the GUI.
			setAtmosphereShowMySkyStatusText("");
			setAtmosphereShowMySkyStoppedWithError(false);

			const auto core = StelApp::getInstance().getCore();
			loadingAtmosphere.reset(new AtmosphereShowMySky(core->getCurrentLocation().altitude));
			if(!atmosphere)
			{
				// We're just loading the first atmosphere in the run of Stellarium. Initialize it synchronously.
				while(loadingAtmosphere->isLoading())
					loadingAtmosphere->stepDataLoading();

				setAtmosphereShowMySkyStoppedWithError(false);
				setAtmosphereShowMySkyStatusText(q_("Loaded successfully"));
			}
			else
			{
				setAtmosphereShowMySkyStoppedWithError(false);
				setAtmosphereShowMySkyStatusText(QString("%1 0% %2").arg(q_("Loading..."), qc_("done","percentage of done")));
			}
		}
		catch(Atmosphere::InitFailure const& error)
		{
			qWarning() << "ERROR: Failed to initialize ShowMySky atmosphere model:" << error.what();
			qWarning() << "WARNING: Falling back to the Preetham's model";
			loadingAtmosphere.reset(new AtmospherePreetham(skylight));
			needResetConfig=true;

			setAtmosphereShowMySkyStoppedWithError(true);
			setAtmosphereShowMySkyStatusText(error.what());
		}
	}
#endif
	else
	{
		qWarning() << "Unsupported atmosphere model" << modelName;
		loadingAtmosphere.reset(new AtmospherePreetham(skylight));
		needResetConfig=true;
	}
	if(!atmosphere)
	{
		// We're just loading the first atmosphere in the run of Stellarium. The atmosphere is fully loaded by this point.
		atmosphere = std::move(loadingAtmosphere);

		const auto conf=StelApp::getInstance().getSettings();
		setFlagAtmosphere(conf->value("landscape/flag_atmosphere", true).toBool());
		setAtmosphereFadeDuration(conf->value("landscape/atmosphere_fade_duration",0.5).toFloat());

		const auto drawer = StelApp::getInstance().getCore()->getSkyDrawer();
		setAtmosphereLightPollutionLuminance(drawer->getLightPollutionLuminance());
	}

	if(needResetConfig)
	{
		// We've failed to apply the setting, so reset to the fallback value
		const auto conf=StelApp::getInstance().getSettings();
		conf->setValue(ATMOSPHERE_MODEL_CONFIG_KEY, ATMOSPHERE_MODEL_CONF_VAL_PREETHAM);
	}

	needToRecreateAtmosphere=false;
}

void LandscapeMgr::resetToFallbackAtmosphere()
{
	StelApp::getInstance().getSettings()->setValue(ATMOSPHERE_MODEL_CONFIG_KEY, ATMOSPHERE_MODEL_CONF_VAL_PREETHAM);
	atmosphere.reset();
	createAtmosphere();
}

void LandscapeMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	StelApp *app = &StelApp::getInstance();
	Q_ASSERT(app);

	landscapeCache.setMaxCost(conf->value("landscape/cache_size_mb", 100).toInt());
	qDebug() << "LandscapeMgr: initialized Cache for" << landscapeCache.maxCost() << "MB.";

	// SET SIMPLE PROPERTIES FIRST, before loading the landscape (Loading may already make use of them! GH#1237)
	setFlagLandscapeSetsLocation(conf->value("landscape/flag_landscape_sets_location",false).toBool());
	setFlagLandscapeAutoSelection(conf->value("viewing/flag_landscape_autoselection", false).toBool());
	setFlagEnvironmentAutoEnable(conf->value("viewing/flag_environment_auto_enable",true).toBool());
	// Set minimal brightness for landscape. This feature has been added for folks which say "landscape is super dark, please add light". --AW
	setDefaultMinimalBrightness(conf->value("landscape/minimal_brightness", 0.01).toDouble());
	setFlagLandscapeUseMinimalBrightness(conf->value("landscape/flag_minimal_brightness", false).toBool());
	setFlagLandscapeSetsMinimalBrightness(conf->value("landscape/flag_landscape_sets_minimal_brightness",false).toBool());

	const auto var = conf->value(ATMOSPHERE_MODEL_PATH_CONFIG_KEY);
	if(!var.isValid())
		conf->setValue(ATMOSPHERE_MODEL_PATH_CONFIG_KEY, getDefaultAtmosphereModelPath());

	createAtmosphere();
	// Put the atmosphere's Skylight under the StelProperty system (simpler and more consistent GUI)
	StelApp::getInstance().getStelPropertyManager()->registerObject(&skylight);

	defaultLandscapeID = conf->value("init_location/landscape_name").toString();

	// We must make sure to allow auto location or command-line location even if landscape usually should set location.
	StelCore *core = StelApp::getInstance().getCore();
	const bool setLocationFromIPorCLI=((conf->value("init_location/location", "auto").toString() == "auto") || (core->getCurrentLocation().state=="CLI"));
	const bool shouldThenSetLocation=getFlagLandscapeSetsLocation();
	if (setLocationFromIPorCLI) setFlagLandscapeSetsLocation(false);
	setCurrentLandscapeID(defaultLandscapeID);
	setFlagLandscapeSetsLocation(shouldThenSetLocation);
	setFlagUseLightPollutionFromDatabase(conf->value("viewing/flag_light_pollution_database", false).toBool());
	setFlagLandscape(conf->value("landscape/flag_landscape", conf->value("landscape/flag_ground", true).toBool()).toBool());
	setFlagFog(conf->value("landscape/flag_fog",true).toBool());
	setFlagIllumination(conf->value("landscape/flag_enable_illumination_layer", true).toBool());
	setFlagLabels(conf->value("landscape/flag_enable_labels", true).toBool());
	setFlagPolyLineDisplayed(conf->value("landscape/flag_polyline_only", false).toBool());
	setPolyLineThickness(conf->value("landscape/polyline_thickness", 1).toInt());
	setLabelFontSize(conf->value("landscape/label_font_size", 18).toInt());
	setLabelColor(Vec3f(conf->value("landscape/label_color", "0.2,0.8,0.2").toString()));

	setFlagLandscapeUseTransparency(conf->value("landscape/flag_transparency", false).toBool());
	setLandscapeTransparency(conf->value("landscape/transparency", 0.5).toDouble());

	cardinalPoints = new Cardinals();
	cardinalPoints->setFlagShow4WCRLabels(conf->value("viewing/flag_cardinal_points", true).toBool());
	cardinalPoints->setFlagShow8WCRLabels(conf->value("viewing/flag_ordinal_points", true).toBool());
	cardinalPoints->setFlagShow16WCRLabels(conf->value("viewing/flag_16wcr_points", false).toBool());
	cardinalPoints->setFlagShow32WCRLabels(conf->value("viewing/flag_32wcr_points", false).toBool());
	// Load colors from config file
	QString defaultColor = conf->value("color/default_color").toString();
	setColorCardinalPoints(Vec3f(conf->value("color/cardinal_color", defaultColor).toString()));

	currentPlanetName = app->getCore()->getCurrentLocation().planetName;
	//Bortle scale is managed by SkyDrawer
	StelSkyDrawer* drawer = app->getCore()->getSkyDrawer();
	Q_ASSERT(drawer);
	setAtmosphereLightPollutionLuminance(drawer->getLightPollutionLuminance());
	connect(app->getCore(), SIGNAL(locationChanged(StelLocation)), this, SLOT(onLocationChanged(StelLocation)));
	connect(app->getCore(), SIGNAL(targetLocationChanged(const StelLocation&, const QString&)), this, SLOT(onTargetLocationChanged(const StelLocation&, const QString&)));
	connect(drawer, &StelSkyDrawer::lightPollutionLuminanceChanged, this, &LandscapeMgr::setAtmosphereLightPollutionLuminance);
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Atmosphere", displayGroup, N_("Atmosphere"), "atmosphereDisplayed", "A");
	addAction("actionShow_Fog", displayGroup, N_("Fog"), "fogDisplayed", "F");
	addAction("actionShow_Cardinal_Points", displayGroup, N_("Cardinal points"), "cardinalPointsDisplayed", "Q");
	addAction("actionShow_Intercardinal_Points", displayGroup, N_("Ordinal (Intercardinal) points"), "ordinalPointsDisplayed");
	addAction("actionShow_Secondary_Intercardinal_Points", displayGroup, N_("Secondary Intercardinal points"), "ordinal16WRPointsDisplayed");
	addAction("actionShow_Tertiary_Intercardinal_Points", displayGroup, N_("Tertiary Intercardinal points"), "ordinal32WRPointsDisplayed");
	addAction("actionShow_Ground", displayGroup, N_("Ground"), "landscapeDisplayed", "G");
	addAction("actionShow_LandscapeIllumination", displayGroup, N_("Landscape illumination"), "illuminationDisplayed", "Shift+G");
	addAction("actionShow_LandscapeLabels", displayGroup, N_("Landscape labels"), "labelsDisplayed", "Ctrl+Shift+G");
	addAction("actionShow_LightPollutionFromDatabase", displayGroup, N_("Light pollution data from locations database"), "flagUseLightPollutionFromDatabase");
	// Details: https://github.com/Stellarium/stellarium/issues/171
	addAction("actionShow_LightPollutionIncrease", displayGroup, N_("Increase light pollution"), "increaseLightPollution()");
	addAction("actionShow_LightPollutionReduce", displayGroup, N_("Reduce light pollution"), "reduceLightPollution()");
	addAction("actionShow_LightPollutionCyclicChange", displayGroup, N_("Cyclic change in light pollution"), "cyclicChangeLightPollution()");
}

bool LandscapeMgr::setCurrentLandscapeID(const QString& id, const double changeLocationDuration)
{
	if (id.isEmpty())
		return false;

	//prevent unnecessary changes/file access
	if(id==currentLandscapeID)
		return false;

	if (!getAllLandscapeIDs().contains(id))
	{
		qDebug() << "LandscapeMgr::setCurrentLandscapeID: unknown landscape" << id << ", using 'zero'";
		return setCurrentLandscapeID("zero", changeLocationDuration);
	}

	Landscape* newLandscape;

	// There is a slight chance that we switch back to oldLandscape while oldLandscape is still fading away.
	// in this case it is not yet stored in cache, but obviously available. So we just swap places.
	if (oldLandscape && oldLandscape->getId()==id)
	{
		newLandscape=oldLandscape;
	}
	else
	{
		// We want to lookup the landscape ID (dir) from the name.
		newLandscape= landscapeCache.take(id);

		if (newLandscape)
		{
#ifndef NDEBUG
			qDebug() << "LandscapeMgr::setCurrentLandscapeID():: taken " << id << "from cache...";
			qDebug() << ".-->LandscapeMgr::setCurrentLandscapeID(): cache contains " << landscapeCache.size() << "landscapes totalling about " << landscapeCache.totalCost() << "MB.";
#endif
		}
		else
		{
#ifndef NDEBUG
			qDebug() << "LandscapeMgr::setCurrentLandscapeID: Loading from file:" << id ;
#endif
			newLandscape = createFromFile(StelFileMgr::findFile("landscapes/" + id + "/landscape.ini"), id);
		}

		if (!newLandscape)
		{
			qWarning() << "ERROR while loading landscape " << "landscapes/" + id + "/landscape.ini";
			return false;
		}
	}

	// Keep current landscape for a while, while new landscape fades in!
	// This prevents subhorizon sun or grid becoming briefly visible.
	if (landscape)
	{
		// Copy display parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		newLandscape->setFlagShowIllumination(landscape->getFlagShowIllumination());
		newLandscape->setFlagShowLabels(landscape->getFlagShowLabels());
		newLandscape->setLabelFontSize(landscape->getLabelFontSize());
		newLandscape->setLabelColor(landscape->getLabelColor());

		// If we have an oldLandscape that is not just swapped back, put that into cache.
		if (oldLandscape && oldLandscape!=newLandscape)
		{
#ifndef NDEBUG
			qDebug() << "LandscapeMgr::setCurrent: moving oldLandscape " << oldLandscape->getId() << "to Cache. Cost:" << oldLandscape->getMemorySize()/(1024*1024)+1;
#endif
			landscapeCache.insert(oldLandscape->getId(), oldLandscape, oldLandscape->getMemorySize()/(1024*1024)+1);
#ifndef NDEBUG
			qDebug() << "-->LandscapeMgr::setCurrentLandscapeId(): cache contains " << landscapeCache.size() << "landscapes totalling about " << landscapeCache.totalCost() << "MB.";
#endif
		}
		oldLandscape = landscape; // keep old while transitioning!
	}
	landscape=newLandscape;
	currentLandscapeID = id;

	if (getFlagLandscapeSetsLocation() && landscape->hasLocation())
	{
		StelCore *core = StelApp::getInstance().getCore();
		core->moveObserverTo(landscape->getLocation(), changeLocationDuration, changeLocationDuration, id);
		StelSkyDrawer* drawer=core->getSkyDrawer();

		if (landscape->getLocation().ianaTimeZone.length())
		{
			core->setCurrentTimeZone(landscape->getLocation().ianaTimeZone);
		}
		if (landscape->getDefaultFogSetting() >-1)
		{
			setFlagFog(static_cast<bool>(landscape->getDefaultFogSetting()));
			landscape->setFlagShowFog(static_cast<bool>(landscape->getDefaultFogSetting()));
		}
		if (landscape->getDefaultLightPollutionLuminance().isValid())
		{
			drawer->setLightPollutionLuminance(landscape->getDefaultLightPollutionLuminance().toFloat());
		}
		if (landscape->getDefaultAtmosphericExtinction() >= 0.0)
		{
			drawer->setExtinctionCoefficient(landscape->getDefaultAtmosphericExtinction());
		}
		if (landscape->getDefaultAtmosphericTemperature() > -273.15)
		{
			drawer->setAtmosphereTemperature(landscape->getDefaultAtmosphericTemperature());
		}
		if (landscape->getDefaultAtmosphericPressure() >= 0.0)
		{
			drawer->setAtmospherePressure(landscape->getDefaultAtmosphericPressure());
		}
		else if (landscape->getDefaultAtmosphericPressure() < 0.0)
		{
			// compute standard pressure for standard atmosphere in given altitude if landscape.ini coded as atmospheric_pressure=-1
			// International altitude formula found in Wikipedia.
			double alt=landscape->getLocation().altitude;
			double p=1013.25*std::pow(1-(0.0065*alt)/288.15, 5.255);
			drawer->setAtmospherePressure(p);
		}
	}

	emit currentLandscapeChanged(currentLandscapeID,getCurrentLandscapeName());

	// else qDebug() << "Will not set new location; Landscape location: planet: " << landscape->getLocation().planetName << "name: " << landscape->getLocation().name;
	return true;
}

bool LandscapeMgr::setCurrentLandscapeName(const QString& name, const double changeLocationDuration)
{
	if (name.isEmpty())
		return false;
	
	QMap<QString,QString> nameToDirMap = getNameToDirMap();
	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
		//qDebug() << "Resolving " << name << "as" << nameToDirMap[name];
		return setCurrentLandscapeID(nameToDirMap[name], changeLocationDuration);
	}
	// Legacy mess-up: e.g. Scenery3D calls name but should mean ID.
	else if (!setCurrentLandscapeID(name, changeLocationDuration))
	{
		qWarning() << "Can't find a landscape with name=" << name << StelUtils::getEndLineChar();
		return false;
	}
	qDebug() << "Loading landscapeID" << name;
	return true;
}

// Load a landscape into cache.
// @param id the ID of a landscape
// @param replace true if existing landscape entry should be replaced (useful during development to reload after edit)
// @return false if landscape could not be found, or existed already and replace was false.
bool LandscapeMgr::precacheLandscape(const QString& id, const bool replace)
{
	if (landscapeCache.contains(id) && (!replace))
		return false;

	Landscape* newLandscape = createFromFile(StelFileMgr::findFile("landscapes/" + id + "/landscape.ini"), id);
	if (!newLandscape)
	{
		qWarning() << "ERROR while preloading landscape " << "landscapes/" + id + "/landscape.ini";
		return false;
	}

	bool res=landscapeCache.insert(id, newLandscape, newLandscape->getMemorySize()/(1024*1024)+1);
#ifndef NDEBUG
	if (res)
	{
		qDebug() << "LandscapeMgr::precacheLandscape(): Successfully added landscape with ID " << id << "to cache";
	}
	qDebug() << "LandscapeMgr::precacheLandscape(): cache contains " << landscapeCache.size() << "landscapes totalling about " << landscapeCache.totalCost() << "MB.";
#endif
	return res;
}

// Remove a landscape from the cache of loaded landscapes.
// @param id the ID of a landscape
// @return false if landscape could not be found
bool LandscapeMgr::removeCachedLandscape(const QString& id)
{
	bool res= landscapeCache.remove(id);
#ifndef NDEBUG
	qDebug() << "LandscapeMgr::removeCachedLandscape(): cache contains " << landscapeCache.size() << "landscapes totalling about " << landscapeCache.totalCost() << "MB.";
#endif
	return res;
}


// Change the default landscape to the landscape with the ID specified.
bool LandscapeMgr::setDefaultLandscapeID(const QString& id)
{
	if (id.isEmpty())
		return false;
	defaultLandscapeID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("init_location/landscape_name", id);
	emit defaultLandscapeChanged(id);
	return true;
}

void LandscapeMgr::updateI18n()
{
	// Translate all labels with the new language
	if (cardinalPoints) cardinalPoints->updateI18n();
	landscape->loadLabels(getCurrentLandscapeID());
}

void LandscapeMgr::setFlagLandscape(const bool displayed)
{
	if (oldLandscape && !displayed)
		oldLandscape->setFlagShow(false);
	if(landscape->getFlagShow() != displayed) {
		landscape->setFlagShow(displayed);
		emit landscapeDisplayedChanged(displayed);
	}
}

bool LandscapeMgr::getFlagLandscape() const
{
	return landscape->getFlagShow();
}

bool LandscapeMgr::getIsLandscapeFullyVisible() const
{
	return landscape->getIsFullyVisible();
}

double LandscapeMgr::getLandscapeSinMinAltitudeLimit() const
{
	if (flagLandscapeUseTransparency && landscapeTransparency>0.)
		return -1.;
	else
		return landscape->getSinMinAltitudeLimit();
}

bool LandscapeMgr::getFlagUseLightPollutionFromDatabase() const
{
	return flagLightPollutionFromDatabase;
}

void LandscapeMgr::setFlagUseLightPollutionFromDatabase(const bool usage)
{
	if (flagLightPollutionFromDatabase != usage)
	{
		flagLightPollutionFromDatabase = usage;

		StelCore* core = StelApp::getInstance().getCore();

		//this was previously logic in ViewDialog, but should really be on a non-GUI layer
		if (usage)
		{
			StelLocation loc = core->getCurrentLocation();
			onLocationChanged(loc);
		}

		emit flagUseLightPollutionFromDatabaseChanged(usage);
	}
}

void LandscapeMgr::onLocationChanged(const StelLocation &loc)
{
	if(flagLightPollutionFromDatabase)
	{
		//this was previously logic in ViewDialog, but should really be on a non-GUI layer
		StelCore* core = StelApp::getInstance().getCore();
		float lum=0.; // location not on Earth...
		if (loc.planetName.contains("Earth"))
		{
			if(loc.lightPollutionLuminance.isValid())
				lum = loc.lightPollutionLuminance.toFloat();
			else // ...or it is an observatory, or it is an unknown location
				lum = loc.DEFAULT_LIGHT_POLLUTION_LUMINANCE;
		}
		core->getSkyDrawer()->setLightPollutionLuminance(lum);
	}
}

// Load landscapeID, but do not load its associated location.
// If landscapeID is empty but flagLandscapeAutoSelection is true, load a location fitting to loc's planet.
void LandscapeMgr::onTargetLocationChanged(const StelLocation &loc, const QString& landscapeID)
{
	//qDebug() << "LandscapeMgr::onTargetLocationChanged:" << loc.serializeToLine().replace('\t', '|') << "Landscape requested:" << landscapeID;
	if (!landscapeID.isEmpty() && getAllLandscapeIDs().contains(landscapeID))
	{
		const bool landscapeSetsLocation = getFlagLandscapeSetsLocation();
		setFlagLandscapeSetsLocation(false);
		setCurrentLandscapeID(landscapeID);
		setFlagLandscapeSetsLocation(landscapeSetsLocation);
	}
	else if(landscapeID.startsWith("ZeroColor("))
	{
		// Load a zero landscape and recolor it.
		// This can happen when clicking on the map. The point on the map can be sampled for color (e.g., desert, greengrass, ocean blue, polar white, ...)
		const bool landscapeSetsLocation = getFlagLandscapeSetsLocation();
		setFlagLandscapeSetsLocation(false);
		setCurrentLandscapeID("zero");
		Vec3f color(0.3);
		static const QRegularExpression zeroColor("^ZeroColor\\(([0-9].[0-9]+,[0-9].[0-9]+,[0-9].[0-9]+)\\)$");
		QRegularExpressionMatch match=zeroColor.match(landscapeID);
		if (match.hasMatch())
			color=Vec3f(match.captured(1));
		else
			qDebug() << "Cannot extract color from landscapeID" << landscapeID;
		LandscapePolygonal *l=static_cast<LandscapePolygonal*>(landscape);
		l->setGroundColor(color);
		setFlagLandscapeSetsLocation(landscapeSetsLocation);
	}
	else if (flagLandscapeAutoSelection && (loc.planetName != currentPlanetName))
	{
		//qDebug() << "landscapeID empty. Try planet name" << loc.planetName << "or zero";
		// If we have a landscape for selected planet then set it, otherwise use zero horizon landscape
		const bool landscapeSetsLocation = getFlagLandscapeSetsLocation();
		setFlagLandscapeSetsLocation(false);
		if (getAllLandscapeNames().indexOf(loc.planetName)>0)
			setCurrentLandscapeName(loc.planetName);
		else
			setCurrentLandscapeID("zero");
		setFlagLandscapeSetsLocation(landscapeSetsLocation);
	}

	if (loc.role==QChar('o')) // observer?
	{
		if (flagEnvironmentAutoEnabling)
		{
			setFlagAtmosphere(false);
			setFlagFog(false);
			setFlagLandscape(false);
			setFlagCardinalPoints(false); // suppresses all
		}
	}
	else
	{
		SolarSystem* ssystem = static_cast<SolarSystem*>(StelApp::getInstance().getModuleMgr().getModule("SolarSystem"));
		PlanetP pl = ssystem->searchByEnglishName(loc.planetName);
		if (pl && flagEnvironmentAutoEnabling && currentPlanetName!=loc.planetName)
		{
			QSettings* conf = StelApp::getInstance().getSettings();
			setFlagAtmosphere(pl->hasAtmosphere() && conf->value("landscape/flag_atmosphere", true).toBool());
			setFlagFog(pl->hasAtmosphere() && conf->value("landscape/flag_fog", true).toBool());
			setFlagLandscape(conf->value("landscape/flag_landscape", true).toBool());
			setFlagCardinalPoints(conf->value("viewing/flag_cardinal_points", true).toBool());
			setFlagOrdinalPoints(conf->value("viewing/flag_ordinal_points", true).toBool());
			setFlagOrdinal16WRPoints(conf->value("viewing/flag_16wcr_points", false).toBool());
			setFlagOrdinal32WRPoints(conf->value("viewing/flag_32wcr_points", false).toBool());
		}
	}
	currentPlanetName = loc.planetName;
	//qDebug() << "LandscapeMgr::onTargetLocationChanged done" ;
}

void LandscapeMgr::setFlagFog(const bool displayed)
{
	if (landscape->getFlagShowFog() != displayed) {
		landscape->setFlagShowFog(displayed);
		emit fogDisplayedChanged(displayed);
	}
}

bool LandscapeMgr::getFlagFog() const
{
	return landscape->getFlagShowFog();
}

void LandscapeMgr::setFlagIllumination(const bool displayed)
{
	if (landscape->getFlagShowIllumination() != displayed) {
		landscape->setFlagShowIllumination(displayed);
		emit illuminationDisplayedChanged(displayed);
	}
}

bool LandscapeMgr::getFlagIllumination() const
{
	return landscape->getFlagShowIllumination();
}

void LandscapeMgr::setLandscapeTransparency(const double f)
{
	landscapeTransparency = f;
	emit landscapeDisplayedChanged(f);
}

double LandscapeMgr::getLandscapeTransparency() const
{
	return landscapeTransparency;
}

void LandscapeMgr::setFlagLabels(const bool displayed)
{
	if (landscape->getFlagShowLabels() != displayed) {
		landscape->setFlagShowLabels(displayed);
		emit labelsDisplayedChanged(displayed);
	}
}

bool LandscapeMgr::getFlagLabels() const
{
	return landscape->getFlagShowLabels();
}

void LandscapeMgr::setLabelFontSize(const int size)
{
	landscape->setLabelFontSize(size);
	emit labelFontSizeChanged(size);
}

int LandscapeMgr::getLabelFontSize() const
{
	return landscape->getLabelFontSize();
}

void LandscapeMgr::setLabelColor(const Vec3f& c)
{
	landscape->setLabelColor(c);
	emit labelColorChanged(c);
}

Vec3f LandscapeMgr::getLabelColor() const
{
	return landscape->getLabelColor();
}

void LandscapeMgr::setFlagLandscapeAutoSelection(bool enableAutoSelect)
{
	if(enableAutoSelect != flagLandscapeAutoSelection)
	{
		flagLandscapeAutoSelection = enableAutoSelect;
		emit flagLandscapeAutoSelectionChanged(enableAutoSelect);
	}
}

bool LandscapeMgr::getFlagLandscapeAutoSelection() const
{
	return flagLandscapeAutoSelection;
}

void LandscapeMgr::setFlagEnvironmentAutoEnable(bool b)
{
	if(b != flagEnvironmentAutoEnabling)
	{
		flagEnvironmentAutoEnabling = b;
		emit setFlagEnvironmentAutoEnableChanged(b);
	}
}

bool LandscapeMgr::getFlagEnvironmentAutoEnable() const
{
	return flagEnvironmentAutoEnabling;
}

/*********************************************************************
 Retrieve list of the names of all the available landscapes
 *********************************************************************/
QStringList LandscapeMgr::getAllLandscapeNames() const
{
	return getNameToDirMap().keys();
}

QStringList LandscapeMgr::getAllLandscapeIDs() const
{
	return getNameToDirMap().values();
}

QStringList LandscapeMgr::getUserLandscapeIDs() const
{
	QStringList result;
	QMapIterator<QString, QString> it(getNameToDirMap());
	while (it.hasNext())
	{
		it.next();
		if(!packagedLandscapeIDs.contains(it.value()))
			result.append(it.value());
	}
	return result;
}

QString LandscapeMgr::getCurrentLandscapeName() const
{
	return landscape->getName();
}

QString LandscapeMgr::getCurrentLandscapeHtmlDescription() const
{
	QString desc = getDescription();

	QString author = landscape->getAuthorName();

	desc += "<p>";
	if (!author.isEmpty())
		desc += QString("<b>%1</b>: %2<br />").arg(q_("Author"), author);

	// This previously showed 0/0 for locationless landscapes!
	if (landscape->hasLocation())
	{
		//TRANSLATORS: Unit of measure for distance - meter
		QString alt = qc_("m", "distance");

		desc += QString("<b>%1</b>: %2, %3, %4 %5").arg(
				q_("Location"),
				StelUtils::radToDmsStrAdapt(static_cast<double>(landscape->getLocation().getLatitude()) *M_PI_180),
				StelUtils::radToDmsStrAdapt(static_cast<double>(landscape->getLocation().getLongitude()) * M_PI_180),
				QString::number(landscape->getLocation().altitude),
				alt);

		QString planetName = landscape->getLocation().planetName;		
		if (!planetName.isEmpty())
		{
			const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
			desc += QString(", %1").arg(trans.qtranslate(planetName, "major planet")); // TODO: Enhance the context support
		}
		desc += "<br />";

		QStringList atmosphere;
		//atmosphere.clear(); // Huh?

		double pressure = landscape->getDefaultAtmosphericPressure();
		if (pressure>0.)
		{
			// 1 mbar = 1 hPa
			//TRANSLATORS: Unit of measure for pressure - hectopascals
			QString hPa = qc_("hPa", "pressure");
			atmosphere.append(QString("%1 %2").arg(QString::number(pressure, 'f', 1), hPa));
		}

		double temperature = landscape->getDefaultAtmosphericTemperature();
		if (temperature>-1000.0)
			atmosphere.append(QString("%1 %2C").arg(QString::number(temperature, 'f', 1)).arg(QChar(0x00B0)));

		double extcoeff = landscape->getDefaultAtmosphericExtinction();
		if (extcoeff>-1.0)
			atmosphere.append(QString("%1: %2").arg(q_("extinction coefficient"), QString::number(extcoeff, 'f', 2)));

		if (!atmosphere.isEmpty())
			desc += QString("<b>%1</b>: %2<br />").arg(q_("Atmospheric conditions"), atmosphere.join(", "));

		const auto lightPollutionLum = landscape->getDefaultLightPollutionLuminance();
		if (lightPollutionLum.isValid())
		{
			const auto lum = lightPollutionLum.toFloat();
			auto scaledLum = lum;
			QString unit = q_("cd/m<sup>2</sup>");
			if(lum < 1e-6f)
			{
				scaledLum = lum*1e9f;
				unit = q_("ncd/m<sup>2</sup>");
			}
			else if(lum < 1e-3f)
			{
				scaledLum = lum*1e6f;
				unit = q_("&mu;cd/m<sup>2</sup>");
			}
			else if(lum < 1)
			{
				scaledLum = lum*1e3f;
				unit = q_("mcd/m<sup>2</sup>");
			}
			desc += q_("<b>Light pollution</b>: %1 %2 (NELM: %3; Bortle class: %4)")
						.arg(scaledLum).arg(unit).arg(StelCore::luminanceToNELM(lum))
						.arg(StelCore::luminanceToBortleScaleIndex(lum));
		}
	}	
	return desc;
}

//! Set flag for displaying cardinal points
void LandscapeMgr::setFlagCardinalPoints(const bool displayed)
{
	if (cardinalPoints->getFlagShow4WCRLabels() != displayed)
	{
		cardinalPoints->setFlagShow4WCRLabels(displayed);
		emit cardinalPointsDisplayedChanged(displayed);
	}
}

//! Get flag for displaying cardinal points
bool LandscapeMgr::getFlagCardinalPoints() const
{
	return cardinalPoints->getFlagShowCardinals();
}

//! Set flag for displaying ordinal points
void LandscapeMgr::setFlagOrdinalPoints(const bool displayed)
{
	if (cardinalPoints->getFlagShow8WCRLabels() != displayed)
	{
		cardinalPoints->setFlagShow8WCRLabels(displayed);
		emit ordinalPointsDisplayedChanged(displayed);
	}
}

//! Get flag for displaying ordinal points
bool LandscapeMgr::getFlagOrdinalPoints() const
{
	return cardinalPoints->getFlagShow8WCRLabels();
}

//! Set flag for displaying ordinal points
void LandscapeMgr::setFlagOrdinal16WRPoints(const bool displayed)
{
	if (cardinalPoints->getFlagShow16WCRLabels() != displayed)
	{
		cardinalPoints->setFlagShow16WCRLabels(displayed);
		emit ordinal16WRPointsDisplayedChanged(displayed);
	}
}

//! Get flag for displaying ordinal points
bool LandscapeMgr::getFlagOrdinal16WRPoints() const
{
	return cardinalPoints->getFlagShow16WCRLabels();
}

//! Set flag for displaying ordinal points
void LandscapeMgr::setFlagOrdinal32WRPoints(const bool displayed)
{
	if (cardinalPoints->getFlagShow32WCRLabels() != displayed)
	{
		cardinalPoints->setFlagShow32WCRLabels(displayed);
		emit ordinal32WRPointsDisplayedChanged(displayed);
	}
}

//! Get flag for displaying ordinal points
bool LandscapeMgr::getFlagOrdinal32WRPoints() const
{
	return cardinalPoints->getFlagShow32WCRLabels();
}

//! Set Cardinals Points color
void LandscapeMgr::setColorCardinalPoints(const Vec3f& v)
{
	if(v != getColorCardinalPoints())
	{
		cardinalPoints->setColor(v);
		emit cardinalPointsColorChanged(v);
	}
}

//! Get Cardinals Points color
Vec3f LandscapeMgr::getColorCardinalPoints() const
{
	return cardinalPoints->getColor();
}

///////////////////////////////////////////////////////////////////////////////////////
// Atmosphere
//! Set flag for displaying Atmosphere
void LandscapeMgr::setFlagAtmosphere(const bool displayed)
{
	if (atmosphere->getFlagShow() != displayed) {
		atmosphere->setFlagShow(displayed);
		StelApp::getInstance().getCore()->getSkyDrawer()->setFlagHasAtmosphere(displayed);
		emit atmosphereDisplayedChanged(displayed);
		//if (StelApp::getInstance().getSettings()->value("landscape/flag_fog", true).toBool())
		//	setFlagFog(displayed); // sync of visibility of fog because this is atmospheric phenomena
		// GZ This did not work as it may have been intended. Switch off fog, switch off atmosphere. Switch on atmosphere, and you have fog?
		// --> Fog is only drawn in Landscape if atmosphere is switched on!
	}
}

void LandscapeMgr::setAtmosphereModel(const QString& model)
{
	const auto modelToSet = model.toLower();
	const auto oldModel = getAtmosphereModel().toLower();
	if(modelToSet == oldModel)
		return;

	StelApp::getInstance().getSettings()->setValue(ATMOSPHERE_MODEL_CONFIG_KEY, model);

	if(!(oldModel.isEmpty() && modelToSet == ATMOSPHERE_MODEL_CONF_VAL_DEFAULT))
	{
		// Can't call createAtmosphere() right now, because we likely have wrong OpenGL context (or even none).
		// So just schedule it for the next draw.
		needToRecreateAtmosphere=true;
	}
}

void LandscapeMgr::setAtmosphereModelPath(const QString& path)
{
	if(getAtmosphereModelPath()==path)
		return;

	StelApp::getInstance().getSettings()->setValue(ATMOSPHERE_MODEL_PATH_CONFIG_KEY, path);
	setAtmosphereModel(ATMOSPHERE_MODEL_CONF_VAL_SHOWMYSKY); // This is the only relevant model for this property
	needToRecreateAtmosphere=true;

	emit atmosphereModelPathChanged(path);
}

void LandscapeMgr::setAtmosphereShowMySkyStoppedWithError(const bool error)
{
	if(atmosphereShowMySkyStoppedWithError == error)
		return;
	atmosphereShowMySkyStoppedWithError = error;
	emit atmosphereStoppedWithErrorChanged(error);
}

void LandscapeMgr::setAtmosphereShowMySkyStatusText(const QString& text)
{
	if(atmosphereShowMySkyStatusText == text)
		return;
	atmosphereShowMySkyStatusText = text;
	emit atmosphereStatusTextChanged(text);
}

void LandscapeMgr::setFlagAtmosphereZeroOrderScattering(const bool enable)
{
	atmosphereZeroOrderScatteringEnabled=enable;
	emit flagAtmosphereZeroOrderScatteringChanged(enable);
}

void LandscapeMgr::setFlagAtmosphereSingleScattering(const bool enable)
{
	atmosphereSingleScatteringEnabled=enable;
	emit flagAtmosphereSingleScatteringChanged(enable);
}

void LandscapeMgr::setFlagAtmosphereMultipleScattering(const bool enable)
{
	atmosphereMultipleScatteringEnabled=enable;
	emit flagAtmosphereMultipleScatteringChanged(enable);
}

void LandscapeMgr::setAtmosphereEclipseSimulationQuality(const int quality)
{
	if(getAtmosphereEclipseSimulationQuality() == quality)
		return;

	StelApp::getInstance().getSettings()->setValue(ATMOSPHERE_ECLIPSE_SIM_QUALITY_CONFIG_KEY, quality);

	emit atmosphereEclipseSimulationQualityChanged(quality);
}

//! Get flag for displaying Atmosphere
bool LandscapeMgr::getFlagAtmosphere() const
{
	return atmosphere->getFlagShow();
}

//! Set flag for displaying Atmosphere
void LandscapeMgr::setFlagAtmosphereNoScatter(const bool noScatter)
{
	atmosphereNoScatter=noScatter;
	emit atmosphereNoScatterChanged(noScatter);
}

//! Get flag for displaying Atmosphere
bool LandscapeMgr::getFlagAtmosphereNoScatter() const
{
	return atmosphereNoScatter;
}

QString LandscapeMgr::getAtmosphereModel() const
{
	const auto conf=StelApp::getInstance().getSettings();
	return conf->value(ATMOSPHERE_MODEL_CONFIG_KEY, ATMOSPHERE_MODEL_CONF_VAL_DEFAULT).toString();
}

QString LandscapeMgr::getAtmosphereModelPath() const
{
	const auto conf=StelApp::getInstance().getSettings();

	const auto var = conf->value(ATMOSPHERE_MODEL_PATH_CONFIG_KEY);
	if(var.isValid()) return var.toString();

	return getDefaultAtmosphereModelPath();
}

QString LandscapeMgr::getDefaultAtmosphereModelPath() const
{
	return QDir::toNativeSeparators(QString("%1/atmosphere/default").arg(StelFileMgr::getInstallationDir()));
}

bool LandscapeMgr::getAtmosphereShowMySkyStoppedWithError() const
{
	return atmosphereShowMySkyStoppedWithError;
}

QString LandscapeMgr::getAtmosphereShowMySkyStatusText() const
{
	return atmosphereShowMySkyStatusText;
}

bool LandscapeMgr::getFlagAtmosphereZeroOrderScattering() const
{
	return atmosphereZeroOrderScatteringEnabled;
}

bool LandscapeMgr::getFlagAtmosphereSingleScattering() const
{
	return atmosphereSingleScatteringEnabled;
}

bool LandscapeMgr::getFlagAtmosphereMultipleScattering() const
{
	return atmosphereMultipleScatteringEnabled;
}

int LandscapeMgr::getAtmosphereEclipseSimulationQuality() const
{
	const auto conf=StelApp::getInstance().getSettings();
	return conf->value(ATMOSPHERE_ECLIPSE_SIM_QUALITY_CONFIG_KEY, 1).toInt();
}

float LandscapeMgr::getAtmosphereFadeIntensity() const
{
	return atmosphere->getFadeIntensity();
}

//! Set atmosphere fade duration in s
void LandscapeMgr::setAtmosphereFadeDuration(const float f)
{
	atmosphere->setFadeDuration(f);
}

//! Get atmosphere fade duration in s
float LandscapeMgr::getAtmosphereFadeDuration() const
{
	return atmosphere->getFadeDuration();
}

//! Set light pollution luminance level
void LandscapeMgr::setAtmosphereLightPollutionLuminance(const float f)
{
	atmosphere->setLightPollutionLuminance(f);
}

//! Get light pollution luminance level
float LandscapeMgr::getAtmosphereLightPollutionLuminance() const
{
	return atmosphere->getLightPollutionLuminance();
}

void LandscapeMgr::setZRotation(const float d)
{
	if (landscape)
		landscape->setZRotation(d);
}

float LandscapeMgr::getLuminance() const
{
	return atmosphere->getRealDisplayIntensityFactor();
}

float LandscapeMgr::getAtmosphereAverageLuminance() const
{
	return atmosphere->getAverageLuminance();
}

// Override auto-computed luminance. Only use when you know what you are doing, and don't forget to unfreeze the average by calling this function with a negative value.
void LandscapeMgr::setAtmosphereAverageLuminance(const float overrideLum)
{
	atmosphere->setAverageLuminance(overrideLum);
}

Landscape* LandscapeMgr::createFromFile(const QString& landscapeFile, const QString& landscapeId)
{
	QSettings landscapeIni(landscapeFile, StelIniFormat);
	QString s;
	if (landscapeIni.status() != QSettings::NoError)
	{
		qWarning() << "ERROR parsing landscape.ini file: " << QDir::toNativeSeparators(landscapeFile);
		s = "";
	}
	else
		s = landscapeIni.value("landscape/type").toString();

	Landscape* landscape = Q_NULLPTR;
	if (s=="old_style")
		landscape = new LandscapeOldStyle();
	else if (s=="spherical")
		landscape = new LandscapeSpherical();
	else if (s=="fisheye")
		landscape = new LandscapeFisheye();
	else if (s=="polygonal")
		landscape = new LandscapePolygonal();
	else
	{
		qDebug() << "Unknown landscape type: \"" << s << "\"";

		// to avoid making this a fatal error, will load as a fisheye
		// if this fails, it just won't draw
		landscape = new LandscapeFisheye();
	}

	landscape->load(landscapeIni, landscapeId);
	return landscape;
}


QString LandscapeMgr::nameToID(const QString& name)
{
	QMap<QString,QString> nameToDirMap = getNameToDirMap();

	if (nameToDirMap.find(name)!=nameToDirMap.end())
	{
		Q_ASSERT(0);
		return "error";
	}
	else
	{
		return nameToDirMap[name];
	}
}

/****************************************************************************
 get a map of landscape names (from landscape.ini name field) to ID (dir name)
 ****************************************************************************/
QMap<QString,QString> LandscapeMgr::getNameToDirMap()
{
	QMap<QString,QString> result;
	const QSet<QString> landscapeDirs = StelFileMgr::listContents("landscapes",StelFileMgr::Directory);

	for (const auto& dir : landscapeDirs)
	{
		QString fName = StelFileMgr::findFile("landscapes/" + dir + "/landscape.ini");
		if (!fName.isEmpty())
		{
			QSettings landscapeIni(fName, StelIniFormat);
			QString k = landscapeIni.value("landscape/name").toString();
			result[k] = dir;
		}
	}
	return result;
}

QString LandscapeMgr::installLandscapeFromArchive(QString sourceFilePath, const bool display, const bool toMainDirectory)
{
	Q_UNUSED(toMainDirectory)
	if (!QFile::exists(sourceFilePath))
	{
		qDebug() << "LandscapeMgr: File does not exist:" << QDir::toNativeSeparators(sourceFilePath);
		emit errorUnableToOpen(sourceFilePath);
		return QString();
	}

	QDir parentDestinationDir;
	parentDestinationDir.setPath(StelFileMgr::getUserDir());

	if (!parentDestinationDir.exists("landscapes"))
	{
		//qDebug() << "LandscapeMgr: No 'landscapes' subdirectory exists in" << parentDestinationDir.absolutePath();
		if (!parentDestinationDir.mkdir("landscapes"))
		{
			qWarning() << "LandscapeMgr: Unable to install landscape: Unable to create sub-directory 'landscapes' in" << QDir::toNativeSeparators(parentDestinationDir.absolutePath());
			emit errorUnableToOpen(QDir::cleanPath(parentDestinationDir.filePath("landscapes")));//parentDestinationDir.absolutePath()
			return QString();
		}
	}
	QDir destinationDir (parentDestinationDir.absoluteFilePath("landscapes"));

	QZipReader reader(sourceFilePath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "LandscapeMgr: Unable to open as a ZIP archive:" << QDir::toNativeSeparators(sourceFilePath);
		emit errorNotArchive();
		return QString();
	}

	//Detect top directory
	QString topDir, iniPath;
	const auto infoList = reader.fileInfoList();
	for (const auto& info : infoList)
	{
		QFileInfo fileInfo(info.filePath);
		if (fileInfo.fileName() == "landscape.ini")
		{
			iniPath = info.filePath;
			topDir = fileInfo.dir().path();
			break;
		}
	}
	if (topDir.isEmpty())
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. There is no directory that contains a 'landscape.ini' file in the source archive.";
		emit errorNotArchive();
		return QString();
	}
	//Determine the landscape's identifier
	QString landscapeID = QFileInfo(topDir).fileName();
	if (landscapeID.length() < 2)
	{
		// If the archive has no top level directory
		// use the first 65 characters of its file name for an identifier
		QFileInfo sourceFileInfo(sourceFilePath);
		landscapeID = sourceFileInfo.baseName().left(65);
	}

	//Check for duplicate IDs
	if (getAllLandscapeIDs().contains(landscapeID))
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. A landscape with the ID" << landscapeID << "already exists.";
		emit errorNotUnique(landscapeID);
		return QString();
	}

	//Read the .ini file and check if the landscape name is unique
	QTemporaryFile tempLandscapeIni("landscapeXXXXXX.ini");
	if (tempLandscapeIni.open())
	{
		QByteArray iniData = reader.fileData(iniPath);
		tempLandscapeIni.write(iniData);
		tempLandscapeIni.close();
		QSettings confLandscapeIni(tempLandscapeIni.fileName(), StelIniFormat);
		QString landscapeName = confLandscapeIni.value("landscape/name").toString();
		if (getAllLandscapeNames().contains(landscapeName))
		{
			qWarning() << "LandscapeMgr: Unable to install landscape. There is already a landscape named" << landscapeName;
			emit errorNotUnique(landscapeName);
			return QString();
		}
	}

	//Copy the landscape directory to the target
	//This case already has been handled - and commented out - above. :)
	if(destinationDir.exists(landscapeID))
	{
		qWarning() << "LandscapeMgr: A subdirectory" << landscapeID << "already exists in" << QDir::toNativeSeparators(destinationDir.absolutePath()) << "Its contents may be overwritten.";
	}
	else if(!destinationDir.mkdir(landscapeID))
	{
		qWarning() << "LandscapeMgr: Unable to install landscape. Unable to create" << landscapeID << "directory in" << QDir::toNativeSeparators(destinationDir.absolutePath());
		emit errorUnableToOpen(QDir::cleanPath(destinationDir.filePath(landscapeID)));
		return QString();
	}
	destinationDir.cd(landscapeID);
	for (const auto& info : infoList)
	{
		QFileInfo fileInfo(info.filePath);
		if (info.isFile && fileInfo.dir().path() == topDir)
		{
			QByteArray data = reader.fileData(info.filePath);
			QFile out(destinationDir.filePath(fileInfo.fileName()));
			if (out.open(QIODevice::WriteOnly))
			{
				out.write(data);
				out.close();
			}
			else
			{
				qWarning() << "LandscapeMgr: cannot open " << QDir::toNativeSeparators(fileInfo.absoluteFilePath());
			}
		}
	}
	reader.close();
	//If necessary, make the new landscape the current landscape
	if (display)
	{
		setCurrentLandscapeID(landscapeID);
	}

	//Make sure that everyone knows that the list of available landscapes has changed
	emit landscapesChanged();

	qDebug() << "LandscapeMgr: Successfully installed landscape directory" << landscapeID << "to" << QDir::toNativeSeparators(destinationDir.absolutePath());
	return landscapeID;
}

bool LandscapeMgr::removeLandscape(const QString landscapeID)
{
	if (landscapeID.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! No landscape ID passed to removeLandscape().";
		return false;
	}

	if (packagedLandscapeIDs.contains(landscapeID))
	{
		qWarning() << "LandscapeMgr: Landscapes that are part of the default installation cannot be removed.";
		return false;
	}

	qDebug() << "LandscapeMgr: Trying to remove landscape" << landscapeID;

	QString landscapePath = getLandscapePath(landscapeID);
	if (landscapePath.isEmpty())
		return false;

	QDir landscapeDir(landscapePath);
	for (auto &fileName : landscapeDir.entryList(QDir::Files | QDir::NoDotAndDotDot))
	{
		if(!landscapeDir.remove(fileName))
		{
			qWarning() << "LandscapeMgr: Unable to remove" << QDir::toNativeSeparators(fileName);
			emit errorRemoveManually(landscapeDir.absolutePath());
			return false;
		}
	}
	landscapeDir.cdUp();
	if(!landscapeDir.rmdir(landscapeID))
	{
		qWarning() << "LandscapeMgr: Error! Landscape" << landscapeID
				   << "could not be removed. "
				   << "Some files were deleted, but not all."
				   << StelUtils::getEndLineChar()
				   << "LandscapeMgr: You can delete manually" << QDir::cleanPath(landscapeDir.filePath(landscapeID));
		emit errorRemoveManually(QDir::cleanPath(landscapeDir.filePath(landscapeID)));
		return false;
	}

	qDebug() << "LandscapeMgr: Successfully removed" << QDir::toNativeSeparators(landscapePath);

	//If the landscape has been selected, revert to the default one
	//TODO: Make this optional?
	if (getCurrentLandscapeID() == landscapeID)
	{
		if(getDefaultLandscapeID() == landscapeID)
		{
			setDefaultLandscapeID(packagedLandscapeIDs.first());
			//TODO: Find what happens if a missing landscape is specified in the configuration file
		}

		setCurrentLandscapeID(getDefaultLandscapeID());
	}

	//Make sure that everyone knows that the list of available landscapes has changed
	emit landscapesChanged();

	return true;
}

QString LandscapeMgr::getLandscapePath(const QString landscapeID)
{
	QString result;
	//Is this necessary? This function is private.
	if (landscapeID.isEmpty())
		return result;

	result = StelFileMgr::findFile("landscapes/" + landscapeID, StelFileMgr::Directory);
	if (result.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! Unable to find" << landscapeID;
		return result;
	}

	return result;
}

QString LandscapeMgr::loadLandscapeName(const QString landscapeID)
{
	QString landscapeName;
	if (landscapeID.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! No landscape ID passed to loadLandscapeName().";
		return landscapeName;
	}

	QString landscapePath = getLandscapePath(landscapeID);
	if (landscapePath.isEmpty())
		return landscapeName;

	QDir landscapeDir(landscapePath);
	if (landscapeDir.exists("landscape.ini"))
	{
		QString landscapeSettingsPath = landscapeDir.filePath("landscape.ini");
		QSettings landscapeSettings(landscapeSettingsPath, StelIniFormat);
		landscapeName = landscapeSettings.value("landscape/name").toString();
	}
	else
	{
		qWarning() << "LandscapeMgr: Error! Landscape directory" << QDir::toNativeSeparators(landscapePath) << "does not contain a 'landscape.ini' file";
	}

	return landscapeName;
}

quint64 LandscapeMgr::loadLandscapeSize(const QString landscapeID) const
{
	quint64 landscapeSize = 0;
	if (landscapeID.isEmpty())
	{
		qWarning() << "LandscapeMgr: Error! No landscape ID passed to loadLandscapeSize().";
		return landscapeSize;
	}

	QString landscapePath = getLandscapePath(landscapeID);
	if (landscapePath.isEmpty())
		return landscapeSize;

	const QDir landscapeDir(landscapePath);
	for (auto &file : landscapeDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
	{
		//qDebug() << "name:" << file.baseName() << "size:" << file.size();
		landscapeSize += static_cast<quint64>(file.size());
	}

	return landscapeSize;
}

QString LandscapeMgr::getDescription() const
{
	QString lang, desc, descFile, locDescriptionFile, engDescriptionFile;
	bool hasFile = true;

	lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	locDescriptionFile = StelFileMgr::findFile("landscapes/" + getCurrentLandscapeID(), StelFileMgr::Directory) + "/description." + lang + ".utf8";
	engDescriptionFile = StelFileMgr::findFile("landscapes/" + getCurrentLandscapeID(), StelFileMgr::Directory) + "/description.en.utf8";

	// OK. Check the file with full name of locale
	if (!QFileInfo::exists(locDescriptionFile))
	{
		// Oops...  File not exists! What about short name of locale?
		lang = lang.split("_").at(0);
		locDescriptionFile = StelFileMgr::findFile("landscapes/" + getCurrentLandscapeID(), StelFileMgr::Directory) + "/description." + lang + ".utf8";
	}

	// Check localized description for landscape
	if (!locDescriptionFile.isEmpty() && QFileInfo::exists(locDescriptionFile))
	{		
		descFile = locDescriptionFile;
	}
	// OK. Localized description of landscape not exists. What about english description of its?
	else if (!engDescriptionFile.isEmpty() && QFileInfo::exists(engDescriptionFile))
	{
		descFile = engDescriptionFile;
	}
	// That file not exists too? OK. Will be used description from landscape.ini file.
	else
	{
		hasFile = false;
	}

	if (hasFile)
	{
		QFile file(descFile);
		if(file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			in.setEncoding(QStringConverter::Utf8);
#else
			in.setCodec("UTF-8");
#endif
			desc = in.readAll();
			file.close();
		}
	}
	else
	{
		desc = QString("<h2>%1</h2>").arg(q_(landscape->getName()));
		desc += landscape->getDescription();
	}

	return desc;
}

void LandscapeMgr::increaseLightPollution()
{
	StelCore* core = StelApp::getInstance().getCore();
	const auto lum = core->getSkyDrawer()->getLightPollutionLuminance();
	auto bidx = core->luminanceToBortleScaleIndex(lum) + 1;
	if (bidx>9)
		bidx = 9;
	const auto newLum = core->bortleScaleIndexToLuminance(bidx);
	core->getSkyDrawer()->setLightPollutionLuminance(newLum);
}

void LandscapeMgr::reduceLightPollution()
{
	StelCore* core = StelApp::getInstance().getCore();
	const auto lum = core->getSkyDrawer()->getLightPollutionLuminance();
	auto bidx = core->luminanceToBortleScaleIndex(lum) - 1;
	if (bidx<1)
		bidx = 1;
	const auto newLum = core->bortleScaleIndexToLuminance(bidx);
	core->getSkyDrawer()->setLightPollutionLuminance(newLum);
}

void LandscapeMgr::cyclicChangeLightPollution()
{
	StelCore* core = StelApp::getInstance().getCore();
	const auto lum = core->getSkyDrawer()->getLightPollutionLuminance();
	auto bidx = core->luminanceToBortleScaleIndex(lum) + 1;
	if (bidx>9)
		bidx = 1;
	const auto newLum = core->bortleScaleIndexToLuminance(bidx);
	core->getSkyDrawer()->setLightPollutionLuminance(newLum);
}

/*
// GZ: Addition to identify landscape transparency. Used for development and debugging only, should be commented out in release builds.
// Also, StelMovementMgr l.382 event->accept() must be commented out for this here to work!
void LandscapeMgr::handleMouseClicks(QMouseEvent *event)
{
	switch (event->button())
	{
	case Qt::LeftButton :
		if (event->type()==QEvent::MouseButtonRelease)
		{
			Vec3d v;
			StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff)->unProject(event->x(),event->y(),v);
			v.normalize();
			float trans=landscape->getOpacity(v);
			qDebug() << "Landscape opacity at screen X=" << event->x() << ", Y=" << event->y() << ": " << trans;
		}
		break;
	default: break;

	}
	// do not event->accept(), so that it is forwarded to other modules.
	return;
}
*/

void LandscapeMgr::showMessage(const QString& message)
{
	messageToShow=message;
	messageFader=true;
	messageTimer->start();
}

void LandscapeMgr::clearMessage()
{
	messageFader = false;
}
