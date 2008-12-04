/*
 * Copyright (C) 2008 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "SimbadSearcher.hpp"
#include <QNetworkReply>
#include <QNetworkAccessManager>

SimbadLookupReply::SimbadLookupReply(QNetworkReply* r) : reply(r)
{
	connect(r, SIGNAL(finished()), this, SLOT((httpQueryFinished())));
}

SimbadLookupReply::~SimbadLookupReply()
{
	reply->deleteLater();
}

void SimbadLookupReply::httpQueryFinished()
{
	
}


SimbadSearcher::SimbadSearcher(QObject* parent) : QObject(parent)
{
	networkMgr = new QNetworkAccessManager(this);
}

//! Lookup in Simbad for the passed object name.
SimbadLookupReply* SimbadSearcher::lookup(const QString& objectName, int maxNbResult)
{
	QUrl url;
	QNetworkReply* netReply = networkMgr->get(QNetworkRequest(url));
	SimbadLookupReply* r = new SimbadLookupReply(netReply);
	return r;
}
