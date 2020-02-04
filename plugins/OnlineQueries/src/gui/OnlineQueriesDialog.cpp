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
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StarnamesSearcher.hpp"

#include <QLineEdit>
#include <QStandardItemModel>
#include <QTimer>

OnlineQueriesDialog::OnlineQueriesDialog(QObject* parent) : StelDialog("OnlineQueries", parent), plugin(Q_NULLPTR)
{
	ui = new Ui_onlineQueriesDialogForm;
}

OnlineQueriesDialog::~OnlineQueriesDialog()
{
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
	//plugin should be finalized at this point. Probably, we can even use the plugin main class as parent?
	plugin = GETSTELMODULE(OnlineQueries);
	Q_ASSERT(plugin);
	//additionally, OnlineQueries::init should have been called to make sure the correct values are set

	//load Ui from form file
	ui->setupUi(dialog);

	// For now, hide tabs that we don't yet attempt to support
	ui->tab_ancientSkies->setEnabled(false);
	ui->tab_ancientSkies->setVisible(false);
	ui->tabWidget->removeTab(2); // Remove Ancient Skies for now.
	ui->tabWidget->removeTab(0); // Remove Wikipedia for now.

	//hook up retranslate event
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &OnlineQueriesDialog::retranslate);

	//connect UI events
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
//	connect(ui->OnlineQueriesListWidget, &QListWidget::currentItemChanged, this, &OnlineQueriesDialog::OnlineQueriesChanged);

	//hook up some OnlineQueries actions
//	StelActionMgr* acMgr = StelApp::getInstance().getStelActionManager();
//	StelAction* ac = acMgr->findAction("actionShow_OnlineQueries_<add-some-action>");
	//connectSlotsByName does not work in our case (because this class does not "own" the GUI in the Qt sense)
	//the "new" syntax is extremly ugly in case signals have overloads
	createUpdateConnections();

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
		ui->onlineQueriesTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	//setToInitialValues();
	updateTextBrowser();
	connect(ui->ptolemaicPushButton, SIGNAL(clicked()), this, SLOT(queryStarnames()));
	connect(ui->wikipediaPushButton, SIGNAL(clicked()), this, SLOT(queryWikipedia()));
}

void OnlineQueriesDialog::createUpdateConnections()
{
	//connect OnlineQueries update events

}



void OnlineQueriesDialog::updateTextBrowser()
{

		ui->onlineQueriesTextBrowser->setHtml("<h1>Hello World</h1><p>This is a test of the OnlineQueries Dialog</p>");
}


void OnlineQueriesDialog::queryStarnames()
{
	ui->onlineQueriesTextBrowser->setHtml("<h1>Starnames</h1><p>WIP...</p>");

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

	StarnamesLookupReply* reply=starnamesSearcher.lookup(hipNr);
	//StarnamesLookupReply* reply=starnamesSearcher.lookup(44699);

	onStarnameStatusChanged();
	connect(reply, SIGNAL(statusChanged()), this, SLOT(onStarnameStatusChanged()));


}

// Called when the current simbad query status changes
void OnlineQueriesDialog::onStarnameStatusChanged()
{
	Q_ASSERT(starnamesLookupReply);
	QString info;
	if (starnamesLookupReply->getCurrentStatus()==StarnamesLookupReply::StarnamesLookupErrorOccured)
	{
		info = QString("%1: %2").arg(q_("Simbad Lookup Error")).arg(starnamesLookupReply->getErrorString());
		//ui->starnamesStatusLabel->setText(info);
		ui->onlineQueriesTextBrowser->setHtml("<p>No result</p>");
	}

	if (starnamesLookupReply->getCurrentStatus()==StarnamesLookupReply::StarnamesLookupFinished)
	{
		starnamesResult = starnamesLookupReply->getResult();
	}

	if (starnamesLookupReply->getCurrentStatus()!=StarnamesLookupReply::StarnamesLookupQuerying)
	{
		disconnect(starnamesLookupReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		delete starnamesLookupReply;
		starnamesLookupReply=Q_NULLPTR;
	}
}



void OnlineQueriesDialog::queryWikipedia()
{
	ui->onlineQueriesTextBrowser->setHtml("<h1>Wikipedia</h1><p>Not yet supported.</p>");
}

