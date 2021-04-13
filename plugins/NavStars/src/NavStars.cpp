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
#include "StarMgr.hpp"
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
	permittedObjects.push_back(QStringLiteral("Sun"));
	permittedObjects.push_back(QStringLiteral("Moon"));
	permittedObjects.push_back(QStringLiteral("Venus"));
	permittedObjects.push_back(QStringLiteral("Mars"));
	permittedObjects.push_back(QStringLiteral("Jupiter"));
	permittedObjects.push_back(QStringLiteral("Saturn"));
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
	addAction("actionShow_NavStars", N_("Navigational Stars"), N_("Mark the navigational stars"), "navStarsVisible", "");

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
						       "actionShow_NavStars");
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
				painter.setColor(markerColor[0], markerColor[1], markerColor[2], markerFader.getInterstate());
				markerTexture->bind();
				painter.drawSprite2dMode(static_cast<float>(pos[0]), static_cast<float>(pos[1]), 11.f);
			}

			// Draw the localized name of the star and its ordinal number
			QString label = stars[i]->getNameI18n();
			if (label.isEmpty())
				label = QString("%1").arg(i+1);
			else
				label = QString("%1 (%2)").arg(label).arg(i+1);
			painter.drawText(static_cast<float>(pos[0]), static_cast<float>(pos[1]), label, 0, 10.f, 10.f, false);
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
	QString txt = "";

	switch(getCurrentNavigationalStarsSet())
	{
		case AngloAmerican:
		{
			// TRANSLATORS: The emphasis tags mark a title.
			txt = q_("The 57 \"selected stars\" that are listed in <em>The Nautical Almanac</em> jointly published by Her Majesty's Nautical Almanac Office and the US Naval Observatory since 1958; consequently, these stars are also used in navigational aids such as the <em>2102D Star Finder</em> and <em>Identifier</em>.");
			break;
		}
		case French:
		{
			// TRANSLATORS: The emphasis tags mark a book title.
			txt = q_("The 81 stars that are listed in the French Nautical Almanac (The original French title is <em>%1</em>) published by the French Bureau des Longitudes.").arg("Ephémérides Nautiques");
			break;
		}
		case Russian:
		{
			// TRANSLATORS: The emphasis tags mark a book title.
			txt = q_("The 160 stars that are listed in the Russian Nautical Almanac (The original Russian title is <em>%1</em>).").arg("Морской астрономический ежегодник");
			break;
		}
		case German:
		{
			// TRANSLATORS: The emphasis tags mark a book title.
			txt = q_("The 80 stars that are listed in the German Nautical Almanac (The original German title is <em>%1</em>) published by the Federal Maritime and Hydrographic Agency of Germany.").arg("Nautisches Jahrbuch");
			break;
		}
	}

	return txt;
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
			starNumbers <<    677 <<   2081 <<   3179 <<   3419 <<   7588 <<   9884 <<  13847
				    <<  14135 <<  15863 <<  21421 <<  24436 <<  24608 <<  25336 <<  25428
				    <<  26311 <<  27989 <<  30438 <<  32349 <<  33579 <<  37279 <<  37826
				    <<  41037 <<  44816 <<  45238 <<  46390 <<  49669 <<  54061 <<  57632
				    <<  59803 <<  60718 <<  61084 <<  62956 <<  65474 <<  67301 <<  68702
				    <<  68933 <<  69673 <<  71683 <<  72603 <<  72607 <<  76267 <<  80763
				    <<  82273 <<  84012 <<  85927 <<  86032 <<  87833 <<  90185 <<  91262
				    <<  92855 <<  97649 << 100751 << 102098 << 107315 << 109268 << 113368
				    << 113963;
			break;
		}
		case French:
		{
			// 81 stars from French Nautical Almanac
			// Original French name: Ephémérides Nautiques
			starNumbers <<    677 <<    746 <<   1067 <<   2081 <<   3179 <<   3419 <<   4427
				    <<   5447 <<   7588 <<   9884 <<  11767 <<  14135 <<  14576 <<  15863
				    <<  21421 <<  24436 <<  24608 <<  25336 <<  25428 <<  26311 <<  26727
				    <<  27989 <<  28360 <<  30324 <<  30438 <<  31681 <<  32349 <<  33579
				    <<  34444 <<  36850 <<  37279 <<  37826 <<  39429 <<  39953 <<  41037
				    <<  42913 <<  44816 <<  45238 <<  45556 <<  46390 <<  49669 <<  53910
				    <<  54061 <<  54872 <<  57632 <<  58001 <<  59803 <<  60718 <<  61084
				    <<  61932 <<  62434 <<  62956 <<  65378 <<  65474 <<  67301 <<  68702
				    <<  68933 <<  69673 <<  71683 <<  72607 <<  76267 <<  80763 <<  82273
				    <<  82396 <<  85927 <<  86032 <<  87833 <<  90185 <<  91262 <<  92855
				    <<  97649 << 100453 << 100751 << 102098 << 105199 << 107315 << 109268
				    << 112122 << 113368 << 113881 << 113963;
			break;
		}
		case Russian:
		{
			// 160 stars from Russian Nautical Almanac
			// Original Russian name: Морской астрономический ежегодник.
			starNumbers <<    677 <<    746 <<   1067 <<   2021 <<   2081 <<   3179 <<   3419
				    <<   4427 <<   5447 <<   6686 <<   7588 <<   8886 <<   8903 <<   9236
				    <<   9640 <<   9884 <<  13847 <<  14135 <<  14576 <<  15863 <<  17702
				    <<  18246 <<  18532 <<  21421 <<  23015 <<  23875 <<  24436 <<  24608
				    <<  25336 <<  25428 <<  25606 <<  25930 <<  25985 <<  26241 <<  26311
				    <<  26451 <<  26634 <<  26727 <<  27913 <<  27989 <<  28360 <<  28380
				    <<  30324 <<  30438 <<  31681 <<  32349 <<  32768 <<  33579 <<  33152
				    <<  34444 <<  35264 <<  35904 <<  36188 <<  36850 <<  37279 <<  37826
				    <<  39429 <<  39757 <<  39953 <<  41037 <<  42913 <<  44816 <<  45238
				    <<  45556 <<  46390 <<  46701 <<  49669 <<  50583 <<  52419 <<  52727
				    <<  53910 <<  54061 <<  54872 <<  57632 <<  58001 <<  59196 <<  59747
				    <<  59774 <<  59803 <<  60718 <<  61084 <<  61359 <<  61585 <<  61932
				    <<  61941 <<  62434 <<  62956 <<  63121 <<  63608 <<  65109 <<  65378
				    <<  65474 <<  66657 <<  67301 <<  67927 <<  68002 <<  68702 <<  68933
				    <<  69673 <<  71075 <<  71352 <<  71681 <<  71860 <<  72105 <<  72603
				    <<  72607 <<  73273 <<  74946 <<  74785 <<  76297 <<  76267 <<  77070
				    <<  78401 <<  78820 <<  79593 <<  80331 <<  80763 <<  80816 <<  81266
				    <<  81377 <<  81693 <<  82273 <<  82396 <<  83081 <<  84012 <<  85258
				    <<  85792 <<  85670 <<  85927 <<  86032 <<  86228 <<  79540 <<  86742
				    <<  87833 <<  88635 <<  89931 <<  90185 <<  90496 <<  91262 <<  95347
				    <<  93506 <<  93747 <<  94141 <<  97165 <<  97278 <<  97649 << 100453
				    << 100751 << 102098 << 102488 << 105199 << 107315 << 107556 << 109268
				    << 110130 << 112122 << 113368 << 113881 << 113963 <<  11767;
			break;
		}
		case German:
		{
			// 80 stars from German Nautical Almanac
			// Original German name: Nautisches Jahrbuch
			// The numbers are identical to the "Nautisches Jahrbuch"
			starNumbers <<    677 <<    1067 <<      2081 <<    3179 <<     3419 <<     4427
					 <<     5447 <<     7588 <<   11767 <<     9640 <<     9884 <<   14135
					 <<   14576 <<   15863 <<   17702 <<   21421 <<   24436 <<   24608
					 <<   25336 <<   25428 <<   26311 <<   26727 <<   27366 <<   27989
					 <<   28360 <<   30324 <<   30438 <<   31681 <<   32349 <<   33579
					 <<   34444 <<   36850 <<   37279 <<   37826 <<   41037 <<   44816
					 <<   45238 <<   46390 <<   49669 <<   52419 <<   54061 <<   57632
					 <<   60718 <<   61084 <<   62434 <<   62956 <<   63608 <<   65378
					 <<   65474 <<   67301 <<   68702 <<   68933 <<   69673 <<   71683
					 <<   72105 <<   72622 <<   72607 <<   74785 <<   76267 <<   77070
					 <<   80763 <<   82273 <<   82396 <<   85927 <<   86032 <<   86228
					 <<   87833 <<   90185 <<   91262 <<   92855 <<   97649 << 100751
					 << 102098 << 105199 << 107315 << 109268 << 112122 << 113368
					 << 113881 << 113963;
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
			QString type = selectedObject->getType();
			if(selectedObject->getType() == QStringLiteral("Star")) {
				for (QVector<StelObjectP>::const_iterator itor = stars.begin();
					itor != stars.end();
					itor++)
				{
					StelObjectP p = *itor;
					if (p->getEnglishName() == selectedObject->getEnglishName())
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
		.setLatDeg(core->getCurrentLocation().latitude)
		.setLonDeg(core->getCurrentLocation().longitude)
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
		// Adjust Ho if target is Sun or Moon by adding/subtracting half the angular diameter.
		double d = selectedObject->getAngularSize(core);
		if (upperLimb)
			d *= -1;
		calc.addAltAppRad(((d / 2) * M_PI) / 180.);
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
	QVector<QString>::const_iterator itor = permittedObjects.begin();
	while (itor != permittedObjects.end())
	{
		if (*itor == s)
			return true;
		++itor;
	}
	return false;
}
