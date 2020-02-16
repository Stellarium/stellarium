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

#ifndef ONLINEQUERIESDIALOG_HPP
#define ONLINEQUERIESDIALOG_HPP

#include "StelDialog.hpp"
#include "ui_onlineQueriesDialog.h"
#include "HipOnlineQuery.hpp"

class OnlineQueries;

class OnlineQueriesDialog : public StelDialog
{
	Q_OBJECT
public:
	OnlineQueriesDialog(QObject* parent = Q_NULLPTR);
	~OnlineQueriesDialog();

public slots:
	void retranslate();
	void setOutputHtml(QString html);

protected:
	void createDialogContent();

private slots:
	void queryStarnames();    //!< Connect from a button that triggers information query
	void queryAncientSkies(); //!< Connect from a button that triggers information query
	void queryWikipedia();    //!< Connect from a button that triggers information query
	void onHipQueryStatusChanged(); //!< To be connected

private:
	Ui_onlineQueriesDialogForm* ui;
	OnlineQueries* plugin;

	// The query target websites:
	// Patrick Gleason's site
	HipOnlineQuery *starnamesHipQuery;
	// ancient-skies.org
	HipOnlineQuery *ancientSkiesHipQuery;

	HipOnlineReply *hipOnlineReply;

	QString queryResult; //!< Keep result of last online query
};

#endif
