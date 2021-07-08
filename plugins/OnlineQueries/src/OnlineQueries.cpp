/*
 * OnlineQueries plug-in for Stellarium
 *
 * Copyright (C) 2020-21 Georg Zotti
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
#include <QXmlStreamReader>

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
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("This plugin allows object information retrieval from selected online services.");
	info.version = ONLINEQUERIES_PLUGIN_VERSION;
	info.license = ONLINEQUERIES_PLUGIN_LICENSE;
	return info;
}

OnlineQueries::OnlineQueries() :
	enabled(false),
	toolbarButton(Q_NULLPTR),
	custom1UseHip(false),
	custom2UseHip(false),
	custom3UseHip(false),
	custom1inBrowser(false),
	custom2inBrowser(false),
	custom3inBrowser(false),
	hipQuery(Q_NULLPTR),
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
	delete hipQuery;
	hipQuery=Q_NULLPTR;
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

	createToolbarButton();

	hipQuery=new HipOnlineQuery("");

	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveConfiguration()));
	addAction("actionShow_OnlineQueries",       N_("Online Queries"), N_("Show window for Online Queries"),           this, "enabled", "");
	addAction("actionShow_OnlineQueries_AS",    N_("Online Queries"), N_("Call ancient-skies on current selection"),  this, "queryAncientSkies()", "");
	addAction("actionShow_OnlineQueries_AAVSO", N_("Online Queries"), N_("Call AAVSO database on current selection"), this, "queryAAVSO()", "");
	addAction("actionShow_OnlineQueries_GCVS",  N_("Online Queries"), N_("Call GCVS database on current selection"),  this, "queryGCVS()", "");
	addAction("actionShow_OnlineQueries_WP",    N_("Online Queries"), N_("Call Wikipedia on current selection"),      this, "queryWikipedia()", "");
	addAction("actionShow_OnlineQueries_C1",    N_("Online Queries"), N_("Call custom site 1 on current selection"),  this, "queryCustomSite1()", "");
	addAction("actionShow_OnlineQueries_C2",    N_("Online Queries"), N_("Call custom site 2 on current selection"),  this, "queryCustomSite2()", "");
	addAction("actionShow_OnlineQueries_C3",    N_("Online Queries"), N_("Call custom site 3 on current selection"),  this, "queryCustomSite3()", "");
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
	if (show)
		dialog->setVisible(true);
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
	ancientSkiesUrl=conf->value("ancientskies_url", "https://www.ancient-skies.org/api.php?apikey=fZdn9QsNdCAY4KggkV2T&response=HTML&entity=star&catalog=HIPPARCOS&id=%1").toString();
	aavsoHipUrl    =conf->value("aavso_hip_url",    "https://www.aavso.org/vsx/index.php?view=api.object&ident=HIP%1").toString();
	aavsoOidUrl    =conf->value("aavso_oid_url",    "https://www.aavso.org/vsx/index.php?view=detail.top&oid=%1").toString();
	gcvsUrl        =conf->value("gcvs_url",         "http://www.sai.msu.su/gcvs/cgi-bin/ident.cgi?cat=Hip+&num=%1").toString();
	wikipediaUrl   =conf->value("wikipedia_url",    "https://en.wikipedia.org/wiki/%1").toString();
	customUrl1     =conf->value("custom1_url",      "https://biblicalastronomy.co/playground/fetch.cfm?Hipp=%1").toString();
	if (!customUrl1.isEmpty() && !customUrl1.contains("%1"))
	{
		qWarning() << "OnlineQueries: custom1_url invalid: no '%1' found in " << customUrl1;
		customUrl1 = "";
	}
	customUrl2=conf->value("custom2_url", "").toString();
	if (!customUrl2.isEmpty() && !customUrl2.contains("%1"))
	{
		qWarning() << "OnlineQueries: custom2_url invalid: no '%1' found in " << customUrl2;
		customUrl2 = "";
	}
	customUrl3=conf->value("custom3_url", "").toString();
	if (!customUrl3.isEmpty() && !customUrl3.contains("%1"))
	{
		qWarning() << "OnlineQueries: custom3_url invalid: no '%1' found in " << customUrl3;
		customUrl3 = "";
	}
	custom1inBrowser=conf->value("custom1_in_browser", false).toBool();
	custom2inBrowser=conf->value("custom2_in_browser", false).toBool();
	custom3inBrowser=conf->value("custom3_in_browser", false).toBool();
	custom1UseHip=conf->value("custom1_use_hip", true).toBool();
	custom2UseHip=conf->value("custom2_use_hip", true).toBool();
	custom3UseHip=conf->value("custom3_use_hip", true).toBool();
	conf->endGroup();
}

void OnlineQueries::saveConfiguration(void)
{
	conf->beginGroup("OnlineQueries");
	conf->setValue("ancientskies_url", ancientSkiesUrl);
	conf->setValue("aavso_hip_url", aavsoHipUrl);
	conf->setValue("aavso_oid_url", aavsoOidUrl);
	conf->setValue("gcvs_url", gcvsUrl);
	conf->setValue("wikipedia_url", wikipediaUrl);
	conf->setValue("custom1_url", customUrl1);
	conf->setValue("custom2_url", customUrl2);
	conf->setValue("custom3_url", customUrl3);
	conf->setValue("custom1_in_browser", custom1inBrowser);
	conf->setValue("custom2_in_browser", custom2inBrowser);
	conf->setValue("custom3_in_browser", custom3inBrowser);
	conf->setValue("custom1_use_hip", custom1UseHip);
	conf->setValue("custom2_use_hip", custom2UseHip);
	conf->setValue("custom3_use_hip", custom3UseHip);
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

void OnlineQueries::queryWikipedia()
{
	query(wikipediaUrl, false, true);
}

// 2-step query.
void OnlineQueries::queryAAVSO()
{
	setOutputHtml("<h1>AAVSO</h1><p>querying...</p>");

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
	hipOnlineReply=hipQuery->lookup(aavsoHipUrl, hipNr);

	// This only delivers the OID for the second AAVSO query
	onAavsoHipQueryStatusChanged();
	connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onAavsoHipQueryStatusChanged()));
}

void OnlineQueries::queryGCVS()
{
	query(gcvsUrl, true, false);
}

void OnlineQueries::queryAncientSkies()
{
	setOutputHtml("<h1>Ancient-Skies</h1><p>querying...</p>");
	// Depending on development of AS, the last argument may be turned to true to have the result in a webbrowser for further refinements.

	query(ancientSkiesUrl, true, false);
}

void OnlineQueries::queryCustomSite1()
{
	query(customUrl1, custom1UseHip, custom1inBrowser);
}

void OnlineQueries::queryCustomSite2()
{
	query(customUrl2, custom2UseHip, custom2inBrowser);
}

void OnlineQueries::queryCustomSite3()
{
	query(customUrl3, custom3UseHip, custom3inBrowser);
}

void OnlineQueries::query(QString url, bool useHip, bool useBrowser)
{
	// dissect url and set output. TODO: Maybe add another arg for the heading to the call?
	QUrl htmlUrl(url); // only used for the next line.
	setOutputHtml(QString("<h1>%1</h1><p>querying %2...</p>").arg(htmlUrl.host()).arg(url));

	const QList<StelObjectP>& sel=GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (sel.length()==0)
		return;
	const StelObjectP obj=sel.at(0);

	if (useHip)
	{
		if (obj->getType()!=STAR_TYPE)
			return;

		QString hipStr=obj->getID();
		if (!hipStr.startsWith("HIP"))
		{
			setOutputHtml(QString("<h1>%1</h1><p>No HIPPARCOS star!</p>").arg(htmlUrl.host()));
			return;
		}

		int hipNr=hipStr.split(' ').at(1).toInt();
		if (useBrowser)
		{
			QDesktopServices::openUrl(QUrl(url.arg(hipNr)));
		}
		else
		{
			hipOnlineReply=hipQuery->lookup(url, hipNr);

			onHipQueryStatusChanged();
			connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
		}
	}
	else
	{
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
			// WP has all Messiers and a few other catalogs with Messier_X and similar fuller names. Use those directly to avoid ambiguity.
			// TODO: What to do with calls to non-Wikipedia sites with different spellings?
			if (objName.startsWith("M "))
				objName=objName.replace("M ", "Messier_");
			else if (objName.startsWith("C "))
				objName=objName.replace("C ", "Caldwell_");
			else if (objName.startsWith("B "))
				objName=objName.replace("B ", "Barnard_");
			else if (objName.startsWith("Cr "))
				objName=objName.replace("Cr ", "Collinder_");
			else if (objName.startsWith("Mel "))
				objName=objName.replace("Mel ", "Melotte_");
			// TODO: Other similar replacements?
			if (objName.isEmpty())
			{
				setOutputHtml(QString("<h1>ERROR</h1><p>This can request data for stars, planets and deep-sky objects. A valid name for this object could not be found. Please enable a few DSO catalogs to form at least a numerical name.</p>"));
				return;
			}
		}
		else
		{
			setOutputHtml(QString("<h1>ERROR</h1><p>This can request data for stars, planets and deep-sky objects only.</p>"));
			return;
		}
		if (useBrowser)
		{
			setOutputHtml(QString("<h1>%1</h1><p>Opened page on '%2' in your webbrowser.</p>").arg(htmlUrl.host()).arg(objName));
			QDesktopServices::openUrl(QUrl(url.arg(objName)));
		}
		else
		{
			hipOnlineReply=hipQuery->lookup(url, objName);

			onHipQueryStatusChanged();
			connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
		}
	}
}

// Called when the current HIP query status changes. This displays simple HTML answers.
void OnlineQueries::onHipQueryStatusChanged()
{
	setEnabled(true); // show dialog
	Q_ASSERT(hipOnlineReply);
	if (hipOnlineReply->getCurrentStatus()==HipOnlineReply::HipQueryErrorOccured)
	{
		setOutputHtml(QString("<p>Lookup error: %1</p>").arg(hipOnlineReply->getErrorString()));
	}

	else if (hipOnlineReply->getCurrentStatus()==HipOnlineReply::HipQueryFinished)
	{
		setOutputHtml(hipOnlineReply->getResult());
	}

	else if (hipOnlineReply->getCurrentStatus()!=HipOnlineReply::HipQueryQuerying)
	{
		disconnect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
		delete hipOnlineReply;
		hipOnlineReply=Q_NULLPTR;
	}
}

// Called when the current HIP query status changes. This displays simple HTML answers.
void OnlineQueries::onAavsoHipQueryStatusChanged()
{
	setEnabled(true); // show dialog
	Q_ASSERT(hipOnlineReply);
	if (hipOnlineReply->getCurrentStatus()==HipOnlineReply::HipQueryErrorOccured)
	{
		setOutputHtml(QString("<p>Lookup error: %1</p>").arg(hipOnlineReply->getErrorString()));
	}

	else if (hipOnlineReply->getCurrentStatus()==HipOnlineReply::HipQueryFinished)
	{
		//setOutputHtml(hipOnlineReply->getResult());
		// Parse XML, extract OID, lookup again.
		QXmlStreamReader xml(hipOnlineReply->getResult());
		int oid=0;
		while (!xml.atEnd()) {
			xml.readNext();
			if (xml.isStartElement() && xml.name()=="OID")
			{
				oid=xml.readElementText().toInt();
			}
		  }
		  if (xml.hasError()) {
			qDebug() << "XML error in AAVSO's answer:" << xml.errorString();
		  }
		//qDebug() << "We have found OID=" << oid;

		//disconnect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onAavsoHipQueryStatusChanged()));
		//delete hipOnlineReply;

		// Trigger second AAVSO query. Note that we mangle the name a bit.
		//hipOnlineReply=hipQuery->lookup(aavsOidUrl, oid);
		//onHipQueryStatusChanged();
		//connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
		// It's prettier to call the browser externally.
		if (oid>0)
		{
			setOutputHtml(QString("<h1>AAVSO</h1><p>Opened AAVSO page on OID=%1 in your webbrowser.</p>").arg(QString::number(oid)));
			QDesktopServices::openUrl(QUrl(aavsoOidUrl.arg(oid)));
		}
		else
			setOutputHtml(QString("<h1>AAVSO</h1><p>AAVSO has no entry for this star.</p>"));
	}

	else if (hipOnlineReply->getCurrentStatus()!=HipOnlineReply::HipQueryQuerying)
	{
		disconnect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onAavsoHipQueryStatusChanged()));
		delete hipOnlineReply;
		hipOnlineReply=Q_NULLPTR;
	}
}

void OnlineQueries::setOutputHtml(QString html)
{
	if (dialog)
		dialog->setOutputHtml(html);
}
