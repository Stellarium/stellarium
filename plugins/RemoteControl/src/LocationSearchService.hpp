/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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

#ifndef LOCATIONSEARCHSERVICE_HPP_
#define LOCATIONSEARCHSERVICE_HPP_

#include "AbstractAPIService.hpp"
#include "StelLocationMgr.hpp"
#include <QMutex>

class LocationSearchService : public AbstractAPIService
{
	Q_OBJECT
public:
	LocationSearchService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~LocationSearchService() {}

	//! we work on a copy of the StelLocationMgr, to prevent hitches as the web user is typing
	bool supportsThreadedOperation() const Q_DECL_OVERRIDE { return true; }
protected:
	virtual void getImpl(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
private slots:
	// connected to the main location manager in the main thread
	void mainLocationManagerUpdated();
private:
	//the location mgr is actually copied to be used in HTTP threads without blocking the main app
	StelLocationMgr locMgr;
	QMutex locMgrMutex;
};



#endif
