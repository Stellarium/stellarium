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

/*
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*/
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
	info.description = N_("This plugin marks the 58 navigational stars of the 2102-D Rude Star Finder, also tabulated in the Nautical Almanac.");
	info.version = NAVSTARS_PLUGIN_VERSION;
	return info;
}

/*
 Constructor
*/
NavStars::NavStars()
	: OnIcon(NULL)
	, OffIcon(NULL)
	, GlowIcon(NULL)
	, toolbarButton(NULL)	
{
	setObjectName("NavStars");
	conf = StelApp::getInstance().getSettings();

	// Set default color
	navStarColor = Vec3f(0.8, 0.0, 0.0);
	if (conf->contains("navstars/navstars_color"))
	{
		// OK, we have color settings
		navStarColor = StelUtils::strToVec3f(conf->value("navstars/navstars_color", "0.8,0.0,0.0").toString());
	}
	else
	{
		// Oops... no value in config, create a default value
		conf->setValue("navstars/navstars_color", "0.8,0.0,0.0");
	}
}

/*
 Destructor
*/
NavStars::~NavStars()
{
	if (GlowIcon)
		delete GlowIcon;
	if (OnIcon)
		delete OnIcon;
	if (OffIcon)
		delete OffIcon;
}

void NavStars::deinit()
{
	markerTexture.clear();	
}

/*
 Reimplementation of the getCallOrder method
*/
double NavStars::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*
 Init our module
*/
void NavStars::init()
{
	// Get the manager of stars for manipulation of the stars labels
	smgr = GETSTELMODULE(StarMgr);

	// List of HIP numbers of navigational stars
	// First star (Polaris) doesn't have number and marked as "*"
	// Comment from the Wikipedia (http://en.wikipedia.org/wiki/List_of_selected_stars_for_navigation#cite_note-Polaris-9):
	// This list uses the assigned numbers from the nautical almanac, which includes only 57 stars. Polaris, which is included in the list given in
	// The American Practical Navigator, is listed here without a number.
	nstar << 11767 << 677 << 2081 << 3179 << 3419 << 7588 << 9884 << 13847 << 14135 << 15863 << 21421 << 24436 << 24608 << 25336 << 25428 << 26311;
	nstar << 27989 << 30438 << 32349 << 33579 << 37279 << 37826 << 41037 << 44816 << 45238 << 46390 << 49669 << 54061 << 57632 << 59803 << 60718;
	nstar << 61084 << 62956 << 65474 << 67301 << 68702 << 68933 << 69673 << 71683 << 72603 << 72607 << 76267 << 80763 << 82273 << 84012 << 85927;
	nstar << 86032 << 87833 << 90185 << 91262 << 92855 << 97649 << 100751 << 102098 << 107315 << 109268 << 113368 << 113963;

	// texture for marker (same texture used for the planet hints)
	markerTexture = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/planet-indicator.png");

	// key bindings and other actions
	addAction("actionShow_NavStars", N_("Navigational Stars"), N_("Mark the navigational stars"), "navStarsVisible", "");

	// Icon for toolbar button
	GlowIcon = new QPixmap(":/graphicsGui/glow32x32.png");
	OnIcon = new QPixmap(":/NavStars/btNavStars-on.png");
	OffIcon = new QPixmap(":/NavStars/btNavStars-off.png");

	// Toolbar button
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (toolbarButton==NULL) {
		// Create the nav. stars button
		toolbarButton = new StelButton(NULL, *OnIcon, *OffIcon, *GlowIcon, "actionShow_NavStars");
	}
	gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");	

	// Sync global settings for stars labels
	connect(smgr, SIGNAL(starLabelsDisplayedChanged(bool)), this, SLOT(starNamesChanged(bool)));
	starNamesState=false;
}

/*
 Draw markers of the navigational stars
*/
void NavStars::draw(StelCore* core)
{
	// Drawing is enabled?
	if (navStarsMarkerFader.getInterstate() <= 0.0)
	{
		return;
	}

	// Set projection frame...
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	// ... prepare object manager for search stars via them HIP numbers
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);

	Vec3d pos;
	QString name, num;
	// Use all HIP numbers from the list of navigational stars
	for (int i = 0; i < nstar.size(); ++i)
	{
		// Search HIP numbers...
		name = QString("HIP %1").arg(nstar.at(i));
		StelObjectP obj = omgr->searchByName(name);
		// Get the current position of the navigational star
		prj->projectCheck(obj->getJ2000EquatorialPos(core), pos);

		// ... and draw marker around star
		glEnable(GL_BLEND);
		painter.enableTexture2d(true);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		painter.setColor(navStarColor[0], navStarColor[1], navStarColor[2], navStarsMarkerFader.getInterstate());
		markerTexture->bind();
		painter.drawSprite2dMode(pos[0], pos[1], 11.f);
		// Get the localized name of the navigational star and it serial number from the list of the stars, and draw them
		if (i>0)
			num.setNum(i);
		else
			num = "*";
		painter.drawText(pos[0], pos[1], QString("%1 (%2)").arg(obj->getNameI18n()).arg(num), 0, 10.f, 10.f, false);
	}

}

void NavStars::update(double deltaTime)
{
	navStarsMarkerFader.update((int)(deltaTime*1000));
}

// Set flag of displaying markers of the navigational stars
void NavStars::setNavStarsMarks(const bool b)
{
	if (b == navStarsMarkerFader)
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

	navStarsMarkerFader = b;
	emit navStarsMarksChanged(b);
}

// Get flag of displaying markers of the navigational stars
bool NavStars::getNavStarsMarks() const
{
	return navStarsMarkerFader;
}

void NavStars::starNamesChanged(const bool b)
{
	if (b && getNavStarsMarks()) {
		starNamesState=true;
		setNavStarsMarks(false);
	}
}
