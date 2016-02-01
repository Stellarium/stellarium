/*
 * Navigational Stars plug-in
 * Copyright (C) 2014 Alexander Wolf
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
#include "StelUtils.hpp"
#include "NavStars.hpp"

#include <QList>
#include <QSharedPointer>


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
	info.contact = "http://stellarium.org/";
	// TRANSLATORS: The emphasis tags mark a book title.
	info.description = N_("This plugin marks 58 navigational stars - Polaris and the 57 \"selected stars\" that are listed in <em>The Nautical Almanac</em> jointly published by Her Majesty's Nautical Almanac Office and the US Naval Observatory since 1958; consequently, these stars are also used in navigational aids such as the 2102D Star Finder and Identifier.");
	info.version = NAVSTARS_PLUGIN_VERSION;
	return info;
}



NavStars::NavStars()
	: starNamesState(false)
	, toolbarButton(NULL)
{
	setObjectName("NavStars");
	conf = StelApp::getInstance().getSettings();

	// Set default color
	if (!conf->contains("NavigationalStars/marker_color"))
		conf->setValue("NavigationalStars/marker_color", "0.8,0.0,0.0");
	
	QVariant var = conf->value("NavigationalStars/marker_color", "0.8,0.0,0.0");
	markerColor = StelUtils::strToVec3f(var.toString());

	// Get the manager of stars for manipulation of the stars labels
	smgr = GETSTELMODULE(StarMgr);
}


NavStars::~NavStars()
{
	//
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
	// List of HIP numbers of the navigational stars: Polaris (index 0) and
	// the 47 "selected stars" from The Nautical Almanac. Polaris is included
	// because it was listed with them in the The American Practical Navigator,
	// or more precisely, because it was included in the list on Wikipedia:
	// http://en.wikipedia.org/wiki/List_of_selected_stars_for_navigation
	starNumbers << 11767 << 677   << 2081  << 3179  << 3419  << 7588  << 9884;
	starNumbers << 13847 << 14135 << 15863 << 21421 << 24436 << 24608 << 25336;
	starNumbers << 25428 << 26311 << 27989 << 30438 << 32349 << 33579 << 37279;
	starNumbers << 37826 << 41037 << 44816 << 45238 << 46390 << 49669 << 54061;
	starNumbers << 57632 << 59803 << 60718 << 61084 << 62956 << 65474 << 67301;
	starNumbers << 68702 << 68933 << 69673 << 71683 << 72603 << 72607 << 76267;
	starNumbers << 80763 << 82273 << 84012 << 85927 << 86032 << 87833 << 90185;
	starNumbers << 91262 << 92855 << 97649 << 100751 << 102098 << 107315;
	starNumbers << 109268 << 113368 << 113963;

	// Marker texture - using the same texture as the planet hints.
	QString path = StelFileMgr::findFile("textures/planet-indicator.png");
	markerTexture = StelApp::getInstance().getTextureManager().createTexture(path);

	// key bindings and other actions
	addAction("actionShow_NavStars",
	          N_("Navigational Stars"),
	          N_("Mark the navigational stars"),
	          "navStarsVisible", "");

	// Toolbar button
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=NULL)
	{
		if (toolbarButton == NULL)
		{
			// Create the nav. stars button
			toolbarButton = new StelButton(NULL,
						       QPixmap(":/NavStars/btNavStars-on.png"),
						       QPixmap(":/NavStars/btNavStars-off.png"),
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_NavStars");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}

	// Sync global settings for stars labels
	connect(smgr, SIGNAL(starLabelsDisplayedChanged(bool)),
	        this, SLOT(starNamesChanged(bool)));
	starNamesState = false;
}


void NavStars::deinit()
{
	markerTexture.clear();
	stars.clear();
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
				painter.enableBlend(true, false, __FILE__, __LINE__);
				painter.enableTexture2d(true);
				painter.setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				painter.setColor(markerColor[0],
						markerColor[1],
						markerColor[2],
						markerFader.getInterstate());
				markerTexture->bind();
				painter.drawSprite2dMode(pos[0], pos[1], 11.f);
			}

			// Draw the localized name of the star and its ordinal number
			QString label = stars[i]->getNameI18n();
			if (i > 0) // Not Polaris
				label = QString("%1 (%2)").arg(label).arg(i);
			painter.drawText(pos[0], pos[1], label, 0, 10.f, 10.f, false);
			// drawText garbles blend mode... force back.
			painter.enableBlend(true, true, __FILE__, __LINE__);

		}
	}

}


void NavStars::update(double deltaTime)
{
	markerFader.update((int)(deltaTime*1000));
}


void NavStars::setNavStarsMarks(const bool b)
{
	if (b == markerFader)
		return;

	if (b)
	{
		// Save the display state of the star labels and hide them.
		starNamesState = smgr->getFlagLabels();
		smgr->setFlagLabels(false);
	}
	else
	{
		// Restore the star labels state.
		smgr->setFlagLabels(starNamesState);
	}

	markerFader = b;
	emit navStarsMarksChanged(b);
}


bool NavStars::getNavStarsMarks() const
{
	return markerFader;
}


void NavStars::starNamesChanged(const bool b)
{
	if (b && getNavStarsMarks())
	{
		starNamesState=true;
		setNavStarsMarks(false);
	}
}
