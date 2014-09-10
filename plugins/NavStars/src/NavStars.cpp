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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelFileMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"
#include "NavStars.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"

#include <QList>
#include <QSharedPointer>
#include <QAction>

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
	//info.version = NAVSTARS_PLUGIN_VERSION;
	return info;
}

Q_EXPORT_PLUGIN2(NavStars, NavStarsStelPluginInterface)

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

	// Toolbar button
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	QAction *showAction = gui->getGuiAction("actionShow_NavStars");
	toolbarButton = new StelButton(NULL, QPixmap(":/NavStars/btNavStars-on.png"), QPixmap(":/NavStars/btNavStars-off.png"), QPixmap(":/graphicGui/glow32x32.png"), showAction);
	gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	connect(showAction, SIGNAL(toggled(bool)), this, SLOT(setNavStarsMarks(bool)));

	showAction->setChecked(markerFader);

	// Sync global settings for stars labels
	// FIXME: Need backport changes in StarMgr class
	//connect(smgr, SIGNAL(starLabelsDisplayedChanged(bool)),
	//        this, SLOT(starNamesChanged(bool)));
	starNamesState = false;
}


void NavStars::deinit()
{
	if(NULL != markerTexture) {delete markerTexture;}
	stars.clear();
}


void NavStars::draw(StelCore* core, StelRenderer* renderer)
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
	if(NULL == markerTexture)
	{
		Q_ASSERT_X(NULL == markerTexture, Q_FUNC_INFO, "Textures need to be created simultaneously");
		// Marker texture - using the same texture as the planet hints.
		markerTexture = renderer->createTexture("textures/planet-indicator.png");
	}

	Vec3d pos, xyz;
	for (int i = 0; i < starNumbers.size(); ++i)
	{
		if (stars[i].isNull())
			continue;
		
		xyz = stars[i]->getJ2000EquatorialPos(core);
		// Get the current position of the navigational star...
		if (prj->projectCheck(xyz, pos))
		{
			markerTexture->bind();
			renderer->setBlendMode(BlendMode_Alpha);
			renderer->setGlobalColor(markerColor[0], markerColor[1], markerColor[2], markerFader.getInterstate());
			// ... and draw a marker around it
			renderer->drawTexturedRect(pos[0] - 7, pos[1] - 7, 14.f, 14.f);

			// Draw the localized name of the star and its ordinal number
			QString label = stars[i]->getNameI18n();
			if (i > 0) // Not Polaris
				label = QString("%1 (%2)").arg(label).arg(i);
			renderer->drawText(TextParams(xyz, prj, label).shift(10.f, 10.f).useGravity());
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
