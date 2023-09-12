/*
 * Stellarium OnlineQueries Plug-in
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef ONLINEQUERIESDIALOG_HPP
#define ONLINEQUERIESDIALOG_HPP

#include "StelDialogSeparate.hpp"
#include "ui_onlineQueriesDialog.h"

class OnlineQueries;

class OnlineQueriesDialog : public StelDialogSeparate
{
	Q_OBJECT
public:
	OnlineQueriesDialog(QObject* parent = nullptr);
	~OnlineQueriesDialog() override;

public slots:
	void retranslate() override;
	void setOutputHtml(QString html) const;
	void setOutputUrl(QUrl url) const;

protected:
	void createDialogContent() override;
	void setAboutHtml();

private:
	Ui_onlineQueriesDialogForm* ui;
	OnlineQueries* plugin;
	QWidget *view;                  // Either a QWebengineView- or a QTextBrowser-like class.
};

#endif
