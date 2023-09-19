/*
 * Navigational Stars plug-in
 * Copyright (C) 2014-2016 Alexander Wolf
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

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelFileMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelUtils.hpp"
#include "NavStars.hpp"
#include "NavStarsWindow.hpp"
#include "NavStarsCalculator.hpp"
#include "Planet.hpp"

#include <QList>
#include <QSharedPointer>
#include <QMetaEnum>

#include "planetsephems/sidereal_time.h"

StelModule* NavStarsStelPluginInterface::getStelModule() const
{
	return new NavStars();
}

StelPluginInfo NavStarsStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(NavStars);
 
	StelPluginInfo info;
	info.id = "NavStars";
	info.displayedName = N_("Navigational Stars");
	info.authors = "Alexander Wolf, Andy Kirkham";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plugin marks navigational stars from a selected set.");
	info.version = NAVSTARS_PLUGIN_VERSION;
	info.license = NAVSTARS_PLUGIN_LICENSE;
	return info;
}

NavStars::NavStars()
	: currentNSSet(AngloAmerican)
	, enableAtStartup(false)	
	, starLabelsState(true)	
	, upperLimb(false)
	, highlightWhenVisible(false)
	, limitInfoToNavStars(false)	
	, tabulatedDisplay(false)
	, useUTCTime(false)
	, toolbarButton(Q_NULLPTR)
{
	setObjectName("NavStars");
	conf = StelApp::getInstance().getSettings();
	propMgr = StelApp::getInstance().getStelPropertyManager();
	mainWindow = new NavStarsWindow();
}

NavStars::~NavStars()
{
	delete mainWindow;
}

double NavStars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance()
		        .getModuleMgr()
		        .getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}

void NavStars::init()
{
	if (!conf->childGroups().contains("NavigationalStars"))
	{
		qDebug() << "[NavStars] no coordinates section exists in main config file - creating with defaults";
		restoreDefaultConfiguration();
	}
	// save default state for star labels and time zone
	starLabelsState = propMgr->getStelPropertyValue("StarMgr.flagLabelsDisplayed").toBool();
	timeZone = propMgr->getStelPropertyValue("StelCore.currentTimeZone").toString();

	// populate settings from main config file.
	loadConfiguration();

	// populate list of navigational stars
	populateNavigationalStarsSet();

	setNavStarsMarks(getEnableAtStartup());

	// Marker texture - using the same texture as the planet hints.
	QString path = StelFileMgr::findFile("textures/planet-indicator.png");
	markerTexture = StelApp::getInstance().getTextureManager().createTexture(path);

	// key bindings and other actions
	addAction("actionShow_NavStars",        N_("Navigational Stars"), N_("Mark the navigational stars"), "navStarsVisible");
	addAction("actionShow_NavStars_dialog", N_("Navigational Stars"), N_("Show settings dialog"),        mainWindow, "visible");

	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveSettings()));
	connect(&StelApp::getInstance(), SIGNAL(flagShowDecimalDegreesChanged(bool)), this, SLOT(setUseDecimalDegrees(bool)));
	setUseDecimalDegrees(StelApp::getInstance().getFlagShowDecimalDegrees());

	// Toolbar button
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=Q_NULLPTR)
	{
		if (toolbarButton == Q_NULLPTR)
		{
			// Create the nav. stars button
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/NavStars/btNavStars-on.png"),
						       QPixmap(":/NavStars/btNavStars-off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_NavStars",
						       false,
						       "actionShow_NavStars_dialog");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}
}

void NavStars::deinit()
{
	if (getFlagUseUTCTime())
		propMgr->setStelPropertyValue("StelCore.currentTimeZone", timeZone);
	markerTexture.clear();
	stars.clear();
	starNumbers.clear();
}

bool NavStars::configureGui(bool show)
{
	if (show)
	{
		mainWindow->setVisible(true);
	}

	return true;
}

void NavStars::draw(StelCore* core)
{
	// Drawing is enabled?
	if (markerFader.getInterstate() <= 0.0f)
	{
		return;
	}
	
	QList<int> sn = getStarsNumbers();

	if (stars.isEmpty())
	{
		StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
		stars.fill(StelObjectP(), sn.size());
		for (int i = 0; i < sn.size(); ++i)
		{
			QString name = QString("HIP %1").arg(sn.at(i));
			stars[i] = omgr->searchByName(name);
		}
	}

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	float mlimit = core->getSkyDrawer()->getLimitMagnitude();

	Vec3d pos;
	for (int i = 0; i < sn.size(); ++i)
	{
		if (stars[i].isNull())
			continue;
		
		// Don't show if magnitude too low for visibility.
		if (highlightWhenVisible && stars[i]->getVMagnitude(core) > mlimit)
			continue;

		// Get the current position of the navigational star...
		if (prj->projectCheck(stars[i]->getJ2000EquatorialPos(core), pos))
		{
			// ... and draw a marker around it
			if (!markerTexture.isNull())
			{
				painter.setBlending(true);
				painter.setColor(markerColor, markerFader.getInterstate());
				markerTexture->bind();
				painter.drawSprite2dMode(static_cast<float>(pos[0]), static_cast<float>(pos[1]), 11.f);
			}

			// Draw the localized name of the star and its ordinal number
			QString label = stars[i]->getNameI18n();
			if (label.isEmpty())
				label = QString("%1").arg(i+1);
			else
				label = QString("%1 (%2)").arg(label).arg(i+1);
			painter.drawText(static_cast<float>(pos[0]), static_cast<float>(pos[1]), label, 0, 15.f, 15.f, false);
		}
	}

	addExtraInfo(core);
}

void NavStars::update(double deltaTime)
{
	markerFader.update(static_cast<int>(deltaTime*1000));
}

void NavStars::setNavStarsMarks(const bool b)
{
	if (b==getNavStarsMarks())
		return;

	propMgr->setStelPropertyValue("StarMgr.flagLabelsDisplayed", b ? !b : starLabelsState);

	if (getFlagUseUTCTime())
		propMgr->setStelPropertyValue("StelCore.currentTimeZone", b ? "UTC" : timeZone);

	markerFader = b;
	emit navStarsMarksChanged(b);
}

bool NavStars::getNavStarsMarks() const
{
	return markerFader;
}

void NavStars::setEnableAtStartup(bool b)
{
	if (b!=getEnableAtStartup())
	{
		enableAtStartup=b;
		emit enableAtStartupChanged(b);
	}
}

void NavStars::setHighlightWhenVisible(bool b)
{
	if (b!=getHighlightWhenVisible())
	{
		highlightWhenVisible=b;
		emit highlightWhenVisibleChanged(b);
	}
}

void NavStars::setLimitInfoToNavStars(bool b)
{
	if (b!=getLimitInfoToNavStars())
	{
		limitInfoToNavStars=b;
		emit limitInfoToNavStarsChanged(b);
	}
}

void NavStars::setUpperLimb(bool b)
{
	if (b!=getUpperLimb())
	{
		upperLimb=b;
		emit upperLimbChanged(b);
	}
}

void NavStars::setTabulatedDisplay(bool b)
{
	if (b!=getTabulatedDisplay())
	{
		tabulatedDisplay=b;
		emit tabulatedDisplayChanged(b);
	}
}

void NavStars::setFlagUseUTCTime(bool b)
{
	if (b!=getFlagUseUTCTime())
	{
		useUTCTime=b;
		emit flagUseUTCTimeChanged(b);

		if (getNavStarsMarks())
			propMgr->setStelPropertyValue("StelCore.currentTimeZone", b ? "UTC" : timeZone);
	}
}

void NavStars::setShowExtraDecimals(bool b)
{
	if (b!=getShowExtraDecimals())
	{
		NavStarsCalculator::useExtraDecimals=b;
		emit showExtraDecimalsChanged(b);
	}
}

void NavStars::setUseDecimalDegrees(bool flag)
{
	NavStarsCalculator::useDecimalDegrees = flag;
}

void NavStars::restoreDefaultConfiguration(void)
{
	// Remove the whole section from the configuration file
	conf->remove("NavigationalStars");
	// Load the default values...
	loadConfiguration();
	// ... then save them.
	saveConfiguration();
}

void NavStars::loadConfiguration(void)
{
	conf->beginGroup("NavigationalStars");

	setCurrentNavigationalStarsSetKey(conf->value("current_ns_set", "AngloAmerican").toString());
	markerColor = Vec3f(conf->value("marker_color", "0.8,0.0,0.0").toString());
	enableAtStartup = conf->value("enable_at_startup", false).toBool();
	highlightWhenVisible = conf->value("highlight_when_visible", false).toBool();
	limitInfoToNavStars  = conf->value("limit_info_to_nav_stars", false).toBool();
	tabulatedDisplay = conf->value("tabulated_display", false).toBool();
	upperLimb = conf->value("upper_limb", false).toBool();
	setShowExtraDecimals(conf->value("extra_decimals", false).toBool());
	useUTCTime = conf->value("use_utc_time", false).toBool();

	conf->endGroup();
}

void NavStars::saveConfiguration(void)
{
	conf->beginGroup("NavigationalStars");

	conf->setValue("current_ns_set", getCurrentNavigationalStarsSetKey());
	conf->setValue("marker_color", markerColor.toStr());
	conf->setValue("enable_at_startup", getEnableAtStartup());
	conf->setValue("highlight_when_visible", getHighlightWhenVisible());
	conf->setValue("limit_info_to_nav_stars", getLimitInfoToNavStars());
	conf->setValue("tabulated_display", getTabulatedDisplay());
	conf->setValue("upper_limb", getUpperLimb());
	conf->setValue("extra_decimals", getShowExtraDecimals());
	conf->setValue("use_utc_time", getFlagUseUTCTime());

	conf->endGroup();
}

void NavStars::setCurrentNavigationalStarsSetKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("NavigationalStarsSet"));
	NavigationalStarsSet nsSet = static_cast<NavigationalStarsSet>(en.keyToValue(key.toLatin1().data()));
	if (nsSet<0)
	{
		qWarning() << "Unknown navigational stars set:" << key << "setting \"AngloAmerican\" instead";
		nsSet = AngloAmerican;
	}
	setCurrentNavigationalStarsSet(nsSet);
	populateNavigationalStarsSet();
}

QString NavStars::getCurrentNavigationalStarsSetKey() const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("NavigationalStarsSet")).key(currentNSSet);
}

QString NavStars::getCurrentNavigationalStarsSetDescription() const
{
	static const QMap<NavigationalStarsSet, QString> description = {
		// TRANSLATORS: The emphasis tags mark a title.
		{ AngloAmerican, N_("The 57 \"selected stars\" that are listed in <em>The Nautical Almanac</em> jointly published by Her Majesty's Nautical Almanac Office and the US Naval Observatory since 1958; consequently, these stars are also used in navigational aids such as the <em>2102D Star Finder</em> and <em>Identifier</em>.") },
		{        French, N_("The 81 stars that are listed in the French Nautical Almanac published by the French Bureau des Longitudes.") },
		{        German, N_("The 80 stars that are listed in the German Nautical Almanac published by the Federal Maritime and Hydrographic Agency of Germany.") },
		{       Russian, N_("The 160 stars that are listed in the Russian Nautical Almanac.") },
		// TRANSLATORS: The emphasis tags mark a title.
		{      USSRAvia, N_("The typical set of navigational stars which was used by aviation of the Soviet Union. These stars can be found in books like <em>Aviation Astronomy</em> or <em>Aviation handbook</em>.") },
		{     USSRSpace, N_("These 151 stars were used in the Voskhod (Soviet) and Soyuz (Soviet and Russian) manned space programs to navigate in space.") },
		{        Apollo, N_("These 37 stars were used by the Apollo space program to navigate to the Moon from 1969-1972, Apollo 11 through Apollo 17.") },
		{     GeminiAPS, N_("Alignment stars from the Gemini Astronomical Positioning System, a professional level computerized device for controlling small to medium German equatorial telescope mounts.") },
		{    MeadeLX200, N_("The Meade LX200 utilizes 33 bright and well known stars to calibrate the telescope’s Object Library in the ALTAZ and POLAR alignments. These stars were selected to allow observers from anywhere in the world on any given night, to be able to easily and quickly make precision alignments.") },
		{      MeadeETX, N_("This list from Meade ETX mount will aid the observer to find alignment stars at various times of the year.") },
		{    MeadeAS494, N_("Alignment stars for the Meade Autostar #494 handset (ETX60AT).") },
		{    MeadeAS497, N_("Alignment stars for the Meade Autostar #497 handset.") },
		{   CelestronNS, N_("Even though there are about 250 named stars in the hand control database, only 82 (stars brighter than or equal to magnitude 2.5) can be used for alignment and related tasks.") },
		{  SkywatcherSS, N_("Alignment stars for the Skywatcher SynScan hand controller and SynScan Pro App.") },
		{       VixenSB, N_("Alignment stars for Vixen Starbook mounts.") },
		{     ArgoNavis, N_("Alignment stars for Argo Navis digital setting circles.") },
		{       OrionIS, N_("Alignment stars for Orion Intelliscope mounts.") },
		{  SkyCommander, N_("Alignment stars for Sky Commander digital setting circles.")}
	};

	// Original titles of almanacs
	static const QMap<NavigationalStarsSet, QString> almanacTitle = {
		{        French, "Ephémérides Nautiques" },
		{       Russian, "Морской астрономический ежегодник" },
		{        German, "Nautisches Jahrbuch" }
	};

	NavigationalStarsSet nsSet = getCurrentNavigationalStarsSet();

	QString almanacText     = almanacTitle.value(nsSet, QString());
	QString descriptionText = description.value(nsSet, QString());
	if (almanacText.isEmpty())
		return q_(descriptionText);
	else
	{
		// TRANSLATORS: The full phrase: The original almanac title is NAME_OF_THE_ALMANAC
		return QString("%1 %2 <em>%3</em>.").arg(q_(descriptionText), q_("The original almanac title is"), almanacText);
	}
}

void NavStars::populateNavigationalStarsSet(void)
{
	bool currentState = getNavStarsMarks();

	setNavStarsMarks(false);
	stars.clear();
	starNumbers.clear();

	// List of HIP numbers of the navigational stars:
	switch(getCurrentNavigationalStarsSet())
	{
		case AngloAmerican:
		{
			// 57 "selected stars" from The Nautical Almanac.
			starNumbers = {
				   677,   2081,   3179,   3419,   7588,   9884,  13847,	 14135,
				 15863,  21421,  24436,  24608,  25336,  25428,	 26311,  27989,
				 30438,  32349,  33579,  37279,  37826,	 41037,  44816,  45238,
				 46390,  49669,  54061,  57632,	 59803,  60718,  61084,  62956,
				 65474,  67301,  68702,	 68933,  69673,  71683,  72622,  72607,
				 76267,  80763,	 82273,  84012,  85927,  86032,  87833,  90185,
				 91262,	 92855,  97649, 100751, 102098, 107315, 109268, 113368,
				113963
			};
			break;
		}
		case French:
		{
			// 81 stars from French Nautical Almanac
			// Original French name: Ephémérides Nautiques
			starNumbers = {
				   677,    746,   1067,   2081,   3179,   3419,   4427,	  5447,
				  7588,   9884,  11767,  14135,  14576,  15863,	 21421,  24436,
				 24608,  25336,  25428,  26311,  26727,	 27989,  28360,  30324,
				 30438,  31681,  32349,  33579,	 34444,  36850,  37279,  37826,
				 39429,  39953,  41037,	 42913,  44816,  45238,  45556,  46390,
				 49669,  53910,	 54061,  54872,  57632,  58001,  59803,  60718,
				 61084,	 61932,  62434,  62956,  65378,  65474,  67301,  68702,
				 68933,  69673,  71683,  72607,  76267,  80763,  82273,	 82396,
				 85927,  86032,  87833,  90185,  91262,  92855,	 97649, 100453,
				100751, 102098, 105199, 107315, 109268,	112122, 113368, 113881,
				113963
			};
			break;
		}
		case Russian:
		{
			// 160 stars from Russian Nautical Almanac
			// Original Russian name: Морской астрономический ежегодник.
			starNumbers = {
				   677,    746,   1067,   2021,   2081,   3179,   3419,	  4427,
				  5447,   6686,   7588,   8886,   8903,   9236,	  9640,   9884,
				 13847,  14135,  14576,  15863,  17702,	 18246,  18532,  21421,
				 23015,  23875,  24436,  24608,	 25336,  25428,  25606,  25930,
				 25985,  26241,  26311,	 26451,  26634,  26727,  27913,  27989,
				 28360,  28380,	 30324,  30438,  31681,  32349,  32768,  33579,
				 33152,	 34444,  35264,  35904,  36188,  36850,  37279,  37826,
				 39429,  39757,  39953,  41037,  42913,  44816,  45238,	 45556,
				 46390,  46701,  49669,  50583,  52419,  52727,	 53910,  54061,
				 54872,  57632,  58001,  59196,  59747,	 59774,  59803,  60718,
				 61084,  61359,  61585,  61932,	 61941,  62434,  62956,  63121,
				 63608,  65109,  65378,	 65474,  66657,  67301,  67927,  68002,
				 68702,  68933,	 69673,  71075,  71352,  71681,  71860,  72105,
				 72622,	 72607,  73273,  74946,  74785,  76297,  76267,  77070,
				 78401,  78820,  79593,  80331,  80763,  80816,  81266,	 81377,
				 81693,  82273,  82396,  83081,  84012,  85258,	 85792,  85670,
				 85927,  86032,  86228,  79540,  86742,	 87833,  88635,  89931,
				 90185,  90496,  91262,  95347,	 93506,  93747,  94141,  97165,
				 97278,  97649, 100453,	100751, 102098, 102488, 105199, 107315,
				107556, 109268,	110130, 112122, 113368, 113881, 113963,  11767
			};
			break;
		}
		case USSRAvia:
		{
			// The typical set of Soviet aviation navigational stars
			starNumbers = {
				   677,  11767,  62956,  49669, 102098, 113368,  80763,	 37826,
				 65474,  27989,  21421,  97649,  37279,  24436,  69673,  24608,
				 91262,  32349,  30438,   7588,  71683,  62434,  82273,  90185,
				100751,   9884
			};
			break;
		}
		case USSRSpace:
		{
			// The set of navigational stars from Soviet and Russian manned space programs
			starNumbers = {
				   677,    746,   1067,   2021,   2081,   3179,   3419,   4427,
				  5447,   7588,   8903,   6686,  11767,   9640,   9884,  10826,
				 14135,  14576,  15863,  17702,  18246,  18532,  21421,  23015,
				 23875,  24436,  24608,  25336,  25428,  25606,  25930,  25985,
				 26241,  26311,  26451,  26634,  26727,  27366,  27989,  28360,
				 28380,  30324,  30438,  31681,  32349,  32768,  33579,  34444,
				 35264,  35904,  36850,  37279,  37826,  39429,  39757,  39953,
				 41037,  42913,  44816,  45238,  45941,  45556,  46390,  49669,
				 50583,  52727,  53910,  54061,  54872,  57632,  57757,  58001,
				 59196,  59774,  59803,  60718,  61084,  61359,  61585,  61932,
				 62434,  62956,  63125,  63608,  65109,  65378,  65474,  66657,
				 67301,  67927,  68702,  68756,  68933,  69673,  71075,  71352,
				 71683,  71860,  72105,  72622,  73273,  72607,  74785,  76297,
				 76267,  77070,  78265,  78401,  78820,  80112,  80331,  80763,
				 80816,  81266,  81693,  82273,  82396,  84012,  84345,  85258,
				 85792,  85670,  85927,  86032,  86228,  86670,  86742,  87833,
				 89931,  90185,  90496,  91262,  92855,  93506,  95947,  97165,
				 97278,  97649, 100453, 100751, 102098, 102488, 105199, 107315,
				107556, 109268, 110130, 112122, 113368, 113881, 113963
			};
			break;
		}
		case German:
		{
			// 80 stars from German Nautical Almanac
			// Original German name: Nautisches Jahrbuch
			// The numbers are identical to the "Nautisches Jahrbuch"
			starNumbers = {
				   677,   1067,   2081,   3179,   3419,   4427,	  5447,   7588,
				 11767,   9640,   9884,  14135,	 14576,  15863,  17702,  21421,
				 24436,  24608,	 25336,  25428,  26311,  26727,  27366,  27989,
				 28360,  30324,  30438,  31681,  32349,  33579,	 34444,  36850,
				 37279,  37826,  41037,  44816,	 45238,  46390,  49669,  52419,
				 54061,  57632,	 60718,  61084,  62434,  62956,  63608,  65378,
				 65474,  67301,  68702,  68933,  69673,  71683,	 72105,  72622,
				 72607,  74785,  76267,  77070,  80763,  82273,  82396,  85927,
				 86032,  86228,	 87833,  90185,  91262,  92855,  97649, 100751,
				102098, 105199, 107315, 109268, 112122, 113368, 113881, 113963
			};
			break;
		}
		case GeminiAPS:
		{
			// Gemini Alignment Star List
			// Source: Gemini Users Manual, Level 4, René Görlich et. al., April 2006
			starNumbers = {
				   677,   1067,   3419,   5447,   7588,   9884,	 11767,	 14135,
				 15863,  18543,  21421,  24436,	 24608,  27989,	 30438,  32349,
				 36850,  37279,	 37826,  46390,  49669,	 54061,  54872,  57632,
				 59803,  60718,  63608,  65378,	 65474,  68702,	 69673,  71683,
				 72603,  77070,  80763,	 80816,	 85927,  86032,  90185,  91262,
				 92855,  95947,	 97649, 102098, 106278, 107315, 109074, 113368,
				113963
			};
			break;
		}
		case MeadeLX200:
		{
			// Meade LX200 Alignment Star Library
			// Source:
			//    Meade Instruction Manual
			//    7" LX200 Maksutov-Cassegrain Telescope
			//    8", 10", and 12" LX200 Schmidt-Cassegrain Telescopes
			// HIP 28380 = Bogardus
			starNumbers = {
				  7588,  60718,  95947,  67301,  21421,  26311,	 46390,	 76267,
				 97649,  80763,  69673,  27989,	 28380,  30438,	 24608,  36850,
				102098,  57632,	  3419, 107315, 113368,	 68702,   9884, 113963,
				 10826,  11767,  37826,  37279,	 49669,  24436,	 32349,  65474,
				 91262
			};
			break;
		}
		case MeadeETX:
		{
			// Meade ETX Alignment Star Library
			// Source:
			//    Meade Instruction Manual
			//    ETX-90EC Astro Telescope
			//    ETX-105EC Astro Telescope
			//    ETX-125EC Astro Telescope
			starNumbers = {
				 69673,  49669,  65474,  91262, 102098,  97649,  80763,	113963,
				113368,  10826,  24436,  27989,  32349,  21421
			};
			break;
		}
		case MeadeAS494:
		{
			// Meade Autostar #494 Alignment Stars
			// Source:
			//    http://www.weasner.com/etx/autostar/as_494align_stars.html
			starNumbers = {
				 13847,   7588,  60718,  33579,  95947,  65477,  17702,  21421,
				105199,   1067,  50583,  14576,  31681,  62956,	 67301,   9640,
				109268,  25428,  26311,  26727,  46390,	 76267,    677,  98036,
				 97649,   2081,  80763,  69673,	 25985,  25336,  27989,  30438,
				 24608,  36850,  63125,	102098,  57632,   3419,  54061, 107315,
				 87833, 113368,	 68702,   9884,  72105,  90185,  72607, 113963,
				 59774,	 13954,  53910,  25930,  10826,   5447,  15863,  65378,
				 25606,  92855,  58001,  37826,  37279,  84345,  86032,	 49669,
				 24436, 109074,  27366, 113881,  85927,   3179,	 32349,  65474,
				 97278,  68756,  77070,  91262,  63608
			};
			break;
		}
		case MeadeAS497:
		{
			// Meade Autostar #497 Alignment Stars
			// Source:
			//    http://www.weasner.com/etx/autostar/as_497align_stars.html
			starNumbers = {
				 13847,   7588,  60718,  33579,  95947,  65477,  17702,	 21421,
				105199,   1067,  50583,  14576,  31681,  62956,  67301,   9640,
				109268,  25428,  26311,  26727,  46390,  76267,    677,  98036,
				 97649,   2081,  80763,  69673,  25985,  25336,  27989,  30438,
				 24608,  36850,  63125, 102098,  57632,   3419,  54061, 107315,
				 87833, 113368,  68702,   9884,  72105,  90185,  72607, 113963,
				 59774,  13954,  53910,  25930,  10826,   5447,  15863,  65378,
				 25606,  92855,  58001,  11767,  37826,  37279,  84345,  86032,
				 49669,  24436, 109074,  27366, 113881,  85927,   3179,  32349,
				 65474,  97278,  68756,  77070,  91262,  63608
			};
			break;
		}
		case CelestronNS:
		{
			// Celestron NexStar Alignment Star List
			// Source: https://www.celestron.com/blogs/knowledgebase/can-all-named-stars-listed-in-the-hand-control-be-used-for-alignment
			starNumbers = {
				 32349,  30438,  69673,  91262,  24608,  24436,  37279,   7588,
				 27989,  68702,  97649,  21421,  65474,  80763,	 37826, 113368,
				 60718,  62434,  71683, 102098,  49669,	 33579,  25336,  61084,
				 85927,  26311,  25428,  45238,	109268,  15863,  34444,  39953,
				 54061,  62956,  28360,	 31681,  41037,  67301,  82273,  86228,
				 90185, 100751,	  3419,   9884,  11767,  30324,  36850,  46390,
				   677,	  5447,  14576,  26727,  27366,  57632,  72607,  68933,
				 86032,  92855,   3179,  25930,  44816,  61932,  76267,	 87833,
				100453,    746,   9640,  66811,  80404,  65378,	 78401,   2081,
				 53910,  58001,  84012, 105199, 107315,	113881,   4427,  14135,
				 35904, 113963
			};
			break;
		}
		case Apollo:
		{
			// Apollo Alignment Star List (37 stars)
			// Sources: https://history.nasa.gov/alsj/alsj-AOTNavStarsDetents.html
			//          https://www.spaceartifactsarchive.com/2013/05/the-star-chart-of-apollo.html
			starNumbers = {
				   677,   3419,   4427,   7588,  11767,  13847,  13954,      0,
				     0,  15863,  21421,  24436,  24608,  30438,  32349,  37279,
				 39953,      0,      0,  44127,  46390,  49669,  57632,  59803,
				 60718,  65474,  67301,      0,      0,  68933,  69673,  76267,
				 80763,  82273,  86032,	 91262,  92855,      0,      0,  97649,
				100345, 100751,	102098, 107315, 113368
			};
			break;
		}
		case SkywatcherSS:
		{
			// Skywatcher SynScan Hand Held Controller and SynScan Pro mobile and desktop App
			// Sources: https://www.iceinspace.com.au/63-501-0-0-1-0.html
			//          https://www.iceinspace.com.au/download.php?10d72300ac5d3762f0529eb11977fb68
			starNumbers  = { 
				 13847,  33579,  95947,  67301,  17702,  21421, 105199,	 15863,
				 50583,  14576,  31681,  62956,  62956,   9640,	 25428,  26311,
				 26727,  81266,  46390,  76267,    677,	 97649,  35904,   2081,
				 80763,  69673,  25985,  23015,  25336,  27989,  28380,  24608,
				   746,  36850, 102098,	 57632,   3419,  78401,  54061, 107315,
				 87833,  81377,	113368,  78159, 102488,  36188,   9884,  23015,
				 72105,	 90185,  72607,   4427, 113963,  32246,  59774,  28360,
				 14135,  53910,  59316,  25930,  10826,   5447,  15863,	 65378,
				 30324,  39429,  92855,  58001,  11767,  37826,	 37279,  28437,
				 84345,  47908,  86032,  49669,  24436,	109074,  27366,  86228,
				113881,   8903,  85927,   3179,	 32349,  65474,  97278,  77070,
				 91262,  34444,  79593
			};
			break;
		}
		case VixenSB:
		{
			// Vixen Starbook mounts
			// Source: https://www.skysafariastronomy.com/repositories/skylist/Alignment%20Star%20Lists/Vixen%20Starbook%20Alignment%20Stars.skylist
			starNumbers  = {
				  7588,  60718,  95947,  21421, 100027, 109268,  46390,    677,
				 97649,  80763,  69673, 100345,  27989,  30438,  24608, 102098,
				 57632,   3419,  54061, 113368,   9884, 113963,   8832,  10826,
				 15863,  65378,  92855,  11767,  37826,  37279,  84345,  86032,
				 49669,  71683,   3179,  65474,  44816,  91262
			};
			break;
		}
		case ArgoNavis:
		{
			// Argo Navis digital setting circles
			// Source: https://www.skysafariastronomy.com/repositories/skylist/Alignment%20Star%20Lists/Argo%20Alignment%20Stars.skylist
			starNumbers  = {
				   677,  97649,  24608,  69673,  32349,  37279,  30438,   4427,
				 68702,  60718,  62434,  95947,   7588,  36850,  37826, 109268,
				 46390,  49669,  57632,  91262,  86032,  27989,  24436,  15863,
				113368,  80763,  21421,  54061,  65378,  11767,  44816,  65474,
				102098,  90185,  71683
			};
			break;
		}
		case OrionIS:
		{
			// Orion Intelliscope mounts
			// Source: https://www.skysafariastronomy.com/repositories/skylist/Alignment%20Star%20Lists/Orion%20Intelliscope%20Alignment%20Stars.skylist
			starNumbers  = {
				 95947,  21421,  46390,    677,  97649,  80763,  69673,  27989,
				 24608,  36850, 102098,  57632, 113368,  15863,  65378,  11767,
				 37279,  86032,  49669,  24436,  32349,  65474,  91262
			};
			break;
		}
		case SkyCommander:
		{
			// Sky Commander DSCs
			// Source: https://www.skysafariastronomy.com/repositories/skylist/Alignment%20Star%20Lists/Sky%20Commander%20Alignment%20Stars.skylist
			starNumbers  = {
				 11767,   3179,   3419,   5447,   9884,  14576,  15863,  18543,
				 21421,  24436,  24608,  27989,  32349,  37279,  37826,  44127,
				 46390,  49669,  50372,  54872,  58001,  63125,  65474,  67301,
				 69673,  73555,  74785,  80763,  81693,  86032,  91262,  97649,
				100453, 102098, 107315, 112158, 113368
			};
			break;
		}
	}

	setNavStarsMarks(currentState);
}

void NavStars::addExtraInfo(StelCore *core)
{
	if (0 == stars.size() || "Earth" != core->getCurrentPlanet()->getEnglishName()) 
		return;

	StelApp& stelApp = StelApp::getInstance();
	bool isSource = stelApp.getStelObjectMgr().getWasSelected();

	if (isSource) 
	{
		bool doExtraInfo = true;
		StelObjectP selectedObject = stelApp.getStelObjectMgr().getSelectedObject()[0];
		if (limitInfoToNavStars) 
		{
			doExtraInfo = false;
			if(selectedObject->getType() == QStringLiteral("Star"))
			{
				for (QVector<StelObjectP>::const_iterator itor = stars.constBegin();
					itor != stars.constEnd();
					itor++)
				{
					StelObjectP p = *itor;
					if (!p.isNull() && p->getEnglishName() == selectedObject->getEnglishName())
					{
						doExtraInfo = true;
						break;
					}
				}
			}
			else
			{
				QString englishName = selectedObject->getEnglishName();
				if (isPermittedObject(englishName))
				{
					doExtraInfo = true;
				}
			}
		}
		if (doExtraInfo)
			extraInfo(core, selectedObject);
	}
}

void NavStars::extraInfo(StelCore* core, const StelObjectP& selectedObject)
{
	double jd, jde, x = 0., y = 0.;
	QString extraText = "", englishName = selectedObject->getEnglishName();

	jd  = core->getJD();
	jde = core->getJDE();

	NavStarsCalculator calc;
	calc.setUTC(StelUtils::julianDayToISO8601String(jd))
		.setLatDeg(core->getCurrentLocation().getLatitude())
		.setLonDeg(core->getCurrentLocation().getLongitude())
		.setJd(jd)
		.setJde(jde)
		.setGmst(get_mean_sidereal_time(jd, jde));

	StelUtils::rectToSphe(&x, &y, selectedObject->getEquinoxEquatorialPos(core));	
	calc.setRaRad(x).setDecRad(y);

	StelUtils::rectToSphe(&x,&y,selectedObject->getAltAzPosGeometric(core)); 
	calc.setAzRad(x).setAltRad(y);

	StelUtils::rectToSphe(&x,&y,selectedObject->getAltAzPosApparent(core)); 
	calc.setAzAppRad(x).setAltAppRad(y);

	calc.execute();

	if ("Sun" == englishName || "Moon" == englishName) 
	{
		// Adjust Ho if target is Sun or Moon by adding/subtracting the angular radius.
		double obj_radius_in_degrees = selectedObject->getAngularRadius(core);
		if (!upperLimb)
			obj_radius_in_degrees *= -1;
		calc.addAltAppRad((obj_radius_in_degrees * M_PI) / 180.);
		extraText = upperLimb ?
			" (" + QString(qc_("upper limb", "the highest part of the Sun or Moon")) + ")" :
			" (" + QString(qc_("lower limb", "the lowest part of the Sun or Moon")) + ")";
	}

	if (tabulatedDisplay)
		displayTabulatedInfo(selectedObject, calc, extraText);
	else
		displayStandardInfo(selectedObject, calc, extraText);
}

void NavStars::displayStandardInfo(const StelObjectP& selectedObject, NavStarsCalculator& calc, const QString& extraText)
{
	Q_UNUSED(extraText)	
	QString temp;
	StelObject::InfoStringGroup infoGroup = StelObject::OtherCoord;
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("GHA", "Greenwich Hour Angle") + "&#9800;", calc.gmstPrintable(), "", false));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("SHA", "object Sidereal Hour Angle (ERA, Earth rotation angle)"), calc.shaPrintable(), "", false));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("LHA", "Local Hour Angle"), calc.lhaPrintable(), "", false));
	temp = calc.ghaPrintable() + "/" + calc.decPrintable();
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("GP: GHA/DEC", "Ground Position of object"), temp, "", false));
	temp = calc.gplatPrintable() + "/" + calc.gplonPrintable();
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("GP: LAT/LON", "geodetic coordinate system, latitude and longitude of ground point"), temp, "", false));
	temp = calc.latPrintable() + "/" + calc.lonPrintable();
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("AP: LAT/LON", "geodetic coordinate system, assumed latitude and longitude of user"), temp, "", false));
	temp = calc.hcPrintable() + "/" + calc.znPrintable();
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("Hc/Zn", "Navigation/horizontal coordinate system, calculated altitude and azimuth"), temp, "", false));
}

void NavStars::displayTabulatedInfo(const StelObjectP& selectedObject, NavStarsCalculator& calc, const QString& extraText)
{
	StelObject::InfoStringGroup infoGroup = StelObject::OtherCoord;		
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("UTC", "Universal Time Coordinated"), calc.getUTC(), "", false));
	selectedObject->addToExtraInfoString(infoGroup, "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>");
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("Ho", "Navigation/horizontal coordinate system, sextant measured altitude"), calc.altAppPrintable(), extraText, true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("GHA", "Greenwich Hour Angle") + "&#9800;", calc.gmstPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("LMST", "Local Hour Angle"), calc.lmstPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("SHA", "object Sidereal Hour Angle (ERA, Earth rotation angle)"), calc.shaPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("GHA", "Greenwich Hour Angle"), calc.ghaPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("DEC", "Declination"), calc.decPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("LHA", "Local Hour Angle"), calc.lhaPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("LAT", "geodetic coordinate system, latitude"), calc.gplatPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("LON", "geodetic coordinate system, longitude"), calc.gplonPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("Hc", "Navigation/horizontal coordinate system, calculated altitude"), calc.hcPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, 
		oneRowTwoCells(qc_("Zn", "Navigation/horizontal coordinate system, calculated azimuth"), calc.znPrintable(), "", true));
	selectedObject->addToExtraInfoString(infoGroup, "</table>");
}

QString NavStars::oneRowTwoCells(const QString& a, const QString& b, const QString &extra, bool tabulatedView)
{
	QString rval;
	if (tabulatedView)
		rval += QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td><td>%3</td></tr>").arg(a, b, extra);
	else
		rval += QString("%1: %2 %3<br />").arg(a, b, extra);
	return rval;
}

bool NavStars::isPermittedObject(const QString& s)
{
	static const QStringList permittedObjects= {"Sun", "Moon", "Venus", "Mars", "Jupiter", "Saturn"};
	return permittedObjects.contains(s);
}
