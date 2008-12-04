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

#ifndef SIMBADSEARCHER_HPP_
#define SIMBADSEARCHER_HPP_

#include "VecMath.hpp"
#include <QObject>
#include <QMap>

class QNetworkReply;

//! @class SimbadLookupReply
//! Contains all the information about a current simbad lookup query
class SimbadLookupReply : QObject
{
	Q_OBJECT;
	Q_ENUMS(SimbadLookupStatus);
	
public:
	friend class SimbadSearcher;
	
	//! Possible status for a simbad query.
	enum SimbadLookupStatus
	{
		SimbadLookupRest,
		SimbadLookupQuerying,
		SimbadLookupErrorOccured,
		SimbadLookupFinished
	};

	~SimbadLookupReply();
	
	//! Get the result list of matching objectName/position.
	QMap<QString, Vec3d> getResults() const {return resultPositions;}
	
signals:
	//! Triggered when the lookup status change.
	void statusChanged(SimbadLookupStatus status);
	
private slots:
	void httpQueryFinished();
	
private:
	//! Private constructor can be called by SimbadSearcher only
	SimbadLookupReply(QNetworkReply* r);
	QNetworkReply* reply;
	
	QMap<QString, Vec3d> resultPositions;
};


//! @class SimbadSearcher
//! Provides lookup features into the online Simbad service from CDS.
//! See http://simbad.u-strasbg.fr for more info.
class SimbadSearcher : public QObject
{
	Q_OBJECT;

public:
	SimbadSearcher(QObject* parent);

	//! Lookup in Simbad for the passed object name.
	//! @param maxNbResult the maximum number of returned result.
	//! @return a new SimbadLookupReply which is owned by the caller.
	SimbadLookupReply* lookup(const QString& objectName, int maxNbResult=1);
		
private:	
	//! The network manager used query simbad
	class QNetworkAccessManager* networkMgr;
};

#endif /*SIMBADSEARCHER_HPP_*/
