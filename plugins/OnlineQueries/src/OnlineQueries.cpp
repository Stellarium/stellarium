/*
 * Pointer Coordinates plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf
 * Copyright (C) 2016 Georg Zotti (Constellation code)
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelMainView.hpp"
#include "SkyGui.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelObjectMgr.hpp"
#include "Star.hpp"
#include "StarMgr.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "StelUtils.hpp"
#include "StelDialog.hpp"
#include "OnlineQueries.hpp"
#include "OnlineQueriesDialog.hpp"

#include <QFontMetrics>
#include <QSettings>
#include <QMetaEnum>
#include <QLoggingCategory>
#include <QDesktopServices>

Q_LOGGING_CATEGORY(onlineQueries,"stel.plugin.OnlineQueries")

StelModule* OnlineQueriesPluginInterface::getStelModule() const
{
	return new OnlineQueries();
}

StelPluginInfo OnlineQueriesPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(OnlineQueries);

	StelPluginInfo info;
	info.id = "OnlineQueries";
	info.displayedName = N_("Online Queries");
	info.authors = "Georg Zotti, Alexander Wolf";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plugin allows object information retrieval from selected online services.");
	info.version = ONLINEQUERIES_PLUGIN_VERSION;
	info.license = ONLINEQUERIES_PLUGIN_LICENSE;
	return info;
}

OnlineQueries::OnlineQueries() :
	enabled(false),
	toolbarButton(Q_NULLPTR),
	starnamesHipQuery(Q_NULLPTR),
	ancientSkiesHipQuery(Q_NULLPTR),
	hipOnlineReply(Q_NULLPTR)
{
	setObjectName("OnlineQueries");
	dialog = new OnlineQueriesDialog();
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
}

OnlineQueries::~OnlineQueries()
{
	if (hipOnlineReply)
	{
		hipOnlineReply->deleteLater();
		hipOnlineReply = Q_NULLPTR;
	}
	delete starnamesHipQuery;
	starnamesHipQuery=Q_NULLPTR;
	delete ancientSkiesHipQuery;
	ancientSkiesHipQuery=Q_NULLPTR;
	delete dialog;
}

void OnlineQueries::init()
{
	if (!conf->childGroups().contains("OnlineQueries"))
	{
		qCDebug(onlineQueries) << "OnlineQueries: no section exists in main config file - creating with defaults";
		restoreDefaultConfiguration();
	}
	// populate settings from main config file.
	loadConfiguration();

	starnamesHipQuery=new HipOnlineQuery(ptolemyUrl);
	ancientSkiesHipQuery=new HipOnlineQuery(ancientSkiesUrl);

	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveConfiguration()));
	addAction("actionShow_OnlineQueries", N_("Online Queries"), N_("Show window for Online Queries"), this, "enabled", "");
	addAction("actionShow_OnlineQueries_SN", N_("Online Queries"), N_("Call a Starnames site on current selection"), this, "queryStarnames()", "");
	addAction("actionShow_OnlineQueries_AS", N_("Online Queries"), N_("Call ancient-skies on current selection"), this, "queryAncientSkies()", "");
	addAction("actionShow_OnlineQueries_WP", N_("Online Queries"), N_("Call Wikipedia on current selection"), this, "queryWikipedia()", "");
	createToolbarButton();
}

void OnlineQueries::deinit()
{
	//
}

void OnlineQueries::setEnabled(bool b)
{
	if (b!=enabled)
	{
		enabled = b;
		emit flagEnabledChanged(b);
	}
	configureGui(b);
}

bool OnlineQueries::configureGui(bool show)
{
	dialog->setVisible(show);
	return true;
}

void OnlineQueries::restoreDefaultConfiguration(void)
{
	// Remove the whole section from the configuration file
	conf->remove("OnlineQueries");
	// Load the default values...
	loadConfiguration();
	// ... then save them.
	saveConfiguration();
}

void OnlineQueries::loadConfiguration(void)
{
	conf->beginGroup("OnlineQueries");
	ptolemyUrl=conf->value("ptolemy_url", "https://biblicalastronomy.co/playground/fetch.cfm?Hipp=%1").toString();
	ancientSkiesUrl=conf->value("ancientskies_url", "https://www.ancient-skies.org/webservice?hip=%1").toString();
	conf->endGroup();
}

void OnlineQueries::saveConfiguration(void)
{
	conf->beginGroup("OnlineQueries");
	conf->setValue("ptolemy_url", ptolemyUrl);
	conf->setValue("ancientskies_url", ancientSkiesUrl);
	conf->endGroup();
}

void OnlineQueries::createToolbarButton() const
{
	// Add toolbar button (copy/paste widely from AngleMeasure).
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());

		if (gui!=Q_NULLPTR)
		{
			StelButton* button =	new StelButton(Q_NULLPTR,
							       QPixmap(":/OnlineQueries/bt_OnlineQueries_On.png"),
							       QPixmap(":/OnlineQueries/bt_OnlineQueries_Off.png"),
							       QPixmap(":/graphicGui/glow32x32.png"),
							       "actionShow_OnlineQueries");
			qCDebug(onlineQueries) << "adding Button to toolbar ...";

			gui->getButtonBar()->addButton(button, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qCWarning(onlineQueries) << "WARNING: unable to create toolbar button for OnlineQueries plugin: " << e.what();
	}
}

void OnlineQueries::queryStarnames()
{
	setOutputHtml("<h1>Starnames</h1><p>querying...</p>");

	const QList<StelObjectP>& sel=GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (sel.length()==0)
		return;

	const StelObjectP obj=sel.at(0);
	if (obj->getType()!=STAR_TYPE)
		return;

	QString hipStr=obj->getID();
	if (!hipStr.startsWith("HIP"))
		return;

	int hipNr=hipStr.split(' ').at(1).toInt();
	hipOnlineReply=starnamesHipQuery->lookup(hipNr);

	onHipQueryStatusChanged();
	connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
}

void OnlineQueries::queryAncientSkies()
{
	setOutputHtml("<h1>Ancient-Skies.org</h1><p>querying...</p>");

	const QList<StelObjectP>& sel=GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (sel.length()==0)
		return;

	const StelObjectP obj=sel.at(0);
	if (obj->getType()!=STAR_TYPE)
		return;

	QString hipStr=obj->getID();
	if (!hipStr.startsWith("HIP"))
		return;

	int hipNr=hipStr.split(' ').at(1).toInt();
	hipOnlineReply=ancientSkiesHipQuery->lookup(hipNr);

	onHipQueryStatusChanged();
	connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
}

// Called when the current HIP query status changes
void OnlineQueries::onHipQueryStatusChanged()
{
	setEnabled(true); // show dialog
	//HipOnlineReply *reply=static_cast<HipOnlineReply*>(sender());
	Q_ASSERT(hipOnlineReply);
	if (hipOnlineReply->getCurrentStatus()==HipOnlineReply::HipQueryErrorOccured)
	{
		setOutputHtml(QString("<p>Lookup error: %1</p>").arg(hipOnlineReply->getErrorString()));
	}

	if (hipOnlineReply->getCurrentStatus()==HipOnlineReply::HipQueryFinished)
	{
		setOutputHtml(hipOnlineReply->getResult());
	}

	if (hipOnlineReply->getCurrentStatus()!=HipOnlineReply::HipQueryQuerying)
	{
		disconnect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
		delete hipOnlineReply;
		hipOnlineReply=Q_NULLPTR;
	}
}

void OnlineQueries::queryWikipedia()
{
	const QList<StelObjectP>& sel=GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (sel.length()==0)
		return;

	const StelObjectP obj=sel.at(0);

	QString objName;
	if (obj->getType()==STAR_TYPE)
	{
		QString hipStr=obj->getID();
		if (!hipStr.startsWith("HIP"))
			return;
		int hipNr=hipStr.split(' ').at(1).toInt();
		objName=StarMgr::getCommonEnglishName(hipNr);
	}
	else if (obj->getType()==Planet::PLANET_TYPE)
	{
		objName=obj->getEnglishName();
	}
	else if (obj->getType()==Nebula::NEBULA_TYPE)
	{
		// clumsy workaround for uncastable pointer...
		objName = GETSTELMODULE(NebulaMgr)->getLatestSelectedDSODesignation();
		if (objName.isEmpty())
		{
			objName=obj->getEnglishName();
		}
		// WP has all Messiers with Messier_X. Use that directly to avoid ambiguity.
		if (objName.startsWith("M "))
		{
			objName=objName.replace("M ", "Messier_");
		}
		// TODO: Other similar replacements?
		if (objName.isEmpty())
		{
			setOutputHtml(QString("<h1>Wikipedia</h1><p>This can request data for stars, planets and deep-sky objects. A valid name for this object could not be found. Please enable a few catalogs to form a numerical name.</p>"));
			return;
		}
	}
	else
	{
		setOutputHtml(QString("<h1>Wikipedia</h1><p>This can request data for stars, planets and deep-sky objects only.</p>"));
		return;
	}
	setOutputHtml(QString("<h1>Wikipedia</h1><p>Opened page on '%1' in your webbrowser.</p>").arg(objName));

	QDesktopServices::openUrl(QUrl(QString("https://en.wikipedia.org/wiki/%1").arg(objName)));
}

void OnlineQueries::setOutputHtml(QString html)
{
	if (dialog)
		dialog->setOutputHtml(html);
}
