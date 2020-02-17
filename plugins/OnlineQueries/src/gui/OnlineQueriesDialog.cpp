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
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

OnlineQueriesDialog::OnlineQueriesDialog(QObject* parent) :
	StelDialog("OnlineQueries", parent),
	plugin(Q_NULLPTR)
{
	setObjectName("OnlineQueriesDialog");
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
	plugin = GETSTELMODULE(OnlineQueries);

	//load UI from form file
	ui->setupUi(dialog);

	// For now, hide tabs that we don't yet attempt to support
	//ui->tabWidget->removeTab(2); // Remove Ancient Skies for now?

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

	connect(ui->ptolemaicPushButton,    SIGNAL(clicked()), plugin, SLOT(queryStarnames()));
	connect(ui->ancientSkiesPushButton, SIGNAL(clicked()), plugin, SLOT(queryAncientSkies()));
	connect(ui->wikipediaPushButton,    SIGNAL(clicked()), plugin, SLOT(queryWikipedia()));
}

void OnlineQueriesDialog::setOutputHtml(QString html)
{
	if (ui->onlineQueriesTextBrowser)
		ui->onlineQueriesTextBrowser->setHtml(html);
}

