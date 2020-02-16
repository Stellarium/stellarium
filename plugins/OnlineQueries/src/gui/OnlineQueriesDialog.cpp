/*
 * Stellarium OnlineQueries Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#include "OnlineQueriesDialog.hpp"
#include "OnlineQueries.hpp"

#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "Star.hpp"
#include "StarMgr.hpp"
#include "Planet.hpp"
#include "NebulaMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

#include <QDesktopServices>
#include <QSharedPointer>

#define STARNAMES_URL "https://biblicalastronomy.co/playground/fetch.cfm?Hipp="
#define ANCIENT_SKIES_URL "https://www.ancient-skies.org/webservice?hip="

OnlineQueriesDialog::OnlineQueriesDialog(QObject* parent) :
	StelDialog("OnlineQueries", parent),
	plugin(Q_NULLPTR),
	starnamesHipQuery(Q_NULLPTR),
	ancientSkiesHipQuery(Q_NULLPTR),
	hipOnlineReply(Q_NULLPTR)
{
	setObjectName("OnlineQueriesDialog");
	ui = new Ui_onlineQueriesDialogForm;
}

OnlineQueriesDialog::~OnlineQueriesDialog()
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
	delete ui;
}

void OnlineQueriesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void OnlineQueriesDialog::createDialogContent()
{
	plugin = GETSTELMODULE(OnlineQueries);
	starnamesHipQuery=new HipOnlineQuery(plugin->getPtolemyUrl());
	ancientSkiesHipQuery=new HipOnlineQuery(plugin->getAncientSkiesUrl());

	//load UI from form file
	ui->setupUi(dialog);

	// For now, hide tabs that we don't yet attempt to support
	//ui->tabWidget->removeTab(2); // Remove Ancient Skies for now.

	//hook up retranslate event
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	//connect UI events
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Kinetic scrolling and style sheet for output
	kineticScrollingList << ui->onlineQueriesTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
		ui->onlineQueriesTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}

	//hook up some OnlineQueries actions
//	StelActionMgr* acMgr = StelApp::getInstance().getStelActionManager();
//	StelAction* ac = acMgr->findAction("actionShow_OnlineQueries_<add-some-action>");
	//connectSlotsByName does not work in our case (because this class does not "own" the GUI in the Qt sense)
	//the "new" syntax is extremly ugly in case signals have overloads

	//setToInitialValues();
	//updateTextBrowser();
	connect(ui->ptolemaicPushButton,    SIGNAL(clicked()), this, SLOT(queryStarnames()));
	connect(ui->ancientSkiesPushButton, SIGNAL(clicked()), this, SLOT(queryAncientSkies()));
	connect(ui->wikipediaPushButton,    SIGNAL(clicked()), this, SLOT(queryWikipedia()));
}

void OnlineQueriesDialog::setOutputHtml(QString html)
{
	if (ui)
		ui->onlineQueriesTextBrowser->setHtml(html);
}

void OnlineQueriesDialog::queryStarnames()
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

	//onStarnameStatusChanged();
	connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
}

// Called when the current HIP query status changes
void OnlineQueriesDialog::onHipQueryStatusChanged()
{
	HipOnlineReply *reply=static_cast<HipOnlineReply*>(sender());
	Q_ASSERT(reply);
	if (reply->getCurrentStatus()==HipOnlineReply::HipQueryErrorOccured)
	{
		QString info = QString("%1: %2").arg(q_("Starnames Lookup Error")).arg(reply->getErrorString());
		//ui->starnamesStatusLabel->setText(info);
		setOutputHtml("<p>No result</p>");
	}

	if (reply->getCurrentStatus()==HipOnlineReply::HipQueryFinished)
	{
		queryResult = reply->getResult();
		setOutputHtml(queryResult);
	}

	if (reply->getCurrentStatus()!=HipOnlineReply::HipQueryQuerying)
	{
		disconnect(reply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
		delete reply;
		reply=Q_NULLPTR;
	}
}

void OnlineQueriesDialog::queryAncientSkies()
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

	//onAncientSkiesStatusChanged();
	connect(hipOnlineReply, SIGNAL(statusChanged()), this, SLOT(onHipQueryStatusChanged()));
}


void OnlineQueriesDialog::queryWikipedia()
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
