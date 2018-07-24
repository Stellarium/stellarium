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

#include <QList>
#include <QSharedPointer>
#include <QMetaEnum>

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
	info.authors = "Alexander Wolf";
	info.contact = "https://stellarium.org/";
	info.description = N_("This plugin marks navigational stars from a selected set.");
	info.version = NAVSTARS_PLUGIN_VERSION;
	info.license = NAVSTARS_PLUGIN_LICENSE;
	return info;
}



NavStars::NavStars()
	: currentNSSet(AngloAmerican)
	, enableAtStartup(false)
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
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_NavStars");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}

	// Sync global settings for stars labels
	connect(GETSTELMODULE(StarMgr), SIGNAL(starLabelsDisplayedChanged(bool)), this, SLOT(starNamesChanged(bool)));
}


void NavStars::deinit()
{
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
	if (markerFader.getInterstate() <= 0.0)
	{
		return;
	}
	
	if (stars.isEmpty())
	{
		StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
		stars.fill(StelObjectP(), starNumbers.size());
		for (int i = 0; i < starNumbers.size(); ++i)
		{
			QString name = QString("HIP %1").arg(starNumbers.at(i));
			stars[i] = omgr->searchByName(name);
		}
	}

	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	
	Vec3d pos;
	for (int i = 0; i < starNumbers.size(); ++i)
	{
		if (stars[i].isNull())
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
				painter.drawSprite2dMode(pos[0], pos[1], 11.f);
			}

			// Draw the localized name of the star and its ordinal number
			QString label = stars[i]->getNameI18n();
			if (label.isEmpty())
				label = QString("%1").arg(i+1);
			else
				label = QString("%1 (%2)").arg(label).arg(i+1);
			painter.drawText(pos[0], pos[1], label, 0, 10.f, 10.f, false);
		}
	}

}


void NavStars::update(double deltaTime)
{
	markerFader.update((int)(deltaTime*1000));
}


void NavStars::setNavStarsMarks(const bool b)
{
	if (b != getNavStarsMarks())
	{
		propMgr->setStelPropertyValue("StarMgr.flagLabelsDisplayed", !b);
		markerFader = b;
		emit navStarsMarksChanged(b);
	}
}


bool NavStars::getNavStarsMarks() const
{
	return markerFader;
}


void NavStars::starNamesChanged(const bool b)
{
	if (b && getNavStarsMarks())
	{
		setNavStarsMarks(false);
	}
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
	markerColor = StelUtils::strToVec3f(conf->value("marker_color", "0.8,0.0,0.0").toString());
	enableAtStartup = conf->value("enable_at_startup", false).toBool();

	conf->endGroup();
}

void NavStars::saveConfiguration(void)
{
	conf->beginGroup("NavigationalStars");

	conf->setValue("current_ns_set", getCurrentNavigationalStarsSetKey());
	conf->setValue("marker_color", StelUtils::vec3fToStr(markerColor));
	conf->setValue("enable_at_startup", enableAtStartup);

	conf->endGroup();
}

void NavStars::setCurrentNavigationalStarsSetKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("NavigationalStarsSet"));
	NavigationalStarsSet nsSet = (NavigationalStarsSet)en.keyToValue(key.toLatin1().data());
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
			txt = q_("The 81 stars that are listed in the <em>%1</em> published by the French Bureau des Longitudes.").arg("Ephémérides Nautiques");
			break;
		}
		case Russian:
		{
			// TRANSLATORS: The emphasis tags mark a book title.
			txt = q_("The 160 stars that are listed in the Russian Nautical Almanac (The original Russian title is <em>%1</em>).").arg("Морской астрономический ежегодник");
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
	}

	setNavStarsMarks(currentState);
}
